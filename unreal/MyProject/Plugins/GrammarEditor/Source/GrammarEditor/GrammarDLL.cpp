#include "GrammarDLL.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"

// Static member initialization
void* FGrammarDLL::DLLHandle = nullptr;
bool FGrammarDLL::bIsLoaded = false;
FGrammarDLL::InitializeFunc FGrammarDLL::Initialize = nullptr;
FGrammarDLL::IterateFunc FGrammarDLL::Iterate = nullptr;

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
    
    if (Initialize == nullptr || Iterate == nullptr) {
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
    
    UE_LOG(LogTemp, Log, TEXT("Grammar DLL unloaded"));
}

bool FGrammarDLL::IsDLLLoaded() {
    return bIsLoaded;
}

bool FGrammarDLL::LoadGrammarFile(const FString& FilePath) {
    if (!bIsLoaded || Initialize == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded!"));
        return false;
    }
    
    // Convert to ANSI for the DLL call
    FTCHARToUTF8 AnsiPath(*FilePath);
    
    char result[1000];
    Initialize(AnsiPath.Get(), result, 1000, 0);
    
    UE_LOG(LogTemp, Log, TEXT("Loading: %s"), ANSI_TO_TCHAR(result));
    return true;
}

bool FGrammarDLL::Step() {
    if (!bIsLoaded || Iterate == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("DLL is not loaded or iterate function not found!"));
        return false;
    }
    
    // Call iterate with 1 step
    Iterate(1);
    
    UE_LOG(LogTemp, Log, TEXT("Grammar step completed successfully!"));
    return true;
} 