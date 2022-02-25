#include <numeric>

#include "search_server.h"
#include "test_example_functions.h"

using namespace std;

#pragma region Служебные методы

ostream& operator << (ostream& os, const DocumentStatus& status) {
	switch (status)
	{
	case DocumentStatus::ACTUAL:
		os << "ACTUAL"s;
		break;
	case DocumentStatus::IRRELEVANT:
		os << "IRRELEVANT"s;
		break;
	case DocumentStatus::BANNED:
		os << "BANNED"s;
		break;
	case DocumentStatus::REMOVED:
		os << "REMOVED"s;
		break;
	default:
		os << "UNKNOWN"s;
		break;
	}
	return os;
}

template <class T>
ostream& operator << (ostream& os, const vector<T>& s) {
	os << "[";
	bool first = true;
	for (const auto& x : s) {
		if (!first) {
			os << ", ";
		}
		first = false;
		os << x;
	}
	return os << "]";
}

template <class T>
ostream& operator << (ostream& os, const set<T>& s) {
	os << "{";
	bool first = true;
	for (const auto& x : s) {
		if (!first) {
			os << ", ";
		}
		first = false;
		os << x;
	}
	return os << "}";
}

template <class K, class V>
ostream& operator << (ostream& os, const map<K, V>& m) {
	os << "{";
	bool first = true;
	for (const auto& kv : m) {
		if (!first) {
			os << ", ";
		}
		first = false;
		os << kv.first << ": " << kv.second;
	}
	return os << "}";
}

#pragma endregion

#pragma region MACROS ASSERT

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line, const string& hint) {
	if (!value) {
		cerr << file << "("s << line << "): "s << func << ": "s;
		cerr << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty()) {
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	}
}

#pragma endregion

#pragma region Модульные тесты поисковой системы

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1u);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
		SearchServer server("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
	}
}

double ComputeTermFreq(const string& term, const map<string, size_t>& all_words) {
	double tf = 0.0;
	if (all_words.count(term)) {
		auto sum = [](size_t accum, const pair<string, size_t>& item) {
			const auto& [_, count] = item;
			return accum + count;
		};
		const size_t total_words = accumulate(all_words.begin(), all_words.end(), 0, sum);
		const size_t term_count = all_words.at(term);
		tf = static_cast<double>(term_count) / total_words;
	}
	return tf;
}

double ComputeInverseDocumentFreq(const string& term, const vector<map<string, size_t>>& documents) {
	//return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
	auto HasTerm = [&term](const map<string, size_t>& words) {
		return (0 < words.count(term));
	};
	const size_t docs_with_term_count = count_if(documents.begin(), documents.end(), HasTerm);
	if (docs_with_term_count == 0)
		return 0;
	return log(static_cast<double>(documents.size()) / docs_with_term_count);
}

//- Добавление документов.
// Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void Test_FindAddedDocumentByWordsQuery() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const DocumentStatus status = DocumentStatus::ACTUAL;
	const vector<int> ratings = { 1, 2, 3 };
	const string query = "in"s;

	SearchServer server;
	server.AddDocument(doc_id, content, status, ratings);

	//Счётчик увеличился
	{
		ASSERT_EQUAL(1u, server.GetDocumentCount());
	}

	//Рейтинг рассчитан
	{
		const double avg_rating = static_cast<double>(accumulate(ratings.begin(), ratings.end(), 0)) / ratings.size();
		const auto found_docs = server.FindTopDocuments(query, status);
		ASSERT_EQUAL(1u, found_docs.size());
		ASSERT_EQUAL(doc_id, found_docs.at(0).id);
		ASSERT_EQUAL(avg_rating, found_docs.at(0).rating);
	}

	//Статус присвоен
	{
		auto [_, matched_status] = server.MatchDocument(query, doc_id);
		ASSERT_EQUAL(status, matched_status);
	}
}

