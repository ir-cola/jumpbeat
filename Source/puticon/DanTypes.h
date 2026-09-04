#pragma once

#include "CoreMinimal.h"
#include "DanTypes.generated.h"

/** トライの進行状態。設計書 6-1 */
UENUM(BlueprintType)
enum class EDanTrialState : uint8
{
	Idle          UMETA(DisplayName = "待機"),
	ChargeIntro   UMETA(DisplayName = "ミニゲーム名表示"),
	Charging      UMETA(DisplayName = "チャージ中"),
	ChargeResult  UMETA(DisplayName = "段位表示"),
	Firing        UMETA(DisplayName = "発射"),
	Selecting     UMETA(DisplayName = "防衛選択"),
	Returning     UMETA(DisplayName = "帰還中"),
	Resolving     UMETA(DisplayName = "判定演出"),
	TrialEnd      UMETA(DisplayName = "トライ終了"),
	Result        UMETA(DisplayName = "リザルト")
};

/** チャージ・ミニゲームの種類。設計書 3章 */
UENUM(BlueprintType)
enum class EChargeMinigameType : uint8
{
	Hold          UMETA(DisplayName = "溜め"),
	Mash          UMETA(DisplayName = "連打"),
	Rotate        UMETA(DisplayName = "回転"),
	ClickTargets  UMETA(DisplayName = "飛来球")
};

/** 防衛方法の種類。設計書 4章 */
UENUM(BlueprintType)
enum class EDefenseType : uint8
{
	Slash        UMETA(DisplayName = "断"),
	CounterShot  UMETA(DisplayName = "弾"),
	StandStill   UMETA(DisplayName = "棒立ち")
};

/** 防衛の判定結果。設計書 6章 */
UENUM(BlueprintType)
enum class EDefenseResult : uint8
{
	Pending       UMETA(DisplayName = "判定前"),
	Success       UMETA(DisplayName = "成功"),
	Failed        UMETA(DisplayName = "失敗"),
	Misfire       UMETA(DisplayName = "不発"),
	Overfilled    UMETA(DisplayName = "過充填"),
	Unmeasurable  UMETA(DisplayName = "測定不能")
};

/** 1トライ分の記録。リザルト表示に使う */
USTRUCT(BlueprintType)
struct FDanTrialRecord
{
	GENERATED_BODY()

	/** チャージ量 0〜1 */
	UPROPERTY(BlueprintReadWrite, Category = "Dan")
	float Charge = 0.f;

	/** 段位 1〜10 */
	UPROPERTY(BlueprintReadWrite, Category = "Dan")
	int32 Dan = 1;

	/** タイミング精度 0〜1 */
	UPROPERTY(BlueprintReadWrite, Category = "Dan")
	float Accuracy = 0.f;

	/** このトライの得点（マイナスあり） */
	UPROPERTY(BlueprintReadWrite, Category = "Dan")
	int32 Score = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Dan")
	EDefenseResult Result = EDefenseResult::Pending;
};
