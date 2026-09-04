#include "DefenseModeBase.h"
#include "DanGameParams.h"

UDefenseModeBase::UDefenseModeBase()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDefenseModeBase::BeginDefense(float InCharge, float InPerfectTime)
{
	Charge      = InCharge;
	PerfectTime = InPerfectTime;
	Accuracy    = 0.f;
	Result      = EDefenseResult::Pending;
}

FText UDefenseModeBase::GetDisplayName() const
{
	return FText::FromString(TEXT("防衛"));
}

EDefenseType UDefenseModeBase::GetDefenseType() const
{
	return EDefenseType::Slash;
}

void UDefenseModeBase::ResolveAtImpact(float Now)
{
	// 着弾しても未入力なら失敗（＝斬らなかった）
	if (Result == EDefenseResult::Pending)
	{
		Finish(EDefenseResult::Failed, 0.f);
	}
}

void UDefenseModeBase::Finish(EDefenseResult InResult, float InAccuracy)
{
	if (Result != EDefenseResult::Pending)
	{
		return; // 二重確定を防ぐ
	}
	Result   = InResult;
	Accuracy = InAccuracy;
	OnResolved(Result, Accuracy);
}

void UDefenseModeBase::EvaluateTiming(float InputTime)
{
	if (!Params || Result != EDefenseResult::Pending)
	{
		return;
	}

	// 設計書5章:
	//   e = t_input - t_perfect
	//   w(c) = WindowBase - WindowK * c      （チャージが大きいほど狭い）
	//   a = clamp(1 - |e| / w(c), 0, 1)
	const float E = InputTime - PerfectTime;
	const float W = Params->GetWindow(Charge);
	const float A = FMath::Clamp(1.f - FMath::Abs(E) / W, 0.f, 1.f);

	if (A > 0.f)
	{
		Finish(EDefenseResult::Success, A);
	}
	else
	{
		Finish(EDefenseResult::Failed, 0.f);
	}
}
