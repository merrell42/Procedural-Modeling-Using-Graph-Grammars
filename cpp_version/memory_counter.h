#pragma once
#include <iostream>

class MemoryCounter {
public:
    static int vertexCreated;
    static int vertexDestroyed;
    static int edgeCreated;
    static int edgeDestroyed;
    static int halfEdgeCreated;
    static int halfEdgeDestroyed;
    static int faceCreated;
    static int faceDestroyed;

    static void printStatistics();
}; 