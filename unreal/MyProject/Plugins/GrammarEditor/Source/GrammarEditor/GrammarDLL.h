#pragma once

#include "CoreMinimal.h"

class GRAMMAREDITOR_API FGrammarDLL {
public:
    static bool LoadDLL();
    static void UnloadDLL();
    static bool IsDLLLoaded();

	static bool LoadGrammarFile(const FString& FilePath);
	static bool Step();

private:
    static void* DLLHandle;
    static bool bIsLoaded;
    
    // Function pointers
    typedef void (*InitializeFunc)(const char*, char*, int, int);
    typedef void (*IterateFunc)(int);
    static InitializeFunc Initialize;
    static IterateFunc Iterate;
}; 