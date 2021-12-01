// search_server_s1_t2_v2.cpp

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <map>
#include <numeric>
#include <sstream>

using namespace std;

template <typename V>
ostream& operator<<(ostream& os, const vector<V>& v) {
	bool flag{ false };
	for (const auto& content : v) {
		if (!flag) {
			os << "["s << content;
			flag = true;
		}
		else if (flag) {
			os << ", "s << content;
		}
	}
	os << "]"s;
	return os;
}

template <typename S>
ostream& operator<<(ostream& os, const set<S>& s) {
	bool flag{ false };
	for (const auto& content : s) {
		if (!flag) {
			os << "{"s << content;
			flag = true;
		}
		else if (flag) {
			os << ", "s << content;
		}
	}
	os << "}"s;
	return os;
}

template <typename K, typename V>
ostream& operator<<(ostream& os, const map<K, V>& m) {
	bool flag{ false };
	for (const auto&[key, value] : m) {
		if (!flag) {
			os << "{"s << key << ": "s << value;
			flag = true;
		}
		else if (flag) {
			os << ", "s << key << ": "s << value;
		}
	}
	os << "}"s;
	return os;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
	const string& func, unsigned line, const string& hint) {
	if (t != u) {
		cout << boolalpha;
		cout << file << "("s << line << "): "s << func << ": "s;
		cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		cout << t << " != "s << u << "."s;
		if (!hint.empty()) {
			cout << " Hint: "s << hint;
		}
		cout << endl;
		abort();
	}
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
	const string& hint) {
	if (!value) {
		cout << file << "("s << line << "): "s << func << ": "s;
		cout << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty()) {
			cout << " Hint: "s << hint;
		}
		cout << endl;
		abort();
	}
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T, typename U>
void RunTestImpl(const T& t, const U& u) {
	t();
	cerr << u << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
	string s;
	getline(cin, s);
	return s;
}

int ReadLineWithNumber() {
	int result;
	cin >> result;
	ReadLine();
	return result;
}

vector<string> SplitIntoWords(const string& text) {
	vector<string> words;
	string word;
	for (const char c : text) {
		if (c == ' ') {
			if (!word.empty()) {
				words.push_back(word);
				word.clear();
			}
		}
		else {
			word += c;
		}
	}
	if (!word.empty()) {
		words.push_back(word);
	}

	return words;
}

struct Document {
	int id;
	double relevance;
	int rating;
};

enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

class SearchServer {
public:
	void SetStopWords(const string& text) {

		for (const string& word : SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {

		const vector<string> words = SplitIntoWordsNoStop(document);
		const double inv_word_count = 1.0 / words.size();
		for (const string& word : words) {
			word_to_document_freqs_[word][document_id] += inv_word_count;
		}
		documents_.emplace(document_id,
			DocumentData{
				ComputeAverageRating(ratings),
				status
			});
	}

	template <typename DocumentPredicate>
	vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate predicate) const {

		const Query query = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(query, predicate);

		sort(matched_documents.begin(), matched_documents.end(),
			[](const Document& lhs, const Document& rhs) {
			if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
				return lhs.rating > rhs.rating;
			}
			else {
				return lhs.relevance > rhs.relevance;
			}
		});

		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}


	vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {

		return FindTopDocuments(raw_query,
			[status](int document_id, DocumentStatus statuses, int raiting) {

			return status == statuses;
		});
	}

	vector<Document> FindTopDocuments(const string& raw_query) const {

		return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
	}

	int GetDocumentCount() const {

		return documents_.size();
	}

	tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {

		const Query query = ParseQuery(raw_query);
		vector<string> matched_words;
		for (const string& word : query.plus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			if (word_to_document_freqs_.at(word).count(document_id)) {
				matched_words.push_back(word);
			}
		}
		for (const string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			if (word_to_document_freqs_.at(word).count(document_id)) {
				matched_words.clear();
				break;
			}
		}
		return { matched_words, documents_.at(document_id).status };
	}

private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};

	set<string> stop_words_;
	map<string, map<int, double>> word_to_document_freqs_;
	map<int, DocumentData> documents_;

	bool IsStopWord(const string& word) const {
		return stop_words_.count(word) > 0;
	}

	vector<string> SplitIntoWordsNoStop(const string& text) const {
		vector<string> words;
		for (const string& word : SplitIntoWords(text)) {
			if (!IsStopWord(word)) {
				words.push_back(word);
			}
		}
		return words;
	}

	static int ComputeAverageRating(const vector<int>& ratings) {

		if (ratings.empty()) {
			return 0;
		}
		int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);

		return rating_sum / static_cast<int>(ratings.size());
	}

	struct QueryWord {
		string data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(string text) const {

		bool is_minus = false;
		// Word shouldn't be empty
		if (text[0] == '-') {
			is_minus = true;
			text = text.substr(1);
		}
		return {
			text,
			is_minus,
			IsStopWord(text)
		};
	}

	struct Query {
		set<string> plus_words;
		set<string> minus_words;
	};

	Query ParseQuery(const string& text) const {
		Query query;
		for (const string& word : SplitIntoWords(text)) {
			const QueryWord query_word = ParseQueryWord(word);
			if (!query_word.is_stop) {
				if (query_word.is_minus) {
					query.minus_words.insert(query_word.data);
				}
				else {
					query.plus_words.insert(query_word.data);
				}
			}
		}
		return query;
	}

	double ComputeWordInverseDocumentFreq(const string& word) const {
		return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
	}
	template <typename DocumentPredicate>
	vector<Document> FindAllDocuments(const Query& query, DocumentPredicate predicate) const {
		map<int, double> document_to_relevance;
		for (const string& word : query.plus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto[document_id, term_freq] : word_to_document_freqs_.at(word)) {
				if (predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
					document_to_relevance[document_id] += term_freq * inverse_document_freq;
				}
			}
		}

		for (const string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			for (const auto[document_id, _] : word_to_document_freqs_.at(word)) {
				document_to_relevance.erase(document_id);
			}
		}

		vector<Document> matched_documents;
		for (const auto[document_id, relevance] : document_to_relevance) {
			matched_documents.push_back({
				document_id,
				relevance,
				documents_.at(document_id).rating
				});
		}
		return matched_documents;
	}
};

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1u);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
		SearchServer server;
		server.SetStopWords("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
	}
}

// Тест проверяет, что документы, содержащие минус-слова поискового запроса, 
//не включаться в результаты поиска.
void TestExcludeMinusWordsFromSearchQuery() {
	// Убеждаемся, что поиск запроса, исключает минус-слово,
	// возвращает результат, который отличается от заданного количества документов
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
		server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		const auto found_docs = server.FindTopDocuments("fluffy wellgroomed cat -collar"s);
		ASSERT_EQUAL(found_docs.size(), 2u);
	}
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
		server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		const auto found_docs = server.FindTopDocuments("fluffy wellgroomed cat collar"s);
		ASSERT_EQUAL(found_docs.size(), 3u);
	}
}

// Тест проверяет, что возвращаются все соответствующие слова пир отсутствие минус-слов
void TestExcludedMatchingWordByMinusWord() {
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
		server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		const auto found_docs = server.MatchDocument("fluffy wellgroomed cat -collar"s, 0);
		ASSERT_EQUAL(get<vector<string>>(found_docs).size(), 0);
	}
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
		server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		const auto found_docs = server.MatchDocument("fluffy wellgroomed cat collar"s, 0);
		ASSERT_EQUAL(get<vector<string>>(found_docs).size(), 2);
	}
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
		server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		const auto found_docs = server.MatchDocument("fluffy wellgroomed cat -collar"s, 1);
		ASSERT_EQUAL(get<vector<string>>(found_docs).size(), 2);
	}
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
		server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		const auto found_docs = server.MatchDocument("fluffy wellgroomed cat collar"s, 1);
		ASSERT_EQUAL(get<vector<string>>(found_docs).size(), 2);
	}
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
		server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		const auto found_docs = server.MatchDocument("fluffy wellgroomed cat -collar"s, 2);
		ASSERT_EQUAL(get<vector<string>>(found_docs).size(), 1);
	}
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
		server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		const auto found_docs = server.MatchDocument("fluffy wellgroomed cat collar"s, 2);
		ASSERT_EQUAL(get<vector<string>>(found_docs).size(), 1);
	}
}

// Тест проверяет сортировку по релевантности
void TestExcludedUnsortedResultByRelevance() {
	SearchServer server;
	server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
	server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });

	const auto found_docs = server.FindTopDocuments("fluffy wellgroomed cat"s);
	const Document& doc0 = found_docs[0];
	const Document& doc1 = found_docs[1];
	const Document& doc2 = found_docs[2];
	ASSERT(doc0.relevance > doc1.relevance);
	ASSERT(doc1.relevance > doc2.relevance);
}

