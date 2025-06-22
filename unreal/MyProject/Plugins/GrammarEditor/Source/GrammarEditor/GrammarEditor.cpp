#include "GrammarEditor.h"
#include "GrammarDLL.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "FGrammarEditorModule"

static const FName GrammarEditorTabName("GrammarEditor");

void FGrammarEditorModule::StartupModule() {
	UE_LOG(LogTemp, Log, TEXT("Hello World! Grammar Editor Plugin Loaded Successfully!"));
	
	// Load the DLL
	if (FGrammarDLL::LoadDLL()) {
		UE_LOG(LogTemp, Log, TEXT("Grammar DLL loaded in StartupModule"));
	} else {
		UE_LOG(LogTemp, Error, TEXT("Failed to load Grammar DLL in StartupModule"));
	}
	
	// Register the tab spawner - UI Window
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(GrammarEditorTabName,
		FOnSpawnTab::CreateRaw(this, &FGrammarEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FGrammarEditorTabTitle", "Grammar Editor"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());
		
	// Add menu entry
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FGrammarEditorModule::RegisterMenus));
}

void FGrammarEditorModule::ShutdownModule() {
	UE_LOG(LogTemp, Log, TEXT("Grammar Editor Plugin Unloaded"));
	
	// Unload the DLL
	FGrammarDLL::UnloadDLL();
	
	// Unregister the tab spawner
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GrammarEditorTabName);
	
	// Unregister menus
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FGrammarEditorModule::RegisterMenus() {
	FToolMenuOwnerScoped OwnerScoped(this);
	
	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = ToolsMenu->FindOrAddSection("Programming");
	Section.AddMenuEntry(
		"OpenGrammarEditor",
		LOCTEXT("OpenGrammarEditor", "Grammar Editor"),
		LOCTEXT("OpenGrammarEditorTooltip", "Open the Grammar Editor window"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FGrammarEditorModule::OpenGrammarEditor))
	);
}

void FGrammarEditorModule::OpenGrammarEditor() {
	FGlobalTabmanager::Get()->TryInvokeTab(GrammarEditorTabName);
}

TSharedRef<SDockTab> FGrammarEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs) {
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(3)
			[
				SNew(SButton)
				.Text(LOCTEXT("LoadGrammarButtonText", "Load Grammar"))
				.OnClicked(FOnClicked::CreateRaw(this, &FGrammarEditorModule::OnLoadGrammarClicked))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(3)
			[
				SAssignNew(StepButton, SButton)
				.Text(LOCTEXT("StepButtonText", "Step"))
				.OnClicked(FOnClicked::CreateRaw(this, &FGrammarEditorModule::OnStepClicked))
				.IsEnabled(false)
			]
		];
}



FReply FGrammarEditorModule::OnLoadGrammarClicked() {
	if (!FGrammarDLL::IsDLLLoaded()) {
		UE_LOG(LogTemp, Error, TEXT("DLL is not loaded!"));
		return FReply::Handled();
	}

	// Open file dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform) {
		TArray<FString> OutFileNames;
		const FString DefaultPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("../../grammar data")));
		
		bool bOpened = DesktopPlatform->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Select Grammar File"),
			DefaultPath,
			TEXT(""),
			TEXT("JSON Files (*.json)|*.json"),
			EFileDialogFlags::None,
			OutFileNames
		);
		
		if (bOpened && OutFileNames.Num() > 0) {
			FString SelectedFile = OutFileNames[0];
			UE_LOG(LogTemp, Log, TEXT("Loading grammar file: %s"), *SelectedFile);
			
			// Load the grammar file using the DLL
			FGrammarDLL::LoadGrammarFile(SelectedFile);
			
			// Enable the Step button now that a grammar is loaded
			if (StepButton.IsValid()) {
				StepButton->SetEnabled(true);
			}
		}
	}
	
	return FReply::Handled();
}

FReply FGrammarEditorModule::OnStepClicked() {
	if (!FGrammarDLL::IsDLLLoaded()) {
		UE_LOG(LogTemp, Error, TEXT("DLL is not loaded!"));
		return FReply::Handled();
	}
	FGrammarDLL::Step();	
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGrammarEditorModule, GrammarEditor) 