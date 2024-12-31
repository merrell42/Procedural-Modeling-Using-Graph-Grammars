#pragma once
#include "decoration_model.h"
#include <string>

namespace ms {

class DecorationView {
public:
    DecorationView(const std::string& containerId, DecorationModel* model);
    ~DecorationView();

    void draw();
    void notify();

private:
    std::string containerId;
    DecorationModel* model;

    void drawBrushCollection(const BrushCollection& collection, int index);
    void createBrushElement(Brush* brush);
};

} // namespace ms 