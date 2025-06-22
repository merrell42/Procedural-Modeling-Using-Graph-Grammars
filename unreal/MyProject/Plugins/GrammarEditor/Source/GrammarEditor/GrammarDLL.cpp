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
    UpdateMeshProcedural();
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
    
    // Find and destroy existing mesh actor
    int32 FoundActors = 0;
    for (TActorIterator<AStaticMeshActor> ActorItr(World); ActorItr; ++ActorItr) {
        if (ActorItr->GetActorLabel() == TEXT("GeneratedMesh")) {
            World->DestroyActor(*ActorItr);
            FoundActors++;
        }
    }
    
    // Create new mesh actor
    AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>();
    MeshActor->SetActorLabel(TEXT("GeneratedMesh"));
    
    // Create static mesh
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
    StaticMesh->SetFlags(RF_Public | RF_Standalone);
    
    // Create mesh description
    FStaticMeshSourceModel& SourceModel = StaticMesh->AddSourceModel();
    FMeshDescription* MeshDescription = StaticMesh->CreateMeshDescription(0);
    
    // Build mesh data
    FStaticMeshAttributes Attributes(*MeshDescription);
    Attributes.Register();
    
    TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
    TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
    
    // Create polygon group for the mesh
    FPolygonGroupID PolygonGroupID = MeshDescription->CreatePolygonGroup();
    
    // Add vertices (convert Y and Z coordinates like Unity/Godot)
    TArray<FVertexID> VertexIDs;
    VertexIDs.Reserve(meshData.numVertices);
    
    for (int32 i = 0; i < meshData.numVertices; i++) {
        FVertexID VertexID = MeshDescription->CreateVertex();
        VertexIDs.Add(VertexID);
        
        FVector3f Position(
            meshData.positions[i * 3] * 100.0f,
            meshData.positions[i * 3 + 1] * 100.0f,
            meshData.positions[i * 3 + 2] * 100.0f + 100.0f
        );
        VertexPositions[VertexID] = Position;
    }
    
    // Add triangles
    for (int32 i = 0; i < meshData.numTriangles; i++) {
        // Reverse the orientation of the triangles.
        int32 i0 = meshData.triangles[i * 3];
        int32 i1 = meshData.triangles[i * 3 + 2];
        int32 i2 = meshData.triangles[i * 3 + 1];
        
        // Validate indices
        if (i0 >= meshData.numVertices || i1 >= meshData.numVertices || i2 >= meshData.numVertices) {
            continue;
        }
        
        // Create vertex instances for this triangle
        TArray<FVertexInstanceID> VertexInstanceIDs;
        VertexInstanceIDs.Reserve(3);
        
        // Create vertex instances (reverse winding order for Unreal)
        FVertexInstanceID Instance0 = MeshDescription->CreateVertexInstance(VertexIDs[i0]);
        FVertexInstanceID Instance1 = MeshDescription->CreateVertexInstance(VertexIDs[i2]);
        FVertexInstanceID Instance2 = MeshDescription->CreateVertexInstance(VertexIDs[i1]);
        
        VertexInstanceIDs.Add(Instance0);
        VertexInstanceIDs.Add(Instance1);
        VertexInstanceIDs.Add(Instance2);
        
        // Set UV coordinates for vertex instances
        VertexInstanceUVs[Instance0] = FVector2f(0.0f, 0.0f);
        VertexInstanceUVs[Instance1] = FVector2f(1.0f, 0.0f);
        VertexInstanceUVs[Instance2] = FVector2f(0.0f, 1.0f);
        
        // Create polygon using vertex instances (let Unreal calculate normals)
        MeshDescription->CreatePolygon(PolygonGroupID, VertexInstanceIDs);
    }
    
    // Commit mesh description
    StaticMesh->CommitMeshDescription(0);
    
    // Build the static mesh
    StaticMesh->Build(false);
    
    // Set the mesh on the actor
    if (UStaticMeshComponent* MeshComponent = MeshActor->GetStaticMeshComponent()) {
        MeshComponent->SetStaticMesh(StaticMesh);
        
        // Set a basic material
        UMaterial* DefaultMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
        if (DefaultMaterial) {
            MeshComponent->SetMaterial(0, DefaultMaterial);
        }
    }
    
    // Clean up DLL memory
    DestroyMesh(meshData);
}

void FGrammarDLL::Reset(int Seed) {
    if (!bIsLoaded || ResetFunction == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded or reset function not found!"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Resetting grammar with seed: %d"), Seed);
    ResetFunction(Seed);
    UpdateMeshProcedural();
}

void FGrammarDLL::UpdateMeshProcedural() {
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
    
    // Find and destroy existing mesh actor
    int32 FoundActors = 0;
    for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr) {
        if (ActorItr->GetActorLabel() == TEXT("GeneratedMesh")) {
            World->DestroyActor(*ActorItr);
            FoundActors++;
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