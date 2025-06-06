#pragma once
#include "decoration.h"
#include <map>
#include <vector>
#include <functional>
#include <any>

using namespace std;

class Brush : public Decoration {
public:
    Brush(const string& fillStyle);
    Brush(const string& fillStyle, const string& strokeStyle);
    static Brush* import(Json json);

    void set(const string& name, double value) override;
    void set(const string& name, bool value) override;
    void set(const string& name, const string& value) override;
    double getDouble(const string& name) const override;
    bool getBool(const string& name) const override;
    string getString(const string& name) const override;

private:
    map<string, double> doubleProperties;
    map<string, bool> boolProperties;
    map<string, string> stringProperties;
};

