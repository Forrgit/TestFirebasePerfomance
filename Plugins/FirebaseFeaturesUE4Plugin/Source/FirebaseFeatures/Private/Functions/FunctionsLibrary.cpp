// Copyright Pandores Marketplace 2022. All Rights Reserved.
#include "Functions/FunctionsLibrary.h"
#include "FirebaseFeatures.h"

#if WITH_FIREBASE_FUNCTIONS
	THIRD_PARTY_INCLUDES_START
#		include "firebase/functions.h"
#		include "firebase/functions/callable_reference.h"
	THIRD_PARTY_INCLUDES_END
#endif

void UFirebaseFunctionsLibrary::UseFunctionsEmulator(const FString& Name)
{
#if WITH_FIREBASE_FUNCTIONS
	using namespace firebase;

	functions::Functions* const Functions = functions::Functions::GetInstance(App::GetInstance());

	check(Functions != nullptr);

	Functions->UseFunctionsEmulator(TCHAR_TO_UTF8(*Name));
#endif
}

FFirebaseHttpsCallableReference UFirebaseFunctionsLibrary::GetHttpsCallable(const FString& Name)
{
#if WITH_FIREBASE_FUNCTIONS
	using namespace firebase;
	
	functions::Functions* const Functions = functions::Functions::GetInstance(App::GetInstance());

	check(Functions != nullptr);

	return Functions->GetHttpsCallable(TCHAR_TO_UTF8(*Name));
#else
	return {};
#endif
}

FFirebaseHttpsCallableReference UFirebaseFunctionsLibrary::GetHttpsCallableWithRegion(const FString& Name, const FString& Region)
{
#if WITH_FIREBASE_FUNCTIONS
	using namespace firebase;

	functions::Functions* const Functions = functions::Functions::GetInstance(App::GetInstance(), TCHAR_TO_UTF8(*Region));

	check(Functions != nullptr);

	return Functions->GetHttpsCallable(TCHAR_TO_UTF8(*Name));
#else
	return {};
#endif
}
