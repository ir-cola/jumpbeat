#include "DanEnergyBall.h"
#include "Components/SceneComponent.h"

ADanEnergyBall::ADanEnergyBall()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void ADanEnergyBall::Fire(const FVector& InOrigin, float InCharge, float FlyDuration)
{
	Origin = InOrigin;
	Charge = InCharge;
	FlyTime = FMath::Max(0.1f, FlyDuration);

	// 正面奥へ飛ばし、そこから横に回り込ませる
	const FVector Forward = GetActorForwardVector().IsNearlyZero()
		? FVector::ForwardVector
		: GetActorForwardVector();

	FarPoint = Origin + Forward * FlyDistance + FVector(0.f, OrbitSideOffset, FlyDistance * 0.25f);

	Phase = EBallPhase::FlyingOut;
	PhaseTime = 0.f;
	bWarned = false;

	SetActorLocation(Origin);
	SetActorHiddenInGame(false);

	OnFired(Charge);
}

void ADanEnergyBall::BeginReturn(float InReturnTime)
{
	ReturnTime = FMath::Max(0.1f, InReturnTime);
	Phase = EBallPhase::Returning;
	PhaseTime = 0.f;
	bWarned = false;

	OnReturnBegan(ReturnTime);
}

void ADanEnergyBall::Vanish()
{
	Phase = EBallPhase::Done;
	SetActorHiddenInGame(true);
	OnVanished();
}

float ADanEnergyBall::GetTimeToImpact() const
{
	if (Phase != EBallPhase::Returning)
	{
		return TNumericLimits<float>::Max();
	}
	return FMath::Max(0.f, ReturnTime - PhaseTime);
}

float ADanEnergyBall::GetReturnAlpha() const
{
	if (Phase != EBallPhase::Returning || ReturnTime <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(PhaseTime / ReturnTime, 0.f, 1.f);
}

void ADanEnergyBall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PhaseTime += DeltaSeconds;

	switch (Phase)
	{
	case EBallPhase::FlyingOut:
	{
		// 発射地点から遠方へ。手前は速く、奥で減速して見せる
		const float Alpha = FMath::Clamp(PhaseTime / FlyTime, 0.f, 1.f);
		const float Eased = 1.f - FMath::Square(1.f - Alpha);
		SetActorLocation(FMath::Lerp(Origin, FarPoint, Eased));

		if (Alpha >= 1.f)
		{
			Phase = EBallPhase::Orbiting;
			PhaseTime = 0.f;
		}
		break;
	}

	case EBallPhase::Orbiting:
	{
		// 帰還を待つあいだ、遠方でゆっくり回り込む
		const FVector Offset(
			FMath::Sin(PhaseTime * 0.8f) * 300.f,
			FMath::Cos(PhaseTime * 0.8f) * 300.f,
			0.f);
		SetActorLocation(FarPoint + Offset);
		break;
	}

	case EBallPhase::Returning:
	{
		const float Alpha = GetReturnAlpha();

		// 遠方から着弾地点へ。終盤ほど速く見えるようにする
		const float Eased = FMath::Square(Alpha);
		const FVector From = FarPoint;
		SetActorLocation(FMath::Lerp(From, Origin, Eased));

		// 着弾1.5秒前に予兆を出す。
		// ★十段で「弾」を選んだときの長押し開始タイミングと一致している
		if (!bWarned && GetTimeToImpact() <= WarningLeadTime)
		{
			bWarned = true;
			OnWarning();
		}
		break;
	}

	default:
		break;
	}
}
