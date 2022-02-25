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

#pragma region Модульные тесты поисковой системы

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent();

double ComputeTermFreq(const string& term, const map<string, size_t>& all_words);

double ComputeInverseDocumentFreq(const string& term, const vector<map<string, size_t>>& documents);

//- Добавление документов.
// Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void Test_FindAddedDocumentByWordsQuery();

//- Поддержка минус-слов.
// Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void Test_QueryDoesntFindMinusWords();

//- Матчинг документов.
// При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса,
// присутствующие в документе.
// Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void Test_FindWordsOfQueryMathedWithDocument();

//- Сортировка найденных документов по релевантности.
// Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void Test_SortFinedDocument();

//- Вычисление рейтинга документов.
// Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void Test_RatingFinedDocument();

//- Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void Test_FindDocumentByUsersFilter();

//- Поиск документов, имеющих заданный статус.
void Test_FindDocumentByFilterStatus();

//- Корректное вычисление релевантности найденных документов.
void Test_CorrectEvalRelevance();

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();

// --------- Окончание модульных тестов поисковой системы -----------

#pragma endregion