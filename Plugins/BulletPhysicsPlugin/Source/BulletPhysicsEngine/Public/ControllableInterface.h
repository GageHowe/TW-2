#pragma once

#include "CoreMinimal.h"
#include "structs.h"
#include "UObject/Interface.h"
#include "ControllableInterface.generated.h"

UINTERFACE()
class UControllableInterface : public UInterface
{
	GENERATED_BODY()
};

class IControllableInterface
{
	GENERATED_BODY()

	public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnPlayerInput(FBulletPlayerInput input);
};
