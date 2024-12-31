#include "line_segment.h"
#include "line_state.h"
#include "line.h"
#include "vertex.h"
#include "timer.h"
#include "util.h"
#include <algorithm>

namespace ms {

bool LineSegment::moveMutable = false;

LineSegment::LineSegment(Stats* stats)
    : lengthLowerBound(0)
    , dirtyLength(true) {
    
    std::map<std::string, std::unique_ptr<Property>> properties;
    properties["lineState"] = std::make_unique<RequiredArray>("lineState");
    properties["line"] = std::make_unique<SingleProperty>("line", true);

    node = std::make_unique<Node>(this, stats, "lineSegment", std::move(properties));
}

Node* LineSegment::getNode() const {
    return node.get();
}

std::vector<LineState*> LineSegment::getStates() const {
    return node->get("lineState");
}

LineState* LineSegment::getState(bool isAtStart) const {
    auto states = getStates();
    return states[isAtStart ? 0 : states.size() - 1];
}

Line* LineSegment::getLine() const {
    return node->get("line");
}

float LineSegment::getLength(float idealLength) {
    timer->start("Segment Length");
    
    // Dirty length could be used here to speed things up, but some states may become immutable
    lengthLowerBound = 0;
    auto states = getStates();
    
    for (auto* state : states) {
        if (!moveMutable || state->isMutable()) {
            lengthLowerBound += state->getLength();
            if (lengthLowerBound >= idealLength) {
                break;
            }
        }
    }
    
    timer->stop("Segment Length");
    return lengthLowerBound;
}

bool LineSegment::findIntersections() {
    return std::any_of(getStates().begin(), getStates().end(),
        [](LineState* state) { return state->countIntersections() > 0; });
}

void LineSegment::addState(LineState* state, bool insertAtStart) {
    dirtyLength = true;
    node->connect(state, insertAtStart);
}

void LineSegment::addStates(const std::vector<LineState*>& states) {
    dirtyLength = true;
    for (auto* state : states) {
        node->connect(state);
    }
}

void LineSegment::destroy() {
    for (auto* state : getStates()) {
        state->getNode()->destroy();
    }
}

void LineSegment::merge(LineSegment* segmentB, LineState* mergedState) {
    auto statesA = getStates();
    auto statesB = segmentB->getStates();
    
    // Throw out the two states that were merged and put in the mergedState
    statesA.back()->getNode()->disconnect(this);
    statesB.front()->getNode()->disconnect(segmentB);
    statesB[0] = mergedState;
    
    addStates(statesB);
    segmentB->node->disconnect(segmentB->getLine());
    segmentB->node->destroy();
}

void LineSegment::split(LineState* unsplitState, const Vec2& splitPoint, 
                       Vertex* vertex, int index) {
    timer->start("split A");
    dirtyLength = true;
    
    if (index == -1) {
        index = std::find(getStates().begin(), getStates().end(), unsplitState) - 
                getStates().begin();
    }
    
    if (index == -1) {
        ms::alert("Cannot find state to split.");
    }

    auto* newSegment = new LineSegment(node->getStats());
    newSegment->addStates(std::vector<LineState*>(
        getStates().begin() + index + 1,
        getStates().end()
    ));

    auto splitStates = unsplitState->split(splitPoint);
    addState(splitStates[0], false);
    newSegment->addState(splitStates[1], true);
    unsplitState->destroy();

    auto* line = getLine();
    auto segmentIndex = std::find(line->getSegments().begin(), 
                                 line->getSegments().end(), 
                                 this) - line->getSegments().begin();
    line->getNode()->splice(newSegment, segmentIndex + 1);

    vertex->addLineState(splitStates[0], 0);
    vertex->addLineState(splitStates[1], 1);
    
    timer->stop("split A");
}

void LineSegment::setPositionsOneState(const std::vector<Vec2>& positions) {
    auto states = getStates();
    while (states.size() > 1) {
        auto* state = states.back();
        state->getNode()->disconnect(this);
        state->destroy();
        states.pop_back();
    }
    
    states[0]->setPosition(positions[0], true);
    states[0]->setPosition(positions[1], false);
    states[0]->refreshCells();
}

void LineSegment::setPositions(const std::vector<Vec2>& positions) {
    auto states = getStates();
    states.erase(
        std::remove_if(states.begin(), states.end(),
            [](LineState* state) { return !moveMutable || state->isMutable(); }),
        states.end()
    );
    
    if (states.empty()) return;
    
    auto p0 = states.front()->getPosition(true);
    auto p1 = states.back()->getPosition(false);
    auto q0 = positions[0];
    auto q1 = positions[1];
    
    auto ua = p1 - p0;
    auto ub = q1 - q0;
    auto va = Vec2(-ua.y, ua.x);
    auto vb = Vec2(-ub.y, ub.x);
    
    Matrix A({ua.x, va.x, ua.y, va.y}, 2, 2);
    Matrix B({ub.x, vb.x, ub.y, vb.y}, 2, 2);
    Matrix P0({p0.x, p0.y}, 2, 1);
    
    auto BAi = B * A.inverse();
    auto BAip0 = BAi * P0;
    auto offset = Vec2(-BAip0.get(0, 0) + q0.x, -BAip0.get(1, 0) + q0.y);
    
    for (auto* state : states) {
        for (int atEnd = 0; atEnd < 2; atEnd++) {
            auto p = state->getPosition(atEnd == 0);
            Matrix P({p.x, p.y}, 2, 1);
            auto BAip = BAi * P;
            auto newPosition = Vec2(BAip.get(0, 0) + offset.x, BAip.get(1, 0) + offset.y);
            state->setPosition(newPosition, atEnd == 0);
            state->refreshCells();
        }
    }
}

void LineSegment::setPosition(const Vec2& newPosition, bool isAtStart) {
    auto states = getStates();
    states.erase(
        std::remove_if(states.begin(), states.end(),
            [](LineState* state) { return !moveMutable || state->isMutable(); }),
        states.end()
    );
    
    if (states.empty()) return;
    
    float totalLength = 0;
    std::vector<float> cumulativeLengths = {0};
    
    for (auto* state : states) {
        totalLength += state->getLength();
        cumulativeLengths.push_back(totalLength);
    }
    
    if (totalLength == 0) {
        ms::alert("Zero mutable length.");
        return;
    }
    
    auto* state = isAtStart ? states.front() : states.back();
    auto position0 = state->getPosition(isAtStart);
    auto fullMovement = newPosition - position0;
    
    std::vector<Vec2> movements;
    for (float length : cumulativeLengths) {
        float scale = (isAtStart ? totalLength - length : length) / totalLength;
        movements.push_back(fullMovement * scale);
    }
    
    for (size_t i = 0; i < states.size(); i++) {
        states[i]->move(movements[i], movements[i + 1]);
    }
}

void LineSegment::highlight(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen) {
    for (auto* state : getStates()) {
        state->highlight(context, convertToScreen);
    }
}

void LineSegment::print() const {
    auto states = getStates();
    std::cout << states.size() << " states" << std::endl;
    states.front()->print();
    states.back()->print();
    ms::highlight(this);
}

void LineSegment::updateLength() {
    if (!dirtyLength) return;
    
    lengthLowerBound = 0;
    for (auto* state : getStates()) {
        if (!moveMutable || state->isMutable()) {
            lengthLowerBound += state->getLength();
        }
    }
    
    dirtyLength = false;
}

} // namespace ms 