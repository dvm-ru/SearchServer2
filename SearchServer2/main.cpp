#ifdef AVRGR
//Задание 3
//Это задание — часть итогового проекта восьмого спринта.
// Вы будете сдавать его на проверку через репозиторий на GitHub.
// Не забудьте сохранить верное решение.

//Иногда ускорение требует существенной переработки кода программы.
// Сейчас — тот самый случай.
// Перейдите с использования класса string на string_view там, где это возможно, и тем самым ускорьте программу.

//Следующие методы теперь должны позволять принять string_view вместо строки:
//- конструктор;
//- AddDocument;
//- FindTopDocuments;
//- MatchDocument.
//Эти методы должны возвращать string_view вместо строк:
//- MatchDocument;
//- GetWordFrequencies.

//Ограничения
//В метод MatchDocument может быть передан некорректный поисковый запрос — в этом случае должно быть выброшено исключение std::invalid_argument.
//Если передан несуществующий document_id, ожидается исключение std::out_of_range.

//Что отправлять на проверку
//Заголовочные файлы и файлы с реализацией, содержащие класс SearchServer и написанные ранее вспомогательные функции.
//Функция main будет проигнорирована.

//Как будет тестироваться ваш код
//Правильность работы метода будет проверена юнит-тестами.
//Производительность всех версий метода будет проверена в таком тестовом сценарии.
// Дана поисковая система с характеристиками:
//- 10 000 документов, не более 70 слов в каждом;
//- не более 500 слов в поисковом запросе, включая минус-слова;
//- все слова — из словаря, состоящего из 1 000 слов длиной не более 10 букв.

//Дан поисковый запрос, состоящий из не более 500 слов, включая минус-слова.
//Последовательно для всех документов вызывается MatchDocument с этим запросом.
// Измеряется общее время работы цикла вызовов.

//Время работы ВАШЕЙ многопоточной версии должно быть по крайней мере вдвое меньше, чем время работы ВАШЕЙ однопоточной версии В ОБОИХ вариантах вызова.
//Время работы ВАШЕЙ однопоточной версии не должно превышать время работы однопоточной версии АВТОРСКОГО решения больше, чем в полтора раза.
//Время работы ВАШЕЙ многопоточной версии не должно превышать время работы многопоточной версии АВТОРСКОГО решения больше, чем в полтора раза.

//Подсказка
//Помните: string_view не владеет строкой.
// Поэтому там, где нужно, используйте string или ссылайтесь на копию переданных в метод данных.
//Вы можете искать string_view во множестве строк set<string> без создания временной строки,
//но для этого вам нужно явно в самом типе множества указать специальный компаратор: set<string, less<>>.
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
