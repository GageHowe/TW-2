#pragma once
#include "BasicPhysicsPawn.h"
#include "AHumanoidPhysicsPawn.generated.h"

UCLASS()
class AHumanoidPhysicsPawn  : public ABasicPhysicsPawn
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	virtual void ApplyInputs(const FTWPlayerInput& input) override;
};
