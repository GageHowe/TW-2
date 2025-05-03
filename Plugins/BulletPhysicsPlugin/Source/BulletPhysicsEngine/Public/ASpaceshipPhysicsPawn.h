#pragma once
#include "BasicPhysicsPawn.h"
#include "ASpaceshipPhysicsPawn.generated.h"

UCLASS()
class BULLETPHYSICSENGINE_API ASpaceshipPhysicsPawn : public ABasicPhysicsPawn
{
	GENERATED_BODY()
public:
	virtual void ApplyInputs(const FTWPlayerInput& input) const override;
	// virtual void AddControllerPitchInput(float Val) override;
	// virtual void AddControllerYawInput(float Val) override;
	void shootProjectile(TSubclassOf<ABasicPhysicsEntity> projectile, FVector direction, FVector inheritedVelocity);
};
