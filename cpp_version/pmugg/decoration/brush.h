#pragma once
#include "decoration.h"
#include <map>
#include <vector>
#include <functional>
#include <any>

namespace ms {

class Brush : public Decoration {
public:
    Brush(const std::string& fillStyle);
    Brush(const std::string& fillStyle, const std::string& strokeStyle);
    static Brush* import(Json json);

    void set(const std::string& name, double value) override;
    void set(const std::string& name, bool value) override;
    void set(const std::string& name, const std::string& value) override;
    double getDouble(const std::string& name) const override;
    bool getBool(const std::string& name) const override;
    std::string getString(const std::string& name) const override;

private:
    std::map<std::string, double> doubleProperties;
    std::map<std::string, bool> boolProperties;
    std::map<std::string, std::string> stringProperties;
};

} // namespace ms 