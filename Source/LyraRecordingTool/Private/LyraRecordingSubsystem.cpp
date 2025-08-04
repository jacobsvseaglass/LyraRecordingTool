#include "LyraRecordingSubsystem.h"
#include <filesystem>
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "RendererInterface.h"

ULyraRecordingSubsystem::ULyraRecordingSubsystem()
{
    LoadConfig();
}

void ULyraRecordingSubsystem::LoadConfig()
{
    //Path to custom ini file
    FString ConfigFilePath = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("LyraRecordingTool/Config/DefaultConfig.ini"));

    if (FPaths::FileExists(ConfigFilePath))
    {
        FString ConfigSection = TEXT("RecordingSettings");

        GConfig->GetBool(*ConfigSection, TEXT("Include_Depth"), IncludeDepth, ConfigFilePath);
        GConfig->GetBool(*ConfigSection, TEXT("Include_SurfaceNormal"), IncludeSurfaceNormal, ConfigFilePath);
        GConfig->GetBool(*ConfigSection, TEXT("Include_Histogram"), IncludeHistogram, ConfigFilePath);
        GConfig->GetString(*ConfigSection, TEXT("Start_Capture_Key"), StartCaptureKeyStr, ConfigFilePath);
        GConfig->GetString(*ConfigSection, TEXT("Stop_Capture_Key"), StopCaptureKeyStr, ConfigFilePath);
        GConfig->GetString(*ConfigSection, TEXT("Videos_dir"), VideosDir, ConfigFilePath);

        UE_LOG(LogTemp, Log, TEXT("Start Capture Key: %s"), *StartCaptureKeyStr);
        UE_LOG(LogTemp, Log, TEXT("Stop Capture Key: %s"), *StopCaptureKeyStr);

        // Convert string keys to FKey
        StartCaptureKey = FKey(*StartCaptureKeyStr);
        StopCaptureKey = FKey(*StopCaptureKeyStr);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Config file %s not found"), *ConfigFilePath);
    }
}

void ULyraRecordingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("LyraRecordingSubsystem Initialized, Instance: %p"), this);
}

void ULyraRecordingSubsystem::Deinitialize()
{
    Super::Deinitialize();

    //If recording, stop and save to disk
    if (bIsRecording)
        StopRecording();
}

void ULyraRecordingSubsystem::StartRecording()
{
    if (bIsRecording) return;
    UE_LOG(LogTemp, Log, TEXT("Recording START, Instance: %p"), this);
    bIsRecording = true;
}

void ULyraRecordingSubsystem::StopRecording()
{
    UE_LOG(LogTemp, Log, TEXT("Recording STOP, Instance: %p"), this);
    if (!bIsRecording) return;
    bIsRecording = false;
    SaveFramesToDisk(CapturedFrames);
}

cv::Mat ULyraRecordingSubsystem::CaptureScreen()
{
    if (GEngine && GEngine->GameViewport)
    {
        FViewport* Viewport = GEngine->GameViewport->Viewport;
        if (Viewport)
        {
            TArray<FColor> Bitmap;
            Viewport->ReadPixels(Bitmap);

            int32 Width = Viewport->GetSizeXY().X;
            int32 Height = Viewport->GetSizeXY().Y;
            cv::Mat CapturedFrame(Height, Width, CV_8UC4, Bitmap.GetData());
            cv::cvtColor(CapturedFrame, CapturedFrame, cv::COLOR_BGRA2BGR);
            UE_LOG(LogTemp, Log, TEXT("Recorded Frame"));
            UE_LOG(LogTemp, Log, TEXT("Frame Width: %d"), Width);
            UE_LOG(LogTemp, Log, TEXT("Frame Height: %d"), Height);
            UE_LOG(LogTemp, Log, TEXT("Number of Pixels: %d"), Bitmap.Num());
            
            return CapturedFrame;
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("CaptureScreen fail, no game viewport viewport"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("CaptureScreen fail, no game viewport"));
    }
    
    return cv::Mat();
}

void ULyraRecordingSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UWorld* World = GetWorld();

    if (World)
    {
        if (!PlayerController)
            PlayerController = GetWorld()->GetFirstPlayerController();
        
        if (PlayerController && PlayerController->WasInputKeyJustPressed(StartCaptureKey))
            StartRecording();

        if (PlayerController && PlayerController->WasInputKeyJustPressed(StopCaptureKey))
            StopRecording();
    }
    
    if (bIsRecording)
    {
        elapsedTime += DeltaTime;

        if (elapsedTime >= frameRate)
        {
            elapsedTime -= frameRate;
            UE_LOG(LogTemp, Log, TEXT("Framerate Tick"));
            CaptureFrame();
        }
    }
}

