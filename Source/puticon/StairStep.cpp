#include "StairStep.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AStairStep::AStairStep()
{
	// 浮き上がりの演出のときだけ回す。普段は止めておく
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		Mesh->SetStaticMesh(CubeAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatAsset(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (MatAsset.Succeeded())
	{
		Mesh->SetMaterial(0, MatAsset.Object);
	}
}

void AStairStep::BeginPlay()
{
	Super::BeginPlay();
	ApplyTileColor();
}

void AStairStep::Setup(int32 InRow, int32 InLane, EStairTile InTile)
{
	Row = InRow;
	Lane = InLane;
	Tile = InTile;
	MissCount = 0;
	ApplyTileColor();
}

void AStairStep::ApplyTileColor()
{
	if (!Mesh)
	{
		return;
	}

	if (!DynMat)
	{
		DynMat = Mesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!DynMat)
	{
		return;
	}

	FLinearColor C = ColorNormal;
	switch (Tile)
	{
	case EStairTile::Red:   C = ColorRed;   break;
	case EStairTile::Green: C = ColorGreen; break;
	case EStairTile::Wall:  C = ColorWall;  break;
	default:                C = ColorNormal; break;
	}

	// ★ヒビの色は「崩れるマス」すべてに出す。
	//   以前は通常マスだけだったため、PERFECT で作った緑の足場が
	//   耐久を失っていても緑のままで、崩れる直前だと分からなかった。
	if (CanCollapse())
	{
		if (MissCount == 1) { C = ColorCracked1; }
		else if (MissCount >= 2) { C = ColorCracked2; }
	}

	// ★ここでは色だけを扱う。
	//   Mesh はこのアクターのルートなので、SetRelativeLocation を呼ぶと
	//   足場そのものが動いてしまい、階段の段差が消える。
	//   壁の高さと位置は Terrain 側（配置を持っている場所）で決める。

	// BasicShapeMaterial のパラメータ名に合わせる。無い場合は無視される
	DynMat->SetVectorParameterValue(TEXT("Color"), C);
	DynMat->SetVectorParameterValue(TEXT("BaseColor"), C);
}

void AStairStep::StartRise(float Depth, float Seconds)
{
	RiseGoal = GetActorLocation();
	RiseDepth = FMath::Max(10.f, Depth);
	RiseSeconds = FMath::Max(0.05f, Seconds);
	RiseElapsed = 0.f;
	bRising = true;

	// いったん下に沈めてから上げる
	SetActorLocation(RiseGoal - FVector(0.f, 0.f, RiseDepth));

	// 浮き上がりのあいだだけ Tick を回す
	SetActorTickEnabled(true);
}

void AStairStep::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bRising)
	{
		SetActorTickEnabled(false);
		return;
	}

	RiseElapsed += DeltaSeconds;
	const float T = FMath::Clamp(RiseElapsed / RiseSeconds, 0.f, 1.f);

	// 勢いよく出て、最後にゆっくり止まる
	const float E = 1.f - FMath::Pow(1.f - T, 3.f);

	SetActorLocation(RiseGoal - FVector(0.f, 0.f, RiseDepth * (1.f - E)));

	if (T >= 1.f)
	{
		bRising = false;
		SetActorLocation(RiseGoal);
		SetActorTickEnabled(false);
	}
}

bool AStairStep::RegisterMiss(int32 MissLimit)
{
	if (!CanCollapse())
	{
		return false; // 赤マスは崩落対象外
	}

	++MissCount;
	ApplyTileColor();

	if (MissCount >= MissLimit)
	{
		OnCollapse();
		return true;
	}

	// 1回目・2回目はヒビ演出で警告
	OnCracked(MissCount);
	return false;
}
