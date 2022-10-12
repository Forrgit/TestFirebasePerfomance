#pragma once

#include "CoreMinimal.h"

#include "WZFFirebasePerfomanceData.generated.h"

USTRUCT()
struct FWZFFireBaseMetricData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	FString MetricName;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UWZFFirebaseMetric> MetricClass;
};

USTRUCT()
struct FWZFFireBaseTraceData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	bool bOnceTrace = false;
	UPROPERTY(EditAnywhere)
	bool bIncrementTraceName = false;

	UPROPERTY(EditAnywhere)
	float TraceDuration = 10.f;

	UPROPERTY(EditAnywhere)
	float MetricRate = 1.f;

	UPROPERTY(EditAnywhere)
	FString TraceName;

	UPROPERTY(EditAnywhere)
	TArray<FWZFFireBaseMetricData> Metrics;
};
