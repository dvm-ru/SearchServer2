#pragma once

#include "search_server.h"
#include <vector>
#include <queue>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        std::string query;
        std::vector<Document> search_result;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int empty_requests_ = 0;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    const auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
    requests_.push_back({ raw_query, result });
    if (requests_.back().search_result.empty()) {
        ++empty_requests_;
    }
    if (requests_.size() > min_in_day_) {
        if (requests_.front().search_result.empty()) {
            --empty_requests_;
        }
        requests_.pop_front();
    }
    return requests_.back().search_result;
}
