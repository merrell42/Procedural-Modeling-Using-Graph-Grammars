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
    
    UE_LOG(LogTemp, Log, TEXT("Received mesh: %d vertices, %d triangles, %d faces"), 
           meshData.numVertices, meshData.numTriangles, meshData.numFaces);
    
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
    for (TActorIterator<AStaticMeshActor> ActorItr(World); ActorItr; ++ActorItr) {
        if (ActorItr->GetName() == TEXT("GeneratedMesh")) {
            ActorItr->Destroy();
            break;
        }
    }
    
    // Create new mesh actor
    AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>();
    MeshActor->SetActorLabel(TEXT("GeneratedMesh"));
    UE_LOG(LogTemp, Log, TEXT("Created new mesh actor"));
    
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
    TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
    
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
        
        // Set normals for vertex instances
        FVector3f Normal0(
            meshData.normals[i0 * 3],     // X
            meshData.normals[i0 * 3 + 2], // Z -> Y  
            meshData.normals[i0 * 3 + 1]  // Y -> Z
        );
        FVector3f Normal1(
            meshData.normals[i2 * 3],     // X
            meshData.normals[i2 * 3 + 2], // Z -> Y  
            meshData.normals[i2 * 3 + 1]  // Y -> Z
        );
        FVector3f Normal2(
            meshData.normals[i1 * 3],     // X
            meshData.normals[i1 * 3 + 2], // Z -> Y  
            meshData.normals[i1 * 3 + 1]  // Y -> Z
        );
        
        VertexInstanceNormals[Instance0] = Normal0;
        VertexInstanceNormals[Instance1] = Normal1;
        VertexInstanceNormals[Instance2] = Normal2;
        
        // Create polygon using vertex instances
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
    
    UE_LOG(LogTemp, Log, TEXT("Mesh updated successfully in scene!"));
} 