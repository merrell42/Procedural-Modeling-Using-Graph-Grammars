#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"
#include "MeshDescription.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "DrawDebugHelpers.h"
#include "Components/LineBatchComponent.h"

class GRAMMAREDITOR_API FGrammarDLL {
public:
    static bool LoadDLL();
    static void UnloadDLL();
    static bool IsDLLLoaded();

	static void LoadGrammarFile(const FString& FilePath);
	static void Step();
	static int StepToTime(float TimeSeconds);
	static void UpdateMesh();
	static void Reset(int Seed = 0);
	static void SetSize(float X, float Y, float Z);

private:
    static void* DLLHandle;
    static bool isLoaded;
    
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
    typedef int (*IterateToTimeFunc)(float);
    typedef MeshCpp (*GetMeshFunc)();
    typedef void (*DestroyMeshFunc)(MeshCpp&);
    typedef void (*ResetFunc)(int);
    typedef void (*SetSizeFunc)(float, float, float);
    static InitializeFunc Initialize;
    static IterateFunc Iterate;
    static IterateToTimeFunc IterateToTime;
    static GetMeshFunc GetMesh;
    static DestroyMeshFunc DestroyMesh;
    static ResetFunc ResetFunction;
    static SetSizeFunc SetSizeFunction;
}; 