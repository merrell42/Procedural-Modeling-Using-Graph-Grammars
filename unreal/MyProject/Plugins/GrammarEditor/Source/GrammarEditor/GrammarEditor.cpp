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
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Templates/Function.h"

#define LOCTEXT_NAMESPACE "FGrammarEditorModule"

static const FName GrammarEditorTabName("GrammarEditor");

static const float AnimationInterval = 0.2f;
static const int32 MaxIterationsPerFile = 50;

void FGrammarEditorModule::StartupModule() {
	if (FGrammarDLL::LoadDLL()) {
		UE_LOG(LogTemp, Log, TEXT("Grammar DLL loaded in StartupModule"));
	} else {
		UE_LOG(LogTemp, Error, TEXT("Failed to load Grammar DLL in StartupModule"));
	}
	
	// Register the tab spawner.
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(GrammarEditorTabName,
		FOnSpawnTab::CreateRaw(this, &FGrammarEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FGrammarEditorTabTitle", "Grammar Editor"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());
		
	// Add menu entry.
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FGrammarEditorModule::RegisterMenus));
}

void FGrammarEditorModule::ShutdownModule() {
	UE_LOG(LogTemp, Log, TEXT("Grammar Editor Plugin Unloaded"));

	FGrammarDLL::UnloadDLL();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GrammarEditorTabName);
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
				SAssignNew(StatusText, STextBlock)
				.Text(LOCTEXT("NoFileLoadedText", "No file loaded"))
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
				SNew(SButton)
				.Text(LOCTEXT("LoadFolderButtonText", "Load Folder"))
				.OnClicked(FOnClicked::CreateRaw(this, &FGrammarEditorModule::OnLoadFolderClicked))
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
				[
					SAssignNew(PlayButtonText, STextBlock)
					.Text(LOCTEXT("PlayButtonText", "Play"))
				]
				.IsEnabled(false)
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
	if (!IsDLLReady()) {
		return FReply::Handled();
	}

	// Open file dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform) {
		TArray<FString> OutFilenames;
		const FString DefaultPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("../../grammar data")));
		
		bool bOpened = DesktopPlatform->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Select Grammar File"),
			DefaultPath,
			TEXT(""),
			TEXT("JSON files (*.json)|*.json"),
			EFileDialogFlags::None,
			OutFilenames
		);
		
		if (bOpened && OutFilenames.Num() > 0) {
			FString SelectedFile = OutFilenames[0];
			UE_LOG(LogTemp, Log, TEXT("Loading grammar file: %s"), *SelectedFile);
			
			CurrentGrammarFile = FPaths::GetBaseFilename(SelectedFile);
			CurrentIteration = 0;
			UpdateStatusText();
			
			FGrammarDLL::LoadGrammarFile(SelectedFile);
			FGrammarDLL::Reset(CurrentSeed);
			SetButtonStates(true);
			
			// Stop folder processing if active
			if (isProcessingFolder) {
				isProcessingFolder = false;
				ClearTimer(FolderProcessingTimerHandle);
			}
		}
	}
	
	return FReply::Handled();
}

void FGrammarEditorModule::UpdateStatusText() {
	if (StatusText.IsValid()) {
		FString StatusString;
		if (isProcessingFolder) {
			StatusString = FString::Printf(TEXT("Processing: %s (%d/%d files, iteration %d/%d)"), 
				*FPaths::GetBaseFilename(JsonFilesToProcess[CurrentFileIndex]),
				CurrentFileIndex + 1, JsonFilesToProcess.Num(),
				CurrentIteration + 1, MaxIterationsPerFile);
		} else {
			if (CurrentIteration == 0) {
				StatusString = FString::Printf(TEXT("%s"), *CurrentGrammarFile);
			} else {
				StatusString = FString::Printf(TEXT("%s (iteration %d)"), *CurrentGrammarFile, CurrentIteration);
			}
		}
		StatusText->SetText(FText::FromString(StatusString));
	}
}

FReply FGrammarEditorModule::OnStepClicked() {
	if (!IsDLLReady()) {
		return FReply::Handled();
	}
	FGrammarDLL::Step();
	if (!isProcessingFolder && !CurrentGrammarFile.IsEmpty()) {
		CurrentIteration++;
	}
	UpdateStatusText();

	return FReply::Handled();
}

FReply FGrammarEditorModule::OnResetClicked() {
	if (!IsDLLReady()) {
		return FReply::Handled();
	}
	FGrammarDLL::Reset(CurrentSeed);
	CurrentIteration = 0;
	UpdateStatusText();
	
	return FReply::Handled();
}

