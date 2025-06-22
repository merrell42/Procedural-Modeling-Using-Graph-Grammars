#pragma once

#include "CoreMinimal.h"

class GRAMMAREDITOR_API FGrammarDLL
{
public:
    static bool LoadDLL();
    static void UnloadDLL();
    static bool IsDLLLoaded();
    
    // Simple test function - just try to call initialize
    static bool TestDLLConnection();

private:
    static void* DLLHandle;
    static bool bIsLoaded;
    
    // Function pointers
    typedef void (*InitializeFunc)(const char*, char*, int, int);
    static InitializeFunc Initialize;
}; 