#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StairTypes.h"
#include "StairTerrain.generated.h"

class AStairStep;
class UStairConfig;

/**
 * 階段の地形生成。横幅は無限。
 *
 * ★到達性の保証:
 *   「どの足場からも、正面・斜め左・斜め右のいずれかは必ず足場」
 *   という制約を生成時に必ず満たす。これで詰みが原理的に起こらないため、
 *   BFSによる経路検証は不要。
 *
 * ★穴の制約:
 *   ・横に連続する穴は MaxHorizontalHoleRun まで
 *   ・上下左右で連結した穴は MaxHoleCluster まで
 *
 * 1マスずつ順に決めて、その都度すべての制約を確認する（逐次生成）。
 * チャンク境界をまたぐ判定漏れを避けるため、行は必ず若い順に確定させ、
 * 確定済みの隣接マスを常に参照する。
 */
UCLASS(ClassGroup = (Stair), meta = (BlueprintSpawnableComponent))
class PUTICON_API UStairTerrain : public UActorComponent
{
	GENERATED_BODY()

public:
	UStairTerrain();

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void Initialize(UStairConfig* InConfig);

	/**
	 * プレイヤーの位置に合わせて前方を生成し、後方を片付ける。
	 * T は曲の進行度（穴の密度と赤マス率に使う）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void UpdateAround(int32 PlayerRow, int32 PlayerLane, float T);

	/** その座標に足場があるか */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool HasStep(int32 Row, int32 Lane) const;

	/**
	 * その座標が既に決まっているか（足場か穴かが確定済み）。
	 * まだ生成されていない前方を「穴」と誤判定しないために使う。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsGenerated(int32 Row, int32 Lane) const
	{
		return HasStep(Row, Lane) || IsKnownHole(Row, Lane);
	}

	UFUNCTION(BlueprintPure, Category = "Stair")
	AStairStep* GetStep(int32 Row, int32 Lane) const;

	/** ワールド座標 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	FVector GetStepLocation(int32 Row, int32 Lane) const;

	/**
	 * 足場を強制的に作る（PERFECTの緑補填、赤マスの5段ジャンプ用）。
	 * 既に足場があれば何もしない。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	AStairStep* ForceCreateStep(int32 Row, int32 Lane, EStairTile Tile);

	/** 足場を消す（崩落） */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void RemoveStep(int32 Row, int32 Lane);

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void ClearAll();

	/** 生成する段のクラス */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
	TSubclassOf<AStairStep> StepClass;

	/**
	 * 最初の行を全レーン埋めるか。
	 * ゲーム本編ではプレイヤーの足場として必要だが、
	 * ★タイトルの背景では板状の土台に見えてしまうので false にする。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
	bool bSolidFirstRow = true;

	/**
	 * 後ろに残す段数を上書きする。-1 で Config の KeepBehind を使う。
	 * ★タイトルの背景ではカメラより後ろの段が画面下に広がって
	 *   「床」に見えてしまうので、0 にして消す。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
	int32 KeepBehindOverride = -1;

	/**
	 * 穴の密度を上書きする。負の値で Config の設定を使う。
	 * ★タイトルの背景は見せるだけなので穴を少なくする。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
	float HoleDensityOverride = -1.f;

	/**
	 * 壁の出やすさを上書きする。負の値で Config の設定を使う。
	 * ★タイトルの背景は見せるだけなので 0 にして壁を出さない。
	 *   壁は高さがあるため、背景に柱が林立して階段が見えなくなる。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
	float WallChanceOverride = -1.f;

protected:
	/** 行 Row を丸ごと確定させる */
	void GenerateRow(int32 Row, int32 MinLane, int32 MaxLane, float T);

	/**
	 * ★足場の大きさと位置を種別に合わせて置き直す。
	 *   壁だけ背を高くする。足場アクターのルートはメッシュなので、
	 *   高さの調整は「配置を持っている側」＝ここで行う。
	 */
	void ApplyStepTransform(class AStairStep* Step, int32 Row, int32 Lane,
		EStairTile Tile) const;

public:
	/** その座標が「穴として確定済み」か。未生成は穴とみなさない */
	bool IsKnownHole(int32 Row, int32 Lane) const;

	/** そこが壁か */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsWall(int32 Row, int32 Lane) const;

	/**
	 * ★そこに「乗れる」足場があるか。
	 *   壁も足場アクターなので HasStep は true を返す。
	 *   通行できるかを見たいときは必ずこちらを使う。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool HasLandableStep(int32 Row, int32 Lane) const
	{
		return HasStep(Row, Lane) && !IsWall(Row, Lane);
	}

	/**
	 * ★通れないマスか（穴 または 壁）。
	 *   横の連続数はこれを合算して数える。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsBlocked(int32 Row, int32 Lane) const
	{
		return IsKnownHole(Row, Lane) || IsWall(Row, Lane);
	}

protected:

	/** 穴を置いたとき、横に連続する穴が上限を超えるか */
	bool WouldExceedHorizontalRun(int32 Row, int32 Lane) const;

	/** 穴を置いたとき、連結した穴の塊が上限を超えるか */
	bool WouldExceedCluster(int32 Row, int32 Lane) const;

	/** 前の行の足場が行き場を失っていないか確認し、必要なら足場を足す */
	void EnsureReachability(int32 Row, int32 MinLane, int32 MaxLane);

	AStairStep* SpawnStep(int32 Row, int32 Lane, EStairTile Tile);

	static int64 Key(int32 Row, int32 Lane)
	{
		return ((int64)Row << 20) | (uint32)(Lane + 500000);
	}

	UPROPERTY()
	TObjectPtr<UStairConfig> Config;

	/** 存在する足場 */
	UPROPERTY()
	TMap<int64, TObjectPtr<AStairStep>> Steps;

	/** 「穴」として確定した座標。生成済み範囲の判定に使う */
	TSet<int64> Holes;

	/** 行ごとに確定済みのレーン範囲 */
	TMap<int32, TPair<int32, int32>> RowRange;

	int32 GeneratedUpTo = -1;
	int32 LastRedRow = -1000;
};
