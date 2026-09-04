#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DanSaveGame.generated.h"

/** ランキング1件分。設計書 11-2 */
USTRUCT(BlueprintType)
struct FDanRankEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Dan")
	int32 Score = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Dan")
	FDateTime Date;

	/** 表示用「2026/08/29」 */
	FString GetDateText() const { return Date.ToString(TEXT("%Y/%m/%d")); }
};

/**
 * ランキングの保存。設計書 11-2。
 * スロット名 "DanRanking"、上位3件を降順で保持する。
 */
UCLASS()
class PUTICON_API UDanSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 降順・最大3件 */
	UPROPERTY(BlueprintReadWrite, Category = "Dan")
	TArray<FDanRankEntry> TopScores;

	static const TCHAR* SlotName() { return TEXT("DanRanking"); }
	static const int32  MaxEntries = 3;

	/** 保存済みランキングを読む。無ければ空のものを作って返す */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	static UDanSaveGame* LoadOrCreate();

	/**
	 * スコアを登録する。上位3件に入ったら true。
	 * ※棒立ちを含むプレイは呼び出さないこと（設計書 11-2）
	 */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	static bool SubmitScore(int32 Score);
};
