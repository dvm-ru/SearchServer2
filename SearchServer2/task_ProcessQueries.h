#pragma once

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

int task_ProcessQueries() {

    cout << endl;
    cout << "task_ProcessQueries:" << endl;

    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }

    const vector<string_view> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    id = 0;
    for (const auto& documents : ProcessQueries(search_server, queries) ) {
        cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << endl;
    }

    return 0;
}