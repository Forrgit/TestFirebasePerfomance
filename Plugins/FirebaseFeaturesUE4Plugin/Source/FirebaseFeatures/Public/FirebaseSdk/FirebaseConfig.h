// Copyright Pandores Marketplace 2022. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FirebaseConfig.generated.h"

UENUM(Blueprintable, BlueprintType)
enum class EFirebaseLogLevel : uint8
{
	NoLogging = 0,
	Fatal,
	Error,
	Warning,
	Display,
	Log,
	Verbose,
	VeryVerbose,
	All = VeryVerbose,
};

UCLASS(Config = Engine, defaultconfig)
class FIREBASEFEATURES_API UFirebaseConfig : public UObject
{
	GENERATED_BODY()
public:
	UFirebaseConfig();

	static const UFirebaseConfig* Get();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Features", meta = (DisplayName = "Enable Firestore", Tooltip = "If you want to enable Firestore."))
	bool bEnableFirestore = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Features", meta = (DisplayName = "Enable Realtime Database", Tooltip = "If you want to enable Realtime Database."))
	bool bEnableDatabase = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Features", meta = (DisplayName = "Enable AdMob", Tooltip = "If you want to enable AdMob (available only on iOS and Android)."))
	bool bEnableAdMob = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Features", meta = (DisplayName = "Enable Analytics", Tooltip = "If you want to enable Firebase Analytics."))
	bool bEnableAnalytics = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Features", meta = (DisplayName = "Enable Firebase Functions", Tooltip = "If you want to enable Firebase Functions."))
	bool bEnableFunctions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Features", meta = (DisplayName = "Enable Firebase Messaging", Tooltip = "If you want to enable Messaging."))
	bool bEnableMessaging = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Features", meta = (DisplayName = "Enable Remote Config", Tooltip = "If you want to enable Messaging."))
	bool bEnableRemoteConfig = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Features", meta = (DisplayName = "Enable Firebase Storage", Tooltip = "If you want to enable Firebase Storage."))
	bool bEnableStorage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Features", meta = (DisplayName = "Enable Firebase Crashlytics", Tooltip = "If you want to enable Firebase Crashlytics."))
	bool bEnableCrashlytics = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Features", meta = (DisplayName = "Enable Firebase Performances", Tooltip = "If you want to enable Firebase Performances."))
	bool bEnablePerformances = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Analytics", meta = (DisplayName = "Analytics Session Timeout (in seconds)", Tooltip = "The duration of inactivity that terminates the current session."))
	int32 FirebaseAnalyticsSessionTimeoutDuration = 1800;

	/**
	 * Disable the prompt for notification. You should call RequestPermission if you suppress it.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Messaging", meta = (DisplayName = "Suppress Notification Permission Prompt"))
	bool bSuppressNotificationPermissionPrompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Messaging", meta = (DisplayName = "Token Registration On Init"))
	bool bTokenRegistrationOnInit = true;

	/**
	 * Enables Google Sign-In on iOS. Note that you need to set your Google's URL scheme or the application will crash
	 * on launch.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Google Sign-In", meta = (DisplayName = "Enable Google Sign-In"))
	bool bEnableGoogleSignIn = false;

	/**
	 * The Google Reversed Client ID for Google Sign-In. (com.googleusercontent.apps.1234567890-abcdefg).
	 * This value will be inserted in the info.plist's URL scheme.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Google Sign-In", meta = (DisplayName = "Google Sign-In Reversed Client ID"))
	FString GoogleReversedClientId;

	/**
	 * If set to true, Google Play Games's libraries will be included in the build.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Google Play Games", Meta = (DisplayName = "Enable Google Play Games Sign In (Android)"))
	bool bEnableGooglePlayGamesSignIn = false;

	// Gets the host of the Firestore backend to connect to.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Firestore", Meta = (DisplayName = "Host"))
	FString Host;

	// Enables or disables SSL for communication.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Firestore", Meta = (DisplayName = "SSL Enabled"))
	bool bSslEnabled = true;

	// Enables or disables local persistent storage.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Firestore", Meta = (DisplayName = "Persistence Enabled"))
	bool bPersistenceEnabled = true;

	/**
 	 * If true, the crashes will be sent automatically, without displaying additional information.
	 * If false, from the beginning information will be received about past crushes, and only then they will be sent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Crashlytics")
	bool bCrashlyticsCollectionEnabled = true;

	/* You can change this parameter from Firebase Remote Config (use key "pr-log-level" e.g.: pr-log-level: Warning) */
	UPROPERTY(Config, EditAnywhere, Category = "Crashlytics|Log")
	EFirebaseLogLevel FirebaseCrashlytics_LogLevel = EFirebaseLogLevel::Warning;

	/* You can change this parameter from Firebase Remote Config (use key "pr-error-log-level" e.g.: pr-error-log-level: Fatal) */
	UPROPERTY(Config, EditAnywhere, Category = "Crashlytics|Log")
	EFirebaseLogLevel FirebaseCrashlytics_ErrorLogLevel = EFirebaseLogLevel::Fatal;

	/* You can change this parameter from Firebase Remote Config (use key "pr-forced-log-categories" e.g.: pr-forced-log-categories: LogCrashlytics, LogPerformance, etc.) */
	UPROPERTY(Config, EditAnywhere, Category = "Crashlytics|Log")
	TArray<FName> FirebaseCrashlytics_ForcedLogCategories = { TEXT("LogFirebaseCrashlytics") };

	/* You can change this parameter from Firebase Remote Config (use key "pr-ignored-log-categories" e.g.: pr-ignored-log-categories: LogSlate, LogGame, etc.) */
	UPROPERTY(Config, EditAnywhere, Category = "Crashlytics|Log")
	TArray<FName> FirebaseCrashlytics_IgnoredLogCategories;
};
