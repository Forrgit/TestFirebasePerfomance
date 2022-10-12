// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "CrashlyticsProxy.generated.h"

/**
 * 
 */
UCLASS()
class FIREBASEFEATURES_API UCrashlyticsProxy : public UObject
{
	GENERATED_BODY()

public:
	void InitializeCrashlytics();

	static void Log(bool bCritical, const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category);

	static void WriteBlueprintCallstack();

	static void WriteError(const FString& Log);

	static FString CrashlyticsLogFormat(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category);

protected:
	void OnGameInstanceStarted(UGameInstance* GameInstance);

	void TrySendReports();

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTrySendReportsAfterCheckDelegate, bool, bWithUnsentReports);
	UPROPERTY(BlueprintAssignable, Category = "Firebase|Crashlytics")
	FTrySendReportsAfterCheckDelegate TrySendReportsAfterInitDelegate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSendUnsentReports = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bGameInstanceStarted = false;

#if PLATFORM_IOS
	bool bUnsentReportsWasChecked = false;
	bool bHasUnsentReports = false;
#endif

protected:
	TWeakObjectPtr<UGameInstance> CachedGameInstance;

	FTimerHandle TrySendReportTimerHandle;
};

#if PLATFORM_IOS || PLATFORM_ANDROID
class FCrashlyticsOutputDeviceError : public FOutputDeviceError
{
private:
	FOutputDeviceError* Original;

public:
	FCrashlyticsOutputDeviceError(FOutputDeviceError* InOriginal)
	    : Original(InOriginal)
	{
		check(InOriginal);
	}

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override;

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category, const double Time) override;

	virtual void Flush() override
	{
		Original->Flush();
	}

	virtual void TearDown() override
	{
		Original->TearDown();
	}

	virtual void Dump(class FArchive& Ar) override
	{
		Original->Dump(Ar);
	}

	virtual bool IsMemoryOnly() const override
	{
		return Original->IsMemoryOnly();
	}

	virtual bool CanBeUsedOnAnyThread() const override
	{
		return Original->CanBeUsedOnAnyThread();
	}

	virtual void HandleError() override
	{
		Original->HandleError();
	}
};

class FCrashlyticsOutputDevice : public FOutputDevice
{
private:
	FOutputDevice* Original;

public:
	FCrashlyticsOutputDevice(FOutputDevice* InOriginal)
	    : Original(InOriginal)
	{
	}

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override;

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category, const double Time) override;
};
#endif