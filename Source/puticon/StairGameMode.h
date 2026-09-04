#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StairTypes.h"
#include "StairGameMode.generated.h"

class AStairStep;
class AStairCharacter;
class UStairConfig;
class UStairTerrain;
class UStairMusicClock;
class UUserWidget;
class ADirectionalLight;
class AExponentialHeightFog;
class ASkyLight;

/**
 * 階段リズムゲームの進行役。
 *
 *   Countdown → Playing → Finished → Result
 *   ★Result中は入力を完全に遮断する。
 *
 * 曲の進行度 t（0→1）はここで1回だけ計算し、地形・空・UIすべてが参照する。
 */
UCLASS()
class PUTICON_API AStairGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AStairGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---------------- 状態 ----------------

	UFUNCTION(BlueprintPure, Category = "Stair")
	EStairGameState GetState() const { return State; }

	/** ★入力を受け付けてよいか。Result中は必ず false */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsInputAllowed() const { return State == EStairGameState::Playing; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Stair")
	void OnStateChanged(EStairGameState NewState);

	// ---------------- 曲と進行度 ----------------

	/** ★これが唯一の t。全システムはここを参照する */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetT() const;

	UFUNCTION(BlueprintPure, Category = "Stair")
	UStairMusicClock* GetMusicClock() const { return MusicClock; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	UStairTerrain* GetTerrain() const { return Terrain; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	UStairConfig* GetConfig() const { return Config; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	FString GetSongTitle() const { return CurrentSong.Title; }

	/** 残り時間（秒）。曲の残り */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetRemainingTime() const;

	/** カウントダウン表示。「3」「2」「1」「START」 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	FString GetCountdownText() const;

	/**
	 * ★カウントダウンの数字。表示と効果音でこれを共通に使う。
	 *   -1 = まだ始まっていない / 3,2,1 = 数字 / 0 = START
	 *
	 *   CountdownSeconds は 3.2 秒などピッタリでないことがあるので、
	 *   残り3.0秒になってから数え始める。こうしないと余りが
	 *   3→2 の間だけに入り、そこだけ間隔が広くなる。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetCountdownNumber() const;

	// ---------------- リズム判定 ----------------

	/** ★いまの瞬間の判定。オーディオ再生位置が基準 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	EStairJudge JudgeNow() const;

	/** ゲージ描画用。判定と同じパラメータから作る */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetBeatPhase() const;

	/** PERFECT ゾーンの幅（拍に対する割合）。ゲージ描画用 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetPerfectZoneHalfWidth() const;

	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetGreatZoneHalfWidth() const;

	// ---------------- スコア ----------------

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetScore() const { return Score; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetPerfectCount() const { return PerfectCount; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetGreatCount() const { return GreatCount; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetMissCount() const { return MissCount; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	EStairEndReason GetEndReason() const { return EndReason; }

	// ---------------- キャラから呼ばれる ----------------

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void NotifyLandedOn(AStairStep* Step);

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void NotifyPlayerFell();

	/** 判定が出たときに集計する */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void NotifyJudge(EStairJudge Judge);

	/** その場ジャンプ＝いま乗っている足場にMISSを記録する */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void NotifyMissOnCurrentStep();

	// ---------------- 遷移 ----------------

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void GoToTitle();

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void GoToSongSelect();

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void RetryGame();

	/**
	 * プレイヤーを取得する。★毎回取り直す。
	 * BeginPlay の時点では Pawn がまだ生成されていないことがあるため、
	 * キャッシュだけに頼ると操作が有効にならない不具合が出る。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	AStairCharacter* GetPlayerChar() const;

protected:
	void SetState(EStairGameState NewState);

	/** 入力モードをゲーム操作に戻す */
	void RestoreGameInput();

	/** Pawn を掴めたら初期配置を行う。掴めるまで毎フレーム試す */
	void TryInitPlayer();

	/** システム系の効果音を鳴らす */
	void PlaySystemSound(class USoundBase* Sound);

	/** カウントダウンの数字が変わったら音を鳴らす */
	void UpdateCountdownSound();

	/** 直近に鳴らしたカウントダウンの数字。0=未再生 */
	int32 LastCountdownNumber = 0;

	/**
	 * ★プレイ中にタイミング補正を耳で合わせる。
	 *   [ と ] で 5ms ずつ動かし、\ で 0 に戻す。
	 *   決まった値を DA_StairConfig の AudioOffsetMs に書き写せば固定できる。
	 */
	void UpdateTimingTuner();

	/** 補正値を変えた直後かどうか。画面に出す時間を測る */
	float TunerShowTime = 0.f;

public:
	/** 画面に出すタイミング補正の文字。空なら出さない */
	UFUNCTION(BlueprintPure, Category = "Stair")
	FString GetTimingTunerText() const;

	// ---------------- コンボ ----------------

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetCombo() const { return Combo; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetMaxCombo() const { return MaxCombo; }

	/**
	 * ★壁に跳んで弾き返されたとき。
	 *   判定は既に Miss として通知済みなので、ここでは足場の耐久だけ減らす。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void NotifyWallBounce();

	// ---------------- 失敗の内訳 ----------------

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetJumpMissCount() const { return JumpMissCount; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetShotMissCount() const { return ShotMissCount; }

	// ---------------- 射撃 ----------------

	/** 撃つ。射程内の隕石を自動で狙う */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void FireShot();

	/** 残りチャージ */
	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetCharges() const { return Charges; }

	/** リロード中か */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsReloading() const { return ReloadLeft > 0.f; }

	/** リロードの残り時間（0〜1） */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetReloadProgress() const;

	// ---------------- ゲージの表示 ----------------

	/**
	 * ★押した位置の記録。自分のクセ（走り気味・遅れ気味）が見えるようにする。
	 *   Phase は押した瞬間の拍の位相、Age は経過時間。
	 */
	struct FStairGhost
	{
		float Phase = 0.f;
		float Age = 0.f;
		EStairJudge Judge = EStairJudge::Miss;
	};

	const TArray<FStairGhost>& GetGhosts() const { return Ghosts; }

	/**
	 * ゲージが去る進み具合。0=通常、1=完全に画面外。
	 * 50段のぼると去り、二度と戻らない。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetGaugeExitAlpha() const { return GaugeExitAlpha; }

	// ---------------- イントロ ----------------

	/** テンポ合わせで何回押せたか */
	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetIntroTaps() const { return IntroTaps; }

	/** テンポ合わせで必要な回数 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetIntroNeeded() const;

	/** イントロ中に SPACE が押された。判定して数える */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void NotifyIntroTap();

	/**
	 * ★カウントダウン中の空押し。
	 *   進みはしないが、押した位置は残像に残す。
	 *   曲が始まる前に拍を確かめ続けられるようにするため。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void NotifyPracticeTap();

	/** 画面に出すイントロの案内文 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	FString GetIntroText() const;

	// ---------------- 譜面づくり ----------------

	/**
	 * ★譜面編集モード。
	 *   クリックした時刻を最寄りの拍に丸めて記録していく。
	 *   拍で持つので、多少クリックがずれても譜面は正確になる。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair|Chart")
	void ChartToggle();

	UFUNCTION(BlueprintPure, Category = "Stair|Chart")
	bool IsCharting() const { return bCharting; }

	/** いまの時刻を最寄りの拍に丸めて1つ置く */
	UFUNCTION(BlueprintCallable, Category = "Stair|Chart")
	void ChartPlace();

	/** 直前の1つを取り消す */
	UFUNCTION(BlueprintCallable, Category = "Stair|Chart")
	void ChartUndo();

	/** 打ち込んだ譜面をログとファイルに書き出す */
	UFUNCTION(BlueprintCallable, Category = "Stair|Chart")
	void ChartExport();

	/** 画面に出す譜面編集の状態 */
	UFUNCTION(BlueprintPure, Category = "Stair|Chart")
	FString GetChartText() const;

protected:
	/**
	 * ★判定のたびに呼ぶ。
	 *   拍を1つも飛ばさずに続けて成功したときだけ伸びる。
	 *   MISS、または拍を待つと 0 に戻る。
	 */
	void UpdateCombo(EStairJudge Judge);

	/** プレイヤーから離れた足場からヒバナを出す */
	void SpawnComboSparks();

	int32 Combo = 0;
	int32 MaxCombo = 0;

	/** MISS の内訳。合計が MissCount になる */
	int32 JumpMissCount = 0;
	int32 ShotMissCount = 0;

	// ---------------- 射撃 ----------------

	/** 残りチャージ。0 になるとリロードが入る */
	int32 Charges = 3;

	/** リロードの残り時間。0 より大きいあいだは撃てない */
	float ReloadLeft = 0.f;

	/** 射程内でいちばん判定に近い隕石を返す */
	class AStairMeteor* FindShotTarget() const;

	// ---------------- 隕石 ----------------

	/** 譜面のどこまで処理したか */
	int32 NextMeteorIndex = 0;

	/** いま落ちている隕石 */
	UPROPERTY()
	TArray<TObjectPtr<class AStairMeteor>> Meteors;

	/** 譜面を見て、時間が来たら隕石を落とす */
	void UpdateMeteors(float DeltaSeconds);

	/** 撃ち漏らした。前方の足場を1つ壊す */
	void HandleMeteorImpact(class AStairMeteor* M);

	// ---------------- ゲージの表示 ----------------

	/** 押した位置の残像。新しいものが先頭 */
	TArray<FStairGhost> Ghosts;

	/** ゲージが去る進み具合。一度1になったら戻さない */
	float GaugeExitAlpha = 0.f;

	/** 残像を進め、ゲージの退場を判断する */
	void UpdateGaugeState(float DeltaSeconds);

	// ---------------- イントロ ----------------

	/** テンポどおりに押せた回数 */
	int32 IntroTaps = 0;

	/** 直前に成功した拍。連続して押せているかの判断に使う */
	int32 IntroLastBeat = -9999;

	/** イントロを進める。ガイド音を鳴らし、押し終わったら次へ */
	void UpdateIntro(float DeltaSeconds);

	/** 直前にガイド音を鳴らした拍 */
	int32 IntroLastGuideBeat = -9999;

	/** 前フレームの位相。拍を跨いだ瞬間を正確に捉えるのに使う */
	float IntroPrevPhase = 0.f;

	// ---------------- 譜面づくり ----------------

	/** 譜面編集モードか */
	bool bCharting = false;

	/** 打ち込んだ拍。書き出すとこれが譜面になる */
	TArray<int32> ChartBeats;

	/** 譜面編集の操作を受け付ける */
	void UpdateCharting();

	/** 直前に成功した判定の拍番号。連続かどうかの判断に使う */
	int32 LastComboBeat = -9999;

public:
	void EndGame(EStairEndReason Reason);
	void UpdateWidgets();

	/** t に応じて空・ライト・フォグを補間する。★背景レイヤーだけを暗くする */
	void UpdateSkyByT(float T);

	void CacheSceneActors();

	// ---------------- 設定 ----------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair")
	TObjectPtr<UStairConfig> Config;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|UI")
	TSubclassOf<UUserWidget> ResultWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Stair|UI")
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Stair|UI")
	TObjectPtr<UUserWidget> ResultWidget;

	// ---------------- 部品 ----------------

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	TObjectPtr<UStairMusicClock> MusicClock;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	TObjectPtr<UStairTerrain> Terrain;

	UPROPERTY()
	mutable TObjectPtr<AStairCharacter> Player;

	/** 初期配置が済んだか */
	bool bPlayerPlaced = false;

	// 背景レイヤー。t に応じて暗くする対象
	UPROPERTY()
	TObjectPtr<ADirectionalLight> SunLight;

	UPROPERTY()
	TObjectPtr<AExponentialHeightFog> Fog;

	UPROPERTY()
	TObjectPtr<ASkyLight> SkyLightActor;

	// ---------------- 内部状態 ----------------

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	EStairGameState State = EStairGameState::Countdown;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	FStairSong CurrentSong;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	int32 PerfectCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	int32 GreatCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	int32 MissCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	EStairEndReason EndReason = EStairEndReason::None;

	float StateTime = 0.f;

	/** Finished から Result までの待ち（演出のため） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair")
	float FinishedHold = 1.4f;
};
