// Fill out your copyright notice in the Description page of Project Settings.

#include "Crashlytics/CrashlyticsProxy.h"

#include "Crashlytics/CrashlyticsLibrary.h"
#include "FirebaseSdk/FirebaseConfig.h"
#include "HAL/PlatformOutputDevices.h"
#include "Misc/OutputDeviceHelper.h"

void UCrashlyticsProxy::InitializeCrashlytics()
{
#if PLATFORM_IOS || PLATFORM_ANDROID
	UCrashlyticsLibrary::SetUserID(FPlatformMisc::GetDeviceId());

	if (!UFirebaseConfig::Get()->bCrashlyticsCollectionEnabled)
	{
		FWorldDelegates::OnStartGameInstance.AddUObject(this, &ThisClass::OnGameInstanceStarted);
	}

	check(GLog);
	check(GError);

	static FCrashlyticsOutputDeviceError ErrorOutput(GError);
	GError = &ErrorOutput;

	const auto PlatformOutputDevices = FPlatformOutputDevices::GetLog();
	check(PlatformOutputDevices);
	check(GLog->IsRedirectingTo(PlatformOutputDevices));
	GLog->RemoveOutputDevice(PlatformOutputDevices);

	static FCrashlyticsOutputDevice DefaultOutput(PlatformOutputDevices);
	GLog->AddOutputDevice(&DefaultOutput);

	UCrashlyticsLibrary::SetCustomStringValue(TEXT("Start Time"), FDateTime::UtcNow().ToString());
#endif
}

void UCrashlyticsProxy::OnGameInstanceStarted(UGameInstance* GameInstance)
{
	FWorldDelegates::OnStartGameInstance.RemoveAll(this);
	bGameInstanceStarted = true;

	UCrashlyticsLibrary::CheckForUnsentReports();

	CachedGameInstance = GameInstance;
	GameInstance->GetTimerManager().SetTimer(TrySendReportTimerHandle, FTimerDelegate::CreateUObject(this, &ThisClass::TrySendReports), .5f, true);
}

void UCrashlyticsProxy::TrySendReports()
{
	if (UCrashlyticsLibrary::IsUnsentReportsWasChecked())
	{
		if (UCrashlyticsLibrary::HasUnsentReports())
		{
			UCrashlyticsLibrary::SendUnsentReports();
			bSendUnsentReports = true;
		}
		TrySendReportsAfterInitDelegate.Broadcast(bSendUnsentReports);

		if (CachedGameInstance.IsValid())
		{
			CachedGameInstance->GetTimerManager().ClearTimer(TrySendReportTimerHandle);
		}
	}
}

void UCrashlyticsProxy::Log(bool bCritical, const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category)
{
	static uint8 RecursionCounter = 0;
	if (RecursionCounter > 0)
	{
		WriteError(TEXT("Crashlytics internal recursion!"));
		return;
	}

	++RecursionCounter;

	if (bCritical)
	{
		WriteBlueprintCallstack();
		WriteError(FString::Printf(TEXT("Critical: %s"), *CrashlyticsLogFormat(V, Verbosity, Category)));
	}
	else if (const auto Config = UFirebaseConfig::Get())
	{
		static EFirebaseLogLevel DefaultLogLevel = Config->FirebaseCrashlytics_LogLevel;
		static EFirebaseLogLevel ErrorLogLevel = Config->FirebaseCrashlytics_ErrorLogLevel;
		static TSet<FName> ForcedLogCategories = TSet<FName>(Config->FirebaseCrashlytics_ForcedLogCategories);
		static TSet<FName> IgnoredLogCategories = TSet<FName>(Config->FirebaseCrashlytics_IgnoredLogCategories);

		const uint8 VerbosityValue = static_cast<uint8>(Verbosity);
		if (VerbosityValue != static_cast<uint8>(EFirebaseLogLevel::NoLogging))
		{
			const uint8 LogLevelValue = static_cast<uint8>(DefaultLogLevel);
			const uint8 ErrorLogLevelValue = static_cast<uint8>(ErrorLogLevel);

			if (ForcedLogCategories.Contains(Category))
			{
				if (ErrorLogLevelValue >= VerbosityValue)
				{
					WriteBlueprintCallstack();
					const auto Msg = CrashlyticsLogFormat(V, Verbosity, Category);
					UCrashlyticsLibrary::Log(Msg);
					WriteError(Msg);
				}
				else
				{
					UCrashlyticsLibrary::Log(CrashlyticsLogFormat(V, Verbosity, Category));
				}
			}
			else if (IgnoredLogCategories.Contains(Category))
			{
				// Do nothing
			}
			else if (ErrorLogLevelValue >= VerbosityValue)
			{
				WriteBlueprintCallstack();
				const auto Msg = CrashlyticsLogFormat(V, Verbosity, Category);
				UCrashlyticsLibrary::Log(Msg);
				WriteError(Msg);
			}
			else if (LogLevelValue >= VerbosityValue)
			{
				UCrashlyticsLibrary::Log(CrashlyticsLogFormat(V, Verbosity, Category));
			}
		}
	}
	--RecursionCounter;
}

