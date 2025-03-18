#pragma once
#include "BasicPhysicsPawn.h"
#include "ASpaceshipPhysicsPawn.generated.h"

UCLASS()
class BULLETPHYSICSENGINE_API ASpaceshipPhysicsPawn : public ABasicPhysicsPawn
{
	GENERATED_BODY()
public:
	virtual void ApplyInputs(const FTWPlayerInput& input) const override;
};
