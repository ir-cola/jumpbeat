#include "StairBullet.h"
#include "StairMeteor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AStairBullet::AStairBullet()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatAsset(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	if (SphereAsset.Succeeded()) { Mesh->SetStaticMesh(SphereAsset.Object); }
	if (MatAsset.Succeeded())    { Mesh->SetMaterial(0, MatAsset.Object); }
}

void AStairBullet::BeginPlay()
{
	Super::BeginPlay();

	if (Mesh)
	{
		Mesh->SetWorldScale3D(FVector(BulletScale));
		if (UMaterialInstanceDynamic* D = Mesh->CreateAndSetMaterialInstanceDynamic(0))
		{
			D->SetVectorParameterValue(TEXT("Color"), BulletColor);
			D->SetVectorParameterValue(TEXT("BaseColor"), BulletColor);
		}
	}

	SetLifeSpan(MaxLife + 0.3f);
}

void AStairBullet::Fire(AStairMeteor* InTarget, const FVector& Forward, float InSpeed)
{
	Target = InTarget;
	Speed = FMath::Max(100.f, InSpeed);

	FVector Dir = Forward;
	if (!Dir.Normalize())
	{
		Dir = FVector::ForwardVector;
	}
	Velocity = Dir * Speed;
}

void AStairBullet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	if (Age > MaxLife)
	{
		Destroy();
		return;
	}

	// ★対象がいれば毎フレーム向き直す。狙いを外す余地を作らない。
	//   撃つタイミングだけが問われるようにするための誘導。
	if (AStairMeteor* M = Target.Get())
	{
		if (M->IsAlive())
		{
			const FVector To = M->GetActorLocation() - GetActorLocation();
			const float Dist = To.Size();

			if (Dist <= HitDistance)
			{
				// 当たった。爆散は隕石側が出す
				M->Destroyed_ByShot();
				Destroy();
				return;
			}

			Velocity = To.GetSafeNormal() * Speed;
		}
		else
		{
			// 既に撃ち落とされていた
			Target = nullptr;
		}
	}

	SetActorLocation(GetActorLocation() + Velocity * DeltaSeconds);
}
