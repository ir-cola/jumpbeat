#include "StairMeteor.h"
#include "StairSpark.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

// リングを構成する粒の数。専用の円テクスチャを持たないので、
// 小さなキューブを円周に並べて環を作る。
static constexpr int32 RingSegments = 20;

// 尾を引く粒の数
static constexpr int32 TrailCount = 14;

AStairMeteor::AStairMeteor()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylAsset(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatAsset(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	auto MakeMesh = [&](const TCHAR* Name, UStaticMesh* Mesh) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* M = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		M->SetupAttachment(Root);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetCastShadow(false);
		if (Mesh) { M->SetStaticMesh(Mesh); }
		if (MatAsset.Succeeded()) { M->SetMaterial(0, MatAsset.Object); }
		return M;
	};

	// ---- 本体。灰色のキューブ ----
	Body = MakeMesh(TEXT("Body"), CubeAsset.Succeeded() ? CubeAsset.Object : nullptr);

	// ---- 判定の輪。カメラに正対させるため RingRoot にまとめる ----
	RingRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RingRoot"));
	RingRoot->SetupAttachment(Root);

	// 赤い円盤（判定の外＝MISS）
	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	Disc->SetupAttachment(RingRoot);
	Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Disc->SetCastShadow(false);
	if (CylAsset.Succeeded()) { Disc->SetStaticMesh(CylAsset.Object); }
	if (MatAsset.Succeeded()) { Disc->SetMaterial(0, MatAsset.Object); }

	// 黄緑のリング（当たりの帯）と、白い縮む円
	for (int32 i = 0; i < RingSegments; ++i)
	{
		UStaticMeshComponent* G = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Hit%d"), i));
		G->SetupAttachment(RingRoot);
		G->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		G->SetCastShadow(false);
		if (CubeAsset.Succeeded()) { G->SetStaticMesh(CubeAsset.Object); }
		if (MatAsset.Succeeded()) { G->SetMaterial(0, MatAsset.Object); }
		HitRing.Add(G);

		UStaticMeshComponent* W = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("App%d"), i));
		W->SetupAttachment(RingRoot);
		W->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		W->SetCastShadow(false);
		if (CubeAsset.Succeeded()) { W->SetStaticMesh(CubeAsset.Object); }
		if (MatAsset.Succeeded()) { W->SetMaterial(0, MatAsset.Object); }
		AppRing.Add(W);
	}

	// ---- 尾を引く粒 ----
	for (int32 i = 0; i < TrailCount; ++i)
	{
		UStaticMeshComponent* T = MakeMesh(
			*FString::Printf(TEXT("Trail%d"), i),
			CubeAsset.Succeeded() ? CubeAsset.Object : nullptr);
		T->SetAbsolute(true, true, true);   // 本体の回転に巻き込まれないようにする
		Trail.Add(T);
	}
}

void AStairMeteor::BeginPlay()
{
	Super::BeginPlay();

	if (Body)
	{
		BodyMat = Body->CreateAndSetMaterialInstanceDynamic(0);
		if (BodyMat)
		{
			BodyMat->SetVectorParameterValue(TEXT("Color"), BodyColor);
			BodyMat->SetVectorParameterValue(TEXT("BaseColor"), BodyColor);
		}
	}

	auto Paint = [](UStaticMeshComponent* M, const FLinearColor& C)
	{
		if (!M) { return; }
		if (UMaterialInstanceDynamic* D = M->CreateAndSetMaterialInstanceDynamic(0))
		{
			D->SetVectorParameterValue(TEXT("Color"), C);
			D->SetVectorParameterValue(TEXT("BaseColor"), C);
		}
	};

	Paint(Disc, FLinearColor(0.95f, 0.06f, 0.08f, 1.f));           // 赤
	for (UStaticMeshComponent* M : HitRing)
	{
		Paint(M, FLinearColor(0.55f, 0.92f, 0.12f, 1.f));          // 黄緑
	}
	for (UStaticMeshComponent* M : AppRing)
	{
		Paint(M, FLinearColor(1.f, 1.f, 1.f, 1.f));                // 白
	}

	// 尾の粒は毎フレーム色を変えるので、ここでは器だけ作る
	TrailMats.Reset();
	for (UStaticMeshComponent* M : Trail)
	{
		TrailMats.Add(M ? M->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		if (M) { M->SetVisibility(false); }
	}
}

void AStairMeteor::Launch(const FVector& ImpactLocation, float LeadSeconds)
{
	Impact = ImpactLocation;
	Lead = FMath::Max(0.2f, LeadSeconds);
	Elapsed = 0.f;
	Approach = 0.f;
	bAlive = true;
	bImpacted = false;

	Start = GetActorLocation();
	History.Reset();
}

EStairJudge AStairMeteor::JudgeShot() const
{
	// ★白い円が黄緑リング（Approach = 1）にどれだけ近いか。
	//   ゲージの位相ではなく、円の縮み具合そのもので判定する。
	const float Diff = FMath::Abs(1.f - Approach);

	if (Diff <= PerfectWidth) { return EStairJudge::Perfect; }
	if (Diff <= GreatWidth)   { return EStairJudge::Great; }
	return EStairJudge::Miss;
}

void AStairMeteor::Destroyed_ByShot()
{
	if (!bAlive)
	{
		return;
	}
	bAlive = false;

	// 爆散させる
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AStairSpark* S = World->SpawnActor<AStairSpark>(
			AStairSpark::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SP))
		{
			S->Burst(FLinearColor(1.f, 0.55f, 0.12f, 1.f), 1.6f);
		}
	}

	Destroy();
}

