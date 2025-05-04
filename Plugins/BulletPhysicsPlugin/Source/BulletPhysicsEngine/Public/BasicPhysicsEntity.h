#pragma once

#include "CoreMinimal.h"
#include "TestActor.h"
#include "BasicPhysicsEntity.generated.h"

UCLASS()
class BULLETPHYSICSENGINE_API ABasicPhysicsEntity : public AActor
{
	GENERATED_BODY()
public:
	ABasicPhysicsEntity();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void AsyncPhysicsTickActor(float DeltaTime, float SimTime) override;
	
	btRigidBody* MyRigidBody = nullptr;
	
	UPROPERTY(EditAnywhere)
	ATestActor* BulletWorld = nullptr;

	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* StaticMesh;
private:
protected:
	UPROPERTY(EditDefaultsOnly)
	ATestActor* world;
};
