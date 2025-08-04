// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <opencv2/core/mat.hpp>

#include "Modules/ModuleManager.h"


class FLyraRecordingToolModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void UnregisterMenus();
	void OpenPluginWindow();
	TSharedRef<SDockTab> SpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);
	FReply StartRecording();
	FReply StopRecording();
};