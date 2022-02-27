#pragma once

#include "search_server.h"
#include "document.h"

#include <string>
#include <vector>
#include <string>

std::vector<std::vector<Document>> ProcessQueriesString(const SearchServer& search_server, const std::vector<std::string>& queries);
std::vector<Document> ProcessQueriesJoinedString(const SearchServer& search_server, const std::vector<std::string>& queries);

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string_view> queries);
std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string_view> queries);