void AStairMeteor::UpdateTrail(float DeltaSeconds)
{
	// 通った位置を覚えておき、その上に粒を並べる。
	// パーティクル素材を持たないので、キューブの列で煙と炎を表現する。
	TrailTimer += DeltaSeconds;
	if (TrailTimer >= TrailInterval)
	{
		TrailTimer = 0.f;
		History.Insert(GetActorLocation(), 0);
		if (History.Num() > TrailCount)
		{
			History.SetNum(TrailCount);
		}
	}

	for (int32 i = 0; i < Trail.Num(); ++i)
	{
		UStaticMeshComponent* M = Trail[i];
		if (!M) { continue; }

		if (!History.IsValidIndex(i))
		{
			M->SetVisibility(false);
			continue;
		}

		M->SetVisibility(true);
		M->SetWorldLocation(History[i]
			+ FVector(FMath::FRandRange(-14.f, 14.f),
				FMath::FRandRange(-14.f, 14.f),
				FMath::FRandRange(-8.f, 8.f)));

		const float T = float(i) / float(FMath::Max(1, TrailCount - 1));

		// 先頭は炎、後ろへ行くほど煙になって薄れる
		const float S = FMath::Lerp(0.42f, 0.10f, T) * MeteorScale;
		M->SetWorldScale3D(FVector(S));

		if (TrailMats.IsValidIndex(i) && TrailMats[i])
		{
			FLinearColor C;
			if (T < 0.35f)
			{
				// 炎：白 → 橙
				C = FMath::Lerp(FLinearColor(1.f, 0.95f, 0.6f, 1.f),
					FLinearColor(1.f, 0.42f, 0.05f, 1.f), T / 0.35f);
			}
			else
			{
				// 煙：橙 → 灰
				C = FMath::Lerp(FLinearColor(0.8f, 0.3f, 0.05f, 1.f),
					FLinearColor(0.22f, 0.22f, 0.24f, 1.f), (T - 0.35f) / 0.65f);
			}
			TrailMats[i]->SetVectorParameterValue(TEXT("Color"), C);
			TrailMats[i]->SetVectorParameterValue(TEXT("BaseColor"), C);
		}
	}
}

void AStairMeteor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bAlive)
	{
		return;
	}

	Elapsed += DeltaSeconds;
	Approach = FMath::Clamp(Elapsed / Lead, 0.f, 1.f);

	// ---- 斜めに落とす ----
	SetActorLocation(FMath::Lerp(Start, Impact, Approach));

	// ---- 回転させる ----
	if (Body)
	{
		Body->AddLocalRotation(Spin * DeltaSeconds);
		Body->SetWorldScale3D(FVector(MeteorScale));
	}

	UpdateTrail(DeltaSeconds);

	// ---- 判定の輪をカメラへ正対させる ----
	if (RingRoot)
	{
		if (APlayerCameraManager* Cam =
			UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			const FVector ToCam = Cam->GetCameraLocation() - GetActorLocation();
			RingRoot->SetWorldRotation(ToCam.Rotation());
		}

		// 円盤は少し手前に出して、本体に埋まらないようにする
		RingRoot->SetRelativeLocation(FVector::ZeroVector);

		const float R = RingRadius * MeteorScale;

		if (Disc)
		{
			// Cylinder は Z 方向に高さを持つので、寝かせて円盤にする
			Disc->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
			Disc->SetRelativeLocation(FVector(-6.f, 0.f, 0.f));
			Disc->SetRelativeScale3D(FVector(R / 50.f, R / 50.f, 0.02f));
		}

		// 黄緑のリング（固定）と白い円（縮む）を円周に並べる
		const float AppR = FMath::Lerp(R * 2.6f, R, Approach);

		for (int32 i = 0; i < RingSegments; ++i)
		{
			const float Ang = 2.f * PI * i / RingSegments;
			const FVector Dir(0.f, FMath::Cos(Ang), FMath::Sin(Ang));

			if (HitRing.IsValidIndex(i) && HitRing[i])
			{
				HitRing[i]->SetRelativeLocation(Dir * R + FVector(-10.f, 0.f, 0.f));
				HitRing[i]->SetRelativeScale3D(FVector(0.06f, 0.16f, 0.16f));
			}
			if (AppRing.IsValidIndex(i) && AppRing[i])
			{
				AppRing[i]->SetRelativeLocation(Dir * AppR + FVector(-14.f, 0.f, 0.f));
				AppRing[i]->SetRelativeScale3D(FVector(0.06f, 0.13f, 0.13f));
			}
		}
	}

	// ---- 着弾 ----
	if (Approach >= 1.f && !bImpacted)
	{
		bImpacted = true;
	}
}
