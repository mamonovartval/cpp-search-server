#include "search_server.h"
#include "remove_duplicates.h"

#include <iterator>

void RemoveDuplicates(SearchServer & search_server)
{
	std::set<int> duplicateID;
	std::set<std::set<std::string>> comparingWord;
	
	for (const auto& document_id : search_server) {
		std::set<std::string> duplicateWord;
		const std::map<std::string, double>& bufferData = search_server.GetWordFrequencies(document_id);		std::transform(bufferData.cbegin(), bufferData.cend(),
			std::inserter(duplicateWord, duplicateWord.begin()),
			[](const std::pair<std::string, double>& key_value)
		{ return key_value.first; });

		if (!comparingWord.count(duplicateWord)) {
			comparingWord.insert(duplicateWord);
		}
		else {
			duplicateID.insert(document_id);
		}
	}

	for (auto id : duplicateID) {
			std::cout << "Found duplicate document id " << id << std::endl;
			search_server.RemoveDocument(id);
		}
}