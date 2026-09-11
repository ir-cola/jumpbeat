#pragma once

#include "CoreMinimal.h"
#include "StairTypes.generated.h"

/** ゲームの進行状態。★Result中は入力を完全に遮断する */
UENUM(BlueprintType)
enum class EStairGameState : uint8
{
	/** Esc で止めている最中 */
	Paused   UMETA(DisplayName = "ポーズ"),

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

/**
 * ★譜面の音符の種類。どう跳ぶかを表す。
 *   打ち込みは SPACE / A / D / W に対応する。
 */
UENUM(BlueprintType)
enum class EStairNote : uint8
{
	Forward  UMETA(DisplayName = "正面 (SPACE)"),
	Left     UMETA(DisplayName = "左 (A)"),
	Right    UMETA(DisplayName = "右 (D)"),
	Red      UMETA(DisplayName = "赤マス (W)")
};

/** 譜面の音符1つ */
USTRUCT(BlueprintType)
struct FStairChartNote
{
	GENERATED_BODY()

	/** 位置。単位は「1拍 ÷ ChartSubdivision」 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Note")
	int32 Slot = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Note")
	EStairNote Type = EStairNote::Forward;

	bool operator<(const FStairChartNote& O) const { return Slot < O.Slot; }
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
	 * ★ジャンプの譜面。ここに置いた時刻で跳ぶ。
	 *   値の単位は「1拍 ÷ ChartSubdivision」（既定4なら 1 = 1/4拍）。
	 *   4 なら 0,1,2,3 が1拍ぶんで、裏拍や16分の位置にも置ける。
	 *
	 *   自動生成ではなく曲ごとに手で置く。
	 *   ゲーム中に O で編集モードに入り、クリックした時刻を
	 *   最寄りのグリッドに丸めて記録する。
	 *   拍で持つので、BPM を変えても譜面はそのまま追従する。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Song")
	TArray<FStairChartNote> Notes;

	/** 難易度表示用（1〜5） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Song",
		meta = (ClampMin = "1", ClampMax = "5"))
	int32 Difficulty = 3;
};
