#pragma once

#include "CoreMinimal.h"

class GRAMMAREDITOR_API FGrammarDLL {
public:
    static bool LoadDLL();
    static void UnloadDLL();
    static bool IsDLLLoaded();

	static void LoadGrammarFile(const FString& FilePath);
	static void Step();
	static void UpdateMesh();

private:
    static void* DLLHandle;
    static bool bIsLoaded;
    
    // Mesh structure matching the DLL
    struct MeshCpp {
        float* positions;
        float* normals;
        int* triangles;
        int* faceIndices;
        int numVertices;
        int numTriangles;
        int numFaces;
    };

    // Function pointers
    typedef void (*InitializeFunc)(const char*, char*, int, int);
    typedef void (*IterateFunc)(int);
    typedef MeshCpp (*GetMeshFunc)();
    typedef void (*DestroyMeshFunc)(MeshCpp&);
    static InitializeFunc Initialize;
    static IterateFunc Iterate;
    static GetMeshFunc GetMesh;
    static DestroyMeshFunc DestroyMesh;
}; 