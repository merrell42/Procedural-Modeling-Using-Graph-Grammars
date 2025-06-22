#include "GrammarEditor.h"
#include "GrammarDLL.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SBoxPanel.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "FGrammarEditorModule"

static const FName GrammarEditorTabName("GrammarEditor");

static const float AnimationInterval = 0.2f;

void FGrammarEditorModule::StartupModule() {
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
				SAssignNew(FileNameText, STextBlock)
				.Text(LOCTEXT("NoFileLoadedText", "No file selected."))
			]
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
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(3)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SeedLabel", "Seed:"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(3)
				[
					SAssignNew(SeedInput, SEditableTextBox)
					.Text(FText::FromString(FString::FromInt(CurrentSeed)))
					.OnTextChanged(FOnTextChanged::CreateRaw(this, &FGrammarEditorModule::OnSeedChanged))
					.MinDesiredWidth(60.0f)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(3)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SizeLabel", "Size:"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(3)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SizeXLabel", "X:"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(3)
				[
					SAssignNew(SizeXInput, SEditableTextBox)
					.Text(FText::FromString(FString::SanitizeFloat(CurrentSizeX)))
					.OnTextChanged(FOnTextChanged::CreateRaw(this, &FGrammarEditorModule::OnSizeXChanged))
					.MinDesiredWidth(50.0f)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(3)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SizeYLabel", "Y:"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(3)
				[
					SAssignNew(SizeYInput, SEditableTextBox)
					.Text(FText::FromString(FString::SanitizeFloat(CurrentSizeY)))
					.OnTextChanged(FOnTextChanged::CreateRaw(this, &FGrammarEditorModule::OnSizeYChanged))
					.MinDesiredWidth(50.0f)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(3)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SizeZLabel", "Z:"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(3)
				[
					SAssignNew(SizeZInput, SEditableTextBox)
					.Text(FText::FromString(FString::SanitizeFloat(CurrentSizeZ)))
					.OnTextChanged(FOnTextChanged::CreateRaw(this, &FGrammarEditorModule::OnSizeZChanged))
					.MinDesiredWidth(50.0f)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(3)
			[
				SAssignNew(PlayButton, SButton)
				.OnClicked(FOnClicked::CreateRaw(this, &FGrammarEditorModule::OnPlayClicked))
				.IsEnabled(false)
				[
					SAssignNew(PlayButtonText, STextBlock)
					.Text(LOCTEXT("PlayButtonText", "Play"))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(3)
			[
				SAssignNew(ResetButton, SButton)
				.Text(LOCTEXT("ResetButtonText", "Reset"))
				.OnClicked(FOnClicked::CreateRaw(this, &FGrammarEditorModule::OnResetClicked))
				.IsEnabled(false)
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
			
			// Store the file name and update the display
			CurrentGrammarFile = FPaths::GetBaseFilename(SelectedFile);
			if (FileNameText.IsValid()) {
				FileNameText->SetText(FText::FromString(FString::Printf(TEXT("%s"), *CurrentGrammarFile)));
			}
			
			// Load the grammar file using the DLL
			FGrammarDLL::LoadGrammarFile(SelectedFile);
			
			// Enable the Step button now that a grammar is loaded
			if (StepButton.IsValid()) {
				StepButton->SetEnabled(true);
			}
			if (ResetButton.IsValid()) {
				ResetButton->SetEnabled(true);
			}
			
			// Enable the Play button now that a grammar is loaded
			if (PlayButton.IsValid()) {
				PlayButton->SetEnabled(true);
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

FReply FGrammarEditorModule::OnResetClicked() {
	if (!FGrammarDLL::IsDLLLoaded()) {
		UE_LOG(LogTemp, Error, TEXT("DLL is not loaded!"));
		return FReply::Handled();
	}
	FGrammarDLL::Reset(CurrentSeed);
	return FReply::Handled();
}

FReply FGrammarEditorModule::OnPlayClicked() {
	if (!FGrammarDLL::IsDLLLoaded()) {
		UE_LOG(LogTemp, Error, TEXT("DLL is not loaded!"));
		return FReply::Handled();
	}
	
	if (!bIsPlaying) {
		// Start playing
		bIsPlaying = true;
		if (PlayButtonText.IsValid()) {
			PlayButtonText->SetText(LOCTEXT("StopButtonText", "Stop"));
		}
		
		// Start timer to call step every AnimationInterval seconds
		if (GEngine && GEngine->GetWorldContexts().Num() > 0) {
			UWorld* World = GEngine->GetWorldContexts()[0].World();
			if (World) {
				World->GetTimerManager().SetTimer(PlayTimerHandle, [this]() {
					if (bIsPlaying && FGrammarDLL::IsDLLLoaded()) {
						FGrammarDLL::Step();
					}
				}, AnimationInterval, true);
			}
		}
		
		UE_LOG(LogTemp, Log, TEXT("Started playing grammar"));
	} else {
		// Stop playing
		bIsPlaying = false;
		if (PlayButtonText.IsValid()) {
			PlayButtonText->SetText(LOCTEXT("PlayButtonText", "Play"));
		}
		
		// Clear timer
		if (GEngine && GEngine->GetWorldContexts().Num() > 0) {
			UWorld* World = GEngine->GetWorldContexts()[0].World();
			if (World) {
				World->GetTimerManager().ClearTimer(PlayTimerHandle);
			}
		}
		
		UE_LOG(LogTemp, Log, TEXT("Stopped playing grammar"));
	}
	
	return FReply::Handled();
}

void FGrammarEditorModule::OnSeedChanged(const FText& NewText) {
	FString TextString = NewText.ToString();
	if (TextString.IsNumeric()) {
		CurrentSeed = FCString::Atoi(*TextString);
		UE_LOG(LogTemp, Log, TEXT("Seed changed to: %d"), CurrentSeed);
	}
}

void FGrammarEditorModule::OnSizeXChanged(const FText& NewText) {
	FString TextString = NewText.ToString();
	if (TextString.IsNumeric()) {
		CurrentSizeX = FCString::Atof(*TextString);
		UE_LOG(LogTemp, Log, TEXT("Size X changed to: %f"), CurrentSizeX);
		FGrammarDLL::SetSize(CurrentSizeX, CurrentSizeY, CurrentSizeZ);
	}
}

void FGrammarEditorModule::OnSizeYChanged(const FText& NewText) {
	FString TextString = NewText.ToString();
	if (TextString.IsNumeric()) {
		CurrentSizeY = FCString::Atof(*TextString);
		UE_LOG(LogTemp, Log, TEXT("Size Y changed to: %f"), CurrentSizeY);
		FGrammarDLL::SetSize(CurrentSizeX, CurrentSizeY, CurrentSizeZ);
	}
}

void FGrammarEditorModule::OnSizeZChanged(const FText& NewText) {
	FString TextString = NewText.ToString();
	if (TextString.IsNumeric()) {
		CurrentSizeZ = FCString::Atof(*TextString);
		UE_LOG(LogTemp, Log, TEXT("Size Z changed to: %f"), CurrentSizeZ);
		FGrammarDLL::SetSize(CurrentSizeX, CurrentSizeY, CurrentSizeZ);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGrammarEditorModule, GrammarEditor) 