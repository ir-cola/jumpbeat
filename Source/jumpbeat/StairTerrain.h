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
class JUMPBEAT_API UStairTerrain : public UActorComponent
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

	/** いま存在する足場の数。背景が敷けたかの判断に使う */
	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetStepCount() const { return Steps.Num(); }

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

	/**
	 * ★譜面のとおりに跳んだときに通る道。
	 *   ここに指定されたマスは、ランダム生成に関係なく必ず足場になる。
	 *   赤マスの指定もここで受け取る。
	 *   地形全体はランダムのまま、通る道だけを保証するための仕組み。
	 */
	void SetChartPath(const TMap<int64, EStairTile>& InPath,
		const TSet<int64>& InClear, const TSet<int32>& InRedRows,
		const TSet<int32>& InHoleRows);

	/**
	 * ★その行の赤を解除して普通の床に戻す。
	 *   赤マスで judgement を外したときに使う。赤のまま残すと
	 *   次のジャンプも5段になり、譜面とずれ続けてしまう。
	 */
	void ClearRedRow(int32 Row);

	/** 譜面の道を全部消す */
	void ClearChartPath();

	/**
	 * ★横に広げる範囲を止める。
	 *   譜面が使う幅は決まっているので、その外は作らない。
	 */
	void SetLaneBounds(int32 InMin, int32 InMax) { LaneMin = InMin; LaneMax = InMax; }
	void ClearLaneBounds() { LaneMin = -1000000; LaneMax = 1000000; }

	/**
	 * ★横一列を丸ごと壊し、それより上を1段ぶん下げる。
	 *   MISS で足踏みしたぶんを詰めて、この先の譜面と足場を合わせ直す。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void CollapseRow(int32 Row);

	/**
	 * ★FromRow より先を、横に Delta レーンぶん動かす。
	 *
	 *   左右の音符を落とすと、譜面の道は横へ1つ進むのに
	 *   プレイヤーはその場に残る。行を詰めても縦しか直らないので、
	 *   横のずれはここで詰める。動かすのは前方だけで、
	 *   いま立っている足場は動かさない。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void ShiftLanesAbove(int32 FromRow, int32 Delta);

	/** 譜面の道を組み立てる側から座標をキーに変換するために公開する */
	static int64 MakeKey(int32 Row, int32 Lane)
	{
		return ((int64)Row << 20) | (uint32)(Lane + 500000);
	}

	/** そのマスが譜面の道に含まれるか。含まれるなら種類を返す */
	bool GetChartTile(int32 Row, int32 Lane, EStairTile& OutTile) const
	{
		if (const EStairTile* Found = ChartPath.Find(Key(Row, Lane)))
		{
			OutTile = *Found;
			return true;
		}
		return false;
	}

	/**
	 * 全部を床にするか。譜面を打ち込むときに使う。
	 * 穴や壁があると、置きたい位置まで進めない。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
	bool bAllFloor = false;

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

	/** キーから座標へ戻す。行を詰めるときに使う */
	static int32 RowOf(int64 K)  { return (int32)(K >> 20); }
	static int32 LaneOf(int64 K) { return (int32)(K & 0xFFFFF) - 500000; }

	UPROPERTY()
	TObjectPtr<UStairConfig> Config;

	/** 存在する足場 */
	UPROPERTY()
	TMap<int64, TObjectPtr<AStairStep>> Steps;

	/** 「穴」として確定した座標。生成済み範囲の判定に使う */
	TSet<int64> Holes;

	/** 譜面が通る道。ここは必ず足場にする */
	TMap<int64, EStairTile> ChartPath;

	/**
	 * ★跳び越える途中のマス。穴でもよいが、壁を置いてはいけない。
	 *   まっすぐ2段跳ぶとき、途中の段に壁があると引っかかるため。
	 */
	TSet<int64> ChartClear;

	/**
	 * ★横一列を丸ごと赤にする行。
	 *   赤マスは譜面で指定された場所にだけ出す。
	 *   一列すべて赤くすることで、長距離ジャンプの踏切だと遠目にも分かる。
	 */
	TSet<int32> ChartRedRows;

	/**
	 * ★赤マスから跳び越していく区間。着地点の手前まで丸ごと穴にする。
	 *   横一列が赤で埋まっているので、その先も床が続いていると
	 *   なぜ5段も跳ぶのかが分からない。谷にして跳ぶ理由を見せる。
	 */
	TSet<int32> ChartHoleRows;

	/** 譜面どおりの道を敷いているか。ランダムの赤マスを止める判断に使う */
	bool bChartRoad = false;

	/** 横に広げてよい範囲 */
	int32 LaneMin = -1000000;
	int32 LaneMax = 1000000;

	/** 行ごとに確定済みのレーン範囲 */
	TMap<int32, TPair<int32, int32>> RowRange;

	int32 GeneratedUpTo = -1;
	int32 LastRedRow = -1000;
};
