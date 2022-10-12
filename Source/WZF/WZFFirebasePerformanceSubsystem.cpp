#include "WZFFirebasePerformanceSubsystem.h"

#include "WZFSettings.h"

#include "HAL/PlatformMisc.h"
#include "Performance/FirebasePerformanceLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogWZFFirebaseTrace, Log, All);

static int TotalTraceCount = 0;

int64 UWZFFPSFirebaseMetric::GetValue() const
{
	extern ENGINE_API float GAverageFPS;
	return GAverageFPS;
}

int64 UWZFDeviceTemperatureFirebaseMetric::GetValue() const
{
	return 0;
}

int64 UWZFDeviceBatteryLevelFirebaseMetric::GetValue() const
{
#if PLATFORM_ANDROID
	return FAndroidMisc::GetBatteryState().Level;
#elif PLATFORM_IOS
	return FIOSPlatformMisc::GetBatteryLevel();
#else
	return 0;
#endif
}

int64 UWZFMemoryFirebaseMetric::GetValue() const
{
	return 5;
}

int64 UWZFDeviceVolumeStateFirebaseMetric::GetValue() const
{
#if PLATFORM_ANDROID
	//FAndroidMisc::GetVolumeState returns 0-15, scale to 0-100
	int baseVolume = FAndroidMisc::GetVolumeState();
	int scaledVolume = (baseVolume * 100) / 15;
	return scaledVolume;
#elif PLATFORM_IOS
	return FPlatformMisc::GetDeviceVolume();
#else
	return 0;
#endif
}

void UWZFFirebaseTrace::InitializeTrace(UGameInstance* InGameInstance)
{
	GameInstance = InGameInstance;

	FTimerHandle timer;
	GameInstance->GetTimerManager().SetTimer(timer, this, &UWZFFirebaseTrace::OnTraceTimer, TraceData.TraceDuration, !TraceData.bOnceTrace, 0.f);
}

void UWZFFirebaseTrace::OnTraceTimer()
{
	FirebaseTrace.Stop();

	FString traceName = TraceData.TraceName;
	if (TraceData.bIncrementTraceName)
	{
		traceName.Append(FString::FromInt(TotalTraceCount));
		TotalTraceCount++;
	}

	FirebaseTrace = UFirebasePerformanceLibrary::CreateTrace(traceName);
	FirebaseTrace.Start();

	Metrics.Empty();
	for (const auto& metricData : TraceData.Metrics)
	{
		if (metricData.MetricClass != nullptr)
		{
			Metrics.Emplace(metricData.MetricName, NewObject<UWZFFirebaseMetric>(this, metricData.MetricClass));
		}
	}

	FTimerHandle timer;
	GameInstance->GetTimerManager().SetTimer(timer, this, &UWZFFirebaseTrace::OnMetricTimer, TraceData.MetricRate, true, 0.f);
}

void UWZFFirebaseTrace::OnMetricTimer()
{
	for (const auto& metric : Metrics)
	{
		if (metric.Value.IsValid())
		{
			FirebaseTrace.SetMetricValue(metric.Key, metric.Value->GetValue());
			UE_LOG(LogWZFFirebaseTrace, Log, TEXT("Firebase metric [%s:%d]"), *metric.Key, metric.Value->GetValue());
		}
	}
}

void UWZFFirebasePerformanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogWZFFirebaseTrace, Log, TEXT("Init UWZFFirebasePerformanceSubsystem"));

	const auto timerCallback = FTimerDelegate::CreateLambda([] {
		UE_LOG(LogWZFFirebaseTrace, Log, TEXT("Enable Firebase Perfomance flags"));
		UFirebasePerformanceLibrary::SetInstrumentationEnabled(true);
		UFirebasePerformanceLibrary::SetDataCollectionEnabled(true);
	});
	FTimerHandle timer;
	GetGameInstance()->GetTimerManager().SetTimer(timer, timerCallback, 5.f, false);

	for (const auto& traceData : UWZFSettings::Get()->Traces)
	{
		const auto trace = NewObject<UWZFFirebaseTrace>(this);
		trace->TraceData = traceData;
		trace->InitializeTrace(GetGameInstance());
		Traces.Add(trace);
	}
}
