#include "search_server.h"

#include <numeric>
#include <algorithm>
#include <execution>

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
	if ((document_id < 0) || (documents_.count(document_id) > 0)) {
		throw std::invalid_argument("Invalid document_id"s);
	}

	const auto [it, inserted] = documents_.emplace(document_id,
		DocumentData{ ComputeAverageRating(ratings), status, std::string(document) });
	
	const auto words = SplitIntoWordsNoStop(it->second.data);

	const double inv_word_count = 1.0 / words.size();
	for (std::string_view word : words) {
		word_to_document_freqs_[word][document_id] += inv_word_count;
		document_to_word_freqs_[document_id][word] += inv_word_count;
	}
	document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(
	std::string_view raw_query, DocumentStatus status) const 
{
	return FindTopDocuments(raw_query,
		[status](int document_id, DocumentStatus document_status, int rating) {
		return document_status == status;
	});
}

std::vector<Document> SearchServer::FindTopDocuments(
	const std::execution::parallel_policy& policy,
	std::string_view raw_query, DocumentStatus status) const
{
	return FindTopDocuments(policy, raw_query,
		[status](int document_id, DocumentStatus document_status, int rating) 
	{
		return document_status == status;
	});
}

std::vector<Document> SearchServer::FindTopDocuments(
	const std::execution::sequenced_policy& policy,
	std::string_view raw_query, DocumentStatus status) const
{
	return FindTopDocuments(raw_query,[status]
	(int document_id, DocumentStatus document_status, int rating) 
	{
		return document_status == status;
	});
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const
{
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(
	const std::execution::parallel_policy& policy,
	std::string_view raw_query) const
{
	return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(
	const std::execution::sequenced_policy& policy,
	std::string_view raw_query) const
{
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
	return documents_.size();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
	static std::map<std::string_view, double> emptyRes;
	if (document_to_word_freqs_.count(document_id)) {
		return document_to_word_freqs_.at(document_id);
	}
	else {
		return emptyRes;
	}
}

void SearchServer::RemoveDocument(int document_id) {
	RemoveDocument(std::execution::seq, document_id);
}

const std::set<int>::const_iterator SearchServer::begin() const
{
	return document_ids_.cbegin();
}

const std::set<int>::const_iterator SearchServer::end() const
{
	return document_ids_.cend();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
	 const std::string_view raw_query, int document_id) const
{
	return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
	const std::execution::parallel_policy & policy, std::string_view raw_query,
	int document_id) const
{
	const auto query = ParseQueryNoUniq(raw_query);
	std::vector<std::string_view> zero{};
	std::vector<std::string_view> matched_words(query.plus_words.size());
	// verifying that there are no minus word
	if (std::any_of(query.minus_words.begin(), query.minus_words.end(),
		[this, &document_id](std::string_view word) {
		return word_to_document_freqs_.at(word).count(document_id);
	})) 
	{
		return { zero , documents_.at(document_id).status };
	}

	// copy matching words plus if there are no word minus
	auto end_vector = std::copy_if(policy,
		query.plus_words.begin(),
		query.plus_words.end(),
		matched_words.begin(),
		[this, &document_id](std::string_view word) {
		return 	word_to_document_freqs_.at(word).count(document_id);
	});
	
	// make sort
	std::sort(matched_words.begin(), end_vector);

	// apply std::unique which remove consequitive equal elements
	auto last = std::unique(matched_words.begin(), end_vector);

	// resize vectors by new size
	size_t newSize = last - matched_words.begin();
	matched_words.resize(newSize);

	return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
	const std::execution::sequenced_policy & policy, std::string_view raw_query,
	int document_id) const
{
	const auto query = ParseQuery(raw_query);

	std::vector<std::string_view> matched_words;
	std::vector<std::string_view> zero;
	for (std::string_view word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			return { zero, documents_.at(document_id).status };
		}
	}

	for (std::string_view word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(word);
		}
	}
	return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(std::string_view word) const
{
	return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word)
{
	// A valid word must not contain special characters
	return std::none_of(word.begin(), word.end(), [](char c) {
		return c >= '\0' && c < ' ';
	});
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(
	std::string_view text) const
{
	std::vector<std::string_view> words;
	for (auto word : SplitIntoWords(text)) {
		if (!IsValidWord(word)) {
			throw std::invalid_argument("Word "s + word.data() + " is invalid"s);
		}
		if (!IsStopWord(word)) {
			words.push_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
	if (ratings.empty()) {
		return 0;
	}
	int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
	return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const
{
	if (text.empty()) {
		throw std::invalid_argument("Query word is empty"s);
	}
	std::string_view word = text;
	bool is_minus = false;
	if (word[0] == '-') {
		is_minus = true;
		word = word.substr(1);
	}
	if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
		throw std::invalid_argument("Query word "s + word.data() + " is invalid"s);
	}

	return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text) const
{
	Query result;
	for (std::string_view word : SplitIntoWords(text)) {
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
	// make sort for minus and plus words
	std::sort(result.minus_words.begin(), result.minus_words.end());
	std::sort(result.plus_words.begin(), result.plus_words.end());

	// apply std::unique which remove consequitive equal elements
	auto last_minus = std::unique(result.minus_words.begin(), result.minus_words.end());
	auto last_plus = std::unique(result.plus_words.begin(), result.plus_words.end());
	
	// resize vectors by new size
	size_t newSize = last_minus - result.minus_words.begin();
	result.minus_words.resize(newSize);

	newSize = last_plus - result.plus_words.begin();
	result.plus_words.resize(newSize);

	return result;
}

SearchServer::Query SearchServer::ParseQueryNoUniq(std::string_view text) const
{
	Query result;

	for (std::string_view word : SplitIntoWords(text)) {
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
	return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const
{
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
