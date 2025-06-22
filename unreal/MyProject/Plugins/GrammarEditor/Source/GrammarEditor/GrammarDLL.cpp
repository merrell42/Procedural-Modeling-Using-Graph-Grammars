#include "GrammarDLL.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Windows/MinWindows.h"

// Static member initialization
void* FGrammarDLL::DLLHandle = nullptr;
bool FGrammarDLL::bIsLoaded = false;
FGrammarDLL::InitializeFunc FGrammarDLL::Initialize = nullptr;

bool FGrammarDLL::LoadDLL()
{
    if (bIsLoaded)
    {
        UE_LOG(LogTemp, Log, TEXT("DLL already loaded"));
        return true;
    }

    // Get the path to the DLL
    FString DLLPath = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("GrammarEditor/Binaries/Win64/pmugg dll.dll"));
    
    UE_LOG(LogTemp, Log, TEXT("Attempting to load DLL from: %s"), *DLLPath);
    
    // Check if file exists
    if (!FPaths::FileExists(DLLPath))
    {
        UE_LOG(LogTemp, Error, TEXT("DLL file does not exist at: %s"), *DLLPath);
        return false;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("DLL file exists at: %s"), *DLLPath);
    }
    
    // Load the DLL
    DLLHandle = FPlatformProcess::GetDllHandle(*DLLPath);
    
    if (DLLHandle == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Grammar DLL from: %s"), *DLLPath);
        UE_LOG(LogTemp, Error, TEXT("GetLastError: %d"), GetLastError());
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("DLL loaded successfully, getting function pointers..."));
    
    // Get function pointers
    Initialize = (InitializeFunc)FPlatformProcess::GetDllExport(DLLHandle, TEXT("initialize"));
    
    if (Initialize == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get 'initialize' function from Grammar DLL"));
        UnloadDLL();
        return false;
    }
    
    bIsLoaded = true;
    UE_LOG(LogTemp, Log, TEXT("Grammar DLL loaded successfully!"));
    return true;
}

void FGrammarDLL::UnloadDLL()
{
    if (DLLHandle)
    {
        FPlatformProcess::FreeDllHandle(DLLHandle);
        DLLHandle = nullptr;
    }
    
    bIsLoaded = false;
    Initialize = nullptr;
    
    UE_LOG(LogTemp, Log, TEXT("Grammar DLL unloaded"));
}

bool FGrammarDLL::IsDLLLoaded()
{
    return bIsLoaded;
}

bool FGrammarDLL::TestDLLConnection()
{
    if (!bIsLoaded || Initialize == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("DLL not loaded or initialize function not found"));
        return false;
    }
    
    // Test with an actual grammar file
    FString GrammarPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("../../grammar data/2D Basic Shapes/diagonal box.json"));
    FString FullPath = FPaths::ConvertRelativePathToFull(GrammarPath);
    
    UE_LOG(LogTemp, Log, TEXT("Testing DLL with grammar file: %s"), *FullPath);
    
    // Check if the grammar file exists
    if (!FPaths::FileExists(FullPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Grammar file not found at: %s"), *FullPath);
        UE_LOG(LogTemp, Log, TEXT("Testing with empty path instead..."));
        
        // Test with empty path
        char result[1000];
        Initialize("", result, 1000, 0);
        UE_LOG(LogTemp, Log, TEXT("DLL test call completed. Result: %s"), ANSI_TO_TCHAR(result));
        return true;
    }
    
    // Convert to ANSI for the DLL call
    FTCHARToUTF8 AnsiPath(*FullPath);
    
    char result[1000];
    Initialize(AnsiPath.Get(), result, 1000, 0);
    
    UE_LOG(LogTemp, Log, TEXT("DLL test call completed. Result: %s"), ANSI_TO_TCHAR(result));
    return true;
} 