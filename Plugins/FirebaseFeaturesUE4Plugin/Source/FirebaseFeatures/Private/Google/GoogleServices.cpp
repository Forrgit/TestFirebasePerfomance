// Copyright Pandores Marketplace 2022. All Rights Reserved.

#include "Google/GoogleServices.h"
#include "Async/Async.h"
#include "FirebaseFeatures.h"

THIRD_PARTY_INCLUDES_START
#include "firebase/app.h"
THIRD_PARTY_INCLUDES_END

#include "FirebaseSdk/FirebaseConfig.h"

#if PLATFORM_ANDROID
#	include "jni.h"
#   include "Android/AndroidJavaEnv.h"
#endif

#if PLATFORM_IOS && WITH_FIREBASE_AUTH
#   include "IOS/IOSAppDelegate.h"
#   import <GoogleSignIn/GoogleSignIn.h>
#endif

DECLARE_LOG_CATEGORY_CLASS(LogGoogleServices, Log, All);

#if PLATFORM_ANDROID
static FGoogleSignInCallback GSI_Callback;
static bool GSI_Guard = false;

extern "C"
JNIEXPORT void JNICALL
Java_com_epicgames_ue4_GameActivity_nativeOnSignInCompleteFb(JNIEnv * env, jobject thiz,
    jboolean success, jstring errorMessage)
{
    FString ErrorMessage = FJavaHelper::FStringFromParam(env, errorMessage);
    bool    bSuccess = (bool)success;

    if (bSuccess)
    {
        UE_LOG(LogGoogleServices, Log, TEXT("Signed in."));
    }
    else
    {
        UE_LOG(LogGoogleServices, Error, TEXT("Failed to sign in: %s"), *ErrorMessage);
    }

    AsyncTask(ENamedThreads::GameThread, [ErrorMessage = MoveTemp(ErrorMessage), bSuccess]() mutable -> void
    {
        GSI_Callback.ExecuteIfBound(bSuccess, MoveTemp(ErrorMessage));
        GSI_Callback = {};
        GSI_Guard = false;
    });
}

static FGetLeaderboardScoreCallback QueryLeaderboard_Callback;
static bool QueryLeaderboard_Guard = false;

extern "C"
JNIEXPORT void JNICALL
Java_com_epicgames_ue4_GameActivity_nativeOnScoreQueryCompleteFb(JNIEnv * env, jobject thiz,
    jboolean success, jstring errorMessage, jint score)
{
    FString ErrorMessage = FJavaHelper::FStringFromParam(env, errorMessage);
    bool    bSuccess = (bool)success;

    if (bSuccess)
    {
        UE_LOG(LogGoogleServices, Log, TEXT("Leaderboard queried."));
    }
    else
    {
        UE_LOG(LogGoogleServices, Error, TEXT("Failed to query leaderboard: %s"), *ErrorMessage);
    }

    AsyncTask(ENamedThreads::GameThread, [ErrorMessage = MoveTemp(ErrorMessage), bSuccess, Score = (int32)score]() mutable -> void
    {
        QueryLeaderboard_Callback.ExecuteIfBound(bSuccess, MoveTemp(ErrorMessage), Score);
        QueryLeaderboard_Callback = {};
        QueryLeaderboard_Guard = false;
    });
}
#endif

#if PLATFORM_IOS && WITH_FIREBASE_AUTH
void FirebaseFeatures_OnOpenURL(UIApplication* application, NSURL* url, NSString* sourceApplication, id annotation)
{
    if (UFirebaseConfig::Get()->bEnableGoogleSignIn)
    {
        [[GIDSignIn sharedInstance] handleURL: url];
    }
}
#endif

void UGoogleServicesLibrary::SignIn(FGoogleSignInCallback Callback)
{
    SignIn({}, true, true, false, MoveTemp(Callback));
}

void UGoogleServicesLibrary::SignIn(const FString& ServerClientID, const bool bSkipIfSigned, const bool bUseSilentSignIn, FGoogleSignInCallback Callback)
{
    SignIn(ServerClientID, bSkipIfSigned, bUseSilentSignIn, false, MoveTemp(Callback));
}

