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

private:
	TSharedPtr<SButton> StepButton;
}; 