TStatId ULyraRecordingSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(ULyraRecordingSubsystem, STATGROUP_Tickables);
}

void ULyraRecordingSubsystem::CaptureFrame()
{
    UE_LOG(LogTemp, Log, TEXT("Capturing Frame"));

    if (bIsRecording)
    {
        cv::Mat Frame = CaptureScreen();
        CapturedFrames.push_back(Frame);
    }
}

void ULyraRecordingSubsystem::CombineModalities(const cv::Mat& rgb, const cv::Mat& depth, const cv::Mat& normal, const cv::Mat& histogram, cv::Mat& combinedFrame)
{
    // Ensure all input images are of the same size
    int width = rgb.cols;
    int height = rgb.rows;
    
    // Create the combined frame
    combinedFrame = cv::Mat(height * 2, width * 2, rgb.type());

    // Black image with text
    cv::Mat notConfigured = cv::Mat::zeros(height, width, rgb.type());
    cv::putText(notConfigured, "Not Configured", cv::Point(width / 4, height / 2), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);

    // Place the images in a 2x2 grid
    if (IncludeDepth)
    {
        depth.copyTo(combinedFrame(cv::Rect(width, 0, width, height)));
    }
    else
    {
        notConfigured.copyTo(combinedFrame(cv::Rect(width, 0, width, height)));
    }

    if (IncludeSurfaceNormal)
    {
        normal.copyTo(combinedFrame(cv::Rect(0, height, width, height)));
    }
    else
    {
        notConfigured.copyTo(combinedFrame(cv::Rect(0, height, width, height)));
    }

    if (IncludeHistogram)
    {
        histogram.copyTo(combinedFrame(cv::Rect(width, height, width, height)));
    }
    else
    {
        notConfigured.copyTo(combinedFrame(cv::Rect(width, height, width, height)));
    }
    
    // Always include the rgb frame
    rgb.copyTo(combinedFrame(cv::Rect(0, 0, width, height)));
}

cv::Mat ApplyHistogramEffect(const cv::Mat& rgbFrame)
{
    std::vector<cv::Mat> bgr_planes;
    split(rgbFrame, bgr_planes);

    int histSize = 256;
    float range[] = {0, 256};
    const float* histRange[] = {range};
    bool uniform = true, accumulate = false;

    cv::Mat b_hist, g_hist, r_hist;
    calcHist(&bgr_planes[0], 1, 0, cv::Mat(), b_hist, 1, &histSize, histRange, uniform, accumulate);
    calcHist(&bgr_planes[1], 1, 0, cv::Mat(), g_hist, 1, &histSize, histRange, uniform, accumulate);
    calcHist(&bgr_planes[2], 1, 0, cv::Mat(), r_hist, 1, &histSize, histRange, uniform, accumulate);

    int hist_w = rgbFrame.cols;
    int hist_h = rgbFrame.rows;
    int bin_w = cvRound((double)hist_w / histSize);

    cv::Mat histImage(hist_h, hist_w, CV_8UC3, cv::Scalar(0, 0, 0));

    normalize(b_hist, b_hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat());
    normalize(g_hist, g_hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat());
    normalize(r_hist, r_hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat());

    for (int i = 1; i < histSize; i++)
    {
        line(histImage, cv::Point(bin_w * (i - 1), hist_h - cvRound(b_hist.at<float>(i - 1))),
            cv::Point(bin_w * i, hist_h - cvRound(b_hist.at<float>(i))),
            cv::Scalar(255, 0, 0), 2, 8, 0);
        line(histImage, cv::Point(bin_w * (i - 1), hist_h - cvRound(g_hist.at<float>(i - 1))),
            cv::Point(bin_w * i, hist_h - cvRound(g_hist.at<float>(i))),
            cv::Scalar(0, 255, 0), 2, 8, 0);
        line(histImage, cv::Point(bin_w * (i - 1), hist_h - cvRound(r_hist.at<float>(i - 1))),
            cv::Point(bin_w * i, hist_h - cvRound(r_hist.at<float>(i))),
            cv::Scalar(0, 0, 255), 2, 8, 0);
    }

    return histImage;
}

