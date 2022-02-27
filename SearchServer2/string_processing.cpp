#include "string_processing.h"

//std::vector<std::string> SplitIntoWords(const std::string& text) {
//    std::vector<std::string> words;
//    std::string word;
//    for (const char c : text) {
//        if (c == ' ') {
//            if (!word.empty()) {
//                words.push_back(word);
//                word.clear();
//            }
//        }
//        else {
//            word += c;
//        }
//    }
//    if (!word.empty()) {
//        words.push_back(word);
//    }
//    return words;
//}

//std::vector<std::string> SplitIntoWords(const std::string_view text) {
//    std::vector<std::string> words;
//    size_t word_start = 0;
//    for (size_t i = 0; i != text.size(); ++i) {
//        if (text[i] == ' ') {
//            if (word_start != i) {
//                words.push_back(std::string{ &text[word_start], i - word_start });
//            }
//            word_start = i + 1;
//        }
//    }
//    if (word_start != text.size()) {
//        words.push_back(std::string{ &text[word_start], text.size() - word_start });
//    }
//    return words;
//}

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> result;
    const int64_t pos_end = text.npos;
    while (true) {
        int64_t space = text.find(' ');
        auto word = (space == pos_end ? text.substr(0) : text.substr(0, space));
        if (!word.empty()) {
            result.push_back(word);
        }
        if (space == pos_end) {
            break;
        }
        else {
            text.remove_prefix(space + 1);
        }
    }
    return result;
}

//std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
//    std::vector<std::string_view> words;
//    std::string word;
//    for (const char c : text) {
//        if (c == ' ') {
//            if (!word.empty()) {
//                words.push_back(word);
//                word.clear();
//            }
//        }
//        else {
//            word += c;
//        }
//    }
//    if (!word.empty()) {
//        words.push_back(word);
//    }
//    return words;
//}