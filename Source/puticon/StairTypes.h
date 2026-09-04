#pragma once

#include "CoreMinimal.h"
#include "StairTypes.generated.h"

/** ゲームの進行状態。★Result中は入力を完全に遮断する */
UENUM(BlueprintType)
enum class EStairGameState : uint8
{
	/**
	 * ★曲が始まる前に、テンポを体に入れてもらう区間。
	 *   拍どおりに SPACE を規定回数押せたらカウントダウンへ進む。
	 */
	Intro    UMETA(DisplayName = "テンポ合わせ"),

	Countdown  UMETA(DisplayName = "カウントダウン"),
	Playing    UMETA(DisplayName = "プレイ中"),
	Finished   UMETA(DisplayName = "終了処理"),
	Result     UMETA(DisplayName = "リザルト")
};

/** 進行方向。正面・斜め左・斜め右の3方向のみ */
UENUM(BlueprintType)
enum class EStairDir : uint8
{
	Left     UMETA(DisplayName = "斜め左"),
	Forward  UMETA(DisplayName = "正面"),
	Right    UMETA(DisplayName = "斜め右")
};

/** リズム判定 */
UENUM(BlueprintType)
enum class EStairJudge : uint8
{
	Perfect  UMETA(DisplayName = "PERFECT"),
	Great    UMETA(DisplayName = "GREAT"),
	Miss     UMETA(DisplayName = "MISS")
};

/** マスの種類 */
UENUM(BlueprintType)
enum class EStairTile : uint8
{
	Normal   UMETA(DisplayName = "通常"),
	Red      UMETA(DisplayName = "赤"),
	Green    UMETA(DisplayName = "緑(補填)"),

	/**
	 * ★壁。跳んでも乗れず、元の位置へ弾き返される。
	 *   穴と違って落ちはしないが、進めないので左右に避けるしかない。
	 */
	Wall     UMETA(DisplayName = "壁")
};

/** 終了理由 */
UENUM(BlueprintType)
enum class EStairEndReason : uint8
{
	None       UMETA(DisplayName = "―"),
	Fell       UMETA(DisplayName = "落下"),
	Collapsed  UMETA(DisplayName = "足場崩壊"),
	SongEnd    UMETA(DisplayName = "曲の終わり")
};

/** 1曲ぶんの設定 */
USTRUCT(BlueprintType)
struct FStairSong
{
	GENERATED_BODY()

	/** 曲名（BGM選択画面に出る） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Song")
	FString Title;

	/** 出典表記 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Song")
	FString Credit = TEXT("魔王魂");

	/** 音源。Content/Stair/Audio に取り込んだものを指定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Song")
	TObjectPtr<class USoundBase> Sound;

	/** ★曲ごとに実測して設定すること。リズム判定の基準になる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Song",
		meta = (ClampMin = "40.0", ClampMax = "300.0"))
	float BPM = 140.f;

	/** 1拍目までのズレ（秒）。曲頭に無音がある場合に使う */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Song")
	float BeatOffset = 0.f;

	/** 曲の長さ（秒）。0なら音源から自動取得 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Song")
	float Duration = 0.f;

	/**
	 * ★隕石を降らせる譜面。値は「曲の頭からの拍番号」。
	 *   自動生成ではなく曲ごとに手で置く。
	 *   デバッグルームでクリックした時刻を最寄りの拍に丸めて記録する。
	 *   拍で持つので、BPM を変えても譜面はそのまま追従する。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Song")
	TArray<int32> MeteorBeats;

	/** 難易度表示用（1〜5） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Song",
		meta = (ClampMin = "1", ClampMax = "5"))
	int32 Difficulty = 3;
};
