#pragma once
#include "../shape/vec2.h"
#include <vector>
#include <memory>

namespace ms {

class Cell {
public:
    Cell(int row, int col);
    ~Cell();

    // Position and size
    void setPosition(const Vec2& pos);
    void setSize(float size);
    Vec2 getPosition() const;
    float getSize() const;

    // Cell state
    bool isActive() const;
    void setActive(bool active);
    bool isHighlighted() const;
    void setHighlighted(bool highlighted);

    // Grid coordinates
    int getRow() const;
    int getCol() const;

    // Drawing
    /*void draw(Context* context);*/

private:
    int row;
    int col;
    Vec2 position;
    float size;
    bool active;
    bool highlighted;

    // Drawing helpers
    /*void drawBackground(Context* context) const;
    void drawBorder(Context* context) const;
    void drawHighlight(Context* context) const;*/
};

} // namespace ms 