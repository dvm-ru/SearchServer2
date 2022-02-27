//https://pastebin.com/3mVY33Eu

#pragma once

#include <iostream>
#include <vector>
#include <string>

#include "document.h"
#include "paginator.h"
#include "search_server.h"
#include "utility.h"

using namespace std;

//ostream& operator<<(ostream& os, const DocumentStatus& ds) {
//    switch (ds) {
//    case DocumentStatus::ACTUAL: os << "ACTUAL"s; break;
//    case DocumentStatus::IRRELEVANT: os << "IRRELEVANT"s; break;
//    case DocumentStatus::BANNED: os << "BANNED"s; break;
//    case DocumentStatus::REMOVED: os << "REMOVED"s; break;
//    }
//    return os;
//}

template <typename A, typename F>
void RunTestImpl_(const A& func, const F& function_name) {

    func();
    cerr << function_name << " "s << "OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl_(func, #func)

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

//�������� ������
void AssertImplStrings(bool value, const string& expr_str, const string& file, const string& func, unsigned line, const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}
#define ASSERT(expr) AssertImplStrings(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImplStrings(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))



// ���� ���������, ��� ��������� ������� ��������� ����-����� ��� ���������� ����������
void TestExcludeStopWordsFromAddedDocumentContentStrings() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {//�������� ���������� ��������� � ���������� ��� �������.
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    {//�������� ����� ���� ����
        SearchServer server("in the"s);
        //        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

// ���� ��������� ������������ ���������� ���������� �������.
void TestSearchServerMatched() {
    const int doc_id = 0;
    const string content = "white cat and  funny collar"s;
    const vector<int> ratings = { 8, -3 };
    {
        //�������� ��� ���� ����
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status] = server.MatchDocument("flurry cat"s, doc_id);
        ASSERT(words[doc_id] == "cat"s);
        ASSERT(words[doc_id] != "flurry"s);
        ASSERT_EQUAL(words.size(), 1);
    }
    {
        //�������� �� ���� �������
        SearchServer server("cat"s);
        //        server.SetStopWords("cat"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status] = server.MatchDocument("flurry cat"s, doc_id);
        ASSERT_EQUAL(words.size(), 0);
    }
    {
        //�������� � ����� �������
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status] = server.MatchDocument("flurry -cat"s, doc_id);
        ASSERT_EQUAL(words.size(), 0); //��������, ��� ����� ������
    }

}

// �������� ���������� �� �������������
void TestSearchServerRelevanse() {
    SearchServer server("and in on"s);
    //    server.SetStopWords("and in on"s);
    server.AddDocument(0, "white cat and funny collar"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "flurry cat flurry tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "lucky dog good eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "lucky starling Eugene"s, DocumentStatus::BANNED, { 9 });

    const auto& documents = server.FindTopDocuments("flurry lucky cat"s);
    int doc_size = documents.size();
    ASSERT_EQUAL(doc_size, 3); //��������, ��� ����� �� ����� ��������� � ���������
    ASSERT_HINT(is_sorted(documents.begin(), documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        }), "Relevance not sorted correctly"s);
}

// �������� ������������ �������� ��������
void TestSearchServerRating() {
    //������� ������ � ����������
    vector<vector<int>> ratings = { {8, -3}, {7, 2, 7}, {5, -12, 2, 1}, {9} };
    //��������� �������� � �������� � ������
    map<int, int> rating_count;
    for (int i = 0; i < ratings.size(); ++i) {
        rating_count[i] = (accumulate(ratings[i].begin(), ratings[i].end(), 0) / ratings[i].size()); // ��������� ������� ������� � ����������� � id
    }

    SearchServer server(""s);
    //server.SetStopWords("and in on"s);
    server.AddDocument(0, "white cat and  funny collar"s, DocumentStatus::ACTUAL, ratings[0]); // ������� ����������� �� ������ 2
    server.AddDocument(1, "flurry cat flurry tail"s, DocumentStatus::ACTUAL, ratings[1]); // ������� ����������� �� ������ 5
    server.AddDocument(2, "lucky dog good eyes"s, DocumentStatus::ACTUAL, ratings[2]); // ������� ����������� �� ������ -1
    server.AddDocument(3, "lucky starling Eugene"s, DocumentStatus::BANNED, ratings[3]); // ������� 9

    const auto& documents = server.FindTopDocuments("flurry lucky cat"s);
    int doc_size = documents.size();
    for (int i = 0; i < doc_size; ++i) {
        ASSERT_HINT(documents[i].rating == rating_count[documents[i].id], "The rating is calculated incorrectly"s);
    }
}

