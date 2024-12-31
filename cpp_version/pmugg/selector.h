#pragma once
#include "driver.h"
#include "internal_selector.h"
#include "vec2.h"
#include "mouse_event.h"
#include <vector>
#include <functional>

namespace ms {

class Selector {
public:
    Selector(Driver* driver, InternalSelector* internalSelector);
    ~Selector();

    static Brush* brush;  // Static brush for selection visualization

    void register_(View* view);
    void setLasso(bool enabled);
    void selectAll();
    void selectNone();
    void cancel();
    void notify();

    // Event handlers
    void onMouseMove(const MouseEvent& event);
    void onMouseDown(const MouseEvent& event);
    void onMouseUp(const MouseEvent& event);

    // Selection helpers
    Vertex* findVertex(const Vec2& position, float scale = 1.0f);
    Selectable* pointSelect(const Vec2& position, const SelectOptions& options);

private:
    struct Selection {
        Vec2 start;
        Vec2 current;
        bool active;
        bool add;
        bool subtract;
    };

    Driver* driver;
    InternalSelector* internalSelector;
    Selection selection;
    std::vector<std::function<void()>> observers;
    bool lassoEnabled;

    void addObserver(const std::function<void()>& observer);
    void removeObserver(const std::function<void()>& observer);
    void drawSelection(const Vec2& start, const Vec2& end);
    void clearSelection();
    void applySelection();
};

} // namespace ms 