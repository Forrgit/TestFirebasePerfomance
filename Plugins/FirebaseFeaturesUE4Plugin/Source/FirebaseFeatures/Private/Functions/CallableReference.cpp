// Copyright Pandores Marketplace 2022. All Rights Reserved.

#include "Functions/CallableReference.h"
#include "FirebaseFeatures.h"

#if WITH_FIREBASE_FUNCTIONS
THIRD_PARTY_INCLUDES_START
#	include "firebase/functions/callable_reference.h"
#	include "firebase/functions/callable_result.h"
THIRD_PARTY_INCLUDES_END
#endif

#include "Async/Async.h"


FFirebaseHttpsCallableReference::FFirebaseHttpsCallableReference()
#if WITH_FIREBASE_FUNCTIONS
	: Reference(MakeUnique<firebase::functions::HttpsCallableReference>())
#endif
{
}

#if WITH_FIREBASE_FUNCTIONS
FFirebaseHttpsCallableReference::FFirebaseHttpsCallableReference(const firebase::functions::HttpsCallableReference& InReference)
	: Reference(MakeUnique<firebase::functions::HttpsCallableReference>(InReference))
{
}
#endif

FFirebaseHttpsCallableReference::FFirebaseHttpsCallableReference(const FFirebaseHttpsCallableReference& Other)
	: FFirebaseHttpsCallableReference()
{
#if WITH_FIREBASE_FUNCTIONS
	*Reference = *Other.Reference;
#endif
}

FFirebaseHttpsCallableReference::FFirebaseHttpsCallableReference(FFirebaseHttpsCallableReference&& Other)
	: FFirebaseHttpsCallableReference()
{
#if WITH_FIREBASE_FUNCTIONS
	*Reference = MoveTemp(*Other.Reference);
#endif
}

FFirebaseHttpsCallableReference::~FFirebaseHttpsCallableReference()
{
}

FFirebaseHttpsCallableReference& FFirebaseHttpsCallableReference::operator=(const FFirebaseHttpsCallableReference& Other)
{
#if WITH_FIREBASE_FUNCTIONS
	*Reference = *Other.Reference;
#endif
	return *this;
}

FFirebaseHttpsCallableReference& FFirebaseHttpsCallableReference::operator=(FFirebaseHttpsCallableReference&& Other)
{
#if WITH_FIREBASE_FUNCTIONS
	*Reference = MoveTemp(*Other.Reference);
#endif
	return *this;
}

#if WITH_FIREBASE_FUNCTIONS
bool FFirebaseHttpsCallableReference::IsValid() const
{
	return Reference->is_valid();
}
#endif

#if WITH_FIREBASE_FUNCTIONS
void FFirebaseHttpsCallableReference::Call(const FFunctionsCallCallback& Callback)
{
	using namespace firebase;
	Reference->Call().OnCompletion([Callback](const Future<functions::HttpsCallableResult>& Future) -> void
	{
		const EFirebaseFunctionsError Error = (EFirebaseFunctionsError)Future.error();
		if (Error != EFirebaseFunctionsError::None)
		{
			UE_LOG(LogFirebaseFunctions, Error, TEXT("Function call failed. Code: %d. Message: %s"),
				Error, UTF8_TO_TCHAR(Future.error_message()));
		}

		if (Callback.IsBound())
		{
			const functions::HttpsCallableResult& Result = Future.result() ? *Future.result() : functions::HttpsCallableResult();

			AsyncTask(ENamedThreads::GameThread, [Callback, Error, Result]() -> void
			{
				Callback.ExecuteIfBound(Error, Result.data());
			});
		}
	});
}
#endif

#if WITH_FIREBASE_FUNCTIONS
static void CallInternal(firebase::functions::HttpsCallableReference& Ref, const FFirebaseVariant& Data, const FFunctionsCallCallback& Callback)
{
	Ref.Call(Data.GetRawVariant()).OnCompletion([Ref, Callback](const firebase::Future<firebase::functions::HttpsCallableResult>& Future) mutable -> void
		{
			const EFirebaseFunctionsError Error = (EFirebaseFunctionsError)Future.error();
			if (Error != EFirebaseFunctionsError::None)
			{
				UE_LOG(LogFirebaseFunctions, Error, TEXT("Function call failed. Code: %d. Message: %s"),
					Error, UTF8_TO_TCHAR(Future.error_message()));
			}

			Ref = firebase::functions::HttpsCallableReference();

			if (Callback.IsBound())
			{
				firebase::Variant Result = Future.result() ? Future.result()->data() : firebase::Variant::Null();

				AsyncTask(ENamedThreads::GameThread, [Callback, Error, Result = MoveTemp(Result)]() -> void
				{
					Callback.ExecuteIfBound(Error, Result);
				});
			}
		});
}
#endif

#if WITH_FIREBASE_FUNCTIONS
void FFirebaseHttpsCallableReference::Call(const FFirebaseVariant& Data, const FFunctionsCallCallback& Callback)
{
	CallInternal(*Reference, Data, Callback);
}
#endif

#if WITH_FIREBASE_FUNCTIONS
void FFirebaseHttpsCallableReference::operator()()
{
	Call();
}
void FFirebaseHttpsCallableReference::operator()(const FFirebaseVariant& Data)
{
	Call(Data);
}
#endif

