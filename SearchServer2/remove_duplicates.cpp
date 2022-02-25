#include <cassert>
#include <iostream>

#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {

	// O(WN * log N), где W — максимальное количество слов в документе.
	// Дубликатами считаются документы, у которых наборы встречающихся слов совпадают.
	// Совпадение частот необязательно.
	// Порядок слов неважен, а стоп-слова игнорируются.
	// Функция должна использовать только доступные к этому моменту методы поискового сервера.
	// При обнаружении дублирующихся документов функция должна удалить документ с большим id из поискового сервера, и при этом сообщить id удалённого документа.
	// Удалять документы внутри цикла нельзя — это может привести к невалидности внутреннего итератора.

	std::set<std::set<std::string_view>> existing_docs;
	std::vector<int> found_duplicates;

	for (int document_id : search_server) {
		auto& freqs = search_server.GetWordFrequencies(document_id);
		std::set<std::string_view> words;

		std::transform(freqs.begin(), freqs.end(), std::inserter(words, words.begin()),
			[](auto p) {
				return p.first;
			});

		if (existing_docs.count(words) > 0) {
			std::cout << "Found duplicate document id " << document_id << std::endl;
			found_duplicates.push_back(document_id);
		}
		else {
			existing_docs.insert(words);
		}
	}

	for (int id : found_duplicates) {
		search_server.RemoveDocument(id);
	}
}