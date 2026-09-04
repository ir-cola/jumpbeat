#include "ChargeMinigame_Rotate.h"
#include "DanGameParams.h"

FText UChargeMinigame_Rotate::GetDisplayName() const
{
	return FText::FromString(TEXT("回転"));
}

EChargeMinigameType UChargeMinigame_Rotate::GetMinigameType() const
{
	return EChargeMinigameType::Rotate;
}

void UChargeMinigame_Rotate::BeginMinigame()
{
	Super::BeginMinigame();
	VirtualPos = FVector2D(100.f, 0.f);
	Turns = 0.f;
	LastAngleDeg = 0.f;
	IdleTime = 0.f;
	bStarted = false;
}

float UChargeMinigame_Rotate::GetProgress() const
{
	if (!Params || Params->RotateTarget <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(Turns / Params->RotateTarget, 0.f, 1.f);
}

void UChargeMinigame_Rotate::OnPointerMove(FVector2D Delta)
{
	if (bFinished || !Params || Delta.IsNearlyZero())
	{
		return;
	}

	const float PrevAngle = FMath::RadiansToDegrees(FMath::Atan2(VirtualPos.Y, VirtualPos.X));

	// 仮想カーソルを動かす。中心から離れすぎ／近づきすぎないよう半径を正規化する
	VirtualPos += Delta;
	const float R = VirtualPos.Size();
	if (R < KINDA_SMALL_NUMBER)
	{
		VirtualPos = FVector2D(100.f, 0.f);
		return;
	}
	VirtualPos = VirtualPos / R * 100.f;

	const float NewAngle = FMath::RadiansToDegrees(FMath::Atan2(VirtualPos.Y, VirtualPos.X));
	LastAngleDeg = NewAngle;

	// -180〜180 に畳んだ差分を積算する（一気に半周以上飛んだ場合は無視）
	float Diff = FMath::UnwindDegrees(NewAngle - PrevAngle);
	if (FMath::Abs(Diff) > 170.f)
	{
		return;
	}

	Turns += Diff / 360.f;
	Turns = FMath::Max(0.f, Turns);
	IdleTime = 0.f;
	bStarted = true;

	// 回しすぎると暴発
	if (Turns > Params->RotateBurst)
	{
		TriggerBurst();
		return;
	}

	Charge = FMath::Clamp(Turns / Params->RotateTarget, 0.f, 1.f);
	UpdateDanAndNotify();
}

void UChargeMinigame_Rotate::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bFinished || !Params)
	{
		return;
	}

	// 手を止めると減衰していく
	Turns = FMath::Max(0.f, Turns - Params->RotateDecay * DeltaTime);
	Charge = FMath::Clamp(Turns / Params->RotateTarget, 0.f, 1.f);

	// 回し始めたあと、一定時間動かさなければ確定
	if (bStarted)
	{
		IdleTime += DeltaTime;
		if (IdleTime >= Params->RotateIdleEnd)
		{
			EndMinigame();
		}
	}
}
