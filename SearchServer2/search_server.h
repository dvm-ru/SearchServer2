#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <numeric>
#include <cmath>
#include <execution>
#include <mutex>

//#include "log_duration.h"

#include <typeinfo>

#include "document.h"
#include "string_processing.h"

const double EPSILON = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {

public:

    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;


    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& exec_policy, const std::string& raw_query, DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& exec_policy, const std::string& raw_query) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& exec_policy, const std::string& raw_query, DocumentStatus status) const;


    size_t GetDocumentCount() const;


    /// ѕоиск документов по словам запроса
    /// ¬ первом элементе кортежа верните все плюс-слова запроса, содержащиес€ в документе. 
    /// —лова не должны дублироватьс€. ѕусть они будут отсортированы по возрастанию.
    /// ≈сли документ не соответствует запросу (нет пересечений по плюс-словам или есть минус-слово), вектор слов нужно вернуть пустым.
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, int document_id) const;

    std::set<int>::const_iterator begin();
    std::set<int>::const_iterator end();

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    // метод удалени€ документов из поискового сервера
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);


private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    //struct Query {
    //    std::set<std::string, std::less<>> plus_words;
    //    std::set<std::string, std::less<>> minus_words;
    //};
    struct Query {
        std::vector<std::string> plus_words;
        std::vector<std::string> minus_words;
    };

    const std::set<std::string, std::less<>> stop_words_;

    std::set<int> document_ids_;
    std::map<int, DocumentData> documents_;
    std::map<int, std::map<std::string, double>> doc_id_to_words_freqs_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;

    //bool IsStopWord(const std::string& word) const;
    bool IsStopWord(const std::string_view word) const;

    //static bool IsValidWord(const std::string& word) {
    //    // A valid word must not contain special characters
    //    return std::none_of(word.begin(), word.end(), [](char c) {
    //        return c >= '\0' && c < ' ';
    //        });
    //}
    static bool IsValidWord(const std::string_view word) {
        // A valid word must not contain special characters
        return std::none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
    //std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }


    //QueryWord ParseQueryWord(const std::string& text) const;
    //QueryWord ParseQueryWord(std::string_view text) const;

    //Query ParseQuery(const std::string_view& text) const;
    //Query ParseQuery(std::string_view text) const;
    //Query ParseQueryExecPol(std::string_view text) const;

    QueryWord ParseQueryWord(const std::string& text) const;
    Query ParseQuery(std::string_view text) const;
    Query ParseQuery(std::execution::parallel_policy par, std::string_view text) const;


    //double ComputeWordInverseDocumentFreq(const std::string& word) const;
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename ExecutionPolicy, class DocumentPredicate>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&& exec_policy, const Query& query, DocumentPredicate document_predicate) const;
};

inline std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy& seq, const std::string_view raw_query, int document_id) const
{
    if (!document_ids_.count(document_id)) {
        using namespace std::literals::string_literals;
        throw std::out_of_range("incorrect document id"s);
    }

    const auto query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words(query.plus_words.size());

    std::transform(
        seq,
        query.plus_words.begin(),
        query.plus_words.end(),
        matched_words.begin(),
        [this, document_id](const auto& word) {
            if (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id)) {
                return std::move(word);
            }
            return std::string{};
        }
    );

    matched_words.erase(
        std::remove_if(
            seq,
            matched_words.begin(),
            matched_words.end(),
            [](const auto& word) {
                return word.empty();
            }
    ), matched_words.end());

    auto find_iter = std::find_if(
        seq,
        query.minus_words.begin(),
        query.minus_words.end(),
        [this, document_id](const auto& word) {
            if (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id)) {
                return true;
            }
            return false;
        }
    );

    if (find_iter != query.minus_words.end()) {
        matched_words.clear();
    }

    return { matched_words, documents_.at(document_id).status };
}

inline std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& par, const std::string_view raw_query, int document_id) const
{
    //ќграничени€:
    // - ≈сли передан некорректный поисковый запрос raw_query Ч должно быть выброшено исключение std::invalid_argument.
    // - ≈сли передан несуществующий document_id, - ожидаетс€ исключение std::out_of_range.
    if (document_ids_.count(document_id) == 0) {
        using namespace std::literals::string_literals;
        throw std::out_of_range("document_id incorrect!"s);
    }
    //const auto query = ParseQuery(par, raw_query);
    const auto query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());

    std::mutex m;
    std::for_each(par, query.plus_words.begin(), query.plus_words.end(),
        [&m, document_id, &raw_query, &matched_words, this](const auto& word) {
            if (word_to_document_freqs_.count(word)) {
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    std::lock_guard<std::mutex> guard(m);
                    matched_words.push_back(std::move(word));
                }
            }});

    std::sort(par, matched_words.begin(), matched_words.end());
    auto last = std::unique(par, matched_words.begin(), matched_words.end());
    matched_words.erase(last, matched_words.end());

    for (const auto& s_word : query.minus_words) {
        std::string word(s_word.begin(), s_word.end());
        if (word_to_document_freqs_.count(word)) {
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    using namespace std::literals::string_literals;
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& exec_policy, const std::string& raw_query) const {
    return FindTopDocuments(exec_policy, raw_query, [](int document_id, DocumentStatus document_status, int rating) {
        return document_status == DocumentStatus::ACTUAL; });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& exec_policy, const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(exec_policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status; });
}

template <class ExecutionPolicy, class DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& exec_policy, const std::string& raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(exec_policy, query, document_predicate);
    std::sort(exec_policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename ExecutionPolicy, class DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& exec_policy, const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    //for (const auto& word : query.plus_words) {
    std::vector<std::string> plus_words{ std::make_move_iterator(query.plus_words.begin()), std::make_move_iterator(query.plus_words.end()) };
    std::vector<std::string> minus_words{ std::make_move_iterator(query.minus_words.begin()), std::make_move_iterator(query.minus_words.end()) };
    for (const auto& word : plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    //for (const auto& word : query.minus_words) {
    for (const auto& word : minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}