void UGoogleServicesLibrary::SignIn(const FString& ServerClientID, const bool bSkipIfSigned, const bool bUseSilentSignIn, const bool bUsePlayGames, FGoogleSignInCallback Callback)
{
#if PLATFORM_ANDROID
    if (GSI_Guard)
    {
        Callback.ExecuteIfBound(false, TEXT("previous sign in attempt not finished"));
        return;
    }

    if (JNIEnv* const Env = AndroidJavaEnv::GetJavaEnv())
    {
        UE_LOG(LogGoogleServices, Log, TEXT("Performing Android native Google Sign-In."));

        GSI_Guard = true;
        GSI_Callback = MoveTemp(Callback);
        
        static jmethodID jSignIn = 0;
        if (jSignIn == 0)
        {
            jclass jActivityClass = AndroidJavaEnv::FindJavaClass("com/epicgames/ue4/GameActivity");
            jSignIn = Env->GetMethodID(jActivityClass, "AndroidThunk_FirebaseFeatures_signIn", "(Ljava/lang/String;ZZZ)V");
            if (Env->ExceptionCheck())
            {
                UE_LOG(LogGoogleServices, Error, TEXT("Failed to find method AndroidThunk_FirebaseFeatures_signIn."));
                Env->ExceptionDescribe();
                Env->ExceptionClear();
                Env->DeleteLocalRef(jActivityClass);
                Callback.ExecuteIfBound(false, TEXT("Exception thrown, failed to get native method."));
                return;
            }
            Env->DeleteLocalRef(jActivityClass);
        }

        if (ServerClientID.IsEmpty())
        {
           UE_LOG(LogGoogleServices, Warning, TEXT("ServerClientID is empty, ID token won't be requested."));
        }

        Env->CallVoidMethod(AndroidJavaEnv::GetGameActivityThis(), jSignIn, *FJavaHelper::ToJavaString(Env, ServerClientID), (jboolean)bSkipIfSigned, (jboolean)bUseSilentSignIn, (jboolean)bUsePlayGames);

        if (Env->ExceptionCheck())
        {
            UE_LOG(LogGoogleServices, Error, TEXT("Exception thrown during the sign in procedure."))
            Env->ExceptionDescribe();
            Env->ExceptionClear();
            Callback.ExecuteIfBound(false, TEXT("Exception thrown during the sign in procedure."));
        }
    }
    else
    {
        Callback.ExecuteIfBound(false, TEXT("failed to get Java Env"));
    }   
#elif PLATFORM_IOS && WITH_FIREBASE_AUTH
    if (!UFirebaseConfig::Get()->bEnableGoogleSignIn)
    {
        Callback.ExecuteIfBound(false, TEXT("Google Sign-In is disabled by default on iOS. You can turn it on in the Plugin's settings. ")
            TEXT("Make sure to add your custom URL scheme or the application will crash."));
        return;
    }

    UE_LOG(LogGoogleServices, Log, TEXT("Signing in with Google."));

    if (bSkipIfSigned && GIDSignIn.sharedInstance.currentUser != nil)
    {
        UE_LOG(LogGoogleServices, Log, TEXT("User is already signed."));

        Callback.ExecuteIfBound(true, {});
        return;
    }

    if (bUseSilentSignIn && [GIDSignIn.sharedInstance hasPreviousSignIn])
    {
        UE_LOG(LogGoogleServices, Log, TEXT("User information available in the keychain. Trying to restore the session."));

        dispatch_async(dispatch_get_main_queue(), [Callback = MoveTemp(Callback)]() mutable -> void
        {
            [GIDSignIn.sharedInstance restorePreviousSignInWithCallback : [Callback = MoveTemp(Callback)] (GIDGoogleUser* _Nullable user, NSError* _Nullable error) mutable -> void
            {
                bool bSuccess = error == nil;
                FString ErrMessage;
                if (bSuccess)
                {
                    ErrMessage = error.description;
                    UE_LOG(LogGoogleServices, Error, TEXT("Failed to restore previous sign in: %d - %s"), error.code, *ErrMessage);
                }
                else
                {
                    UE_LOG(LogGoogleServices, Log, TEXT("Previous sign in restored."));
                }

                AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback), bSuccess, ErrMessage = MoveTemp(ErrMessage)]() mutable -> void
                {
                    Callback.ExecuteIfBound(bSuccess, MoveTemp(ErrMessage));
                });
            }];
        });
        return;
    }

    if (firebase::App::GetInstance())
    {
        UE_LOG(LogGoogleServices, Log, TEXT("Performing a full sign in."));

        dispatch_async(dispatch_get_main_queue(), [Callback = MoveTemp(Callback)]() mutable -> void
        {
            firebase::App* const App = firebase::App::GetInstance();

            GIDConfiguration* config = [[GIDConfiguration alloc] initWithClientID: FString(UTF8_TO_TCHAR(App->options().GetClientID().c_str())).GetNSString()];
            [GIDSignIn.sharedInstance signInWithConfiguration: config 
                                     presentingViewController: [IOSAppDelegate GetDelegate].IOSController
                                                     callback: [Callback = MoveTemp(Callback)] (GIDGoogleUser* _Nullable user, NSError* _Nullable error) mutable -> void
            {
                bool bSuccess = error == nil;
                FString ErrMessage;
                if (bSuccess)
                {
                    ErrMessage = error.description;
                    UE_LOG(LogGoogleServices, Error, TEXT("Failed to sign in: %d - %s"), error.code, *ErrMessage);
                }
                else
                {
                    UE_LOG(LogGoogleServices, Log, TEXT("The user signed in through a full sign in procedure."));
                }

                AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback), bSuccess, ErrMessage = MoveTemp(ErrMessage)]() mutable -> void
                {
                    Callback.ExecuteIfBound(bSuccess, MoveTemp(ErrMessage));
                });
            }];

            [config release];
        });
    }
    else
    {
        Callback.ExecuteIfBound(false, TEXT("Failed to get firebase app instance."));
    }
