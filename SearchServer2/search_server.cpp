#include "search_server.h"
#include "string_processing.h"

#include <execution>
#include <mutex>

//#include "log_duration.h"

using namespace std::literals::string_literals;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }

    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const auto& s_word : words) {
        std::string word(s_word.begin(), s_word.end());
        word_to_document_freqs_[word][document_id] += inv_word_count;
        doc_id_to_words_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}

SearchServer::MatchDocumentResult SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {

    const auto query = ParseQuery(raw_query);

    std::vector<std::string> matched_words;
    for (const auto& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const auto& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }

    //std::sort(matched_words.begin(), matched_words.end());
    //auto last = std::unique(matched_words.begin(), matched_words.end());
    //matched_words.erase(last, matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}

std::set<int>::const_iterator SearchServer::begin() {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() {
    return document_ids_.end();
}

//Ìåòîä ïîëó÷åíèÿ ÷àñòîò ñëîâ ïî id äîêóìåíòà
const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (doc_id_to_words_freqs_.count(document_id)) {
        return doc_id_to_words_freqs_.at(document_id);
    }
    static const std::map<std::string, double> dummy_;
    return dummy_;
}

//Ìåòîä óäàëåíèÿ äîêóìåíòîâ èç ïîèñêîâîãî ñåðâåðà
void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    if (document_ids_.count(document_id) == 1) {
        for (const auto [search_word, _] : doc_id_to_words_freqs_[document_id]) {
            std::string found_word = static_cast<std::string>(search_word);
            word_to_document_freqs_[found_word].erase(document_id);
            if (word_to_document_freqs_[found_word].empty()) {
                word_to_document_freqs_.erase(found_word);
            }
        }
        document_ids_.erase(document_id);
        documents_.erase(document_id);
        doc_id_to_words_freqs_.erase(document_id);
    }
}

////Âû ìîæåòå ñòîëêíóòüñÿ ñ òåì, ÷òî íóæíûé àëãîðèòì íå ïàðàëëåëèòñÿ, êîãäà ïåðåäà¸òå â íåãî èòåðàòîðû íå ïðîèçâîëüíîãî äîñòóïà.
//Ïîïðîáóéòå ïåðåëîæèòü íóæíûå ýëåìåíòû â âåêòîð è çàïóñòèòü àëãîðèòì äëÿ íåãî.
void SearchServer::RemoveDocument(const std::execution::parallel_policy& police, int document_id) {
    if (!documents_.count(document_id)) return;
    const auto& word_freqs = doc_id_to_words_freqs_.at(document_id);
    std::vector<std::string> words(word_freqs.size());
    std::transform(
        std::execution::par,
        word_freqs.begin(), word_freqs.end(),
        words.begin(),
        [](const auto& item) { return item.first; }
    );
    std::for_each(
        std::execution::par,
        words.begin(), words.end(),
        [this, document_id](const auto& word) {
            word_to_document_freqs_.at(word).erase(document_id);
        });
    document_ids_.erase(document_id);
    documents_.erase(document_id);
    doc_id_to_words_freqs_.erase(document_id);
}


//bool SearchServer::IsStopWord(const std::string& word) const {
//    std::string s_word{ word };
//    return stop_words_.count(s_word) > 0;
//}
bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string> words;
    for (const auto word : SplitIntoWords(text)) {
        //std::string s_word(word.begin(), word.end());
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + static_cast<std::string>(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(static_cast<std::string>(word));
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {
    std::string word = static_cast<std::string>(text);
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + word + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    Query result;
    std::vector<std::string_view> words = SplitIntoWords(text);

    std::sort(words.begin(), words.end());
    auto last = std::unique(words.begin(), words.end());
    words.erase(last, words.end());

    result.minus_words.reserve(words.size());
    result.plus_words.reserve(words.size());

    for (const auto& word : words) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    //std::sort(result.minus_words.begin(), result.minus_words.end());
    //auto last2 = std::unique(result.minus_words.begin(), result.minus_words.end());
    //result.minus_words.erase(last2, result.minus_words.end());

    //std::sort(result.plus_words.begin(), result.plus_words.end());
    //auto last3 = std::unique(result.plus_words.begin(), result.plus_words.end());
    //result.plus_words.erase(last3, result.plus_words.end());

    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::sequenced_policy& seq, std::string_view text) const {
    Query result;
    std::vector<std::string_view> words = SplitIntoWords(text);

    std::sort(seq, words.begin(), words.end());
    auto last = std::unique(seq, words.begin(), words.end());
    words.erase(last, words.end());

    result.minus_words.reserve(words.size());
    result.plus_words.reserve(words.size());

    for (const auto& word : words) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    //std::sort(seq, result.minus_words.begin(), result.minus_words.end());
    //auto last2 = std::unique(seq, result.minus_words.begin(), result.minus_words.end());
    //result.minus_words.erase(last2, result.minus_words.end());

    //std::sort(seq, result.plus_words.begin(), result.plus_words.end());
    //auto last3 = std::unique(seq, result.plus_words.begin(), result.plus_words.end());
    //result.plus_words.erase(last3, result.plus_words.end());

    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy& par, std::string_view text) const {
    Query result;
    std::vector<std::string_view> words = SplitIntoWords(text);

    std::sort(par, words.begin(), words.end());
    auto last = std::unique(par, words.begin(), words.end());
    words.erase(last, words.end());

    result.minus_words.reserve(words.size());
    result.plus_words.reserve(words.size());

    for (const auto& word : words) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    //std::sort(par, result.minus_words.begin(), result.minus_words.end());
    //auto last2 = std::unique(par, result.minus_words.begin(), result.minus_words.end());
    //result.minus_words.erase(last2, result.minus_words.end());

    //std::sort(par, result.plus_words.begin(), result.plus_words.end());
    //auto last3 = std::unique(par, result.plus_words.begin(), result.plus_words.end());
    //result.plus_words.erase(last3, result.plus_words.end());

    return result;
}

//double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
//    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
//}
double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(static_cast<std::string>(word)).size());
}