FReply FGrammarEditorModule::OnPlayClicked() {
	if (!IsDLLReady()) {
		return FReply::Handled();
	}

	if (isPlaying || isProcessingFolder) {
		// Stop playing or folder processing
		isPlaying = false;
		if (PlayButtonText.IsValid()) {
			PlayButtonText->SetText(LOCTEXT("PlayButtonText", "Play"));
		}
		ClearTimer(PlayTimerHandle);
		
		// Stop folder processing if active
		if (isProcessingFolder) {
			isProcessingFolder = false;
			ClearTimer(FolderProcessingTimerHandle);
			UpdateStatusText();
		}
	} else {
		// Start playing
		isPlaying = true;
		if (PlayButtonText.IsValid()) {
			PlayButtonText->SetText(LOCTEXT("StopButtonText", "Stop"));
		}
		
		// Start timer
		StartTimer(PlayTimerHandle, AnimationInterval, [this]() { 
			if (isPlaying) {
				FGrammarDLL::Step();
				// Update iteration count for single file processing
				if (!isProcessingFolder && !CurrentGrammarFile.IsEmpty()) {
					CurrentIteration++;
					UpdateStatusText();
				}
			}
		}, true);
	}
	
	return FReply::Handled();
}

void FGrammarEditorModule::OnSeedChanged(const FText& NewText) {
	FString TextString = NewText.ToString();
	if (TextString.IsNumeric()) {
		CurrentSeed = FCString::Atoi(*TextString);
	}
}

void FGrammarEditorModule::OnSizeXChanged(const FText& NewText) {
	FString TextString = NewText.ToString();
	if (TextString.IsNumeric()) {
		CurrentSizeX = FCString::Atof(*TextString);
		FGrammarDLL::SetSize(CurrentSizeX, CurrentSizeY, CurrentSizeZ);
	}
}

void FGrammarEditorModule::OnSizeYChanged(const FText& NewText) {
	FString TextString = NewText.ToString();
	if (TextString.IsNumeric()) {
		CurrentSizeY = FCString::Atof(*TextString);
		FGrammarDLL::SetSize(CurrentSizeX, CurrentSizeY, CurrentSizeZ);
	}
}

void FGrammarEditorModule::OnSizeZChanged(const FText& NewText) {
	FString TextString = NewText.ToString();
	if (TextString.IsNumeric()) {
		CurrentSizeZ = FCString::Atof(*TextString);
		FGrammarDLL::SetSize(CurrentSizeX, CurrentSizeY, CurrentSizeZ);
	}
}

FReply FGrammarEditorModule::OnLoadFolderClicked() {
	if (!IsDLLReady()) {
		return FReply::Handled();
	}

	// Open directory dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform) {
		FString SelectedDirectory;
		const FString DefaultPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("../../grammar data")));
		
		bool bOpened = DesktopPlatform->OpenDirectoryDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Select Folder with Grammar Files (*.json)"),
			DefaultPath,
			SelectedDirectory
		);
		
		if (bOpened && !SelectedDirectory.IsEmpty()) {
			UE_LOG(LogTemp, Log, TEXT("Loading folder: %s"), *SelectedDirectory);
			
			FindJsonFiles(SelectedDirectory);
			if (JsonFilesToProcess.Num() > 0) {
				CurrentFileIndex = 0;
				CurrentIteration = 0;
				isProcessingFolder = true;
				
				// Change Play button to Stop.
				if (PlayButtonText.IsValid()) {
					PlayButtonText->SetText(LOCTEXT("StopButtonText", "Stop"));
				}
				
				// Load the first file
				FString FirstFile = JsonFilesToProcess[CurrentFileIndex];
				CurrentGrammarFile = FPaths::GetBaseFilename(FirstFile);
				CurrentIteration = 0;
				UpdateStatusText();
				
				FGrammarDLL::LoadGrammarFile(FirstFile);
				FGrammarDLL::Reset(CurrentSeed);
				SetButtonStates(true);
				
				// Start timer for processing
				StartTimer(FolderProcessingTimerHandle, AnimationInterval, [this]() { 
					ProcessNextFileInFolder(); 
				}, false);
				UE_LOG(LogTemp, Log, TEXT("Started processing %d JSON files"), JsonFilesToProcess.Num());
			} else {
				UE_LOG(LogTemp, Warning, TEXT("No JSON files found in directory: %s"), *SelectedDirectory);
				if (StatusText.IsValid()) {
					StatusText->SetText(LOCTEXT("NoJsonFilesFoundText", "No JSON files found in selected directory"));
				}
			}
		}
	}
	return FReply::Handled();
}

