#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StairTypes.h"
#include "StairStep.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * 階段の一段。
 * 行(Row)とレーン(Lane)を持つ。レーンは負の値も取る（横幅は無限）。
 */
UCLASS()
class PUTICON_API AStairStep : public AActor
{
	GENERATED_BODY()

public:
	AStairStep();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void Setup(int32 InRow, int32 InLane, EStairTile InTile);

	/** MISSを1回記録する。上限に達したら true（＝崩れる） */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	bool RegisterMiss(int32 MissLimit);

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetMissCount() const { return MissCount; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	EStairTile GetTile() const { return Tile; }

	/** 崩落の対象になるか。壁と赤は崩れない */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool CanCollapse() const
	{
		return Tile != EStairTile::Red && Tile != EStairTile::Wall;
	}

	/** ★乗れるマスか。壁だけ乗れない */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsLandable() const { return Tile != EStairTile::Wall; }

	/** 色を種別に応じて塗り直す */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void ApplyTileColor();

	// ---- 演出フック ----

	UFUNCTION(BlueprintImplementableEvent, Category = "Stair")
	void OnStepped();

	/** ヒビが入った（1回目・2回目のMISS警告） */
	UFUNCTION(BlueprintImplementableEvent, Category = "Stair")
	void OnCracked(int32 MissCountNow);

	UFUNCTION(BlueprintImplementableEvent, Category = "Stair")
	void OnCollapse();

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	int32 Row = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	int32 Lane = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stair")
	TObjectPtr<UStaticMeshComponent> Mesh;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	EStairTile Tile = EStairTile::Normal;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	int32 MissCount = 0;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynMat;

	/** ★階段のコントラストは背景が暗くなっても維持する。暗転させない */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|色")
	FLinearColor ColorNormal = FLinearColor(0.72f, 0.74f, 0.80f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|色")
	FLinearColor ColorRed = FLinearColor(1.0f, 0.16f, 0.18f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|色")
	FLinearColor ColorGreen = FLinearColor(0.20f, 1.0f, 0.35f, 1.f);

	/** ヒビの段階ごとの色 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|色")
	FLinearColor ColorCracked1 = FLinearColor(0.85f, 0.70f, 0.35f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|色")
	FLinearColor ColorCracked2 = FLinearColor(0.95f, 0.45f, 0.20f, 1.f);

	/** 壁の色。他と混同しないよう暗く冷たい色にする */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|色")
	FLinearColor ColorWall = FLinearColor(0.16f, 0.19f, 0.30f, 1.f);

	/** 壁を何倍の高さにするか。色だけでなく形でも分かるようにする */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|色")
	float WallHeightScale = 4.5f;

	/** 生成時の高さ。壁の伸縮の基準にする */
	float BaseScaleZ = -1.f;

public:
	/**
	 * ★下から浮き上がってくる演出を始める。
	 *   PERFECT で補填された緑の足場に使う。
	 *   いきなり現れるより、生えてきたことが分かる。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void StartRise(float Depth = 220.f, float Seconds = 0.22f);

protected:
	/** 浮き上がりの進行 */
	bool bRising = false;
	float RiseElapsed = 0.f;
	float RiseSeconds = 0.22f;
	float RiseDepth = 220.f;
	FVector RiseGoal = FVector::ZeroVector;
};
