#include "pch.h"
#include "memory_counter.h"

// Initialize static variables
int MemoryCounter::vertexCreated = 0;
int MemoryCounter::vertexDestroyed = 0;
int MemoryCounter::edgeCreated = 0;
int MemoryCounter::edgeDestroyed = 0;
int MemoryCounter::halfEdgeCreated = 0;
int MemoryCounter::halfEdgeDestroyed = 0;
int MemoryCounter::faceCreated = 0;
int MemoryCounter::faceDestroyed = 0;

void MemoryCounter::printStatistics() {
    std::cout << "\n=== Memory Leak Detection Statistics ===" << std::endl;
    std::cout << "Vertices:  Created=" << vertexCreated << ", Destroyed=" << vertexDestroyed 
              << ", Leaked=" << (vertexCreated - vertexDestroyed) << std::endl;
    std::cout << "Edges:     Created=" << edgeCreated << ", Destroyed=" << edgeDestroyed 
              << ", Leaked=" << (edgeCreated - edgeDestroyed) << std::endl;
    std::cout << "HalfEdges: Created=" << halfEdgeCreated << ", Destroyed=" << halfEdgeDestroyed 
              << ", Leaked=" << (halfEdgeCreated - halfEdgeDestroyed) << std::endl;
    std::cout << "Faces:     Created=" << faceCreated << ", Destroyed=" << faceDestroyed 
              << ", Leaked=" << (faceCreated - faceDestroyed) << std::endl;
    std::cout << "=========================================" << std::endl;
} 