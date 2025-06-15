#include "readJsonFile.h"
#include "jsonVersionManager.h"
#include "jsonMigrations.h"
#include <fstream>

using namespace std;
using Json = nlohmann::json;

Json readJsonFile(string filePath, bool writeNewVersion) {
	ifstream file(filePath);
	if (!file.is_open()) {
		throw runtime_error("Could not open file");
	}
		
	string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
	file.close();
	Json parsed = Json::parse(content);
	registerJsonMigrations();
	JsonVersionManager::updateToLatest(parsed);

	if (writeNewVersion) {
		ofstream newFile(filePath + ".new");
		newFile << parsed.dump(2, ' ', true);
		newFile.close();
	}
	return parsed;
}