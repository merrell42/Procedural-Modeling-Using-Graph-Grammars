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
	FReply OnStepClicked();
	FReply OnResetClicked();
	FReply OnPlayClicked();
	void OnSeedChanged(const FText& NewText);
	void OnSizeXChanged(const FText& NewText);
	void OnSizeYChanged(const FText& NewText);
	void OnSizeZChanged(const FText& NewText);

private:
	TSharedPtr<SButton> StepButton;
	TSharedPtr<SButton> ResetButton;
	TSharedPtr<SButton> PlayButton;
	TSharedPtr<STextBlock> PlayButtonText;
	TSharedPtr<STextBlock> FileNameText;
	TSharedPtr<SEditableTextBox> SeedInput;
	TSharedPtr<SEditableTextBox> SizeXInput;
	TSharedPtr<SEditableTextBox> SizeYInput;
	TSharedPtr<SEditableTextBox> SizeZInput;
	FString CurrentGrammarFile;
	bool bIsPlaying = false;
	FTimerHandle PlayTimerHandle;
	int32 CurrentSeed = 0;
	float CurrentSizeX = 30.0f;
	float CurrentSizeY = 20.0f;
	float CurrentSizeZ = 10.0f;
}; 