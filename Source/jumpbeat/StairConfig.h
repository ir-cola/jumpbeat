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
class JUMPBEAT_API UStairConfig : public UPrimaryDataAsset
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
	 * ★滞空時間の上限（秒）。1段でも2段でも5段でも必ずこの時間で着地する。
	 *   段数で時間が変わるとリズムが取れなくなるため、ここを固定にして
	 *   重力と初速の方を段数に合わせて計算する。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_ジャンプ",
		meta = (ClampMin = "0.15", ClampMax = "1.5"))
	float JumpFlightTime = 0.42f;

	/**
	 * ★音符が詰まっているところでは滞空時間を自動で縮める。
	 *
	 *   着地するまで次のジャンプは受け付けないので、
	 *   滞空時間より短い間隔で音符が並ぶと、その音符は物理的に押せない。
	 *   たとえば BPM200 の8分刻みは 300ms しかなく、
	 *   0.42秒のままでは譜面の大半が入力を受け付けなくなる。
	 *
	 *   そこで「次の音符までの残り時間 × この割合」を滞空時間にする。
	 *   1.0 にすると着地と同時に次が来るので、少し余裕を残す。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_ジャンプ",
		meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float JumpFlightGapRatio = 0.88f;

	/** 縮めてもこれより短くはしない（秒）。速すぎて見えなくなるため */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_ジャンプ",
		meta = (ClampMin = "0.05", ClampMax = "0.4"))
	float MinJumpFlightTime = 0.12f;

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
	 * ★赤マスから跳ぶときは、一拍ぶん浮いたままにする。
	 *   すぐ着地せずゆっくり滞空させることで、
	 *   5段先まで一気に跳ぶ大技だと分かるようにする。
	 *   浮いているあいだは操作できない（着地するまで次を受け付けないため）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_ジャンプ",
		meta = (ClampMin = "0.25", ClampMax = "4.0"))
	float RedFlightBeats = 1.0f;

	/** 赤マスから跳ぶときの弧の高さ。普段よりずっと高く跳ばせる */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "03_ジャンプ")
	float RedJumpApex = 420.f;

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

	/**
	 * 壁の高さ。
	 * 1 で「1段上の足場と同じ高さ」（上の段から見て床が続いて見える）。
	 * 大きくするとそこからさらに上へ伸びる。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "04_地形",
		meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float WallHeightScale = 3.33f;

	// ================= UI =================

	/** ゲージに残す残像の数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "11_UI")
	int32 GhostCount = 3;

	/** 残像が消えるまでの時間（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "11_UI")
	float GhostFadeSeconds = 1.6f;

	/** 画面の開閉（丸）にかける時間（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "11_UI")
	float IrisSeconds = 0.405f;

	/**
	 * ★真っ黒のまま止めておく時間（秒）。
	 *
	 *   閉じきったフレームでそのままレベルを切り替えると、
	 *   最後の1枚が描かれる前に画面が入れ替わり、
	 *   暗くなりきる前に次の画面が見えてしまう。
	 *   開くときも、読み込みで詰まった1フレームで一気に開ききらないよう、
	 *   まず黒いまま待ってから開き始める。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "11_UI",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IrisHoldSeconds = 0.1f;

	/**
	 * ★起動してタイトルが出るまでの、白からのフェードイン（秒）。
	 *   立ち上がりの一瞬だけエンジンの初期画面が見えてしまうので、
	 *   白で覆っておいて、そこからタイトルを浮かび上がらせる。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "11_UI",
		meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float BootFadeSeconds = 1.2f;

	/**
	 * ★白のまま待つ最低時間（秒）。
	 *   段は BeginPlay で作り終わっているが、パッケージ版は
	 *   マテリアルの準備が済むまで描画されない。
	 *   その完了をゲームから知る手立てが無いので時間で待つ。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "11_UI",
		meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float BootMinSeconds = 1.8f;

	/** 保険。何があってもこの秒数で白は明ける */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "11_UI",
		meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float BootMaxSeconds = 8.f;

	// ================= 開始 =================

	/**
	 * ★画面のサークルワイプが開ききってから、
	 *   カウントダウンが始まるまでの待ち（秒）。
	 *   開くのと同時に数字が出ると慌ただしいので少し置く。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "12_開始")
	float CountdownLeadSeconds = 1.0f;

	/**
	 * ★スタート地点から何段ぶんは、穴も壁も出さないか。
	 *   開始直後にいきなり避けさせられるのを防ぐ。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "12_開始")
	int32 SafeStartRows = 3;

	// ================= 譜面 =================

	/**
	 * ★譜面の音符1つで進む段数。
	 *   譜面があるときは判定（PERFECT/GREAT）で距離を変えない。
	 *   変えると着地点が二通りになり、通る道が定まらなくなるため。
	 *   判定はスコアとコンボにだけ効く。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "13_譜面",
		meta = (ClampMin = "1", ClampMax = "4"))
	int32 ChartStepsPerNote = 1;

	// ★エンドレスの段数はここでは決めない。
	//   道を敷かないので判定で距離を変えてよく、
	//   PerfectSteps（2段）と GreatSteps（1段）がそのまま効く。

	/**
	 * ★譜面が使う幅の外側に、何レーンぶん余裕を持たせるか。
	 *   譜面どおりに進むぶんには要らないが、
	 *   端が切り立って見えないよう少しだけ広げる。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "13_譜面",
		meta = (ClampMin = "0", ClampMax = "8"))
	int32 ChartLaneMargin = 2;

	/**
	 * ★足踏みで行を詰めるとき、段が滑るのにかける時間（秒）。
	 *   0 にすると瞬間移動になる。短くしすぎると詰めたことに気づけない。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "13_譜面",
		meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float RowShiftSlideSeconds = 0.12f;

	/** エンドレスで次の曲に移るまでの間（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "13_譜面",
		meta = (ClampMin = "0.2", ClampMax = "5.0"))
	float EndlessGapSeconds = 1.6f;

	/**
	 * ★譜面を打ち込むときの細かさ。
	 *   1拍を何分割して置けるようにするか。
	 *   4 にすると裏拍や16分の位置にも置ける。
	 *   JumpBeats の値はこの単位で持つ（4 なら 1 = 1/4拍）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "10_障害物",
		meta = (ClampMin = "1", ClampMax = "8"))
	int32 ChartSubdivision = 4;

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
