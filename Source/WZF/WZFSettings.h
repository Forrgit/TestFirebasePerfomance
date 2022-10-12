#pragma once

#include "Engine/DeveloperSettings.h"
#include "WZFFirebasePerfomanceData.h"

#include "WZFSettings.generated.h"

UCLASS(BlueprintType, config = Game, defaultconfig)
class UWZFSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetWZFSettings"))
	static UWZFSettings* Get()
	{
		return StaticClass()->GetDefaultObject<UWZFSettings>();
	}
	
	UPROPERTY(EditAnywhere, Config, Category = "Firebase|Perfomance")
	TArray<FWZFFireBaseTraceData> Traces;
};