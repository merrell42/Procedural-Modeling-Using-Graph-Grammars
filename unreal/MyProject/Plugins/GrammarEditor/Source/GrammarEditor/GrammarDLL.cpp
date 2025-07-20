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
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"
#include "Components/LineBatchComponent.h"

// Static member initialization.
void* FGrammarDLL::DLLHandle = nullptr;
bool FGrammarDLL::isLoaded = false;
FGrammarDLL::InitializeFunc FGrammarDLL::Initialize = nullptr;
FGrammarDLL::IterateFunc FGrammarDLL::Iterate = nullptr;
FGrammarDLL::IterateToTimeFunc FGrammarDLL::IterateToTime = nullptr;
FGrammarDLL::GetMeshFunc FGrammarDLL::GetMesh = nullptr;
FGrammarDLL::DestroyMeshFunc FGrammarDLL::DestroyMesh = nullptr;
FGrammarDLL::ResetFunc FGrammarDLL::ResetFunction = nullptr;
FGrammarDLL::SetSizeFunc FGrammarDLL::SetSizeFunction = nullptr;

bool FGrammarDLL::LoadDLL() {
    if (isLoaded) {
        return true;
    }

    // Load the DLL.
    FString DLLPath = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("GrammarEditor/Binaries/Win64/pmugg release.dll"));
    DLLHandle = FPlatformProcess::GetDllHandle(*DLLPath);
    
    if (DLLHandle == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Grammar DLL from: %s"), *DLLPath);
        return false;
    }
    
    // Get function pointers.
    Initialize = (InitializeFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("initialize"));
    Iterate = (IterateFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("iterate"));
    IterateToTime = (IterateToTimeFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("iterateToTime"));
    GetMesh = (GetMeshFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("getMesh"));
    DestroyMesh = (DestroyMeshFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("destroyMesh"));
    ResetFunction = (ResetFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("reset"));
    SetSizeFunction = (SetSizeFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("setSize"));
    
    if (Initialize == nullptr || Iterate == nullptr || IterateToTime == nullptr || GetMesh == nullptr || DestroyMesh == nullptr ||
        ResetFunction == nullptr || SetSizeFunction == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("Failed to get required functions from Grammar DLL"));
        UnloadDLL();
        return false;
    }
    
    isLoaded = true;
    UE_LOG(LogTemp, Log, TEXT("Grammar DLL loaded successfully!"));
    return true;
}

void FGrammarDLL::UnloadDLL() {
    if (DLLHandle) {
        FPlatformProcess::FreeDllHandle(DLLHandle);
        DLLHandle = nullptr;
    }
    
    isLoaded = false;
    Initialize = nullptr;
    Iterate = nullptr;
    IterateToTime = nullptr;
    GetMesh = nullptr;
    DestroyMesh = nullptr;
    ResetFunction = nullptr;
    SetSizeFunction = nullptr;
    
    UE_LOG(LogTemp, Log, TEXT("Grammar DLL unloaded."));
}

bool FGrammarDLL::IsDLLLoaded() {
    return isLoaded;
}

void FGrammarDLL::LoadGrammarFile(const FString& FilePath) {
    if (!isLoaded || Initialize == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded!"));
        return;
    }
    
    // Convert to ANSI for the DLL call.
    FTCHARToUTF8 AnsiPath(*FilePath);
    
    char result[1000];
    Initialize(AnsiPath.Get(), result, 1000, 0);
    
    UE_LOG(LogTemp, Log, TEXT("Loading: %s"), ANSI_TO_TCHAR(result));
    Reset(0);
}

void FGrammarDLL::Step() {
    if (!isLoaded || Iterate == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded or iterate function not found!"));
        return;
    }

    Iterate(1);
    UpdateMesh();
}

int FGrammarDLL::StepToTime(float TimeSeconds) {
    if (!isLoaded || IterateToTime == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded or iterateToTime function not found!"));
        return 0;
    }

    int stepsCompleted = IterateToTime(TimeSeconds);
    UpdateMesh();
    return stepsCompleted;
}

void FGrammarDLL::Reset(int Seed) {
    if (!isLoaded || ResetFunction == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded or reset function not found!"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Resetting grammar with seed: %d"), Seed);
    ResetFunction(Seed);
    UpdateMesh();
}

void FGrammarDLL::SetSize(float X, float Y, float Z) {
    if (!isLoaded || SetSizeFunction == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded or setSize function not found!"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Setting size to: X=%f, Y=%f, Z=%f"), X, Y, Z);
    SetSizeFunction(X, Y, Z);
}

