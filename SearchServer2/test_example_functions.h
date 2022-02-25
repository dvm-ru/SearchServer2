#pragma once

#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "document.h"

using namespace std;

#pragma region MACROS ASSERT

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file, const string& func, unsigned line, const string& hint) {
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

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line, const string& hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename TestFunc>
void RunTestImpl(TestFunc test_func, const string& test_name, const string& file, const string& func, unsigned line, const string& hint) {
	try {
		test_func();
		cerr << test_name << " OK"s << endl;
	}
	catch (exception& e) {
		cerr << file << "("s << line << "): "s << func << ": "s;
		cerr << test_name << " fail: " << e.what() << endl;
		if (!hint.empty()) {
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
	}
	catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}
#define RUN_TEST(func) RunTestImpl((func), #func, __FILE__, __FUNCTION__, __LINE__, ""s)

#pragma endregion

#pragma region ��������� ����� ��������� �������

// -------- ������ ��������� ������ ��������� ������� ----------

// ���� ���������, ��� ��������� ������� ��������� ����-����� ��� ���������� ����������
void TestExcludeStopWordsFromAddedDocumentContent();

double ComputeTermFreq(const string& term, const map<string, size_t>& all_words);

double ComputeInverseDocumentFreq(const string& term, const vector<map<string, size_t>>& documents);

//- ���������� ����������.
// ����������� �������� ������ ���������� �� ���������� �������, ������� �������� ����� �� ���������.
void Test_FindAddedDocumentByWordsQuery();

//- ��������� �����-����.
// ���������, ���������� �����-����� ���������� �������, �� ������ ���������� � ���������� ������.
void Test_QueryDoesntFindMinusWords();

//- ������� ����������.
// ��� �������� ��������� �� ���������� ������� ������ ���� ���������� ��� ����� �� ���������� �������,
// �������������� � ���������.
// ���� ���� ������������ ���� �� �� ������ �����-�����, ������ ������������ ������ ������ ����.
void Test_FindWordsOfQueryMathedWithDocument();

//- ���������� ��������� ���������� �� �������������.
// ������������ ��� ������ ���������� ���������� ������ ���� ������������� � ������� �������� �������������.
void Test_SortFinedDocument();

//- ���������� �������� ����������.
// ������� ������������ ��������� ����� �������� ��������������� ������ ���������.
void Test_RatingFinedDocument();

//- ���������� ����������� ������ � �������������� ���������, ����������� �������������.
void Test_FindDocumentByUsersFilter();

//- ����� ����������, ������� �������� ������.
void Test_FindDocumentByFilterStatus();

//- ���������� ���������� ������������� ��������� ����������.
void Test_CorrectEvalRelevance();

// ������� TestSearchServer �������� ������ ����� ��� ������� ������
void TestSearchServer();

// --------- ��������� ��������� ������ ��������� ������� -----------

#pragma endregion