// Тест проверяет расчет среднего рейтинга
void TestExcludedWrongComputeAverageAmountRating() {
	const int ratings0 = (8 + (-3)) / 2;
	const int ratings1 = (7 + 2 + 7) / 3;
	const int ratings2 = (5 + (-12) + 2 + 1) / 4;
	{
		SearchServer server;
		server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
		server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });

		const auto found_docs = server.FindTopDocuments("fluffy wellgroomed cat"s);
		const Document& doc1 = found_docs[0];
		const Document& doc2 = found_docs[1];
		const Document& doc0 = found_docs[2];
		ASSERT_EQUAL(doc0.rating, ratings0);
		ASSERT_EQUAL(doc1.rating, ratings1);
		ASSERT_EQUAL(doc2.rating, ratings2);
	}
}

// Тест проверяет фильтрацию с помощью предиката, в данном случае по четному Id документа
void TestExcludedWrongFilterByMeansPredicate() {
	SearchServer server;
	server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
	server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	const auto found_docs = server.FindTopDocuments("fluffy wellgroomed cat"s,
		[](int document_id, DocumentStatus status, int rating) {
		return document_id % 2 == 0; });
	const Document& doc0 = found_docs[0];
	ASSERT_EQUAL(doc0.id, 2);
}

// Тест проверяет поиск документов с заданным статусом
void TestExcludedWrongSearchByMeansStatus() {
	SearchServer server;
	server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
	server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::BANNED, { 7, 2, 7 });
	server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::IRRELEVANT, { 5, -12, 2, 1 });
	server.AddDocument(3, "wellgroomed starling Eugene"s, DocumentStatus::REMOVED, { 9 });

	const auto found_docs = server.FindTopDocuments("fluffy wellgroomed cat"s, DocumentStatus::ACTUAL);
	const auto found_docs1 = server.FindTopDocuments("fluffy wellgroomed cat"s, DocumentStatus::BANNED);
	const auto found_docs2 = server.FindTopDocuments("fluffy wellgroomed cat"s, DocumentStatus::IRRELEVANT);
	const auto found_docs3 = server.FindTopDocuments("fluffy wellgroomed cat"s, DocumentStatus::REMOVED);
	const Document& doc0 = found_docs[0];
	const Document& doc1 = found_docs1[0];
	const Document& doc2 = found_docs2[0];
	const Document& doc3 = found_docs3[0];
	ASSERT_EQUAL(doc0.id, 0);
	ASSERT_EQUAL(doc1.id, 1);
	ASSERT_EQUAL(doc2.id, 2);
	ASSERT_EQUAL(doc3.id, 3);
}

// Тест проверяет корректное вычисление релевантности найденных документов
void TestExcludeWrongComputeRelevanceFoundDocuments() {
	const double tf0 = 1.0 / 5; //for cat
	const double idf0 = log(3.0 / 2);

	const double tf1 = 2.0 / 4; //for fluffy/
	const double tf1_1 = 1.0 / 4; //for cat
	const double idf1 = log(3.0 / 1);
	const double idf1_1 = log(3.0 / 2);

	const double tf2 = 1.0 / 4; //for weelgroomed
	const double idf2 = log(3.0 / 1);


	const double relevance0 = tf0 * idf0;
	const double relevance1 = tf1 * idf1 + tf1_1 * idf1_1;
	const double relevance2 = tf2 * idf2;
	{
		SearchServer server;
		server.AddDocument(0, "white cat and fashion collar"s, DocumentStatus::ACTUAL, { 8, -3 });
		server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		server.AddDocument(2, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });

		const auto found_docs = server.FindTopDocuments("fluffy wellgroomed cat"s);
		const Document& doc1 = found_docs[0];
		const Document& doc2 = found_docs[1];
		const Document& doc0 = found_docs[2];
		ASSERT_EQUAL(doc0.relevance, relevance0);
		ASSERT_EQUAL(doc1.relevance, relevance1);
		ASSERT_EQUAL(doc2.relevance, relevance2);
	}
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestExcludeMinusWordsFromSearchQuery);
	RUN_TEST(TestExcludedMatchingWordByMinusWord);
	RUN_TEST(TestExcludedUnsortedResultByRelevance);
	RUN_TEST(TestExcludedWrongComputeAverageAmountRating);
	RUN_TEST(TestExcludedWrongFilterByMeansPredicate);
	RUN_TEST(TestExcludedWrongSearchByMeansStatus);
	RUN_TEST(TestExcludeWrongComputeRelevanceFoundDocuments);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
	TestSearchServer();
	// Если вы видите эту строку, значит все тесты прошли успешно
	cout << "Search server testing finished"s << endl;

	return 0;
}