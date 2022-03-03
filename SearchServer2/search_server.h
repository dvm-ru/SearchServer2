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

#include "document.h"
#include "string_processing.h"

#include "log_duration.h"
#include "concurrent_map.h"

const double EPSILON = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {

public:

    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);

    explicit SearchServer(std::string_view stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& exec_policy, const std::string_view raw_query) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& exec_policy, const std::string_view raw_query, DocumentStatus status) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& exec_policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;

    size_t GetDocumentCount() const;

    // Поиск документов по словам запроса
    using MatchDocumentResult = std::tuple<std::vector<std::string_view>, DocumentStatus>;

    MatchDocumentResult MatchDocument(const std::string_view raw_query, int document_id) const;
    MatchDocumentResult MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query, int document_id) const;
    MatchDocumentResult MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, int document_id) const;

    template<typename ExecutionPolicy>
    MatchDocumentResult MatchDocument(const ExecutionPolicy&& exec_policy, const std::string_view raw_query, int document_id) const;

    std::set<int>::const_iterator begin();
    std::set<int>::const_iterator end();

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    // метод удаления документов из поискового сервера
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string text;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    const std::set<std::string, std::less<>> stop_words_;

    std::set<int> document_ids_;
    std::map<int, DocumentData> documents_;
    std::map<int, std::map<std::string_view, double>> doc_id_to_words_freqs_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::set<std::string> words_in_docs_;

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word) {
        // A valid word must not contain special characters
        return std::none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    QueryWord ParseQueryWord(const std::string_view text) const;

    Query ParseQuery(const std::string_view text) const;
    Query ParseQuery(const std::execution::sequenced_policy& policy, const std::string_view text) const;
    Query ParseQuery(const std::execution::parallel_policy& policy, const std::string_view text) const;

    template<typename ExecutionPolicy>
    Query ParseQuery(const ExecutionPolicy&& exec_policy, const std::string_view text) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename ExecutionPolicy, class DocumentPredicate>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&& exec_policy, const Query& query, DocumentPredicate document_predicate) const;
};

template<typename ExecutionPolicy>
inline SearchServer::MatchDocumentResult SearchServer::MatchDocument(const ExecutionPolicy&& exec_policy, const std::string_view raw_query, int document_id) const {
    if (!document_ids_.count(document_id)) {
        using namespace std::literals::string_literals;
        throw std::out_of_range("document_id incorrect!"s);
    }
    const auto query = std::move(ParseQuery(*exec_policy, raw_query));

    if (std::any_of(*exec_policy, std::make_move_iterator(query.minus_words.begin()), std::make_move_iterator(query.minus_words.end()),
        [this, document_id](const auto& word) {
            return doc_id_to_words_freqs_.at(document_id).count(word);
        })) {
        return { {}, documents_.at(document_id).status };
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto last = std::copy_if(*exec_policy, std::make_move_iterator(query.plus_words.begin()), std::make_move_iterator(query.plus_words.end()),
        matched_words.begin(),
        [this, document_id](const auto& word) {
            return doc_id_to_words_freqs_.at(document_id).count(word);
        });
    std::sort(*exec_policy, matched_words.begin(), last);
    auto it = std::unique(*exec_policy, matched_words.begin(), last);
    matched_words.erase(it, matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}

template<typename ExecutionPolicy>
inline SearchServer::Query SearchServer::ParseQuery(const ExecutionPolicy&& exec_policy, const std::string_view text) const {

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

    std::sort(*exec_policy, result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(std::unique(result.minus_words.begin(), result.minus_words.end()), result.minus_words.end());

    std::sort(*exec_policy, result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(std::unique(result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());

    return result;
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
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& exec_policy, const std::string_view raw_query) const {
    return FindTopDocuments(exec_policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& exec_policy, const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(exec_policy, raw_query, [&status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status; });
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& exec_policy, const std::string_view raw_query, DocumentPredicate document_predicate) const {

    auto query = ParseQuery(raw_query);

    std::sort(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(std::unique(query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());

    std::sort(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(std::unique(query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());

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
    ConcurrentMap<int, double> document_to_relevance(6);

    const auto plus_word_checker =
        [this, &document_predicate, &document_to_relevance](std::string_view word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id].ref_to_value += static_cast<double>(term_freq * inverse_document_freq);
            }
        }
    };
    std::for_each(exec_policy, query.plus_words.begin(), query.plus_words.end(), plus_word_checker);

    const auto minus_word_checker =
        [this, &document_predicate, &document_to_relevance](std::string_view word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.Erase(document_id);
        }
    };
    std::for_each(exec_policy, query.minus_words.begin(), query.minus_words.end(), minus_word_checker);

    std::map<int, double> m_doc_to_relevance = document_to_relevance.BuildOrdinaryMap();

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : m_doc_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}