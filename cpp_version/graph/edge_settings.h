#pragma once
#include <map>
#include <vector>
#include <functional>
#include <any>
#include "../third_party/json.h"

using namespace std;
using Json = nlohmann::json;

// A settings that multiple edges can share.
class EdgeSettings {
public:
    EdgeSettings();
    static EdgeSettings* import(Json json);

    void set(const string& name, double value);
    void set(const string& name, bool value);
    void set(const string& name, const string& value);
    double getDouble(const string& name) const;
    bool getBool(const string& name) const;
    string getString(const string& name) const;

private:
    map<string, double> doubleProperties;
    map<string, bool> boolProperties;
    map<string, string> stringProperties;
};

