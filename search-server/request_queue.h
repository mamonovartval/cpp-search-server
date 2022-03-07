#pragma once
#include "search_server.h"

#include <queue>

class RequestQueue
{
public:
	RequestQueue(const SearchServer& search_server);
	// сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
	template <typename DocumentPredicate>
	std::vector<Document> AddFindRequest(const std::string& raw_query,
		DocumentPredicate document_predicate);

	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

	std::vector<Document> AddFindRequest(const std::string& raw_query);

	int GetNoResultRequests() const;
private:
	struct QueryResult {
		bool isEmptyReply;
	};
	std::deque<QueryResult> requests_;
	const static int min_in_day_ = 1440;
	const SearchServer& search_server_;
	int countQuery{ 0 };

	void ManageQuery(const std::vector<Document>& doc);
};

//implementation templates
template<typename DocumentPredicate>
inline std::vector<Document> RequestQueue::AddFindRequest(
	const std::string & raw_query, DocumentPredicate document_predicate)
{
	const auto searchedDocument = search_server_.FindTopDocuments(raw_query, document_predicate);
	ManageQuery(searchedDocument);
	return searchedDocument;
}
