#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BasicPhysicsEntity.h"
#include "BasicPhysicsPawn.h"
#include "BansheeBomb.generated.h"

UCLASS()
class BULLETPHYSICSENGINE_API ABansheeBomb : public ABasicPhysicsEntity
{
	GENERATED_BODY()
public:
	ABansheeBomb();
	virtual void BeginPlay() override;
};
