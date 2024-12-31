#include "shape_view.h"
#include "context.h"
#include "global_settings.h"
#include "timer.h"

namespace ms {

ShapeView::ShapeView(Canvas* canvas)
    : canvas(canvas)
    , viewport(std::make_unique<Viewport>(1.0f, 0.0f, 0.0f))
    , isOver(false) {
}

ShapeView::~ShapeView() = default;

void ShapeView::activate(int controllerType) {
    canvas->style.display = "block";
    isOver = false;
}

void ShapeView::deactivate() {
    canvas->style.display = "none";
}

void ShapeView::activateOver() {
    canvas->style.display = "block";
    isOver = true;
}

void ShapeView::resize(int width, int height) {
    canvas->resize(width, height);
}

void ShapeView::redraw(Driver* driver, bool fullRedraw) {
    Timer::start("ShapeView::redraw");

    if (!driver) {
        auto* context = canvas->getContext();
        context->clearRect(0, 0, canvas->getWidth(), canvas->getHeight());
        Timer::stop("ShapeView::redraw");
        return;
    }

    if (isOver) {
        auto* context = canvas->getContext();
        context->setGlobalAlpha(0.5f);
    }

    drawShape(driver);
    drawSelection(driver);
    drawHighlight(driver);
    
    if (!GlobalSettings::get("MVP Mode")) {
        drawDecorations(driver);
    }

    Timer::stop("ShapeView::redraw");
}

Viewport* ShapeView::getViewport() {
    return viewport.get();
}

void ShapeView::drawShape(Driver* driver) {
    Timer::start("ShapeView::drawShape");

    auto* context = canvas->getContext();
    context->clearRect(0, 0, canvas->getWidth(), canvas->getHeight());
    context->save();

    auto offset = driver->offset();
    auto convertToScreen = [this, &offset](const Vec2& pos) {
        Vec2 result;
        viewport->transform(pos, offset, result);
        return result;
    };

    // Draw faces
    for (auto* face : driver->getRenderables()) {
        face->draw(context, convertToScreen);
    }

    context->restore();
    Timer::stop("ShapeView::drawShape");
}

void ShapeView::drawSelection(Driver* driver) {
    Timer::start("ShapeView::drawSelection");

    auto* context = canvas->getContext();
    context->save();

    auto offset = driver->offset();
    auto convertToScreen = [this, &offset](const Vec2& pos) {
        Vec2 result;
        viewport->transform(pos, offset, result);
        return result;
    };

    // Draw selected elements
    for (auto* selectable : driver->getSelected()) {
        selectable->drawSelected(context, convertToScreen);
    }

    context->restore();
    Timer::stop("ShapeView::drawSelection");
}

void ShapeView::drawHighlight(Driver* driver) {
    Timer::start("ShapeView::drawHighlight");

    auto* context = canvas->getContext();
    context->save();

    auto offset = driver->offset();
    auto convertToScreen = [this, &offset](const Vec2& pos) {
        Vec2 result;
        viewport->transform(pos, offset, result);
        return result;
    };

    // Draw highlighted elements
    for (auto* highlight : driver->getHighlighted()) {
        highlight->drawHighlighted(context, convertToScreen);
    }

    context->restore();
    Timer::stop("ShapeView::drawHighlight");
}

void ShapeView::drawDecorations(Driver* driver) {
    Timer::start("ShapeView::drawDecorations");

    auto* context = canvas->getContext();
    context->save();

    auto offset = driver->offset();
    auto convertToScreen = [this, &offset](const Vec2& pos) {
        Vec2 result;
        viewport->transform(pos, offset, result);
        return result;
    };

    // Draw decorative elements
    for (auto* decoration : driver->getDecorations()) {
        decoration->draw(context, convertToScreen);
    }

    context->restore();
    Timer::stop("ShapeView::drawDecorations");
}

} // namespace ms 