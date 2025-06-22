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
	static void Reset(int Seed = 0);

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
    typedef void (*ResetFunc)(int);
    static InitializeFunc Initialize;
    static IterateFunc Iterate;
    static GetMeshFunc GetMesh;
    static DestroyMeshFunc DestroyMesh;
    static ResetFunc ResetFunction;
}; 