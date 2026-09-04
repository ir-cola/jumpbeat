#include "DefenseMode_Slash.h"

FText UDefenseMode_Slash::GetDisplayName() const
{
	return FText::FromString(TEXT("断"));
}

EDefenseType UDefenseMode_Slash::GetDefenseType() const
{
	return EDefenseType::Slash;
}

void UDefenseMode_Slash::OnInputPressed(float Now)
{
	// 押した瞬間が t_input。共通の判定に丸投げする。
	EvaluateTiming(Now);
}
