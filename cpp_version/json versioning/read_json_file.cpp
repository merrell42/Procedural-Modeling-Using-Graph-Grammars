#include "pch.h"
#include "read_json_file.h"
#include "json_version_manager.h"
#include "json_migrations.h"
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
		size_t lastDot = filePath.find_last_of('.');
		string basePath = (lastDot != string::npos) ? filePath.substr(0, lastDot) : filePath;
		string extension = (lastDot != string::npos) ? filePath.substr(lastDot) : "";

		ofstream newFile(basePath + "_v" + to_string(parsed["version"].get<int>()) + extension);
		newFile << parsed.dump(2, ' ', true);
		newFile.close();
	}
	return parsed;
}