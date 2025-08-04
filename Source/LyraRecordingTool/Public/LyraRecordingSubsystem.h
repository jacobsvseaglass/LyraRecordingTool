#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#undef check

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

#include "LyraRecordingSubsystem.generated.h"

#define check(condition)

UCLASS()
class LYRARECORDINGTOOL_API ULyraRecordingSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	ULyraRecordingSubsystem();
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void StartRecording();
	void StopRecording();

private:
	FTimerHandle CaptureTimerHandle;
	bool bIsRecording = false;
	float frameRate = 1.0f / 30.0f; // 30 FPS
	float elapsedTime = 0.0f;
	std::vector<cv::Mat> CapturedFrames;
	FString VideosDir = "Saved/LyraRecordings";
	
	APlayerController* PlayerController;


	bool IncludeDepth = true;
	bool IncludeSurfaceNormal = true;
	bool IncludeHistogram = true;
	FString StartCaptureKeyStr;
	FString StopCaptureKeyStr;

	FKey StartCaptureKey;
	FKey StopCaptureKey;

	UFUNCTION()
	void CaptureFrame();
	void CombineModalities(const cv::Mat& rgb, const cv::Mat& depth, const cv::Mat& normal, const cv::Mat& histogram,
	                       cv::Mat& combinedFrame);
	cv::Mat CaptureDepth();
	cv::Mat CaptureNormals();

	void SaveFramesToDisk(const std::vector<cv::Mat>& frames);
	void ExportVideo();
    
	cv::Mat CaptureScreen();

protected:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	void LoadConfig();
};
