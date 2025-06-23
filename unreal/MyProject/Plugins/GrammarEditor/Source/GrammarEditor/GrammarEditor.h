#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Commands/UIAction.h"

class SDockTab;
class FSpawnTabArgs;

class FGrammarEditorModule : public IModuleInterface {
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);
	void RegisterMenus();
	void OpenGrammarEditor();
	FReply OnLoadGrammarClicked();
	FReply OnLoadFolderClicked();
	FReply OnStepClicked();
	FReply OnResetClicked();
	FReply OnPlayClicked();
	void OnSeedChanged(const FText& NewText);
	void OnSizeXChanged(const FText& NewText);
	void OnSizeYChanged(const FText& NewText);
	void OnSizeZChanged(const FText& NewText);
	void ProcessNextFileInFolder();
	void FindJsonFiles(const FString& DirectoryPath);
	bool IsDLLReady();
	void UpdateStatusText();
	void SetButtonStates(bool bEnabled);

private:
	TSharedPtr<SButton> StepButton;
	TSharedPtr<SButton> ResetButton;
	TSharedPtr<SButton> PlayButton;
	TSharedPtr<SButton> LoadFolderButton;
	TSharedPtr<STextBlock> PlayButtonText;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SEditableTextBox> SeedInput;
	TSharedPtr<SEditableTextBox> SizeXInput;
	TSharedPtr<SEditableTextBox> SizeYInput;
	TSharedPtr<SEditableTextBox> SizeZInput;

	FString CurrentGrammarFile;
	bool isPlaying = false;
	FTimerHandle PlayTimerHandle;
	int32 CurrentSeed = 0;
	float CurrentSizeX = 30.0f;
	float CurrentSizeY = 20.0f;
	float CurrentSizeZ = 10.0f;
	int32 CurrentIteration = 0;
	
	// Folder processing variables
	TArray<FString> JsonFilesToProcess;
	int32 CurrentFileIndex = 0;
	bool isProcessingFolder = false;
	FTimerHandle FolderProcessingTimerHandle;
}; 