void FGrammarEditorModule::FindJsonFiles(const FString& DirectoryPath) {
	JsonFilesToProcess.Empty();
	
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*DirectoryPath)) {
		UE_LOG(LogTemp, Error, TEXT("Directory does not exist: %s"), *DirectoryPath);
		return;
	}
	
	// Function to recursively find JSON files.
	TFunction<void(const FString&)> FindJsonFilesRecursive = [&](const FString& CurrentPath) {
		UE_LOG(LogTemp, Log, TEXT("Searching in directory: %s"), *CurrentPath);
		
		// Iterate through all items in the directory to find files and subdirectories
		PlatformFile.IterateDirectory(*CurrentPath, [&](const TCHAR* FilenameOrDirectory, bool isDirectory) -> bool {
			if (isDirectory) {
				// Skip . and .. directories
				FString DirName = FPaths::GetBaseFilename(FilenameOrDirectory);
				if (DirName != TEXT(".") && DirName != TEXT("..")) {
					UE_LOG(LogTemp, Log, TEXT("Recursing into subdirectory: %s"), FilenameOrDirectory);
					FindJsonFilesRecursive(FilenameOrDirectory);
				}
			} else {
				// Check if it's a JSON file
				FString FilePath = FilenameOrDirectory;
				if (FPaths::GetExtension(FilePath).ToLower() == TEXT("json")) {
					UE_LOG(LogTemp, Log, TEXT("Adding JSON file: %s"), *FilePath);
					JsonFilesToProcess.Add(FilePath);
				}
			}
			return true;
		});
	};

	FindJsonFilesRecursive(DirectoryPath);
	UE_LOG(LogTemp, Log, TEXT("Total JSON files found: %d"), JsonFilesToProcess.Num());
}

void FGrammarEditorModule::ProcessNextFileInFolder() {
	if (!isProcessingFolder || CurrentFileIndex >= JsonFilesToProcess.Num()) {
		// Finished processing all files
		isProcessingFolder = false;
		if (StatusText.IsValid()) {
			StatusText->SetText(LOCTEXT("FolderProcessingCompleteText", "Folder processing complete!"));
		}
		// Change Stop button back to Play
		if (PlayButtonText.IsValid()) {
			PlayButtonText->SetText(LOCTEXT("PlayButtonText", "Play"));
		}
		UE_LOG(LogTemp, Log, TEXT("Finished processing all files in folder"));
		return;
	}
	
	// Perform one iteration
	FGrammarDLL::Step();
	CurrentIteration++;
	
	
	// Check if we've reached max iterations for current file
	if (CurrentIteration >= MaxIterationsPerFile) {
		// Move to next file
		CurrentFileIndex++;
		CurrentIteration = 0;		
		if (CurrentFileIndex < JsonFilesToProcess.Num()) {
			FString NextFile = JsonFilesToProcess[CurrentFileIndex];
			CurrentGrammarFile = FPaths::GetBaseFilename(NextFile);
			
			FGrammarDLL::LoadGrammarFile(NextFile);
			FGrammarDLL::Reset(CurrentSeed);
			
			UE_LOG(LogTemp, Log, TEXT("Moving to next file: %s (%d/%d)"), *CurrentGrammarFile, CurrentFileIndex + 1, JsonFilesToProcess.Num());
		}
	}
	UpdateStatusText();
	
	// Schedule next iteration
	if (isProcessingFolder) {
		StartTimer(FolderProcessingTimerHandle, AnimationInterval, [this]() { 
			ProcessNextFileInFolder(); 
		}, false);
	}
}

bool FGrammarEditorModule::IsDLLReady() {
	if (!FGrammarDLL::IsDLLLoaded()) {
		UE_LOG(LogTemp, Error, TEXT("DLL is not loaded!"));
		return false;
	}
	return true;
}

void FGrammarEditorModule::SetButtonStates(bool bEnabled) {
	if (StepButton.IsValid()) {
		StepButton->SetEnabled(bEnabled);
	}
	if (ResetButton.IsValid()) {
		ResetButton->SetEnabled(bEnabled);
	}
	if (PlayButton.IsValid()) {
		PlayButton->SetEnabled(bEnabled);
	}
}

void FGrammarEditorModule::StartTimer(FTimerHandle& TimerHandle, float Interval, TFunction<void()> Callback, bool bLooping) {
	if (GEngine && GEngine->GetWorldContexts().Num() > 0) {
		UWorld* World = GEngine->GetWorldContexts()[0].World();
		if (World) {
			FTimerManagerTimerParameters TimerParams;
			TimerParams.bLoop = bLooping;
			World->GetTimerManager().SetTimer(TimerHandle, MoveTemp(Callback), Interval, TimerParams);
		}
	}
}

void FGrammarEditorModule::ClearTimer(FTimerHandle& TimerHandle) {
	if (GEngine && GEngine->GetWorldContexts().Num() > 0) {
		UWorld* World = GEngine->GetWorldContexts()[0].World();
		if (World) {
			World->GetTimerManager().ClearTimer(TimerHandle);
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGrammarEditorModule, GrammarEditor) 