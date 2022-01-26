// search_server_s5_t4_v4.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include "test_example_functions.h"
#include "remove_duplicates.h"
#include "paginator.h"
#include "request_queue.h"

#include <cassert>

std::ostream& operator<<(std::ostream& out, const Document& document) {
	out << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating << " }"s;
	return out;
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range) {
	for (Iterator it = range.begin(); it != range.end(); ++it) {
		out << *it;
	}
	return out;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
	return Paginator(begin(c), end(c), page_size);
}

void test() {
	//1
	{
		SearchServer search_server("and in at"s);
		RequestQueue request_queue(search_server);

		search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
		search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
		search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

		// 1439 запросов с нулевым результатом
		for (int i = 0; i < 1439; ++i) {
			request_queue.AddFindRequest("empty request"s);
		}
		// все еще 1439 запросов с нулевым результатом
		request_queue.AddFindRequest("curly dog"s);
		// новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
		request_queue.AddFindRequest("big collar"s);
		// первый запрос удален, 1437 запросов с нулевым результатом
		request_queue.AddFindRequest("sparrow"s);
		assert(request_queue.GetNoResultRequests() == 1437);
	}
	//2
	{
		SearchServer search_server("and in at"s);
		RequestQueue request_queue(search_server);

		search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
		search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
		search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

		// 1450 запросов с ненулевым результатом
		for (int i = 0; i < 1450; ++i) {
			request_queue.AddFindRequest("curly dog"s);
		}
		assert(request_queue.GetNoResultRequests() == 0);
	}
	//3
	{
		SearchServer search_server("and in at"s);
		RequestQueue request_queue(search_server);

		search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
		search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
		search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

		// 1450 запросов с нулевым результатом
		for (int i = 0; i < 1450; ++i) {
			request_queue.AddFindRequest("empty request"s);
		}
		assert(request_queue.GetNoResultRequests() == 1440);
	}
	//4
	{
		SearchServer search_server("and in at"s);
		RequestQueue request_queue(search_server);

		search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
		search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
		search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

		// 10 запросов с ненулевым результатом
		for (int i = 0; i < 10; ++i) {
			request_queue.AddFindRequest("curly dog"s);
		}

		// 1430 запросов с нулевым результатом
		for (int i = 0; i < 1430; ++i) {
			request_queue.AddFindRequest("empty"s);
		}

		// 10 запросов с ненулевым результатом
		for (int i = 0; i < 10; ++i) {
			request_queue.AddFindRequest("curly cat"s);
		}
		
		assert(request_queue.GetNoResultRequests() == 1430);
	}
	//5
	{
		SearchServer search_server("and in at"s);
		RequestQueue request_queue(search_server);

		search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
		search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
		search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

		// 10 запросов с нулевым результатом
		for (int i = 0; i < 10; ++i) {
			request_queue.AddFindRequest("empty"s);
		}

		// 1430 запросов с ненулевым результатом
		for (int i = 0; i < 1430; ++i) {
			request_queue.AddFindRequest("curly cat"s);
		}

		// 10 запросов с нулевым результатом
		for (int i = 0; i < 10; ++i) {
			request_queue.AddFindRequest("empty"s);
		}
		assert(request_queue.GetNoResultRequests() == 10);
	}
	std::cout << "Test was done"s << std::endl;
}

int main() {
	test();
	SearchServer search_server("and with"s);

	AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

	// дубликат документа 2, будет удалён
	AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

	// отличие только в стоп-словах, считаем дубликатом
	AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

	// множество слов такое же, считаем дубликатом документа 1
	AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

	// добавились новые слова, дубликатом не является
	AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

	// множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
	AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

	// есть не все слова, не является дубликатом
	AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

	// слова из разных документов, не является дубликатом
	AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

	std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
	RemoveDuplicates(search_server);
	std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
}