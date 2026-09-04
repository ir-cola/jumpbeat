#include "ChargeMinigameBase.h"
#include "DanGameParams.h"

UChargeMinigameBase::UChargeMinigameBase()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UChargeMinigameBase::BeginMinigame()
{
	Charge = 0.f;
	bBurst = false;
	bFinished = false;
	LastNotifiedDan = 0;
	SetComponentTickEnabled(true);
}

void UChargeMinigameBase::EndMinigame()
{
	bFinished = true;
	SetComponentTickEnabled(false);
}

FText UChargeMinigameBase::GetDisplayName() const
{
	return FText::FromString(TEXT("チャージ"));
}

EChargeMinigameType UChargeMinigameBase::GetMinigameType() const
{
	return EChargeMinigameType::Hold;
}

void UChargeMinigameBase::TriggerBurst()
{
	if (bBurst || bFinished)
	{
		return;
	}
	bBurst = true;
	Charge = 0.f;
	OnBurst();
	EndMinigame();
}

void UChargeMinigameBase::UpdateDanAndNotify()
{
	if (!Params)
	{
		return;
	}

	const int32 NowDan = Params->GetDan(Charge);
	if (NowDan > LastNotifiedDan)
	{
		LastNotifiedDan = NowDan;
		OnChargeStepUp(NowDan);
	}
}