// Function to apply a depth effect to the given frame
cv::Mat ApplyDepthEffect(const cv::Mat& rgbFrame)
{
    cv::Mat depthFrame;
    cv::cvtColor(rgbFrame, depthFrame, cv::COLOR_BGR2GRAY);
    cv::applyColorMap(depthFrame, depthFrame, cv::COLORMAP_JET); // Use a colormap to simulate depth effect
    return depthFrame;
}

// Function to apply a surface normal effect to the given frame
cv::Mat ApplySurfaceNormalEffect(const cv::Mat& rgbFrame)
{
    cv::Mat gray, gradientX, gradientY, normalFrame;
    cv::cvtColor(rgbFrame, gray, cv::COLOR_BGR2GRAY);

    cv::Sobel(gray, gradientX, CV_32F, 1, 0, 3);
    cv::Sobel(gray, gradientY, CV_32F, 0, 1, 3);
    
    cv::normalize(gradientX, gradientX, 0, 1, cv::NORM_MINMAX);
    cv::normalize(gradientY, gradientY, 0, 1, cv::NORM_MINMAX);
    
    std::vector<cv::Mat> channels = {gradientX, gradientY, cv::Mat::ones(gray.size(), CV_32F)};
    cv::merge(channels, normalFrame);
    
    normalFrame.convertTo(normalFrame, CV_8UC3, 255);
    applyColorMap(normalFrame, normalFrame, cv::COLORMAP_PARULA); // Use a colormap to highlight normals

    return normalFrame;
}

std::string GetCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm timeInfo;
    localtime_s(&timeInfo, &in_time_t); // Use localtime_s for thread safety

    std::stringstream ss;
    ss << std::put_time(&timeInfo, "%Y%m%d_%H%M%S");
    return ss.str();
}

void ULyraRecordingSubsystem::SaveFramesToDisk(const std::vector<cv::Mat>& frames)
{
    if (frames.empty())
    {
        return;
    }
    
    FString CurrentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    UE_LOG(LogTemp, Warning, TEXT("Current working directory: %s"), *CurrentDir);
    
    std::string fullOutputDirectory = TCHAR_TO_UTF8(*(CurrentDir + VideosDir));
    
    if (!std::filesystem::exists(fullOutputDirectory))
    {
        std::filesystem::create_directories(fullOutputDirectory);
    }

    std::string timestamp = GetCurrentTimestamp();
    std::string outputVideo = fullOutputDirectory + "/Lyra_vid_" + timestamp + ".mp4";
    cv::Size frameSize = cv::Size(frames[0].cols * 2, frames[0].rows * 2);
    cv::VideoWriter writer(outputVideo, cv::VideoWriter::fourcc('X', '2', '6', '4'), 30, frameSize, true);
    
    for (const auto& frame : frames)
    {
        cv::Mat rgbFrame = frame;
        cv::Mat depthFrame = ApplyDepthEffect(rgbFrame);
        cv::Mat normalFrame = ApplySurfaceNormalEffect(rgbFrame);
        cv::Mat histogramFrame = ApplyHistogramEffect(rgbFrame);

        cv::Mat combinedFrame;
        CombineModalities(rgbFrame, depthFrame, normalFrame, histogramFrame, combinedFrame);
        writer.write(combinedFrame);
    }

    writer.release();
}