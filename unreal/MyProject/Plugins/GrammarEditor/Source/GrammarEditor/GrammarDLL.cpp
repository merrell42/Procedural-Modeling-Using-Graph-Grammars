#include "GrammarDLL.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "RenderingThread.h"
#include "PhysicsEngine/BodySetup.h"

// Static member initialization
void* FGrammarDLL::DLLHandle = nullptr;
bool FGrammarDLL::bIsLoaded = false;
FGrammarDLL::InitializeFunc FGrammarDLL::Initialize = nullptr;
FGrammarDLL::IterateFunc FGrammarDLL::Iterate = nullptr;
FGrammarDLL::GetMeshFunc FGrammarDLL::GetMesh = nullptr;
FGrammarDLL::DestroyMeshFunc FGrammarDLL::DestroyMesh = nullptr;

bool FGrammarDLL::LoadDLL() {
    if (bIsLoaded) {
        return true;
    }

    // Get the path to the DLL
    FString DLLPath = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("GrammarEditor/Binaries/Win64/pmugg dll.dll"));
    
    // Load the DLL
    DLLHandle = FPlatformProcess::GetDllHandle(*DLLPath);
    
    if (DLLHandle == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Grammar DLL from: %s"), *DLLPath);
        return false;
    }
    
    // Get function pointers
    Initialize = (InitializeFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("initialize"));
    Iterate = (IterateFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("iterate"));
    GetMesh = (GetMeshFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("getMesh"));
    DestroyMesh = (DestroyMeshFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("destroyMesh"));
    
    if (Initialize == nullptr || Iterate == nullptr || GetMesh == nullptr || DestroyMesh == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("Failed to get required functions from Grammar DLL"));
        UnloadDLL();
        return false;
    }
    
    bIsLoaded = true;
    UE_LOG(LogTemp, Log, TEXT("Grammar DLL loaded successfully!"));
    return true;
}

void FGrammarDLL::UnloadDLL() {
    if (DLLHandle) {
        FPlatformProcess::FreeDllHandle(DLLHandle);
        DLLHandle = nullptr;
    }
    
    bIsLoaded = false;
    Initialize = nullptr;
    Iterate = nullptr;
    GetMesh = nullptr;
    DestroyMesh = nullptr;
    
    UE_LOG(LogTemp, Log, TEXT("Grammar DLL unloaded"));
}

bool FGrammarDLL::IsDLLLoaded() {
    return bIsLoaded;
}

void FGrammarDLL::LoadGrammarFile(const FString& FilePath) {
    if (!bIsLoaded || Initialize == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded!"));
        return;
    }
    
    // Convert to ANSI for the DLL call
    FTCHARToUTF8 AnsiPath(*FilePath);
    
    char result[1000];
    Initialize(AnsiPath.Get(), result, 1000, 0);
    
    UE_LOG(LogTemp, Log, TEXT("Loading: %s"), ANSI_TO_TCHAR(result));
}

void FGrammarDLL::Step() {
    if (!bIsLoaded || Iterate == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded or iterate function not found!"));
        return;
    }
    Iterate(1);
    UpdateMesh();
}

void FGrammarDLL::UpdateMesh() {
    if (!bIsLoaded || GetMesh == nullptr || DestroyMesh == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded or mesh functions not found!"));
        return;
    }
    
    // Get mesh data from DLL
    MeshCpp meshData = GetMesh();
    
    /*if (meshData.numVertices == 0 || meshData.numTriangles == 0) {
        UE_LOG(LogTemp, Warning, TEXT("Empty mesh received from DLL"));
        DestroyMesh(meshData);
        return false;
    } */
    
    UE_LOG(LogTemp, Log, TEXT("Received mesh: %d vertices, %d triangles, %d faces"), 
           meshData.numVertices, meshData.numTriangles, meshData.numFaces);
    
    // Clean up DLL memory
    DestroyMesh(meshData);
} 