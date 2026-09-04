#include "ChargeMinigame_Mash.h"
#include "DanGameParams.h"

FText UChargeMinigame_Mash::GetDisplayName() const
{
	return FText::FromString(TEXT("連打"));
}

EChargeMinigameType UChargeMinigame_Mash::GetMinigameType() const
{
	return EChargeMinigameType::Mash;
}

void UChargeMinigame_Mash::BeginMinigame()
{
	Super::BeginMinigame();
	Elapsed = 0.f;
	TapCount = 0;
}

float UChargeMinigame_Mash::GetProgress() const
{
	if (!Params || Params->MashDuration <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(Elapsed / Params->MashDuration, 0.f, 1.f);
}

float UChargeMinigame_Mash::GetRemainingTime() const
{
	if (!Params)
	{
		return 0.f;
	}
	return FMath::Max(0.f, Params->MashDuration - Elapsed);
}

void UChargeMinigame_Mash::OnPressStart(float Now)
{
	if (bFinished || !Params)
	{
		return;
	}

	++TapCount;
	Charge += Params->MashPerTap;

	// 打ちすぎると過熱暴発
	if (Charge > Params->MashBurst)
	{
		TriggerBurst();
		return;
	}

	Charge = FMath::Clamp(Charge, 0.f, 1.f);
	UpdateDanAndNotify();
}

void UChargeMinigame_Mash::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bFinished || !Params)
	{
		return;
	}

	Elapsed += DeltaTime;

	// 自然減衰。手を止めると下がっていく
	Charge = FMath::Max(0.f, Charge - Params->MashDecay * DeltaTime);

	// 制限時間で確定
	if (Elapsed >= Params->MashDuration)
	{
		Charge = FMath::Clamp(Charge, 0.f, 1.f);
		EndMinigame();
	}
}
