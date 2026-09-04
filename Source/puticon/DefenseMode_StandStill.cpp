#include "DefenseMode_StandStill.h"

FText UDefenseMode_StandStill::GetDisplayName() const
{
	return FText::FromString(TEXT("棒立ち"));
}

EDefenseType UDefenseMode_StandStill::GetDefenseType() const
{
	return EDefenseType::StandStill;
}

void UDefenseMode_StandStill::ResolveAtImpact(float Now)
{
	// 何も起きない。判定も加減点もしない。
	// BP側の OnResolved で「…………何も、なかった」の演出を出す。
	Finish(EDefenseResult::Unmeasurable, 0.f);
}
