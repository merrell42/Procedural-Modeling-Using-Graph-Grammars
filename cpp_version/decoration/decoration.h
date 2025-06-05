#pragma once
#include "../geometry/vec2.h"
#include <functional>
#include <string>

using namespace std;

namespace ms {

class Decoration {
public:
    Decoration(const string& fillStyle, const string& strokeStyle,
              const function<void()>& onChange = nullptr);
    virtual ~Decoration() = default;

    // Property accessors
    virtual void set(const string& name, double value);
    virtual void set(const string& name, bool value);
    virtual void set(const string& name, const string& value);
    virtual double getDouble(const string& name) const;
    virtual bool getBool(const string& name) const;
    virtual string getString(const string& name) const;

    // Style accessors
    string getFillStyle() const { return fillStyle; }
    string getStrokeStyle() const { return strokeStyle; }

protected:
    string fillStyle;
    string strokeStyle;
    function<void()> onChange;

    void notifyChange();
};

} // namespace ms 