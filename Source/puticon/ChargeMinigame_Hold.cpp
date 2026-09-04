#include "ChargeMinigame_Hold.h"
#include "DanGameParams.h"

FText UChargeMinigame_Hold::GetDisplayName() const
{
	return FText::FromString(TEXT("溜め"));
}

EChargeMinigameType UChargeMinigame_Hold::GetMinigameType() const
{
	return EChargeMinigameType::Hold;
}

void UChargeMinigame_Hold::BeginMinigame()
{
	Super::BeginMinigame();
	bHolding = false;
	HeldTime = 0.f;
}

float UChargeMinigame_Hold::GetProgress() const
{
	if (!Params || Params->HoldTimeBurst <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(HeldTime / Params->HoldTimeBurst, 0.f, 1.f);
}

void UChargeMinigame_Hold::OnPressStart(float Now)
{
	if (bFinished)
	{
		return;
	}
	bHolding = true;
	HeldTime = 0.f;
}

void UChargeMinigame_Hold::OnPressEnd(float Now)
{
	if (bFinished || !bHolding)
	{
		return;
	}
	bHolding = false;

	// 離した瞬間にチャージ量が確定する
	EndMinigame();
}

void UChargeMinigame_Hold::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bFinished || !bHolding || !Params)
	{
		return;
	}

	HeldTime += DeltaTime;

	// c = clamp(押していた時間 / 最大チャージ時間, 0, 1)
	Charge = FMath::Clamp(HeldTime / Params->HoldTimeMax, 0.f, 1.f);
	UpdateDanAndNotify();

	// 溜めすぎたら暴発。十段を狙うなら 3.00〜3.30秒 の 0.3秒の窓で離す必要がある
	if (HeldTime > Params->HoldTimeBurst)
	{
		bHolding = false;
		TriggerBurst();
	}
}
