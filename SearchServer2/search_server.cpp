#include "search_server.h"
#include "string_processing.h"

#include <execution>
#include <mutex>

using namespace std::literals::string_literals;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{}

SearchServer::SearchServer(const std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("document contains wrong id"s);
    }

    document_ids_.insert(document_id);
    const auto [it, inserted] = documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, std::string(document) });
    std::vector<std::string_view> words = SplitIntoWordsNoStop(it->second.text);

    const double inv_word_count = 1.0 / words.size();
    for (const auto word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        doc_id_to_words_freqs_[document_id][word] += inv_word_count;
    }
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() {
    return document_ids_.end();
}

//Метод получения частот слов по id документа
const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (doc_id_to_words_freqs_.count(document_id)) {
        static std::map<std::string_view, double> m;
        for (auto p : doc_id_to_words_freqs_.at(document_id)){
            m[p.first] = p.second;
        }
        return m;
    }
    static const std::map<std::string_view, double> dummy_;
    return dummy_;
}

//Метод удаления документов из поискового сервера
void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    if (document_ids_.count(document_id) == 1) {
        for (const auto [search_word, _] : doc_id_to_words_freqs_[document_id]) {
            std::string found_word = static_cast<std::string>(search_word);
            word_to_document_freqs_[found_word].erase(document_id);
            if (word_to_document_freqs_[found_word].empty()) {
                word_to_document_freqs_.erase(found_word);
            }
        }
        document_ids_.erase(document_id);
        documents_.erase(document_id);
        doc_id_to_words_freqs_.erase(document_id);
    }
}

////Вы можете столкнуться с тем, что нужный алгоритм не параллелится, когда передаёте в него итераторы не произвольного доступа.
//Попробуйте переложить нужные элементы в вектор и запустить алгоритм для него.
void SearchServer::RemoveDocument(const std::execution::parallel_policy& police, int document_id) {
    if (!documents_.count(document_id)) return;
    const auto& word_freqs = doc_id_to_words_freqs_.at(document_id);
    std::vector<std::string_view> words(word_freqs.size());
    //std::vector<std::string_view> words;
    //words.reserve(word_freqs.size());
    //std::transform(
    //    std::execution::par,
    //    word_freqs.begin(), word_freqs.end(),
    //    words.begin(),
    //    [](const auto& item) {
    //        return std::move(item.first);/*std::move remove, but not clear why. with it faster*/
    //    }
    //);
    std::transform(
        std::execution::par,
        word_freqs.begin(), word_freqs.end(),
        words.begin(),
        [](const auto& item) { return item.first; }
    );
    //std::mutex m;
    std::for_each(
        std::execution::par,
        words.begin(), words.end(),
        [//&m, 
        this, &document_id](const auto& word) {
            //std::lock_guard<std::mutex> guard(m);
            if (word_to_document_freqs_.count(word)){
                word_to_document_freqs_.at(word).erase(document_id);
            }
        });
    document_ids_.erase(document_id);
    documents_.erase(document_id);
    doc_id_to_words_freqs_.erase(document_id);
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + static_cast<std::string>(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw std::invalid_argument("Query word "s + static_cast<std::string>(text) + " is invalid");
    }

    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text, const bool sort) const {

    Query result;

    std::vector<std::string_view> words = SplitIntoWords(text);
	result.minus_words.reserve(words.size());
	result.plus_words.reserve(words.size());

    for (const auto& word : words) {
        const auto& query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if (sort == true) {
        std::sort(result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(std::unique(result.minus_words.begin(), result.minus_words.end()), result.minus_words.end());

        std::sort(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(std::unique(result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());

    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}