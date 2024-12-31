#include "cell.h"
#include "context.h"

namespace ms {

Cell::Cell(int row, int col)
    : row(row)
    , col(col)
    , position(0, 0)
    , size(1.0f)
    , active(false)
    , highlighted(false) {
}

Cell::~Cell() = default;

void Cell::setPosition(const Vec2& pos) {
    position = pos;
}

void Cell::setSize(float newSize) {
    size = newSize;
}

Vec2 Cell::getPosition() const {
    return position;
}

float Cell::getSize() const {
    return size;
}

bool Cell::isActive() const {
    return active;
}

void Cell::setActive(bool isActive) {
    active = isActive;
}

bool Cell::isHighlighted() const {
    return highlighted;
}

void Cell::setHighlighted(bool isHighlighted) {
    highlighted = isHighlighted;
}

int Cell::getRow() const {
    return row;
}

int Cell::getCol() const {
    return col;
}

void Cell::draw(Context* context) {
    context->save();

    // Draw in order: background, highlight (if any), border
    drawBackground(context);
    
    if (highlighted) {
        drawHighlight(context);
    }
    
    drawBorder(context);

    context->restore();
}

void Cell::drawBackground(Context* context) const {
    context->setFillStyle(active ? "#eee" : "#fff");
    context->fillRect(position.x, position.y, size, size);
}

void Cell::drawBorder(Context* context) const {
    context->setStrokeStyle("#ccc");
    context->setLineWidth(1);
    context->strokeRect(position.x, position.y, size, size);
}

void Cell::drawHighlight(Context* context) const {
    context->setFillStyle("rgba(0, 0, 255, 0.2)");
    context->fillRect(position.x, position.y, size, size);
}

} // namespace ms 