#else
    Callback.ExecuteIfBound(false, TEXT("invalid platform, Android or IOS expected"));
#endif
}

void UGoogleServicesLibrary::SignOut()
{
#if PLATFORM_ANDROID
    if (JNIEnv* const Env = AndroidJavaEnv::GetJavaEnv())
    {
        static jmethodID jSignOut = 0;
        if (jSignOut == 0)
        {
            jclass jActivityClass = AndroidJavaEnv::FindJavaClass("com/epicgames/ue4/GameActivity");
            jSignOut = Env->GetMethodID(jActivityClass, "AndroidThunk_FirebaseFeatures_signOut", "()V");
            Env->DeleteLocalRef(jActivityClass);
        }

        Env->CallVoidMethod(AndroidJavaEnv::GetGameActivityThis(), jSignOut);
    }
#elif PLATFORM_IOS
    [GIDSignIn.sharedInstance signOut];
#endif
}

bool UGoogleServicesLibrary::IsSignedIn()
{
#if PLATFORM_ANDROID
    if (JNIEnv* const Env = AndroidJavaEnv::GetJavaEnv())
    {
        static jmethodID jIsSigned = 0;
        if (jIsSigned == 0)
        {
            jclass jActivityClass = AndroidJavaEnv::FindJavaClass("com/epicgames/ue4/GameActivity");
            jIsSigned = Env->GetMethodID(jActivityClass, "AndroidThunk_FirebaseFeatures_isSigned", "()Z");
            Env->DeleteLocalRef(jActivityClass);
        }

        return (bool)Env->CallBooleanMethod(AndroidJavaEnv::GetGameActivityThis(), jIsSigned);
    }
#elif PLATFORM_IOS && WITH_FIREBASE_AUTH
    return GIDSignIn.sharedInstance.currentUser != nil;
#endif
    return false;
}

void UGoogleServicesLibrary::SubmitScore(const FString& LeaderboardID, int64 Score)
{
#if PLATFORM_ANDROID
    if (JNIEnv* const Env = AndroidJavaEnv::GetJavaEnv())
    {
        static jmethodID jSubmitScore = 0;
        if (jSubmitScore == 0)
        {
            jclass jActivityClass = AndroidJavaEnv::FindJavaClass("com/epicgames/ue4/GameActivity");
            jSubmitScore = Env->GetMethodID(jActivityClass, "AndroidThunk_FirebaseFeatures_SubmitScore", "(Ljava/lang/String;J)V");
            Env->DeleteLocalRef(jActivityClass);
        }

        auto jLeaderboardId = FJavaHelper::ToJavaString(Env, LeaderboardID);

        Env->CallVoidMethod(AndroidJavaEnv::GetGameActivityThis(), jSubmitScore, (jstring)*jLeaderboardId, (jlong)Score);
    }
    else
#endif
    {
        UE_LOG(LogGoogleServices, Error, TEXT("Leaderboard can only be written on Android with JNI."));
    }
}

void UGoogleServicesLibrary::ShowLeaderboard(const FString& LeaderboardID)
{
#if PLATFORM_ANDROID
    if (JNIEnv* const Env = AndroidJavaEnv::GetJavaEnv())
    {
        static jmethodID jShowLeaderboard = 0;
        if (jShowLeaderboard == 0)
        {
            jclass jActivityClass = AndroidJavaEnv::FindJavaClass("com/epicgames/ue4/GameActivity");
            jShowLeaderboard = Env->GetMethodID(jActivityClass, "AndroidThunk_FirebaseFeatures_showLeaderboard", "(Ljava/lang/String;)V");
            Env->DeleteLocalRef(jActivityClass);
        }

        auto jLeaderboardId = FJavaHelper::ToJavaString(Env, LeaderboardID);

        Env->CallVoidMethod(AndroidJavaEnv::GetGameActivityThis(), jShowLeaderboard, (jstring)*jLeaderboardId);

    }
    else
#endif
    {
        UE_LOG(LogGoogleServices, Error, TEXT("Leaderboard can only be shown on Android with JNI."));
    }
}

