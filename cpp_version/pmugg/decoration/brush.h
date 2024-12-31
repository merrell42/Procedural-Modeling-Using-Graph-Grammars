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

    void set(const std::string& name, float value) override;
    void set(const std::string& name, bool value) override;
    void set(const std::string& name, const std::string& value) override;
    float getFloat(const std::string& name) const override;
    bool getBool(const std::string& name) const override;
    std::string getString(const std::string& name) const override;

    // void draw(Context* context, 
    //          const std::function<Vec2(const Vec2&)>& transform) override;

    static Brush* import(Json json);

private:
    std::map<std::string, float> floatProperties;
    std::map<std::string, bool> boolProperties;
    std::map<std::string, std::string> stringProperties;
};

} // namespace ms 