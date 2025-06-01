#pragma once
#include "../shape/vec2.h"
#include <functional>
#include <string>

namespace ms {

class Decoration {
public:
    Decoration(const std::string& fillStyle, const std::string& strokeStyle,
              const std::function<void()>& onChange = nullptr);
    virtual ~Decoration() = default;

    // Property accessors
    virtual void set(const std::string& name, float value);
    virtual void set(const std::string& name, bool value);
    virtual void set(const std::string& name, const std::string& value);
    virtual float getFloat(const std::string& name) const;
    virtual bool getBool(const std::string& name) const;
    virtual std::string getString(const std::string& name) const;

    // Style accessors
    std::string getFillStyle() const { return fillStyle; }
    std::string getStrokeStyle() const { return strokeStyle; }

protected:
    std::string fillStyle;
    std::string strokeStyle;
    std::function<void()> onChange;

    void notifyChange();
};

} // namespace ms 