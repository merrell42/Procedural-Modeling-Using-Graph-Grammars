#include "pch.h"
#include "jsonVersionManager.h"

// Example update function from version 0 to 1
/*void jsonMigration1(Json& json) {
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
//}

void graphMigration1(Json& json) {
    Json newJson;
    
    if (json.contains("interior")) {
        newJson["interior"] = json["interior"];
    }
    if (json.contains("vertices")) {
        newJson["vertexTypes"] = json["vertices"];
    }
    if (json.contains("edges")) {
        newJson["edgeTypes"] = json["edges"];
    }
    if (json.contains("faces")) {
        newJson["faceTypes"] = json["faces"];
    }
    if (json.contains("morphism")) {
        newJson["morphism"] = json["morphism"];
    }
    
    // Replace the original JSON with our modified version
    json = newJson;
}

void productionRuleMigration1(Json& json) {
    if (json.contains("n") && json["n"].is_array()) {
        for (auto& element : json["n"]) {
            graphMigration1(element);
        }
    }
}

// Migration to keep only name and solution fields
void transitionArraysMigration(Json& json) {
    // Map of old names to new names
    const map<string, string> arrayRenames = {
        {"starterTransitions", "starterRules"},
        {"transitions", "rules"},
        {"groundTransitions", "groundRules"}
    };
    
    // Process each transition array and rename it
    for (const auto& [oldName, newName] : arrayRenames) {
        if (json.contains(oldName) && json[oldName].is_array()) {
            json[newName] = json[oldName];
            for (auto& element : json[newName]) {
                productionRuleMigration1(element);
            }
            json.erase(oldName);
        }
    }
}

void jsonMigration1(Json& json) {
    Json newJson;
    if (json.contains("name")) {
        newJson["name"] = json["name"];
    }
    
    // If solution exists and is an object, promote all its fields to root level
    if (json.contains("solution") && json["solution"].is_object()) {
        for (auto& [key, value] : json["solution"].items()) {
            newJson[key] = value;
        }
    }
    json = newJson;
    if (json.contains("emptyNet")) {
        json["emptyGraph"] = json["emptyNet"];
        json.erase("emptyNet");
        graphMigration1(json["emptyGraph"]);
    }
    // Apply migration to all transition arrays
    transitionArraysMigration(json);
}

void registerJsonMigrations() {
    if (JsonVersionManager::isInitialized()) {
        return;
    }
    JsonVersionManager::registerUpdateFunction(jsonMigration1);
    JsonVersionManager::setInitialized(true);
}
