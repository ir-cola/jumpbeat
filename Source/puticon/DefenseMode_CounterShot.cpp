#include "DefenseMode_CounterShot.h"
#include "DanGameParams.h"

FText UDefenseMode_CounterShot::GetDisplayName() const
{
	return FText::FromString(TEXT("弾"));
}

EDefenseType UDefenseMode_CounterShot::GetDefenseType() const
{
	return EDefenseType::CounterShot;
}

void UDefenseMode_CounterShot::BeginDefense(float InCharge, float InPerfectTime)
{
	Super::BeginDefense(InCharge, InPerfectTime);
	bHolding = false;
	PressTime = 0.f;
}

float UDefenseMode_CounterShot::GetRequiredHold() const
{
	return Params ? Params->GetHoldRequired(Charge) : 0.f;
}

void UDefenseMode_CounterShot::OnInputPressed(float Now)
{
	if (Result != EDefenseResult::Pending || bHolding)
	{
		return;
	}
	bHolding = true;
	PressTime = Now;
}

void UDefenseMode_CounterShot::OnInputReleased(float Now)
{
	if (Result != EDefenseResult::Pending || !bHolding || !Params)
	{
		return;
	}
	bHolding = false;

	const float Held     = Now - PressTime;
	const float Required = GetRequiredHold();

	// 溜めが足りない → 不発。タイミングが完璧でも被弾する
	if (Held < Required)
	{
		Finish(EDefenseResult::Misfire, 0.f);
		return;
	}

	// 溜めすぎ → 過充填で手元暴発（HoldMargin <= 0 なら無効）
	if (Params->HoldMargin > 0.f && Held > Required + Params->HoldMargin)
	{
		Finish(EDefenseResult::Overfilled, 0.f);
		return;
	}

	// ここまで来たら通常判定。離した瞬間が t_input
	EvaluateTiming(Now);
}
