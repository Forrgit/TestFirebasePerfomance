// Copyright Pandores Marketplace 2022. All Rights Reserved.

#include "GoogleNodes.h"
#include "Google/GoogleServices.h"

UFbGoogleSignInProxy* UFbGoogleSignInProxy::SignIn(FString ServerClientID, bool bSkipIfSigned, bool bUseSilentSignIn)
{
	ThisClass* const Proxy = NewObject<ThisClass>();

	Proxy->bSkipSigned = bSkipIfSigned;
	Proxy->bSilentSignIn = bUseSilentSignIn;
	Proxy->Token = MoveTemp(ServerClientID);

	return Proxy;
}

void UFbGoogleSignInProxy::Activate()
{
	UGoogleServicesLibrary::SignIn(Token, bSkipSigned, bSilentSignIn, FGoogleSignInCallback::CreateUObject(this, &ThisClass::OnTaskOver));
}

void UFbGoogleSignInProxy::OnTaskOver(const bool bSuccess, FString Message)
{
	(bSuccess ? SignedIn : Failed).Broadcast(Message);
	SetReadyToDestroy();
}

UGetLeaderboardScoreProxy* UGetLeaderboardScoreProxy::GetScore(FString LeaderboardID)
{
	ThisClass* const Proxy = NewObject<ThisClass>();

	Proxy->LeaderboardID = MoveTemp(LeaderboardID);

	return Proxy;
}

void UGetLeaderboardScoreProxy::Activate()
{
	UGoogleServicesLibrary::GetLeaderboardScore(LeaderboardID, FGetLeaderboardScoreCallback::CreateUObject(this, &ThisClass::OnTaskOver));
}

void UGetLeaderboardScoreProxy::OnTaskOver(const bool bSuccess, FString Message, int32 Score)
{
	(bSuccess ? ScoreRead : Failed).Broadcast(Message, Score);
	SetReadyToDestroy();
}


UGoogleSignInCredentialProxy* UGoogleSignInCredentialProxy::GetCredentialFromPlayGames(FString ServerClientID, bool bUseSilentSignIn, bool bSkipIfSigned)
{
	ThisClass* const Proxy = NewObject<ThisClass>();

	Proxy->bUseSilentSignIn = bUseSilentSignIn;
	Proxy->Token = MoveTemp(ServerClientID);
	Proxy->bUsePlayGames = true;
	Proxy->bSkipIfSigned = bSkipIfSigned;

	return Proxy;
}

UGoogleSignInCredentialProxy* UGoogleSignInCredentialProxy::GetCredential(FString ServerClientID, bool bUseSilentSignIn, bool bSkipIfSigned)
{
	ThisClass* const Proxy = NewObject<ThisClass>();

	Proxy->bUseSilentSignIn = bUseSilentSignIn;
	Proxy->Token = MoveTemp(ServerClientID);
	Proxy->bUsePlayGames = false;
	Proxy->bSkipIfSigned = bSkipIfSigned;

	return Proxy;
}

void UGoogleSignInCredentialProxy::Activate()
{
#if !PLATFORM_ANDROID
	if (bUsePlayGames)
	{
		Failed.Broadcast({}, TEXT("Google Play Games sign-in is unavailable on this platform."));
		return;
	}
#endif
	UGoogleServicesLibrary::SignIn(Token, true, bUseSilentSignIn, bUsePlayGames, FGoogleSignInCallback::CreateUObject(this, &ThisClass::OnTaskOver));
}

void UGoogleSignInCredentialProxy::OnTaskOver(const bool bSuccess, FString Message)
{
	if (bSuccess)
	{
		FString IdToken     = UGoogleServicesLibrary::GetIdToken();
		FString AccessToken = UGoogleServicesLibrary::GetAccessToken();

#if PLATFORM_IOS
		if (AccessToken.IsEmpty())
		{
			UE_LOG(LogFirebaseAuth, Log, TEXT("Google Sign-In Access token was empty."));
		}
#endif

		if (IdToken.IsEmpty())
		{
			Failed.Broadcast({}, TEXT("IdToken was empty. Make sure you specified a correct ServerClientID."));
		}
		else
		{
			if (!bUsePlayGames)
			{
				UE_LOG(LogFirebaseAuth, Log, TEXT("Successfully created credentials after a Google Sign-In."));
				Success.Broadcast(UCredentialLibrary::GetCredentialFromGoogle(IdToken, AccessToken), {});
			}
			else
			{
				UE_LOG(LogFirebaseAuth, Log, TEXT("Successfully created credentials after a Google Play Games Sign-In."));
				Success.Broadcast(UCredentialLibrary::GetCredentialFromPlayGames(IdToken), {});
			}
		}
	}
	else
	{
		Failed.Broadcast({}, Message);
	}
}


