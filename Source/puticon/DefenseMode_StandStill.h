#pragma once

#include "CoreMinimal.h"
#include "DefenseModeBase.h"
#include "DefenseMode_StandStill.generated.h"

/**
 * 防衛「棒立ち」＝何もしない。設計書 4-4。
 *
 * ★点数を競う選択肢ではない。完全なネタ枠。
 *   判定も得点処理も一切行わず、無条件で「測定不能」を返す。
 *   そのトライは合計スコアの計算から除外される。
 *
 * 隠し方: 選択フェーズで何も選ばずに時間切れになると、これが選ばれる。
 *         「何もしない」という行為そのものが発見方法になっている。
 */
UCLASS(Blueprintable, ClassGroup = (Dan), meta = (BlueprintSpawnableComponent))
class PUTICON_API UDefenseMode_StandStill : public UDefenseModeBase
{
	GENERATED_BODY()

public:
	virtual FText GetDisplayName() const override;
	virtual EDefenseType GetDefenseType() const override;

	/** 着弾しても何も起きない。無条件で測定不能 */
	virtual void ResolveAtImpact(float Now) override;
};
