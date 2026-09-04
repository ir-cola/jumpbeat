#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DanTypes.h"
#include "DanGameParams.generated.h"

/**
 * ゲームの全パラメータ。設計書 付録。
 * ★数値の調整はこのアセット(DA_GameParams)だけを触ること。
 *   C++に直書きすると調整のたびにビルド待ちが発生して詰みます。
 */
UCLASS(BlueprintType)
class PUTICON_API UDanGameParams : public UDataAsset
{
	GENERATED_BODY()

public:
	// ---------------- 共通 ----------------

	/** 1プレイのトライ数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "01_共通")
	int32 TrialCount = 3;

	/** 得点の基準値 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "01_共通")
	float ScoreBase = 100000.f;

	/** チャージ量の指数。★最重要（設計書5章「なぜ c^2 なのか」） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "01_共通")
	float ScorePow = 2.0f;

	/** 失敗時の減点係数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "01_共通")
	float FailRate = 0.5f;

	/** 暴発時の減点係数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "01_共通")
	float BurstRate = 0.3f;

	// ---------------- 帰還と判定 ----------------

	/** 帰還時間の基準(秒) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "02_判定")
	float ReturnBase = 5.0f;

	/** 帰還時間のチャージ係数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "02_判定")
	float ReturnK = 2.0f;

	/** 判定窓の基準(秒・片側) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "02_判定")
	float WindowBase = 0.50f;

	/** 判定窓のチャージ係数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "02_判定")
	float WindowK = 0.42f;

	// ---------------- チャージ: 長押し ----------------

	/** 最大チャージ時間(秒) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_長押し")
	float HoldTimeMax = 3.00f;

	/** 暴発までの猶予(秒) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_長押し")
	float HoldTimeBurst = 3.30f;

	// ---------------- チャージ: 連打 ----------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_連打")
	float MashDuration = 3.00f;

	/** 1打あたりの増加量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_連打")
	float MashPerTap = 0.030f;

	/** 毎秒の自然減衰 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_連打")
	float MashDecay = 0.25f;

	/** これを超えると過熱暴発 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_連打")
	float MashBurst = 1.05f;

	// ---------------- チャージ: 回転 ----------------

	/** 満タンに必要な周回数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_回転")
	float RotateTarget = 10.0f;

	/** 静止時の毎秒減衰（周） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_回転")
	float RotateDecay = 0.20f;

	/** これを超えると暴発（周） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_回転")
	float RotateBurst = 10.5f;

	/** 何秒操作しなければ終了とみなすか */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_回転")
	float RotateIdleEnd = 1.0f;

	// ---------------- チャージ: 飛来球 ----------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_飛来球")
	float ClickDuration = 3.00f;

	/** 青球の数（これを全部取ると c=1.0） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_飛来球")
	int32 ClickBlueCount = 10;

	/** 赤球（爆）の数。クリックすると即暴発 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_飛来球")
	int32 ClickRedCount = 3;

	/** クリック判定の半径（スクリーン座標・ピクセル） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_チャージ_飛来球")
	float ClickRadius = 60.f;

	// ---------------- 防衛: 弾（撃ち返す） ----------------

	/** 必要長押し時間の基準(秒) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "05_防衛_弾")
	float HoldReqBase = 0.30f;

	/** 必要長押し時間のチャージ係数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "05_防衛_弾")
	float HoldReqK = 1.20f;

	/** 過充填までの猶予(秒)。0 にすると過充填判定を無効化 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "05_防衛_弾")
	float HoldMargin = 0.50f;

	// ---------------- トライごとのミニゲーム割り当て ----------------

	/** 設計書3章「ミニゲームの割り当て」。順番固定を推奨 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_進行")
	TArray<EChargeMinigameType> MinigameOrder = {
		EChargeMinigameType::Hold,
		EChargeMinigameType::Mash,
		EChargeMinigameType::Rotate
	};

	// ---------------- 各ステートの滞在時間 ----------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_演出時間")
	float TimeChargeIntro = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_演出時間")
	float TimeChargeResult = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_演出時間")
	float TimeFiring = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_演出時間")
	float TimeSelecting = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_演出時間")
	float TimeResolving = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_演出時間")
	float TimeTrialEnd = 0.5f;

	// ---------------- 計算ヘルパ ----------------

	/** 判定窓（片側・秒）。チャージが大きいほど狭くなる */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetWindow(float Charge) const
	{
		return FMath::Max(0.01f, WindowBase - WindowK * Charge);
	}

	/** 弾が帰ってくるまでの時間(秒)。チャージが大きいほど速い */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetReturnTime(float Charge) const
	{
		return FMath::Max(0.5f, ReturnBase - ReturnK * Charge);
	}

	/** 段位 1〜10 */
	UFUNCTION(BlueprintPure, Category = "Dan")
	int32 GetDan(float Charge) const
	{
		return FMath::Clamp(FMath::FloorToInt(Charge * 10.f) + 1, 1, 10);
	}

	/** 弾（撃ち返す）に必要な長押し時間(秒)。設計書 4-3 */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetHoldRequired(float Charge) const
	{
		return HoldReqBase + HoldReqK * Charge;
	}

	/** 段位の漢数字表記。「五段」など */
	UFUNCTION(BlueprintPure, Category = "Dan")
	FString GetDanText(float Charge) const
	{
		static const TCHAR* Kanji[] = {
			TEXT("一"), TEXT("二"), TEXT("三"), TEXT("四"), TEXT("五"),
			TEXT("六"), TEXT("七"), TEXT("八"), TEXT("九"), TEXT("十")
		};
		const int32 D = GetDan(Charge);
		return FString::Printf(TEXT("%s段"), Kanji[FMath::Clamp(D - 1, 0, 9)]);
	}

	/** 指定トライで使うミニゲーム */
	UFUNCTION(BlueprintPure, Category = "Dan")
	EChargeMinigameType GetMinigameForTrial(int32 Index) const
	{
		if (MinigameOrder.Num() == 0)
		{
			return EChargeMinigameType::Hold;
		}
		return MinigameOrder[FMath::Clamp(Index, 0, MinigameOrder.Num() - 1)];
	}
};
