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
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"

// Static member initialization
void* FGrammarDLL::DLLHandle = nullptr;
bool FGrammarDLL::bIsLoaded = false;
FGrammarDLL::InitializeFunc FGrammarDLL::Initialize = nullptr;
FGrammarDLL::IterateFunc FGrammarDLL::Iterate = nullptr;
FGrammarDLL::GetMeshFunc FGrammarDLL::GetMesh = nullptr;
FGrammarDLL::DestroyMeshFunc FGrammarDLL::DestroyMesh = nullptr;
FGrammarDLL::ResetFunc FGrammarDLL::ResetFunction = nullptr;

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
    ResetFunction = (ResetFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("reset"));
    
    if (Initialize == nullptr || Iterate == nullptr || GetMesh == nullptr || DestroyMesh == nullptr || ResetFunction == nullptr) {
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
    ResetFunction = nullptr;
    
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
    Reset(0);
}

void FGrammarDLL::Step() {
    if (!bIsLoaded || Iterate == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded or iterate function not found!"));
        return;
    }
    Iterate(1);
    UpdateMesh();
}

void FGrammarDLL::Reset(int Seed) {
    if (!bIsLoaded || ResetFunction == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded or reset function not found!"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Resetting grammar with seed: %d"), Seed);
    ResetFunction(Seed);
    UpdateMesh();
}

void FGrammarDLL::UpdateMesh() {
    if (!bIsLoaded || GetMesh == nullptr || DestroyMesh == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded or mesh functions not found!"));
        return;
    }
    
    // Get mesh data from DLL
    MeshCpp meshData = GetMesh();
    
    // Get the current world
    UWorld* World = nullptr;
    if (GEngine && GEngine->GetWorldContexts().Num() > 0) {
        World = GEngine->GetWorldContexts()[0].World();
    }
    
    if (!World) {
        UE_LOG(LogTemp, Error, TEXT("No world found for mesh creation"));
        DestroyMesh(meshData);
        return;
    }
    
    // Find and destroy existing mesh actor.
    for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr) {
        if (ActorItr->GetActorLabel() == TEXT("GeneratedMesh")) {
            World->DestroyActor(*ActorItr);
            break;
        }
    }
    
    // Create new actor with procedural mesh component
    AActor* MeshActor = World->SpawnActor<AActor>();
    MeshActor->SetActorLabel(TEXT("GeneratedMesh"));
    
    // Add procedural mesh component
    UProceduralMeshComponent* ProcMeshComponent = NewObject<UProceduralMeshComponent>(MeshActor);
    MeshActor->SetRootComponent(ProcMeshComponent);
    ProcMeshComponent->RegisterComponent();
    
    // Prepare mesh data for procedural component
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;
    
    Vertices.Reserve(meshData.numVertices);
    Triangles.Reserve(meshData.numTriangles * 3);
    Normals.Reserve(meshData.numVertices);
    UVs.Reserve(meshData.numVertices);
    VertexColors.Reserve(meshData.numVertices);
    Tangents.Reserve(meshData.numVertices);
    
    // Add vertices (convert Y and Z coordinates)
    for (int32 i = 0; i < meshData.numVertices; i++) {
        FVector Position(
            meshData.positions[i * 3] * 100.0f,
            meshData.positions[i * 3 + 1] * 100.0f,
            meshData.positions[i * 3 + 2] * 100.0f + 100.0f
        );
        Vertices.Add(Position);
        
        // Add default values for other attributes
        Normals.Add(FVector(0, 0, 1));
        UVs.Add(FVector2D(0, 0));
        VertexColors.Add(FColor::White);
        Tangents.Add(FProcMeshTangent(FVector(1, 0, 0), false));
    }
    
    for (int32 i = 0; i < meshData.numTriangles; i++) {
        int32 i0 = meshData.triangles[i * 3];
        int32 i1 = meshData.triangles[i * 3 + 1];
        int32 i2 = meshData.triangles[i * 3 + 2];
        
        // Validate indices
        if (i0 >= meshData.numVertices || i1 >= meshData.numVertices || i2 >= meshData.numVertices) {
            continue;
        }
        
        Triangles.Add(i0);
        Triangles.Add(i1);
        Triangles.Add(i2);
    }
    
    // Create the procedural mesh section
    ProcMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
    
    // Set a basic material
    UMaterial* DefaultMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
    if (DefaultMaterial) {
        ProcMeshComponent->SetMaterial(0, DefaultMaterial);
    }
    
    // Clean up DLL memory
    DestroyMesh(meshData);
} 