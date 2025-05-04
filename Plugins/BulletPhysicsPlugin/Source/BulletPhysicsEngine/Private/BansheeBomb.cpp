#include "BansheeBomb.h"

ABansheeBomb::ABansheeBomb()
{
	// Super constructor called automatically
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BansheeBombMesh"));
	RootComponent = StaticMesh;
}

void ABansheeBomb::BeginPlay()
{
	Super::BeginPlay();
}
