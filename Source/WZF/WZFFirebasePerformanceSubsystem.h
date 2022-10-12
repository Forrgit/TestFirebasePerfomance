#pragma once

#include "WZFFirebasePerfomanceData.h"

#include "Performance/FirebasePerformanceLibrary.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/StrongObjectPtr.h"

#include "WZFFirebasePerformanceSubsystem.generated.h"

struct FFirebaseTrace;

UCLASS(Abstract)
class WZF_API UWZFFirebaseMetric : public UObject
{
	GENERATED_BODY()
public:
	virtual int64 GetValue() const { PURE_VIRTUAL(UWZFFirebaseMetric::GetValue, return {};) }
};

UCLASS()
class WZF_API UWZFFPSFirebaseMetric : public UWZFFirebaseMetric
{
	GENERATED_BODY()
public:
	virtual int64 GetValue() const override;
};

UCLASS()
class WZF_API UWZFDeviceTemperatureFirebaseMetric : public UWZFFirebaseMetric
{
	GENERATED_BODY()
public:
	virtual int64 GetValue() const override;
};

UCLASS()
class WZF_API UWZFDeviceBatteryLevelFirebaseMetric : public UWZFFirebaseMetric
{
	GENERATED_BODY()
public:
	virtual int64 GetValue() const override;
};

UCLASS()
class WZF_API UWZFMemoryFirebaseMetric : public UWZFFirebaseMetric
{
	GENERATED_BODY()
public:
	virtual int64 GetValue() const override;
};

UCLASS()
class WZF_API UWZFDeviceVolumeStateFirebaseMetric : public UWZFFirebaseMetric
{
	GENERATED_BODY()
public:
	virtual int64 GetValue() const override;
};

UCLASS()
class WZF_API UWZFFirebaseTrace : public UObject
{
	GENERATED_BODY()
public:
	virtual void InitializeTrace(class UGameInstance* InGameInstance);

public:
	FWZFFireBaseTraceData TraceData;

private:
	void OnTraceTimer();
	void OnMetricTimer();

private:
	FFirebaseTrace FirebaseTrace;

	TArray<TPair<FString, TStrongObjectPtr<UWZFFirebaseMetric>>> Metrics;

	TWeakObjectPtr<class UGameInstance> GameInstance;
};

UCLASS()
class WZF_API UWZFFirebasePerformanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	// UFUNCTION(BlueprintCallable)
	// FFirebaseTrace& GetTrace(const FString& Name);

protected:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY()
	TArray<UWZFFirebaseTrace*> Traces;
};
