#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer & search_server)
	: search_server_(search_server) {}

std::vector<Document> RequestQueue::AddFindRequest(const std::string & raw_query, DocumentStatus status)
{
	auto searchedDocument = search_server_.FindTopDocuments(raw_query, status);
	ManageQuery(searchedDocument);
	return searchedDocument;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string & raw_query)
{
	auto searchedDocument = search_server_.FindTopDocuments(raw_query);
	ManageQuery(searchedDocument);
	return searchedDocument;
}

int RequestQueue::GetNoResultRequests() const
{
	return countQuery;
}

void RequestQueue::ManageQuery(const std::vector<Document>& doc)
{
	QueryResult result;
	result.isEmptyReply = (doc.empty());

	if (requests_.size() >= min_in_day_) {
		if (requests_.front().isEmptyReply == true) {
			--countQuery;
		}
		requests_.pop_front();
	}

	if (result.isEmptyReply) {
		++countQuery;
	}
	requests_.push_back(result);
}