#include "pch.h"
#include "generate.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <chrono>
#include "../graph_grammar.h"
#include "../mutator.h"
#include "../primitives/primitives.h"
#include "../graph_drawing/model.h"
#include "../util/util.h"
#include "../settings.h"
#include "../json versioning/read_json_file.h"
#include "../util/diagnostics.h"
#include "../graph/debug_mesh.h"
#include "../grammar_rules/production_rule.h"

using namespace std;
using Json = nlohmann::json;

Model* model;
Mutator* mutator;
GraphGrammar* grammar;

namespace {

const vector<ProductionRule*>& getRulesByCategory(int category) {
    if (!grammar) {
        throw runtime_error("No grammar loaded.");
    }
    switch (category) {
        case 0: return grammar->getNormalRules();
        case 1: return grammar->getStarterRules();
        case 2: return grammar->getGroundRules();
        default: throw runtime_error("Invalid production rule category.");
    }
}

const Graph* getProductionRuleGraph(int category, int ruleIndex, int graphIndex) {
    const auto& rules = getRulesByCategory(category);
    if (ruleIndex < 0 || ruleIndex >= (int)rules.size()) {
        throw runtime_error("Production rule index out of range.");
    }
    const auto& graphs = rules[ruleIndex]->getStartGraphs();
    if (graphIndex < 0 || graphIndex >= (int)graphs.size()) {
        throw runtime_error("Production rule graph index out of range.");
    }
    return graphs[graphIndex];
}

void resetGenerationState() {
    delete mutator;
    mutator = nullptr;
    delete model;
    model = nullptr;
    delete grammar;
    grammar = nullptr;
}

} // namespace

// Cross-platform safe string copy
void safeCopy(char* dest, int len, const char* src) {
#ifdef _WIN32
	strcpy_s(dest, len, src);
#else
	strncpy(dest, src, len - 1);
	dest[len - 1] = '\0';
#endif
}

static void logDllWarningFromGrammar(const GraphGrammar* grammar) {
	if (!grammar->hasStarterRules(true) && !grammar->hasStarterRules(false)) {
		Diagnostics::setWarning(
			"Warning: Graph grammar has no starter or ground rules; start productions will be skipped.");
	}
}

void getLastWarning(char* result, int len) {
	safeCopy(result, len, Diagnostics::getWarning().c_str());
	Diagnostics::clearWarning();
}

// Initialize the model and mutator from the JSON file. Return messages in the result string.
void initialize(const char* filePath, char* result, int len, int seed) {
	resetRandom(seed);
	Diagnostics::clearWarning();
	resetGenerationState();

	try {
		Json parsed = readJsonFile(filePath);
		grammar = GraphGrammar::import(parsed);
		logDllWarningFromGrammar(grammar);
		model = new Model();
		mutator = new Mutator(model, grammar);
		safeCopy(result, len, "Success");
	} catch (const Json::exception& e) {
		string errorMsg = "Error: JSON parsing failed - ";
		errorMsg += e.what();
		safeCopy(result, len, errorMsg.c_str());
	} catch (const exception& e) {
		string errorMsg = "Error: ";
		errorMsg += e.what();
		safeCopy(result, len, errorMsg.c_str());
	}
}

// Reset the model and mutator.
void reset(int seed) {
	model->reset();
	mutator->reset();
	resetRandom(seed);
}

// Iterate some number of steps.
void iterate(int steps) {
	try {
		mutator->iterate(steps);
	} catch (const exception& e) {
		Diagnostics::setWarning(string("Warning: ") + e.what());
	}
}

// Iterate until a certain amount of time has passed.
int iterateToTime(float timeSeconds) {
	int steps = 0;
	auto startTime = std::chrono::high_resolution_clock::now();
	auto targetDuration = std::chrono::duration<float>(timeSeconds);
	
	while (std::chrono::high_resolution_clock::now() - startTime < targetDuration) {
		try {
			mutator->iterate(1);
		} catch (const exception& e) {
			Diagnostics::setWarning(string("Warning: ") + e.what());
			break;
		}
		steps++;
	}
	return steps;
}

// Return the number of faces in the current model.
int getNumFaces() {
	return (int)model->getCurrent()->getFaceMap().size();
}

// Return the current mesh.
MeshCpp getMesh() {
	return model->getCurrent()->exportMesh();
}

// Set the size of the model.
void setSize(float x, float y, float z) {
	std::vector<double> extents = {x, y, z};
	globalSettings["Extents"] = extents;
}

// Free memory for the mesh.
void destroyMesh(MeshCpp& mesh) {
	freeMeshMemory(mesh);
}

int getNumProductionRules(int category) {
	try {
		return (int)getRulesByCategory(category).size();
	} catch (...) {
		return 0;
	}
}

int getProductionRuleGraphCount(int category, int ruleIndex) {
	try {
		const auto& rules = getRulesByCategory(category);
		if (ruleIndex < 0 || ruleIndex >= (int)rules.size()) {
		 return 0;
		}
		return (int)rules[ruleIndex]->getStartGraphs().size();
	} catch (...) {
		return 0;
	}
}

MeshCpp getProductionRuleGraphMesh(int category, int ruleIndex, int graphIndex) {
	try {
		return createDebugMesh(getProductionRuleGraph(category, ruleIndex, graphIndex));
	} catch (...) {
		MeshCpp mesh{};
		mesh.submeshes = nullptr;
		mesh.numSubmeshes = 0;
		return mesh;
	}
}
