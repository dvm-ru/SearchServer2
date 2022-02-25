#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
	return AddFindRequest(raw_query,
		[status](int document_id, DocumentStatus status_search, int rating)
		{ return status == status_search; });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
	return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
	return empty_requests_;
}

