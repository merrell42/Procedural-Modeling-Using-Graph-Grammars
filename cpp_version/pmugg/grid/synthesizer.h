#pragma once
#include "model.h"
#include "timer.h"
#include <functional>

namespace ms {

class View;
class CameraController;
class Seeder;
class Mutator;
class MutationArea;

class Synthesizer {
public:
    Synthesizer(Example* example, View* view, CameraController* cameraController);
    ~Synthesizer();

    // Core functionality
    Model* getModel();
    Vec2 offset() const;
    void notify(bool fullRedraw = false);
    void setExample(Example* example);
    void initializeMutationArea();
    void synthesize(const std::vector<int>& extents, bool keepPaused, std::function<void(const std::string&)> callback);
    void finish();
    void mutateUntilDone();
    void pause();
    void resume();
    void step(int taskNum);
    void startMutation();
    void mutate();
    void mutateGround();
    void onMouseDown(float x, float y);
    std::string exportOutput();

    static constexpr float BORDER_THRESHOLD = 0.1f;

private:
    Seeder* seeder;
    View* observer;
    Model* model;
    Mutator* mutator;
    MutationArea* mutationArea;
    int numAttempts;
    Timer* timer;
    CameraController* cameraController;
    bool is3D;
    std::function<void(const std::string&)> callback;
    Vec3 mutationOrigin;
    bool timedOut;
};

} // namespace ms 