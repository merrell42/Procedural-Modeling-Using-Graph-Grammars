#include "pch.h"
#include "json_version_manager.h"
#include "../geometry/vec3.h"

void edgeTypesMigration1(Json& json) {
    for (auto& edgeType : json["edgeTypes"]) {
        Json newEdgeType;
        // Only keep specified fields
        const vector<string> allowedFields = {
            "faceData", "dir", "isRigid", "isRigidTiled", "spliced"
        };
        
        for (const auto& field : allowedFields) {
            if (edgeType.contains(field)) {
                newEdgeType[field] = edgeType[field];
            }
        }

        if (edgeType.contains("brush")) {
            Json edgeSettings = edgeType["brush"];
            Json newSettings;
            const vector<string> allowedFields = {
                "Min Length", "Max Length", "Tile Length", "Rigid Tiled"
            };
            for (const auto& field : allowedFields) {
                if (edgeSettings.contains(field)) {
                    newSettings[field] = edgeSettings[field];
                }
            }
            newEdgeType["edgeSettings"] = newSettings;
        }

        edgeType = newEdgeType;
    }
}

void faceTypesMigration1(Json& json) {
    for (auto& faceType : json["faceTypes"]) {
        Json newFaceType;
        const vector<string> allowedFields = {
            "material", "normal", "color"
        };
        
        for (const auto& field : allowedFields) {
            if (faceType.contains(field)) {
                newFaceType[field] = faceType[field];
            }
        }
        if (newFaceType.contains("material") && !newFaceType["material"].is_string()) {
            newFaceType.erase("material");
        }
        faceType = newFaceType;
    }
}

void vertexTypesMigration1(Json& json) {
    for (auto& vertexType : json["vertexTypes"]) {
        vertexType.erase("decoration");
        if (vertexType.contains("connections")) {
            Json halfEdges = vertexType["connections"];
            for (auto& halfEdge : halfEdges) {
                Json newHalfEdge;
                const vector<string> allowedFields = {
                    "edge", "isAtStart", "dir"
                };
                for (const auto& field : allowedFields) {
                    if (halfEdge.contains(field)) {
                        newHalfEdge[field] = halfEdge[field];
                    }
                }
                halfEdge = newHalfEdge;
            }
            vertexType["halfEdgeTypes"] = halfEdges;
            vertexType.erase("connections");
        }
    }
}

void edgeOrFaceTypeMigration1(Json& types) {
    Json newTypes = Json::array();
    for (const auto& type : types) {
        if (type.contains("type")) {
            newTypes.push_back(type["type"]);
        }
    }
    types = newTypes;
}

void vertexTypeMigration1(Json& types) {
    Json newTypes = Json::array();
    for (const auto& type : types) {
        Json newType;
        if (type.contains("type")) {
            newType["type"] = type["type"];
        }
        if (type.contains("kind")) {
            newType["kind"] = type["kind"];
        }
        newTypes.push_back(newType);
    }
    types = newTypes;
}

void interiorMigration1(Json& interior) {
    interior.erase("connectorGroups");
    
    // Filter vertices to only keep halfEdges
    Json vertices = interior["vertices"];
    for (auto& vertex : vertices) {
        Json newVertex;
        if (vertex.contains("halfEdges")) {
            newVertex["halfEdges"] = vertex["halfEdges"];
        }
        vertex = newVertex;
    }
    interior["vertices"] = vertices;

    // Filter faces to only keep outerComponent
    Json faces = interior["faces"];
    for (auto& face : faces) {
        Json newFace;
        if (face.contains("outerComponent")) {
            newFace["outerComponent"] = face["outerComponent"];
        }
        face = newFace;
    }
    interior["faces"] = faces;
}

void graphMigration1(Json& json) {
    Json newJson;
    
    if (json.contains("interior")) {
        Json interior = json["interior"];
        interiorMigration1(interior);
        newJson["interior"] = interior;
    }
    if (json.contains("vertices")) {
        Json vertexTypes = json["vertices"];
        vertexTypeMigration1(vertexTypes);
        newJson["vertexTypes"] = vertexTypes;
    }
    if (json.contains("edges")) {
        Json edgeTypes = json["edges"];
        edgeOrFaceTypeMigration1(edgeTypes);
        newJson["edgeTypes"] = edgeTypes;
    }
    if (json.contains("faces")) {
        Json faceTypes = json["faces"];
        edgeOrFaceTypeMigration1(faceTypes);
        newJson["faceTypes"] = faceTypes;
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
    json.erase("useNetworks");
    json["types"].erase("xml");
    edgeTypesMigration1(json["types"]);
    faceTypesMigration1(json["types"]);
    vertexTypesMigration1(json["types"]);
}

Vec3 createColorForIndex(int index) {   
    switch (index % 6) {
        case 0: return Vec3(1.0, 1.0, 1.0);
        case 1: return Vec3(0.75, 0.95, 1.0);
        case 2: return Vec3(1.0, 0.75, 0.75);
        case 3: return Vec3(0.75, 1.0, 0.75);
        case 4: return Vec3(0.75, 1.0, 1.0);
        case 5: return Vec3(1.0, 0.75, 1.0);
        default: return Vec3(1.0, 1.0, 0.75);
    }
}

void jsonMigration2(Json& json) {
    // Go through each face type and create colors for them if no color is assigned.
    if (json.contains("types") && json["types"].contains("faceTypes")) {
        Json& faceTypes = json["types"]["faceTypes"];
        int faceTypeIndex = 0;
        
        for (auto& faceType : faceTypes) {
            if (!faceType.contains("color") || faceType["color"].is_null()) {
                Vec3 color = createColorForIndex(faceTypeIndex);
                Json colorJson;
                colorJson["x"] = color.getX();
                colorJson["y"] = color.getY();
                colorJson["z"] = color.getZ();
                faceType["color"] = colorJson;
            }
            faceTypeIndex++;
        }
    }
}

void registerJsonMigrations() {
    if (JsonVersionManager::isInitialized()) {
        return;
    }
    JsonVersionManager::registerUpdateFunction(jsonMigration1);
    JsonVersionManager::registerUpdateFunction(jsonMigration2);
    JsonVersionManager::setInitialized(true);
}
