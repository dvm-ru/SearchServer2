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

int task_1_of_3_RemoveDocument() {

    cout << endl;
    cout << "task_1_of_3_RemoveDocument:" << endl;

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

    const string query = "curly and funny"s;

    auto report = [&search_server, &query] {
        cout << search_server.GetDocumentCount() << " documents total, "s
            << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
    };

    //std::vector<std::string> text;
    //text.push_back("funny pet and nasty rat"s);
    //text.push_back("funny pet with curly hair"s);
    //PrintMatchDocumentResult(1, text, DocumentStatus::ACTUAL);

    report();
    // однопоточная версия
    search_server.RemoveDocument(5);
    report();
    // однопоточная версия
    search_server.RemoveDocument(std::execution::seq, 1);
    report();
    // многопоточная версия
    search_server.RemoveDocument(std::execution::par, 2);
    report();

    return 0;
}