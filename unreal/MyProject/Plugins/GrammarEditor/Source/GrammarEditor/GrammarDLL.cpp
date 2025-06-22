#include "GrammarDLL.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"

// Static member initialization
void* FGrammarDLL::DLLHandle = nullptr;
bool FGrammarDLL::bIsLoaded = false;
FGrammarDLL::InitializeFunc FGrammarDLL::Initialize = nullptr;

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
    
    if (Initialize == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("Failed to get 'initialize' function from Grammar DLL"));
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
    
    UE_LOG(LogTemp, Log, TEXT("Grammar DLL unloaded"));
}

bool FGrammarDLL::IsDLLLoaded() {
    return bIsLoaded;
}

bool FGrammarDLL::TestDLLConnection() {
    if (!bIsLoaded || Initialize == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded!"));
        return false;
    }
    
    // Test with a sample grammar file
    FString GrammarPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("../../grammar data/2D Basic Shapes/diagonal box.json"));
    FString FullPath = FPaths::ConvertRelativePathToFull(GrammarPath);
    
    if (!FPaths::FileExists(FullPath)) {
        UE_LOG(LogTemp, Warning, TEXT("Grammar file not found, testing with empty path"));
        FullPath = TEXT("");
    }
    
    // Convert to ANSI for the DLL call
    FTCHARToUTF8 AnsiPath(*FullPath);
    
    char result[1000];
    Initialize(AnsiPath.Get(), result, 1000, 0);
    
    UE_LOG(LogTemp, Log, TEXT("DLL test successful! Result: %s"), ANSI_TO_TCHAR(result));
    return true;
} 