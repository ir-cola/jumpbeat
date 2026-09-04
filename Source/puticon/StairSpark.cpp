#include "StairSpark.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AStairSpark::AStairSpark()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatAsset(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// 粒はコンストラクタで作っておく。実行中の生成コストを避ける
	for (int32 i = 0; i < 16; ++i)
	{
		UStaticMeshComponent* M = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Spark%d"), i));
		M->SetupAttachment(Root);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetCastShadow(false);
		if (CubeAsset.Succeeded()) { M->SetStaticMesh(CubeAsset.Object); }
		if (MatAsset.Succeeded())  { M->SetMaterial(0, MatAsset.Object); }
		M->SetVisibility(false);
		Meshes.Add(M);
	}
}

void AStairSpark::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(Life + 0.5f);
}

void AStairSpark::BurstRainbow(float Power)
{
	// 粒ごとに色相を変える。派手さは色数で出す
	bRainbow = true;
	NumParticles = Meshes.Num();      // 全部使う
	ParticleSize = 0.22f;
	Life = 1.15f;
	Burst(FLinearColor::White, Power);
}

void AStairSpark::Burst(const FLinearColor& Color, float Power)
{
	SparkColor = Color;
	Age = 0.f;
	bBurst = true;

	const int32 N = FMath::Clamp(NumParticles, 1, Meshes.Num());

	Velocities.Reset();
	Starts.Reset();
	Colors.Reset();
	Mats.Reset();

	for (int32 i = 0; i < Meshes.Num(); ++i)
	{
		UStaticMeshComponent* M = Meshes[i];
		if (!M) { continue; }

		if (i >= N)
		{
			M->SetVisibility(false);
			continue;
		}

		M->SetVisibility(true);
		M->SetRelativeLocation(FVector::ZeroVector);
		M->SetRelativeScale3D(FVector(ParticleSize));

		// ★虹色モードなら粒ごとに色相をずらす
		FLinearColor MyColor = SparkColor;
		if (bRainbow)
		{
			const float Hue = 360.f * float(i) / float(FMath::Max(1, N));
			MyColor = FLinearColor::MakeFromHSV8(
				uint8(Hue / 360.f * 255.f), 235, 255);
		}
		Colors.Add(MyColor);

		if (UMaterialInstanceDynamic* D = M->CreateAndSetMaterialInstanceDynamic(0))
		{
			// 段と同じパラメータ名。無い場合は無視される
			D->SetVectorParameterValue(TEXT("Color"), MyColor);
			D->SetVectorParameterValue(TEXT("BaseColor"), MyColor);
			Mats.Add(D);
		}
		else
		{
			Mats.Add(nullptr);
		}

		// 上向きの半球にばらまく
		const float Ang = FMath::FRandRange(0.f, 2.f * PI);
		const float Rad = FMath::FRandRange(0.35f, 1.f);
		const float Up = FMath::FRandRange(0.55f, 1.f);
		FVector Dir(FMath::Cos(Ang) * Rad, FMath::Sin(Ang) * Rad, Up);
		Dir.Normalize();

		const float Speed = FMath::FRandRange(260.f, 620.f) * Power;
		Velocities.Add(Dir * Speed);
		Starts.Add(FVector::ZeroVector);
	}
}

void AStairSpark::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bBurst)
	{
		return;
	}

	Age += DeltaSeconds;
	const float T = FMath::Clamp(Age / FMath::Max(0.01f, Life), 0.f, 1.f);

	for (int32 i = 0; i < Velocities.Num() && i < Meshes.Num(); ++i)
	{
		UStaticMeshComponent* M = Meshes[i];
		if (!M) { continue; }

		// 放物線
		Velocities[i].Z -= Gravity * DeltaSeconds;
		Starts[i] += Velocities[i] * DeltaSeconds;
		M->SetRelativeLocation(Starts[i]);

		// 縮めながら消す
		const float S = ParticleSize * (1.f - T);
		M->SetRelativeScale3D(FVector(FMath::Max(0.f, S)));

		if (Mats.IsValidIndex(i) && Mats[i])
		{
			FLinearColor C = Colors.IsValidIndex(i) ? Colors[i] : SparkColor;
			C.A = 1.f - T;
			// 消え際に白へ寄せて、光が散る見た目にする
			C = FMath::Lerp(C, FLinearColor::White, T * 0.5f);

			// 虹色モードは瞬かせて花火らしくする
			if (bRainbow)
			{
				const float Flick = 0.75f + 0.25f * FMath::Sin(Age * 42.f + i);
				C *= Flick;
			}

			Mats[i]->SetVectorParameterValue(TEXT("Color"), C);
			Mats[i]->SetVectorParameterValue(TEXT("BaseColor"), C);
		}
	}

	if (T >= 1.f)
	{
		Destroy();
	}
}
