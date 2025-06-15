#include "jsonVersionManager.h"

// Example update function from version 0 to 1
void jsonMigration1(Json& json) {
    if (!json.contains("newField")) {
        json["newField"] = "default value";
    }
    
    // Example: Rename a field
    /*if (json.contains("oldFieldName")) {
        json["newFieldName"] = json["oldFieldName"];
        json.erase("oldFieldName");
    }
    
    // Example: Transform data
    if (json.contains("numbers") && json["numbers"].is_array()) {
        for (auto& num : json["numbers"]) {
            // Example: Double all numbers
            num = num.get<double>() * 2;
        }
    } */
}

void registerJsonMigrations() {
    if (JsonVersionManager::isInitialized()) {
        return;
    }
    JsonVersionManager::registerUpdateFunction(jsonMigration1);
    JsonVersionManager::setInitialized(true);
}