void UGoogleServicesLibrary::GetLeaderboardScore(const FString& LeaderboardID, FGetLeaderboardScoreCallback Callback)
{
#if PLATFORM_ANDROID
    if (QueryLeaderboard_Guard)
    {
        Callback.ExecuteIfBound(false, TEXT("previous query not finished"), 0);
        return;
    }

    if (JNIEnv* const Env = AndroidJavaEnv::GetJavaEnv())
    {
        static jmethodID jGetScore = 0;
        if (jGetScore == 0)
        {
            jclass jActivityClass = AndroidJavaEnv::FindJavaClass("com/epicgames/ue4/GameActivity");
            jGetScore = Env->GetMethodID(jActivityClass, "AndroidThunk_FirebaseFeatures_getLeaderboardScore", "(Ljava/lang/String;)V");
            Env->DeleteLocalRef(jActivityClass);
        }

        QueryLeaderboard_Guard = true;
        QueryLeaderboard_Callback = MoveTemp(Callback);

        auto jLeaderboardId = FJavaHelper::ToJavaString(Env, LeaderboardID);

        Env->CallVoidMethod(AndroidJavaEnv::GetGameActivityThis(), jGetScore, (jstring)*jLeaderboardId);
    }
    else
#endif
    {
        UE_LOG(LogGoogleServices, Error, TEXT("Leaderboard can only be shown an Android with JNI."));
        Callback.ExecuteIfBound(false, TEXT("invalid platform, Android expected"), 0);
    }
}

FString UGoogleServicesLibrary::GetIdToken()
{
#if PLATFORM_ANDROID
    if (JNIEnv* const Env = AndroidJavaEnv::GetJavaEnv())
    {
        static jmethodID jGetId = 0;
        if (jGetId == 0)
        {
            jclass jActivityClass = AndroidJavaEnv::FindJavaClass("com/epicgames/ue4/GameActivity");
            jGetId = Env->GetMethodID(jActivityClass, "AndroidThunk_FirebaseFeatures_getIdToken", "()Ljava/lang/String;");
            Env->DeleteLocalRef(jActivityClass);
        }

        jstring tokenId = (jstring)Env->CallObjectMethod(AndroidJavaEnv::GetGameActivityThis(), jGetId);

        return FJavaHelper::FStringFromLocalRef(Env, tokenId);
    }
    return TEXT("");
#elif PLATFORM_IOS && WITH_FIREBASE_AUTH
    if (GIDSignIn.sharedInstance.currentUser != nil && GIDSignIn.sharedInstance.currentUser.authentication != nil)
    {
        return GIDSignIn.sharedInstance.currentUser.authentication.idToken;
    }

    return TEXT("");
#else
    return TEXT("INVALID_PLATFORM");
#endif
}

FString UGoogleServicesLibrary::GetAccessToken()
{
#if PLATFORM_IOS
    if (GIDSignIn.sharedInstance.currentUser != nil && GIDSignIn.sharedInstance.currentUser.authentication != nil)
    {
        return GIDSignIn.sharedInstance.currentUser.authentication.accessToken;
    }

    return TEXT("");
#else
    return FString();
#endif
}

void UGoogleServicesLibrary::ShowAchievements()
{
#if PLATFORM_ANDROID
    if (JNIEnv* const Env = AndroidJavaEnv::GetJavaEnv())
    {
        static jmethodID jShowAchievements = 0;
        if (jShowAchievements == 0)
        {
            jclass jActivityClass = AndroidJavaEnv::FindJavaClass("com/epicgames/ue4/GameActivity");
            jShowAchievements = Env->GetMethodID(jActivityClass, "AndroidThunk_FirebaseFeatures_showAchievementsScreen", "()V");
            Env->DeleteLocalRef(jActivityClass);
        }

        Env->CallVoidMethod(AndroidJavaEnv::GetGameActivityThis(), jShowAchievements);

        if (Env->ExceptionCheck())
        {
            UE_LOG(LogGoogleServices, Error, TEXT("Failed to show achievements: exception occurred."));
            Env->ExceptionDescribe();
            Env->ExceptionClear();
        }
    }
#endif
}
