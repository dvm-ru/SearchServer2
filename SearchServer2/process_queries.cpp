#include <string>
#include <numeric>
#include <execution>

#include "document.h"
#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& s_queries) {    
    std::vector<std::string_view> queries{ s_queries.begin(), s_queries.end() };
    std::vector<std::vector<Document>> result(queries.size());
    transform_inclusive_scan(std::execution::par,
        queries.begin(), queries.end(),
        result.begin(),
        [](std::vector<Document> lhs, std::vector<Document> rhs) {
            return rhs;
        },
        [&search_server](const std::string_view query) {
            return search_server.FindTopDocuments(std::execution::par, query); }
        );
    return result;
}

//����������: ��� ������ ���������� ������������ ��������� �������.
//����� ������� � ��������������� ���������� ������� �� ���������� `ProcessQueries`. ����� ����������.
//����� ��������� � ������������ ��������� `list` � � reduce-������ ���������� ������ �� O(1).
//����� ������� � �������� ������ ��� �������� �������� �� ����� ����������, ������� ��������� ��������������� ��������� ��� �������� ���� ��������
std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> docs_to_queries = ProcessQueries(search_server, queries);
    std::vector<Document> result;
    for (auto docs : docs_to_queries) {
        for (const Document& doc : docs) {
            result.push_back(doc);
        }
    }
    return result;
}
