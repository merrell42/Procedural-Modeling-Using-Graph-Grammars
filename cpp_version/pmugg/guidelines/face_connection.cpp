#include "face_connection.h"
#include "face.h"
#include "cell.h"
#include "endpoint.h"
#include "line_state.h"
#include "intersector.h"
#include <algorithm>
#include <iostream>
#include <cmath>

namespace ms {

FaceConnection::FaceConnection(Face* face, Endpoint* start, Endpoint* end)
    : face(face)
    , cells()
    , endpoints({start, end}) {
    
    if (start && end) {
        coordinates = {
            start->getVertex()->getPosition(),
            end->getVertex()->getPosition()
        };
    }
}

void FaceConnection::addCell(Cell* cell) {
    if (!cell) return;
    if (std::find(cells.begin(), cells.end(), cell) == cells.end()) {
        cells.push_back(cell);
    }
}

void FaceConnection::removeCell(Cell* cell) {
    auto it = std::find(cells.begin(), cells.end(), cell);
    if (it != cells.end()) {
        cells.erase(it);
    }
}

void FaceConnection::setFace(Face* newFace) {
    face = newFace;
}

Face* FaceConnection::getFace() const {
    return face;
}

std::vector<Cell*> FaceConnection::getCells() const {
    return cells;
}

std::vector<Endpoint*> FaceConnection::getEndpoints() const {
    return endpoints;
}

bool FaceConnection::isLeft(Endpoint* endpoint) const {
    if (endpoints.empty()) return false;
    return endpoints[0] == endpoint;
}

void FaceConnection::addCells() {
    if (coordinates.size() < 2) return;

    int x0 = static_cast<int>(std::floor(coordinates[0].x));
    int x1 = static_cast<int>(std::floor(coordinates[1].x));
    int y = static_cast<int>(std::floor(coordinates[0].y));

    auto model = face->getFaceType()->getModel();
    for (int x = x0; x <= x1; ++x) {
        if (Cell* cell = model->getCell(x, y)) {
            cell->addFaceConnection(this);
        }
    }
}

void FaceConnection::removeCells() {
    auto oldCells = cells;
    for (auto* cell : oldCells) {
        cell->removeFaceConnection(this);
    }
}

bool FaceConnection::intersectsState(const LineState& lineState) const {
    if (coordinates.size() < 2) return false;

    auto stateEndpoints = lineState.getLine()->getEndpoints();
    auto state0 = stateEndpoints[0]->getPosition();
    auto state1 = stateEndpoints[1]->getPosition();

    return Intersector::intersect(coordinates[0], coordinates[1], 
                                state0, state1, 0.0f);
}

void FaceConnection::highlight(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen) {
    draw(context, convertToScreen, true);
}

void FaceConnection::draw(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen, bool highlighted) {
    if (coordinates.size() < 2) return;

    // Note: This is a placeholder for the actual drawing code
    // In a real implementation, you would need to handle the actual drawing context
    // based on your graphics system (e.g., OpenGL, DirectX, etc.)
    
    auto p0 = convertToScreen(coordinates[0]);
    auto p1 = convertToScreen(coordinates[1]);

    // Example of what the JavaScript version does:
    // context.strokeStyle = highlighted ? '#f0f' : '#fcad03';
    // context.lineWidth = highlighted ? 2 : 1;
    // context.globalAlpha = 1;
    // context.beginPath();
    // context.moveTo(p0.x, p0.y);
    // context.lineTo(p1.x, p1.y);
    // context.stroke();
}

void FaceConnection::print() const {
    std::cout << "FaceConnection:" << std::endl;
    std::cout << "Number of cells: " << cells.size() << std::endl;
    std::cout << "Number of endpoints: " << endpoints.size() << std::endl;
    if (face) {
        std::cout << "Has associated face" << std::endl;
    }
    if (coordinates.size() >= 2) {
        std::cout << "Start coordinate: ";
        coordinates[0].print();
        std::cout << "End coordinate: ";
        coordinates[1].print();
    }
}

} // namespace ms 