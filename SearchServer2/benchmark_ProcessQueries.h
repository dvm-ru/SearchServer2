#pragma once

#include "search_server.h"
#include "request_queue.h"
#include "process_queries.h"
#include "paginator.h"
#include "log_duration.h"
#include "remove_duplicates.h"
#include "utility.h"

#include <iostream>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include <execution>
#include <random>

using namespace std::literals::string_literals;
using namespace std;

string GenerateWord_for_benchmark_ProcessQueries(mt19937 & generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution<int>('a', 'z')(generator));
    }
    return word;
}

vector<string> GenerateDictionary_for_benchmark_ProcessQueries(mt19937 & generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord_for_benchmark_ProcessQueries(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery_for_benchmark_ProcessQueries(mt19937& generator, const vector<string>&dictionary, int max_word_count) {
    const int word_count = uniform_int_distribution(1, max_word_count)(generator);
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries_for_benchmark_ProcessQueries(mt19937 & generator, const vector<string>&dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery_for_benchmark_ProcessQueries(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename QueriesProcessor>
void Test_for_benchmark_ProcessQueries(string_view mark, QueriesProcessor processor, const SearchServer& search_server, const vector<string>& queries) {
    LOG_DURATION(mark);
    const auto documents_lists = processor(search_server, queries);
}

#define TEST(processor) Test_for_benchmark_ProcessQueries(#processor, processor, search_server, queries)

int benchmark_ProcessQueries() {

    cout << endl;
    cout << "benchmark_ProcessQueries:" << endl;

    mt19937 generator;
    const auto dictionary = GenerateDictionary_for_benchmark_ProcessQueries(generator, 2'000, 25);
    const auto documents = GenerateQueries_for_benchmark_ProcessQueries(generator, dictionary, 20'000, 10);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    const auto queries = GenerateQueries_for_benchmark_ProcessQueries(generator, dictionary, 2'000, 7);
    TEST(ProcessQueriesString);

    return 0;
}