//�������� ������ �� ������� ���������
void TestSearchServerStatus() {
    SearchServer server("and in on"s);
    //    server.SetStopWords("and in on"s);
    server.AddDocument(0, "white cat and  funny collar"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "flurry cat flurry tail"s, DocumentStatus::IRRELEVANT, { 7, 2, 7 });
    server.AddDocument(2, "lucky dog good eyes"s, DocumentStatus::BANNED, { 5, -12, 2, 1 });
    server.AddDocument(3, "lucky starling Eugene"s, DocumentStatus::REMOVED, { 9 });
    {// ������� ������� ������ ��������� �� �������� ACTUAL
        const auto& documents = server.FindTopDocuments("flurry lucky cat"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(documents.size(), 1);
        ASSERT(documents[0].id == 0);
    }
    {// ������� ������� ������ ��������� �� �������� IRRELEVANT
        const auto& documents = server.FindTopDocuments("flurry lucky cat"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(documents.size(), 1);
        ASSERT(documents[0].id == 1);
    }
    {// ������� ������� ������ ��������� �� �������� BANNED
        const auto& documents = server.FindTopDocuments("flurry lucky cat"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(documents.size(), 1);
        ASSERT(documents[0].id == 2);
    }
    {// ������� ������� ������ ��������� �� �������� REMOVED
        const auto& documents = server.FindTopDocuments("flurry lucky cat"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(documents.size(), 1);
        ASSERT(documents[0].id == 3);
    }
    {// ������� ������� ������ ��������� �� �������� �� ��������� (ACTUAL)
        const auto& documents = server.FindTopDocuments("flurry lucky cat"s);
        ASSERT_EQUAL(documents.size(), 1);
        ASSERT(documents[0].id == 0);
    }
}

void TestSearchServerPredictate() {
    SearchServer server("and in on"s);
    //    server.SetStopWords("and in on"s);
    server.AddDocument(0, "white cat and  funny collar"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "flurry cat flurry tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "lucky dog good eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "lucky starling Eugene"s, DocumentStatus::BANNED, { 9 });
    { //�������� id ���������
        const auto& documents = server.FindTopDocuments("flurry lucky cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        for (const auto& document : documents) {
            ASSERT_HINT(document.id % 2 == 0, ""s);
        }
    }
    { //�������� �������
        const auto& documents = server.FindTopDocuments("flurry lucky cat"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
        ASSERT_EQUAL(documents.size(), 3);
    }
    { //�������� ��������
        const auto& documents = server.FindTopDocuments("flurry lucky cat"s, [](int document_id, DocumentStatus status, int rating) { return rating > 3; });
        for (const auto& document : documents) {
            ASSERT_HINT(document.rating > 3, "Rating does not match"s);
        }
    }
}

void TestSearchServerMinus() {
    SearchServer server("and in on"s);
    //    server.SetStopWords("and in on"s);
    server.AddDocument(0, "white cat and  funny collar"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "flurry cat flurry tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "lucky dog good eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "lucky starling Eugene"s, DocumentStatus::ACTUAL, { 9 });
    {
        const auto& documents = server.FindTopDocuments("flurry -lucky -cat"s);
        ASSERT_EQUAL(documents.size(), 0);
    }
    {
        const auto& documents = server.FindTopDocuments("flurry lucky -cat"s);
        // ��������, ��� ������ ����� 2 ��������� id 2 � 3.
        ASSERT_EQUAL(documents.size(), 2);
        // ��������, ��� � ���������� � ��������� id ��� ����� ����� "cat", ���������� ������ ������ �� ��������� ����� �����
        for (const auto& document : documents) {
            const auto& [words, status] = server.MatchDocument("flurry lucky cat"s, document.id);
            for (const auto& word : words) {
                ASSERT(word != "cat"s);
            }
        }
    }

}

void TestSearchServerCalcRelevance() {
    SearchServer server("and in on"s);
    //    server.SetStopWords("and in on"s);
    server.AddDocument(0, "white cat and  funny collar"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "flurry cat flurry tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "lucky dog good eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "lucky starling Eugene"s, DocumentStatus::BANNED, { 9 });
    //���������� ��� ������� ���������� TF-IDF
    //��������� ��������� �� ��������� ����� ������ ���� �����
    //������ ������� �� ������������ ���� �� �������
    map<int, vector<string>> doc_for_tf_idf = { {0, {"white"s,"cat"s, "funny"s, "collar"s }},
                                               {1, {"flurry"s,"cat"s, "flurry"s, "tail"s }},
                                               {2, {"lucky"s,"dog"s, "good"s, "eyes"s }},
                                               {3, {"lucky"s,"starling"s, "Eugene"s }} };
    //��������� ������ �� ��������� �����
    map<int, string> query = { {0, "flurry"s}, {1, "lucky"s}, {2, "cat"s} };
    map<int, double> query_idf; // ���� id ����� �� �������
    map<int, map<int, double>> doc_tf;
    //��������� ����������� �� ����� � ��������� ��� ���, �� ����� ������� ���.
    //����� ������� IDF ��� ������� ����� �� ������� idf=log(���������� ���������� / ���������� ����������)
    //�������� ����� "flurry" ����������� � ����� ���������, ������ ��� idf=log(4/1)
    for (int i = 0; i < query.size(); ++i) {
        int freq_query_count = 0; //���������� ��� �������� � ���������� �����
        doc_tf[i];
        for (int j = 0; j < doc_for_tf_idf.size(); ++j) {
            //��������� ���� �� ����� � ���������, �� ����� ������� ���
            if (count(doc_for_tf_idf.at(j).begin(), doc_for_tf_idf.at(j).end(), query.at(i))) {
                ++freq_query_count;
            }
            //��������� tf, ���� ����� �����. � ���������� ��������, ���� ����� ���������
            doc_tf.at(i)[j] = count(doc_for_tf_idf.at(j).begin(), doc_for_tf_idf.at(j).end(), query.at(i)) * 1.0 / doc_for_tf_idf.size();
        }
        //��������� idf, ���� ����� �����
        query_idf[i] = log(doc_for_tf_idf.size() / freq_query_count);
    }
    //����������� tf � idf � ���������� �� ����������, ���� ����� ���������
    map<int, double> tf_idf;
    for (int j = 0; j < doc_for_tf_idf.size(); ++j) {
        for (int i = 0; i < query_idf.size(); ++i) {
            tf_idf[j] += doc_tf.at(i).at(j) * query_idf.at(i);
        }
    }

    const auto& documents = server.FindTopDocuments("flurry lucky cat"s);
    for (const Document& document : documents) {
        double diff = abs(document.relevance - tf_idf.at(document.id));
        cout << document.id << " : "s << diff << endl;
        ASSERT(abs(document.relevance - tf_idf.at(document.id)) < 1e-6);
    }
}



// ������� TestSearchServer �������� ������ ����� ��� ������� ������
void TestSearchServerStrings() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContentStrings);
    RUN_TEST(TestSearchServerMatched);
    RUN_TEST(TestSearchServerRelevanse);
    RUN_TEST(TestSearchServerRating);
    RUN_TEST(TestSearchServerStatus);
    RUN_TEST(TestSearchServerPredictate);
    RUN_TEST(TestSearchServerMinus);
    RUN_TEST(TestSearchServerCalcRelevance);

}


int main_print_docs() {
    SearchServer search_server("and in on"s);
    //    search_server.SetStopWords("and in on"s);

    search_server.AddDocument(0, "white cat and  funny collar"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "flurry cat flurry tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "lucky dog good eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "lucky starling Eugene"s, DocumentStatus::BANNED, { 9 });

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("flurry lucky cat"s)) {
        PrintDocument(document);
    }

    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("flurry lucky cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("flurry lucky cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    cout << endl;

    TestSearchServerStrings();
    // ���� �� ������ ��� ������, ������ ��� ����� ������ �������
    cout << "Search server testing finished"s << endl;


    return 0;
}