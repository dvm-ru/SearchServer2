#ifdef AVRGR
//Задание 2
//Это задание — часть итогового проекта восьмого спринта.
// Вы будете сдавать его на проверку через репозиторий на GitHub.
// Не забудьте сохранить верное решение.
//Реализуйте многопоточную версию метода MatchDocument в дополнение к однопоточной.

//Пример
//#include "search_server.h"
//
//#include <iostream>
//#include <string>
//#include <vector>
//
//using namespace std;
//
//int main() {
//    SearchServer search_server("and with"s);
//
//    int id = 0;
//    for (
//        const string& text : {
//            "funny pet and nasty rat"s,
//            "funny pet with curly hair"s,
//            "funny pet and not very nasty rat"s,
//            "pet with rat and rat and rat"s,
//            "nasty rat with curly hair"s,
//        }
//        ) {
//        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
//    }
//
//    const string query = "curly and funny -not"s;
//
//    {
//        const auto [words, status] = search_server.MatchDocument(query, 1);
//        cout << words.size() << " words for document 1"s << endl;
//        // 1 words for document 1
//    }
//
//    {
//        const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
//        cout << words.size() << " words for document 2"s << endl;
//        // 2 words for document 2
//    }
//
//    {
//        const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
//        cout << words.size() << " words for document 3"s << endl;
//        // 0 words for document 3
//    }
//
//    return 0;
//}

//Вывод:
//1 words for document 1
//2 words for document 2
//0 words for document 3

//Ограничения
//- Как и прежде, в метод MatchDocument может быть передан некорректный поисковый запрос — в этом случае должно быть выброшено исключение std::invalid_argument.
//- Если передан несуществующий document_id, ожидается исключение std::out_of_range.

//Что отправлять на проверку
//Заголовочные файлы и файлы с реализацией, содержащие класс SearchServer и написанные ранее вспомогательные функции.
// Функция main будет проигнорирована.

//Как будет тестироваться ваш код
//Правильность работы метода будет проверена юнит-тестами.
//Производительность всех версий метода будет проверена в таком тестовом сценарии.
// Дана поисковая система с характеристиками:
//- 10 000 документов, не более 70 слов в каждом;
//- не более 500 слов в поисковом запросе, включая минус-слова;
//- все слова — из словаря, состоящего из 1 000 слов длиной не более 10 букв.
//- Дан поисковый запрос, состоящий из не более 500 слов, включая минус-слова.
//- Последовательно для всех документов вызывается MatchDocument с этим запросом.
// Измеряется общее время работы цикла вызовов.
//- Для многопоточной версии вашего метода это время должно быть по крайней мере на 30% меньше, чем у авторской однопоточной версии.
//- Также это время должно быть по крайней мере на 30% меньше, чем у вашей однопоточной версии в обоих вариантах вызова.

//Вы можете ориентироваться на следующий бенчмарк:
//#include "search_server.h"
//
//#include <execution>
//#include <iostream>
//#include <random>
//#include <string>
//#include <vector>
//
//#include "log_duration.h"
//
//using namespace std;
//
//string GenerateWord(mt19937 & generator, int max_length) {
//    const int length = uniform_int_distribution(1, max_length)(generator);
//    string word;
//    word.reserve(length);
//    for (int i = 0; i < length; ++i) {
//        word.push_back(uniform_int_distribution('a', 'z')(generator));
//    }
//    return word;
//}
//
//vector<string> GenerateDictionary(mt19937 & generator, int word_count, int max_length) {
//    vector<string> words;
//    words.reserve(word_count);
//    for (int i = 0; i < word_count; ++i) {
//        words.push_back(GenerateWord(generator, max_length));
//    }
//    sort(words.begin(), words.end());
//    words.erase(unique(words.begin(), words.end()), words.end());
//    return words;
//}
//
//string GenerateQuery(mt19937 & generator, const vector<string>&dictionary, int word_count, double minus_prob = 0) {
//    string query;
//    for (int i = 0; i < word_count; ++i) {
//        if (!query.empty()) {
//            query.push_back(' ');
//        }
//        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
//            query.push_back('-');
//        }
//        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
//    }
//    return query;
//}
//
//vector<string> GenerateQueries(mt19937 & generator, const vector<string>&dictionary, int query_count, int max_word_count) {
//    vector<string> queries;
//    queries.reserve(query_count);
//    for (int i = 0; i < query_count; ++i) {
//        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
//    }
//    return queries;
//}
//
//template <typename ExecutionPolicy>
//void Test(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
//    LOG_DURATION(mark);
//    const int document_count = search_server.GetDocumentCount();
//    int word_count = 0;
//    for (int id = 0; id < document_count; ++id) {
//        const auto [words, status] = search_server.MatchDocument(policy, query, id);
//        word_count += words.size();
//    }
//    cout << word_count << endl;
//}
//
//#define TEST(policy) Test(#policy, search_server, query, execution::policy)
//
//int main() {
//    mt19937 generator;
//
//    const auto dictionary = GenerateDictionary(generator, 1000, 10);
//    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);
//
//    const string query = GenerateQuery(generator, dictionary, 500, 0.1);
//
//    SearchServer search_server(dictionary[0]);
//    for (size_t i = 0; i < documents.size(); ++i) {
//        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
//    }
//
//    TEST(seq);
//    TEST(par);
//}

//Подсказка
//Иногда для получения эффективного параллельного кода недостаточно поменять код самого метода.
// Приходится вносить изменения в другие методы, от которых зависит оптимизируемый.
// Скорее всего, здесь как раз тот случай.
//Не любое ускорение означает именно распараллеливание.
//Вероятно, вам поможет взглянуть на задачу чуть шире — как на приведение в соответствие бенчмарку.
#endif // AVRGR

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

#include "task_RemoveDuplicates.h"
#include "task_ProcessQueries.h"
#include "benchmark_ProcessQueries.h"
#include "task_ProcessQueriesJoined.h"
#include "task_1_of_3_RemoveDocument.h"
#include "task_2_of_3_MatchDocument.h"
#include "benchmark_MatchDocument.h"



using namespace std::literals::string_literals;
using namespace std;

int main() {

	benchmark_MatchDocument();
	task_2_of_3_MatchDocument();

	//task_1_of_3_RemoveDocument();

	//task_ProcessQueriesJoined();

	//benchmark_ProcessQueries();
	//task_ProcessQueries();

	//task_RemoveDuplicates();
}