void UCrashlyticsProxy::WriteBlueprintCallstack()
{
#if DO_BLUEPRINT_GUARD
	const FBlueprintContextTracker* BlueprintExceptionTracker = FBlueprintContextTracker::TryGet();
	if (BlueprintExceptionTracker && BlueprintExceptionTracker->GetScriptStack().Num() > 0)
	{
		FString ScriptStack = FString::Printf(TEXT("Script Stack (%d frames):\n"), BlueprintExceptionTracker->GetScriptStack().Num());
		for (int32 FrameIdx = BlueprintExceptionTracker->GetScriptStack().Num() - 1; FrameIdx >= 0; --FrameIdx)
		{
			ScriptStack += BlueprintExceptionTracker->GetScriptStack()[FrameIdx]->GetStackDescription() + TEXT("\n");
		}

		UCrashlyticsLibrary::Log(ScriptStack);
	}
#endif
}

void UCrashlyticsProxy::WriteError(const FString& Log)
{
	UCrashlyticsLibrary::RecordError({}, Log, 0);
}

FString UCrashlyticsProxy::CrashlyticsLogFormat(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category)
{
	if (Category == NAME_None)
	{
		return FString::Printf(TEXT("%s"), V);
	}
	else if (Verbosity > ELogVerbosity::Type::NoLogging)
	{
		return FString::Printf(TEXT("%s: %s: %s"), *Category.ToString(), ToString(Verbosity), V);
	}
	else
	{
		return FString::Printf(TEXT("%s: %s"), *Category.ToString(), V);
	}
}

#if PLATFORM_IOS || PLATFORM_ANDROID
void FCrashlyticsOutputDeviceError::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category)
{
	UCrashlyticsProxy::Log(true, V, Verbosity, Category);

	Original->Serialize(V, Verbosity, Category);
}

void FCrashlyticsOutputDeviceError::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category, const double Time)
{
	UCrashlyticsProxy::Log(true, V, Verbosity, Category);

	Original->Serialize(V, Verbosity, Category, Time);
}

void FCrashlyticsOutputDevice::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category)
{
	UCrashlyticsProxy::Log(false, V, Verbosity, Category);

#if (!UE_BUILD_SHIPPING)
	Original->Serialize(V, Verbosity, Category);
#endif //!UE_BUILD_SHIPPING
}

void FCrashlyticsOutputDevice::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category, const double Time)
{
	UCrashlyticsProxy::Log(false, V, Verbosity, Category);

#if (!UE_BUILD_SHIPPING)
	Original->Serialize(V, Verbosity, Category, Time);
#endif //!UE_BUILD_SHIPPING
}
#endif