void FGrammarDLL::UpdateMesh() {
    if (!isLoaded || GetMesh == nullptr || DestroyMesh == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded or mesh functions not found!"));
        return;
    }
    
    // Helper function for drawing lines.
    auto DrawLine = [](ULineBatchComponent* LineBatch, const FVector& Start, const FVector& End) {
        LineBatch->DrawLine(Start, End, FLinearColor::Black, SDPG_World, 2.0f, 0.0f);
    };
    
    // Get mesh data from DLL.
    MeshCpp meshData = GetMesh();
    
    // Get the current world.
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
    
    // Create new actor with procedural mesh component.
    AActor* MeshActor = World->SpawnActor<AActor>();
    MeshActor->SetActorLabel(TEXT("GeneratedMesh"));
    
    // Add procedural mesh component.
    UProceduralMeshComponent* ProcMeshComponent = NewObject<UProceduralMeshComponent>(MeshActor);
    MeshActor->SetRootComponent(ProcMeshComponent);
    ProcMeshComponent->RegisterComponent();
    
    // Add line batch component for drawing edges.
    ULineBatchComponent* LineBatchComponent = NewObject<ULineBatchComponent>(MeshActor);
    LineBatchComponent->RegisterComponent();
    
    // Process each submesh
    for (int32 submeshIndex = 0; submeshIndex < meshData.numSubmeshes; submeshIndex++) {
        SubmeshCpp submesh = meshData.submeshes[submeshIndex];
        
        // Prepare mesh data for this submesh
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FColor> VertexColors;
        TArray<FProcMeshTangent> Tangents;
        
        Vertices.Reserve(submesh.numVertices);
        Triangles.Reserve(submesh.numTriangles * 3);
        Normals.Reserve(submesh.numVertices);
        UVs.Reserve(submesh.numVertices);
        VertexColors.Reserve(submesh.numVertices);
        Tangents.Reserve(submesh.numVertices);

        // Convert vertices and normals
        for (int32 i = 0; i < submesh.numVertices; i++) {
            FVector Position(
                submesh.positions[i * 3] * 100.0f,
                submesh.positions[i * 3 + 1] * 100.0f,
                submesh.positions[i * 3 + 2] * 100.0f + 0.1f
            );
            Vertices.Add(Position);

            FVector Normal(
                submesh.normals[i * 3],
                submesh.normals[i * 3 + 1],
                submesh.normals[i * 3 + 2]
            );
            Normals.Add(Normal);
            
            UVs.Add(FVector2D(0, 0));
            VertexColors.Add(FColor::White);
            Tangents.Add(FProcMeshTangent(FVector(1, 0, 0), false));
        }
        
        // Convert triangles
        for (int32 i = 0; i < submesh.numTriangles; i++) {
            int32 i0 = submesh.triangles[i * 3];
            int32 i1 = submesh.triangles[i * 3 + 1];
            int32 i2 = submesh.triangles[i * 3 + 2];
            
            // Validate indices.
            if (i0 >= submesh.numVertices || i1 >= submesh.numVertices || i2 >= submesh.numVertices) {
                continue;
            }
            
            Triangles.Add(i0);
            Triangles.Add(i1);
            Triangles.Add(i2);
        }
        
        // Create the procedural mesh section for this submesh
        ProcMeshComponent->CreateMeshSection(submeshIndex, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
        
        // Create material for this submesh based on color
        UMaterial* SubmeshMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
        if (SubmeshMaterial) {
            // Create a dynamic material instance to set the color
            UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(SubmeshMaterial, nullptr);
            if (DynamicMaterial) {
                DynamicMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(submesh.red, submesh.green, submesh.blue, 1.0f));
                ProcMeshComponent->SetMaterial(submeshIndex, DynamicMaterial);
            } else {
                ProcMeshComponent->SetMaterial(submeshIndex, SubmeshMaterial);
            }
        }
        
        // Generate edges from face indices and draw them using LineBatchComponent
        int32 startIndex = 0;
        for (int32 i = 0; i < submesh.numFaces; i++) {
            int32 endIndex = submesh.faceIndices[i];
            
            // Create edges for each face
            for (int32 j = startIndex; j < endIndex - 1; j++) {
                DrawLine(LineBatchComponent, Vertices[j], Vertices[j + 1]);
            }
            DrawLine(LineBatchComponent, Vertices[endIndex - 1], Vertices[startIndex]);
            
            startIndex = endIndex;
        }
    }
    
    // Clean up DLL memory.
    DestroyMesh(meshData);
} 