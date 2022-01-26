#pragma once

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
	const std::string& func, unsigned line, const std::string& hint) {
	if (t != u) {
		std::cout << std::boolalpha;
		std::cout << file << "("s << line << "): "s << func << ": "s;
		std::cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		std::cout << t << " != "s << u << "."s;
		if (!hint.empty()) {
			std::cout << " Hint: "s << hint;
		}
		std::cout << std::endl;
		abort();
	}
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
	const std::string& hint) {
	if (!value) {
		std::cout << file << "("s << line << "): "s << func << ": "s;
		std::cout << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty()) {
			std::cout << " Hint: "s << hint;
		}
		std::cout << std::endl;
		abort();
	}
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T, typename U>
void RunTestImpl(const T& t, const U& u) {
	t();
	std::cerr << u << " OK"s << std::endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)


/*// -------- Начало модульных тестов поисковой системы ----------

void TestExludeRequestQueue() {
	
	{
		//1. 1450 не пустых запросов, ожидаемое значение пустых запросов за последние сутки 0
		SearchServer search_server("and in at"s);
		RequestQueue request_queue(search_server);

		search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
		search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
		search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

		// 1450 запросов с ненулевым результатом
		for (int i = 0; i < 1450; ++i) {
			request_queue.AddFindRequest("curly dog"s); //"empty request"
		}
		ASSERT_EQUAL(request_queue.GetNoResultRequests(), 0);
	}

	{
		//2. 1450 пустых, ожидаемое значение пустых запросов за последние сутки 1440
		SearchServer search_server("and in at"s);
		RequestQueue request_queue(search_server);

		search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
		search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
		search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

		// 1450 запросов с нулевым результатом
		for (int i = 0; i < 1450; ++i) {
			request_queue.AddFindRequest("empty"s); //"empty request"
		}
		ASSERT_EQUAL(request_queue.GetNoResultRequests(), 1440);
	}

	{
		// 3. 10 не пустых, затем 1430 пустых, затем 10 не пустых - за последние сутки 1430 пустых
		SearchServer search_server("and in at"s);
		RequestQueue request_queue(search_server);

		search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
		search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
		search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

		// 10 запросов с ненулевым результатом
		for (int i = 0; i < 10; ++i) {
			request_queue.AddFindRequest("curly dog"s); //"empty request"
		}
		// 1430 запросов с нулевым результатом
		for (int i = 0; i < 1430; ++i) {
			request_queue.AddFindRequest("empty"s); //"empty request"
		}
		// 10 запросов с ненулевым результатом
		for (int i = 0; i < 10; ++i) {
			request_queue.AddFindRequest("curly cat"s); //"empty request"
		}

		ASSERT_EQUAL(request_queue.GetNoResultRequests(), 1430);
	}

	{
		//4. 10 пустых, затем 1430 не пустых, затем 10 пустых - за последние сутки 10 пустых
		SearchServer search_server("and in at"s);
		RequestQueue request_queue(search_server);

		search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
		search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
		search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

		// 10 запросов с нулевым результатом
		for (int i = 0; i < 10; ++i) {
			request_queue.AddFindRequest("empty"s); //"empty request"
		}
		// 1430 запросов с нулевым результатом
		for (int i = 0; i < 1430; ++i) {
			request_queue.AddFindRequest("curly cat"s); //"empty request"
		}
		// 10 запросов с нулевым результатом
		for (int i = 0; i < 10; ++i) {
			request_queue.AddFindRequest("empty"s); //"empty request"
		}

		ASSERT_EQUAL(request_queue.GetNoResultRequests(), 10);
	}
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
	RUN_TEST(TestExludeRequestQueue);
}

// --------- Окончание модульных тестов поисковой системы -----------*/