//- Поддержка минус-слов.
// Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void Test_QueryDoesntFindMinusWords() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	SearchServer server;
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

	const int doc_id2 = 52;
	const string content2 = "dog of the city"s;
	const vector<int> ratings2 = { 2, 3, 4 };
	server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);

	// Поиск по минус-словам, возвращает пустой результат
	{
		const string query = "cat -city"s;
		const auto search_result = server.FindTopDocuments(query);
		ASSERT_HINT(search_result.empty(), "Search by negative keywords, must return empty result!"s);
	}

	//Поиск без минус-слов, возвращает не пустой результат
	{
		const string query = "cat city"s;
		const auto search_result = server.FindTopDocuments(query);
		ASSERT_HINT(search_result.size() > 0, "Search by keywords, must return not empty result!"s);
	}
}

//- Матчинг документов.
// При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса,
// присутствующие в документе.
// Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void Test_FindWordsOfQueryMathedWithDocument() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	const DocumentStatus status = DocumentStatus::ACTUAL;
	SearchServer server;
	server.AddDocument(doc_id, content, status, ratings);

	// Поиск по словам, возвращает plus_words
	{
		const string query = "cat city dog"s;
		const auto [matched_words, matched_status] = server.MatchDocument(query, doc_id);
		vector<string> expected = { "cat"s, "city"s };
		for (const auto& find_word : matched_words) {
			ASSERT_HINT(count(expected.begin(), expected.end(), find_word) > 0u, "All words from the search query that appear in the document should be returned.");
		}
	}

	// Поиск по минус-словам, возвращает пустой результат
	{
		const string query = "cat -city"s;
		const auto [matched_words, _] = server.MatchDocument(query, doc_id);
		ASSERT_HINT(matched_words.empty(), "Search by negative keywords, must return empty result!"s);
	}
}

//- Сортировка найденных документов по релевантности.
// Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void Test_SortFinedDocument() {
	SearchServer server;

	// Сортировка по убыванию релевантности
	{
		server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		server.AddDocument(33, "cat of the city"s, DocumentStatus::BANNED, { 1, 2, 2 });
		server.AddDocument(36, "city of cats"s, DocumentStatus::ACTUAL, { 1, 2, 2 });
		server.AddDocument(54, "dog of the city"s, DocumentStatus::ACTUAL, { 1, 4, 2 });

		// Убедиться, что возвращаемые при поиске документов результаты отсортированы в порядке убывания релевантности.
		const auto& matched_docs = server.FindTopDocuments("city"s);
		size_t length = matched_docs.size();
		for (size_t i = 0; i < length - 1; ++i)
		{
			if (abs(matched_docs[i].relevance - matched_docs[i + 1].relevance) < EPSILON)
				ASSERT_HINT(abs(matched_docs[i].relevance - matched_docs[i + 1].relevance) < EPSILON, "Results must be sorted in descending order of relevance!"s);
			else
				ASSERT_HINT(matched_docs[i].relevance > matched_docs[i + 1].relevance, "Results must be sorted in descending order of relevance!"s);
		}
	}
}

//- Вычисление рейтинга документов.
// Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void Test_RatingFinedDocument() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const DocumentStatus status = DocumentStatus::ACTUAL;
	const vector<int> ratings = { 1, 2, 3 };
	const string query = "in"s;

	SearchServer server;
	server.AddDocument(doc_id, content, status, ratings);

	//Рейтинг рассчитан
	{
		const double avg_rating = static_cast<double>(accumulate(ratings.begin(), ratings.end(), 0)) / ratings.size();
		const auto found_docs = server.FindTopDocuments(query, status);
		ASSERT_EQUAL(1u, found_docs.size());
		ASSERT_EQUAL(doc_id, found_docs.at(0).id);
		ASSERT_EQUAL(avg_rating, found_docs.at(0).rating);
	}

	// Граничный случай для Рейтинга = 0
	const int doc_id2 = 52;
	const string content2 = "dog in the city"s;
	const DocumentStatus status2 = DocumentStatus::ACTUAL;
	const vector<int> ratings2 = { };
	const string query2 = "dog"s;
	server.AddDocument(doc_id2, content2, status2, ratings2);
	{
		const int avg_rating2 = 0;
		const auto found_docs2 = server.FindTopDocuments(query2, status2);
		ASSERT_EQUAL(1u, found_docs2.size());
		ASSERT_EQUAL(doc_id2, found_docs2.at(0).id);
		ASSERT_EQUAL(avg_rating2, found_docs2.at(0).rating);
	}
}

//- Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void Test_FindDocumentByUsersFilter() {
	const DocumentStatus status_actual = DocumentStatus::ACTUAL;
	const vector<int> ratings = { 1, 2, 3 };
	// Фильтрация результатов поиска с использованием предиката
	{
		SearchServer server;
		server.AddDocument(42, "cat in the city"s, status_actual, ratings);
		server.AddDocument(33, "cat of the city"s, status_actual, ratings);
		server.AddDocument(36, "the city of cats"s, status_actual, ratings);
		server.AddDocument(54, "dog of the city"s, status_actual, ratings);
		auto predicat = [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; };
		const auto& search_result = server.FindTopDocuments("the city"s, predicat);
		vector<int> expected = { 36, 42, 54 };
		ASSERT(expected.size() == search_result.size());
		for (auto& doc : search_result) {
			ASSERT_HINT(count(expected.begin(), expected.end(), doc.id) > 0, "The search result must match the search predicate."s);
		}
	}
}

//- Поиск документов, имеющих заданный статус.
void Test_FindDocumentByFilterStatus() {
	// Фильтрация результатов поиска с использованием предиката
	{
		SearchServer server;
		server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		server.AddDocument(33, "cat of the city"s, DocumentStatus::BANNED, { 1, 2, 2 });
		server.AddDocument(54, "dog of the city"s, DocumentStatus::ACTUAL, { 1, 4, 2 });
		const auto& matched_docs = server.FindTopDocuments("city"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
		ASSERT_EQUAL(1u, matched_docs.size());
		ASSERT_EQUAL(33u, matched_docs[0].id);
	}
}

//- Корректное вычисление релевантности найденных документов.
void Test_CorrectEvalRelevance() {
	{
		SearchServer server;
		server.AddDocument(33, "cat of the city"s, DocumentStatus::BANNED, { 1, 2, 2 });
		server.AddDocument(36, "the city of cats"s, DocumentStatus::ACTUAL, { 1, 4, 3 });
		server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		server.AddDocument(54, "dog of the city"s, DocumentStatus::ACTUAL, { 1, 4, 2 });
		const string raw_query = "city cat star"s;
		auto search_result = server.FindTopDocuments(raw_query);

		vector<int> expected = { 36, 42, 54 };
		ASSERT_HINT(expected.size() == search_result.size(), "Incorrect count of found documents"s);

		static const vector<map<string, size_t>> words_in_docs = {
			{
				{"cat"s, 1},
				{"of"s, 1},
				{"the"s, 1},
				{"city"s, 1},
			},
			{
				{"the"s, 1},
				{"city"s, 1},
				{"of"s, 1},
				{"cats"s, 1},
			},
			{
				{"cat"s, 1},
				{"in"s, 1},
				{"the"s, 1},
				{"city"s, 1},
			},
			{
				{"dog"s, 1},
				{"of"s, 1},
				{"the"s, 1},
				{"city"s, 1},
			},
		};
		string term1 = "city"s;
		string term2 = "cat"s;
		string term3 = "star"s;
		size_t doc_id;

		{
			doc_id = 36;
			auto HasId = [doc_id](const Document& doc) {
				return doc.id == doc_id;
			};
			auto iter = find_if(search_result.begin(), search_result.end(), HasId);
			ASSERT(iter != search_result.end());

			const map<string, size_t> words = words_in_docs.at(1);

			const double tf1 = ComputeTermFreq(term1, words);
			const double idf1 = ComputeInverseDocumentFreq(term1, words_in_docs);
			const double expected_relevance1 = tf1 * idf1;

			const double tf2 = ComputeTermFreq(term2, words);
			const double idf2 = ComputeInverseDocumentFreq(term2, words_in_docs);
			const double expected_relevance2 = tf2 * idf2;

			const double tf3 = ComputeTermFreq(term3, words);
			const double idf3 = ComputeInverseDocumentFreq(term3, words_in_docs);
			const double expected_relevance3 = tf3 * idf3;

			const double expected_relevance = expected_relevance1 + expected_relevance2 + expected_relevance3;

			ASSERT_HINT(abs(iter->relevance - expected_relevance) < EPSILON, "doc_id=36: Incorrectly calculated relevance by TF-IDF!"s);
		}
		{
			doc_id = 42;
			auto HasId = [doc_id](const Document& doc) {
				return doc.id == doc_id;
			};
			auto iter = find_if(search_result.begin(), search_result.end(), HasId);
			ASSERT(iter != search_result.end());

			const map<string, size_t> words = words_in_docs.at(2);

			const double tf1 = ComputeTermFreq(term1, words);
			const double idf1 = ComputeInverseDocumentFreq(term1, words_in_docs);
			const double expected_relevance1 = tf1 * idf1;

			const double tf2 = ComputeTermFreq(term2, words);
			const double idf2 = ComputeInverseDocumentFreq(term2, words_in_docs);
			const double expected_relevance2 = tf2 * idf2;

			const double tf3 = ComputeTermFreq(term3, words);
			const double idf3 = ComputeInverseDocumentFreq(term3, words_in_docs);
			const double expected_relevance3 = tf3 * idf3;

			const double expected_relevance = expected_relevance1 + expected_relevance2 + expected_relevance3;

			ASSERT_HINT(abs(iter->relevance - expected_relevance) < EPSILON, "doc_id=42: Incorrectly calculated relevance by TF-IDF!"s);
		}
		{
			doc_id = 54;
			auto HasId = [doc_id](const Document& doc) {
				return doc.id == doc_id;
			};
			auto iter = find_if(search_result.begin(), search_result.end(), HasId);
			ASSERT(iter != search_result.end());

			const map<string, size_t> words = words_in_docs.at(3);

			const double tf1 = ComputeTermFreq(term1, words);
			const double idf1 = ComputeInverseDocumentFreq(term1, words_in_docs);
			const double expected_relevance1 = tf1 * idf1;

			const double tf2 = ComputeTermFreq(term2, words);
			const double idf2 = ComputeInverseDocumentFreq(term2, words_in_docs);
			const double expected_relevance2 = tf2 * idf2;

			const double tf3 = ComputeTermFreq(term3, words);
			const double idf3 = ComputeInverseDocumentFreq(term3, words_in_docs);
			const double expected_relevance3 = tf3 * idf3;

			const double expected_relevance = expected_relevance1 + expected_relevance2 + expected_relevance3;

			ASSERT_HINT(abs(iter->relevance - expected_relevance) < EPSILON, "doc_id=54: Incorrectly calculated relevance by TF-IDF!"s);
		}
	}
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(Test_FindAddedDocumentByWordsQuery);
	RUN_TEST(Test_QueryDoesntFindMinusWords);
	RUN_TEST(Test_FindWordsOfQueryMathedWithDocument);
	RUN_TEST(Test_SortFinedDocument);
	RUN_TEST(Test_RatingFinedDocument);
	RUN_TEST(Test_FindDocumentByUsersFilter);
	RUN_TEST(Test_FindDocumentByFilterStatus);
	RUN_TEST(Test_CorrectEvalRelevance);
}

// --------- Окончание модульных тестов поисковой системы -----------

#pragma endregion