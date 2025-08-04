// Copyright Epic Games, Inc. All Rights Reserved.

// Include your plugin headers first
#include "LyraRecordingTool.h"

#include "LyraRecordingSubsystem.h"
#include "ToolMenus.h"


#define LOCTEXT_NAMESPACE "FLyraRecordingToolModule"

IMPLEMENT_MODULE(FLyraRecordingToolModule, FLyraRecordingTool);

void FLyraRecordingToolModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FLyraRecordingToolModule::RegisterMenus));
}

void FLyraRecordingToolModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
}
//Not needed because tool is application level
void FLyraRecordingToolModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	{
		FToolMenuSection& Section = Menu->AddSection("CustomTools", LOCTEXT("CustomToolsSection", "Custom Tools"));
		Section.AddMenuEntry("OpenPluginWindow", LOCTEXT("LyraRecordingTool", "Lyra Recording Tool"),
			LOCTEXT("OpenPluginWindow_Tooltip", "Open the custom plugin window."),
			FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FLyraRecordingToolModule::OpenPluginWindow)));
	}
}
//Not needed because tool is application level
void FLyraRecordingToolModule::UnregisterMenus()
{
	UToolMenus::UnregisterOwner(this);
}
//Not needed because tool is application level
void FLyraRecordingToolModule::OpenPluginWindow()
{
	TSharedPtr<SDockTab> ExistingTab = FGlobalTabmanager::Get()->FindExistingLiveTab(FName("CustomPluginWindow"));
	if (ExistingTab.IsValid())
	{
		ExistingTab->DrawAttention();
	}
	else
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner("CustomPluginWindow", FOnSpawnTab::CreateRaw(this, &FLyraRecordingToolModule::SpawnPluginTab))
			.SetDisplayName(LOCTEXT("FYourPluginTabTitle", "Custom Plugin Window"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->TryInvokeTab(FName("CustomPluginWindow"));
	}
}
//Not needed because tool is application level
TSharedRef<SDockTab> FLyraRecordingToolModule::SpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text(LOCTEXT("StartRecordingButtonText", "Start Recording"))
					.OnClicked_Raw(this, &FLyraRecordingToolModule::StartRecording) // Bind to member function using OnClicked_Raw
				]
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text(LOCTEXT("StopRecordingButtonText", "Stop Recording"))
					.OnClicked_Raw(this, &FLyraRecordingToolModule::StopRecording) // Bind to member function using OnClicked_Raw
				]
			]
		];
}

//Not needed because tool is application level
FReply FLyraRecordingToolModule::StartRecording()
{
	//Would put start record on subsystem here
	return FReply::Handled();
}

//Not needed because tool is application level
FReply FLyraRecordingToolModule::StopRecording()
{
	//Would put stop record on subsystem here
	return FReply::Handled();
}