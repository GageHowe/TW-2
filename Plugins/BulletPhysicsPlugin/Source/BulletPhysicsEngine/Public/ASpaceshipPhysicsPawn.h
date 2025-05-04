#pragma once
#include "BasicPhysicsPawn.h"
#include "ASpaceshipPhysicsPawn.generated.h"

UCLASS()
class BULLETPHYSICSENGINE_API ASpaceshipPhysicsPawn : public ABasicPhysicsPawn
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	virtual void ApplyInputs(const FTWPlayerInput& input) override;
	// virtual void AddControllerPitchInput(float Val) override;
	// virtual void AddControllerYawInput(float Val) override;
	void shootProjectile(TSubclassOf<ABasicPhysicsEntity> projectile, FVector direction, FVector inheritedVelocity);
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ABasicPhysicsEntity> projectile1ToSpawn;
};
