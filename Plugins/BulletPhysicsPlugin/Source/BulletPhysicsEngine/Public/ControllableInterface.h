#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "structs.h"
#include "ControllableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UControllableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for objects that can be controlled via player input
 * Implement this on any actor/pawn that needs to receive input
 */
class BULLETPHYSICSENGINE_API IControllableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// Called when player input is received for this object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Physics Networking")
	void OnPlayerInput(const FBulletPlayerInput& Input);
};