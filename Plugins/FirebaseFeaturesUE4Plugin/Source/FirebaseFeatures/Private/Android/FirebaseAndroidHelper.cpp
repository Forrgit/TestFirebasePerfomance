// Copyright Pandores Marketplace 2022. All Rights Reserved.

#include "FirebaseAndroidHelper.h"

jclass FirebaseUtils::GetGameActivityClass()
{
	static jobject GameActivity = nullptr;

	if (GameActivity == nullptr)
	{
		JNIEnv* const Env = AndroidJavaEnv::GetJavaEnv();

		check(Env);

		jclass jActivityClass = AndroidJavaEnv::FindJavaClass("com/epicgames/ue4/GameActivity");
		check(jActivityClass);

		GameActivity = (jclass)Env->NewGlobalRef((jobject)jActivityClass);
		Env->DeleteLocalRef(jActivityClass);
	}

	return (jclass)GameActivity;
}


