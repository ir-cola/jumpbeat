#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StairTypes.h"
#include "StairConfig.generated.h"

/**
 * ゲーム全体の設定。曲リストと数値を1箇所にまとめる。
 * ★調整は DA_StairConfig だけを触ること（ビルド不要）。
 */
UCLASS(BlueprintType)
class PUTICON_API UStairConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ================= 曲 =================

	/** BGM選択画面に並ぶ曲 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "01_曲")
	TArray<FStairSong> Songs;

	// ================= リズム判定（ミリ秒） =================

	/** PERFECT の判定幅（拍からの片側ズレ・ms） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "02_判定")
	float PerfectWindowMs = 55.f;

	/** GREAT の判定幅（ms）。これを超えると MISS */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "02_判定")
	float GreatWindowMs = 130.f;

	// ================= ジャンプ =================

	/** PERFECT で進む段数（通常マス） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_ジャンプ")
	int32 PerfectSteps = 2;

	/** GREAT で進む段数（通常マス） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_ジャンプ")
	int32 GreatSteps = 1;

	/** 赤マスから跳んだときの段数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_ジャンプ")
	int32 RedSteps = 5;

	/**
	 * ★滞空時間（秒）。1段でも2段でも5段でも必ずこの時間で着地する。
	 *   段数で時間が変わるとリズムが取れなくなるため、ここを固定にして
	 *   重力と初速の方を段数に合わせて計算する。
	 *   次のジャンプが打てるまでの間隔もこの値で決まる。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_ジャンプ",
		meta = (ClampMin = "0.15", ClampMax = "1.5"))
	float JumpFlightTime = 0.42f;

	/**
	 * 弧の頂点を、着地点から何ユニット上に取るか。
	 * 平らな弧だと段の前面に引っかかるので余裕を持たせる。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_ジャンプ")
	float JumpApexClearance = 140.f;

	/** MISS（その場ジャンプ）の頂点の高さ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_ジャンプ")
	float MissJumpApex = 120.f;

	/**
	 * ★ジャンプの速さ。大きいほどキビキビ跳ぶ。
	 *   重力を強めるだけなので弧の形（頂点の高さ）は変わらず、
	 *   段に引っかからないまま滞空時間だけが短くなる。
	 *   2.2 → 約0.82秒 / 5.0 → 約0.54秒 / 7.0 → 約0.46秒
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_ジャンプ",
		meta = (ClampMin = "1.0", ClampMax = "15.0"))
	float JumpGravityScale = 6.0f;

	// ================= 地形 =================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形")
	float StepHeight = 62.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形")
	float StepDepth = 230.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形")
	float LaneWidth = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形")
	FVector StepScale = FVector(2.0f, 2.2f, 0.45f);

	/** プレイヤーの左右何レーンぶんを生成しておくか（横幅は無限） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形")
	int32 LaneRadius = 7;

	/** 何段先まで生成するか */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形")
	int32 GenerateAhead = 24;

	/** 何段後ろまで残すか */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形")
	int32 KeepBehind = 10;

	/** 穴の密度。曲の序盤 → 終盤 で補間する */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形",
		meta = (ClampMin = "0.0", ClampMax = "0.8"))
	float HoleDensityStart = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形",
		meta = (ClampMin = "0.0", ClampMax = "0.8"))
	float HoleDensityEnd = 0.42f;

	/** 横に連続してよい穴の数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形")
	int32 MaxHorizontalHoleRun = 2;

	/** 上下左右で連結してよい穴の数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形")
	int32 MaxHoleCluster = 4;

	// ================= 赤マス =================

	/** 赤マスの出現割合 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "05_赤マス",
		meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float RedChanceStart = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "05_赤マス",
		meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float RedChanceEnd = 0.03f;

	/** 赤マス同士の最低間隔（段）。偏りを防ぐ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "05_赤マス")
	int32 RedMinRowGap = 8;

	// ================= 崩落 =================

	/** 同じ足場で何回MISSしたら崩れるか */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_崩落")
	int32 MissLimit = 3;

	/** 赤マスを崩落の対象外にする */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_崩落")
	bool bRedTilesNeverCollapse = true;

	// ================= 効果音 =================

	/** ジャンプ音 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	TObjectPtr<class USoundBase> JumpSound;

	/** ジャンプ音の音量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	float JumpSoundVolume = 0.7f;

	/** PERFECT のときのピッチ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	float JumpPitchPerfect = 1.25f;

	/** GREAT のときのピッチ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	float JumpPitchGreat = 1.0f;

	/** MISS のときのピッチ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	float JumpPitchMiss = 0.72f;

	/** ボタンを押したとき */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	TObjectPtr<class USoundBase> ButtonSound;

	/** カウントダウン 3・2・1 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	TObjectPtr<class USoundBase> CountdownSound;

	/** 次のMISSで足場が崩れる警告 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	TObjectPtr<class USoundBase> WarningSound;

	/** ゲームオーバーでリザルトが出るとき */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	TObjectPtr<class USoundBase> ResultSound;

	/** 死なずに完走したとき */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	TObjectPtr<class USoundBase> ClearSound;

	/** タイトル画面で流すループBGM */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	TObjectPtr<class USoundBase> TitleBGM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	float TitleBGMVolume = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "06_効果音")
	float SystemSoundVolume = 0.9f;

	/**
	 * ★音の遅れを補正する（ミリ秒）。全曲に効く。
	 *   環境によってスピーカーやドライバの遅延が違うため、
	 *   全曲まとめて「早すぎる／遅すぎる」と感じるときにここで調整する。
	 *   プラス = 音が遅れて聞こえる分だけ判定を遅らせる。
	 *   曲ごとのズレは各曲の BeatOffset で直すこと。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "02_判定",
		meta = (ClampMin = "-200.0", ClampMax = "200.0"))
	float AudioOffsetMs = 0.f;

	/**
	 * ★何拍ぶんを1回の判定にするか。
	 *   1 だと拍が速すぎて、着地したときには次のゲージが過ぎており
	 *   連続で跳べない（1拍待たされる）。2 にするとゲージが半分の速さになり
	 *   毎回のゲージで跳べるようになる。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "02_判定",
		meta = (ClampMin = "1.0", ClampMax = "4.0"))
	float BeatsPerJudge = 2.0f;

	// ================= コンボ =================

	/** これ以上たまったら画面に出す */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "08_コンボ")
	int32 ComboShowFrom = 3;

	/** 何コンボごとにヒバナを出すか */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "08_コンボ")
	int32 ComboSparkEvery = 10;

	/** ヒバナを出さないプレイヤー周辺の半径（マス） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "08_コンボ")
	int32 SparkExcludeRadius = 2;

	/** 一度に出すヒバナの数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "08_コンボ")
	int32 SparkCount = 5;

	// ================= 壁 =================

	/** 穴の密度に対する壁の出やすさ。0.5 で穴の半分 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WallChanceRatio = 0.5f;

	/**
	 * ★穴と壁を合わせて、横に何マスまで連続してよいか。
	 *   1 なら「塞がったマス」は横に1つまで。必ず隣は通れる。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形",
		meta = (ClampMin = "1", ClampMax = "4"))
	int32 MaxBlockedRun = 1;

	/** 壁の高さ。通常の足場の何倍にするか */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形",
		meta = (ClampMin = "1.0", ClampMax = "12.0"))
	float WallHeightScale = 4.5f;

	// ================= 射撃 =================

	/** 1マガジンのチャージ数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "09_射撃")
	int32 MagazineSize = 3;

	/** チャージを使い切ったときのリロード時間（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "09_射撃")
	float ReloadSeconds = 2.0f;

	/** 弾の速さ（cm/秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "09_射撃")
	float BulletSpeed = 4200.f;

	/** 弾が届く段数。1 なら1段先まで */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "09_射撃")
	int32 BulletRangeRows = 1;

	/** 弾の大きさ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "09_射撃")
	float BulletScale = 0.22f;

	// ================= 障害物（隕石） =================

	/** 落下の開始から着弾までの時間（秒）。＝撃つ猶予 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "10_障害物")
	float MeteorLeadSeconds = 2.2f;

	/** 落ちてくる高さ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "10_障害物")
	float MeteorSpawnHeight = 2600.f;

	/** 斜めに落とすための横方向のずらし */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "10_障害物")
	FVector MeteorSpawnOffset = FVector(-1500.f, 900.f, 0.f);

	/** 隕石の大きさ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "10_障害物")
	float MeteorScale = 0.85f;

	/** 回転の速さ（度/秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "10_障害物")
	FRotator MeteorSpin = FRotator(220.f, 160.f, 190.f);

	/** 撃ち落とせなかったとき、何段先の足場を壊すか */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "10_障害物")
	int32 MeteorBreakRowMin = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "10_障害物")
	int32 MeteorBreakRowMax = 5;

	/** 壊す位置の横のばらつき（プレイヤーのレーン±この値） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "10_障害物")
	int32 MeteorBreakLaneSpread = 1;

	/**
	 * ★撃つ判定の幅。縮む円の半径に対する割合で持つ。
	 *   0 で黄緑リングにぴったり重なった状態。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "10_障害物",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShotPerfectWidth = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "10_障害物",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShotGreatWidth = 0.26f;

	// ================= UI =================

	/** この段数までのぼると、拍ゲージが左へ去って二度と出ない */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "11_UI")
	int32 GaugeExitRow = 50;

	/** ゲージが去るのにかける時間（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "11_UI")
	float GaugeExitSeconds = 0.9f;

	/** ゲージに残す残像の数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "11_UI")
	int32 GhostCount = 3;

	/** 残像が消えるまでの時間（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "11_UI")
	float GhostFadeSeconds = 1.6f;

	/** 画面の開閉（丸）にかける時間（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "11_UI")
	float IrisSeconds = 0.2f;

	// ================= イントロ =================

	/** 曲が始まる前に、テンポどおり押させる回数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "12_イントロ")
	int32 IntroTapCount = 3;

	/** イントロで MISS したら数え直すか */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "12_イントロ")
	bool bIntroResetOnMiss = true;

	/**
	 * ★テンポを示すガイド音。拍ごとに鳴らす。
	 *   これに合わせて押してもらう。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "12_イントロ")
	TObjectPtr<class USoundBase> IntroClapSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "12_イントロ")
	float IntroClapVolume = 0.85f;

	// ================= 演出 =================

	/** 曲頭のカウントダウン秒数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "07_演出")
	float CountdownSeconds = 3.2f;

	/** 通常時のカメラ距離 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "07_演出")
	float CameraArmNormal = 720.f;

	/** 5段ジャンプ時のカメラ距離 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "07_演出")
	float CameraArmWide = 1250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "07_演出")
	float CameraPitch = -34.f;

	/** 日没の色（t=0 昼寄り → t=1 夜） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "07_演出")
	FLinearColor SkyColorStart = FLinearColor(0.35f, 0.55f, 0.95f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "07_演出")
	FLinearColor SkyColorEnd = FLinearColor(0.02f, 0.03f, 0.10f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "07_演出")
	FLinearColor FogColorStart = FLinearColor(0.55f, 0.62f, 0.80f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "07_演出")
	FLinearColor FogColorEnd = FLinearColor(0.04f, 0.05f, 0.12f, 1.f);

	/** 太陽の明るさ（t=0 → t=1） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "07_演出")
	float SunIntensityStart = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "07_演出")
	float SunIntensityEnd = 0.15f;

	// ================= ヘルパ =================

	/** t（0〜1）に応じた穴の密度 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetHoleDensity(float T) const
	{
		return FMath::Lerp(HoleDensityStart, HoleDensityEnd, FMath::Clamp(T, 0.f, 1.f));
	}

	/** t（0〜1）に応じた赤マス出現率 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetRedChance(float T) const
	{
		return FMath::Lerp(RedChanceStart, RedChanceEnd, FMath::Clamp(T, 0.f, 1.f));
	}
};
