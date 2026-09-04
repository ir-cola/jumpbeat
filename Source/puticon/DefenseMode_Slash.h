#pragma once

#include "CoreMinimal.h"
#include "DefenseModeBase.h"
#include "DefenseMode_Slash.generated.h"

/**
 * 防衛「断」＝剣で斬る。設計書 4-2。
 *
 * 押した瞬間の時刻をそのまま判定に使う。基準となる選択肢。
 * 実装は OnInputPressed で EvaluateTiming を呼ぶだけ。
 */
UCLASS(Blueprintable, ClassGroup = (Dan), meta = (BlueprintSpawnableComponent))
class PUTICON_API UDefenseMode_Slash : public UDefenseModeBase
{
	GENERATED_BODY()

public:
	virtual FText GetDisplayName() const override;
	virtual EDefenseType GetDefenseType() const override;
	virtual void OnInputPressed(float Now) override;
};
