#include "test_scenario.h"
#include <cmath>

namespace ms {

TestScenario::TestScenario() {
    exampleShape = createExampleShape();
    decorationModel = createDecorationModel(exampleShape.get());
    shapeMaker = createShapeMaker();
}

TestScenario::~TestScenario() = default;

std::unique_ptr<ShapeMaker> TestScenario::createShapeMaker() {
    auto maker = std::make_unique<ShapeMaker>(exampleShape.get(), decorationModel.get());
    return maker;
}

std::unique_ptr<ExampleShape> TestScenario::createExampleShape() {
    return std::make_unique<ExampleShape>();
}

std::unique_ptr<DecorationModel> TestScenario::createDecorationModel(ExampleShape* exampleShape) {
    auto model = std::make_unique<DecorationModel>(exampleShape);
    setupTestDecorations(model.get());
    return model;
}

void TestScenario::addTestObject() {
    // Create a simple test object (e.g., a square)
    auto positions = createSquare(2.0f);
    auto* brush = createTestBrush("#000");
    shapeMaker->addShape(positions, brush);
}

void TestScenario::addTestBoundary() {
    // Create a boundary around the test object
    auto positions = createSquare(4.0f);
    auto* brush = createTestBrush("#000", [this]() {
        decorationModel->notify();
    });
    
    brush->set("Boundary", true);
    decorationModel->brushCollections[0].addBrush(brush);
    
    auto* boundaryArea = new Area("#bde", "#bde", [this]() {
        decorationModel->notify();
    });
    boundaryArea->set("Boundary", true);
    decorationModel->brushCollections[1].addBrush(boundaryArea);
    
    shapeMaker->addBoundary(brush, boundaryArea);
}

void TestScenario::addTestDecorations() {
    // Add some test decorations
    auto* brush1 = createTestBrush("#f00");
    auto* brush2 = createTestBrush("#00f");
    
    decorationModel->brushCollections[0].addBrush(brush1);
    decorationModel->brushCollections[0].addBrush(brush2);
    
    auto positions1 = createTriangle(1.0f);
    auto positions2 = createCircle(0.5f);
    
    shapeMaker->addShape(positions1, brush1);
    shapeMaker->addShape(positions2, brush2);
}

std::vector<Vec2> TestScenario::createSquare(float size) {
    float halfSize = size / 2.0f;
    return {
        Vec2(-halfSize, -halfSize),
        Vec2(halfSize, -halfSize),
        Vec2(halfSize, halfSize),
        Vec2(-halfSize, halfSize)
    };
}

std::vector<Vec2> TestScenario::createTriangle(float size) {
    float halfSize = size / 2.0f;
    return {
        Vec2(0.0f, -halfSize),
        Vec2(halfSize, halfSize),
        Vec2(-halfSize, halfSize)
    };
}

std::vector<Vec2> TestScenario::createCircle(float radius, int segments) {
    std::vector<Vec2> positions;
    positions.reserve(segments);
    
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * static_cast<float>(i) / static_cast<float>(segments);
        float x = radius * std::cos(angle);
        float y = radius * std::sin(angle);
        positions.emplace_back(x, y);
    }
    
    return positions;
}

Brush* TestScenario::createTestBrush(const std::string& color,
                                   const std::function<void()>& onChange) {
    auto* brush = new Brush(color, color, onChange);
    brush->set("Width", 0.1f);
    return brush;
}

void TestScenario::setupTestDecorations(DecorationModel* model) {
    // Add default brush collections
    model->brushCollections.resize(2);
    
    // Add some default brushes
    auto* defaultBrush = createTestBrush();
    model->brushCollections[0].addBrush(defaultBrush);
    
    // Add some default areas
    auto* defaultArea = new Area("#eee", "#eee");
    model->brushCollections[1].addBrush(defaultArea);
}

} // namespace ms 