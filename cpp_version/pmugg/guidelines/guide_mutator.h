#pragma once
#include "node_stats.h"
#include "optimizer.h"
#include <functional>
#include <memory>

namespace ms {

class Classifier;
class Timer;
class Model;
class MutationArea;
class HierarchyMutator;

class GuideMutator {
public:
    enum class Status {
        FAILED = 0,
        UNRESOLVED = 1,
        PAUSED = 2,
        SUCCESS = 3,
        FINISHED = 4
    };

    GuideMutator(Classifier* classifier, Timer* timer, std::function<void(bool)> notifyFunc);
    ~GuideMutator() = default;

    // Core functionality
    void seed(MutationArea* mutationArea);
    Status resolve(Model* outputModel);
    void mutate();
    void accept();
    void reject();
    void save(Model* model = nullptr);
    bool initializeModel(Model* model);
    void reset();
    void pause();
    void unresolve();
    void step(int numTasks);
    void setBreakTime(double breakTime);
    void setEndTime(double earlyEndTime, double endTime);
    Status getStatus() const;
    
    // Stats and reporting
    void displayStats();
    std::string report() const;
    NodeStats* getNodeStats();
    Classifier* getClassifier() const;

    // Static members
    static int taskCount;
    static constexpr int findMutableVertexAttempts = 20;
    static constexpr int findMutableLineAttempts = 20;
    static bool forceContinued;
    static bool closeInspection(int taskCount);
    static void forceContinue();
    static void hideStats();

private:
    Classifier* classifier;
    Timer* timer;
    std::function<void(bool)> notify;
    std::vector<void*> tasks;  // Generic tasks placeholder
    Status status;
    double breakTime;
    double earlyEndTime;
    double endTime;
    int seedCount;
    int pauseCount;
    NodeStats nodeStats;
    Optimizer optimizer;
    std::unique_ptr<HierarchyMutator> hierarchyMutator;
    
    // Helper methods
    Vertex* findMutableVertex();
    void initializeHierarchyMutator();
};

} // namespace ms 