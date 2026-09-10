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
	bool IsInputAllowed() const
	{
		// ★曲と曲のあいだは押しても意味がないので受け付けない。
		//   受けてしまうと、待っているだけで MISS が積み上がる。
		return State == EStairGameState::Playing && !IsBetweenSongs();
	}

	// ---------------- エンドレス ----------------

	/** ★曲を順番に流し続け、死ぬまで終わらないモードか */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsEndless() const { return bEndless; }

	/** エンドレスで、次の曲が鳴り出すのを待っているあいだか */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsBetweenSongs() const;

	/** これまでに流れ終わった曲の数 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetSongsPlayed() const { return SongsPlayed; }

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

	/**
	 * ★指定した再生位置で押したものとして判定する。
	 *   跳んでいる最中に押された入力は着地まで持ち越すので、
	 *   「押した瞬間の時刻」で判定しないと不当に遅れた扱いになる。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	EStairJudge JudgeAt(float AtSongTime) const;

	/**
	 * ★いまの瞬間のズレ（秒）。符号つき。
	 *   マイナス＝音符より早い（FAST）、プラス＝遅い（SLOW）。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetSignedJudgeOffset() const;

	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetSignedJudgeOffsetAt(float AtSongTime) const;

	/** いまの再生位置（秒）。入力を押した時刻を覚えておくのに使う */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetSongTimeNow() const;

	/**
	 * ★押した向きが、いま待っている音符と合っているか。
	 *   合っていなければ、タイミングが良くても MISS にする。
	 *   譜面どおりに叩くゲームなので、向きが違えば叩けていない。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool DoesDirectionMatch(EStairDir Dir) const;

	/** 直前に跳んだときのズレ（秒）。FAST / SLOW の表示に使う */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetLastJudgeOffset() const { return LastJudgeOffset; }

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

	/** 判定が出たときに集計する。SignedOffset は押した瞬間のズレ（秒） */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void NotifyJudge(EStairJudge Judge, float SignedOffset);

	/** その場ジャンプ＝いま乗っている足場にMISSを記録する */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void NotifyMissOnCurrentStep();

	/**
	 * ★拍を外して足踏みしたとき。
	 *
	 *   譜面があるときは、ここでは数えない。
	 *   外した音符は「叩かれずに通り過ぎた」ほうでも数えるので、
	 *   両方で数えると1回のミスが2回に見えてしまう。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void NotifyMissJump();

	// ---------------- 遷移 ----------------

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void GoToTitle();

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void GoToSongSelect();

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void RetryGame();

	/**
	 * ★Esc でポーズを切り替える。
	 *   ゲームを止めると Tick が来なくなり時計も進まないので、
	 *   曲の位置は保たれる。音だけは別に止める必要がある。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void TogglePause();

	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsPaused() const { return State == EStairGameState::Paused; }

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

	/** ポーズに入る前の状態。再開するときに戻す */
	EStairGameState StateBeforePause = EStairGameState::Playing;

	UPROPERTY()
	TObjectPtr<UUserWidget> PauseWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|UI")
	TSubclassOf<UUserWidget> PauseWidgetClass;

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

	// ---------------- ゲージの表示 ----------------

	/**
	 * ★押した位置の記録。自分のクセ（走り気味・遅れ気味）が見えるようにする。
	 *   Phase は押した瞬間の拍の位相、Age は経過時間。
	 */
	struct FStairGhost
	{
		/** 音符からのズレ（秒）。マイナス＝早い */
		float Offset = 0.f;
		float Age = 0.f;
		EStairJudge Judge = EStairJudge::Miss;
	};

	const TArray<FStairGhost>& GetGhosts() const { return Ghosts; }

	/**
	 * ★カウントダウン中の空押し。
	 *   進みはしないが、押した位置は残像に残る。
	 *   START までのあいだ拍を確かめられるようにするため。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void NotifyPracticeTap();

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

	/** 編集中に音符を置く。キーごとに種類が変わる */
	UFUNCTION(BlueprintCallable, Category = "Stair|Chart")
	void ChartPlaceTyped(EStairNote Type);

	// ---------------- 譜面にそった進行 ----------------

	/**
	 * ★次に来る音符までの時間（秒）。マイナスなら通り過ぎている。
	 *   A案では「一定間隔の拍」ではなく、この音符に対して判定する。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetTimeToNextNote() const;

	/** 次の音符の種類 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	EStairNote GetNextNoteType() const;

	/**
	 * ★いま跳んだとして、次に跳べるようになるまでの残り時間（秒）。
	 *
	 *   着地するまで次のジャンプは受け付けないので、
	 *   この時間より滞空時間が長いと、次の音符が押せなくなる。
	 *   譜面が無いときは BIG_NUMBER（＝制限なし）。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetNextNoteWindow() const;

	/** 譜面があるか。無ければ従来どおり全拍で跳べる */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool HasChart() const { return CurrentSong.Notes.Num() > 0; }

	/**
	 * ★譜面どおりの道を地形に敷いているか。
	 *   エンドレスは地形を完全ランダムにするので false。
	 *   「譜面に合わせて地形を直す」処理はすべてこれで分岐する。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool HasChartRoad() const { return HasChart() && !bEndless; }

	/**
	 * ★音符1つで進む段数。
	 *   通常プレイは1段、エンドレスは2段。
	 *   道を作る側・跳ぶ側・MISSで詰める側が
	 *   同じ値を見るように、ここ1箇所で決める。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetStepsPerNote() const;

	/**
	 * ★カウントダウンの最後、START と同時に押されたときに遊びを始める。
	 *   間に合っていれば true。そのまま最初の1歩として跳ばせる。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	bool TryStartFromCountdown();

	// ---- ゲージに音符を並べるための情報 ----

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetNoteCount() const { return CurrentSong.Notes.Num(); }

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetNextNoteIndex() const { return NextNoteIndex; }

	/** その音符が「いまから何秒後」か。マイナスなら通り過ぎている */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetNoteTimeFromNow(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Stair")
	EStairNote GetNoteType(int32 Index) const;

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

	/** 拍を外した回数。MissCount と同じ意味だが、内訳表示に使う */
	int32 JumpMissCount = 0;

	// ---------------- ゲージの表示 ----------------

	/** 押した位置の残像。新しいものが先頭 */
	TArray<FStairGhost> Ghosts;

	/** 残像を古くしていく */
	void UpdateGaugeState(float DeltaSeconds);

	/** 直前に跳んだときのズレ（秒） */
	float LastJudgeOffset = 0.f;

	// ---------------- エンドレス ----------------

	bool bEndless = false;
	int32 EndlessSongIndex = 0;
	int32 SongsPlayed = 0;

	/** Config の曲を読み込んで、譜面ファイルがあれば差し替える */
	void LoadSong(int32 Index);

	/** いまの曲を鳴らし始める。Lead 秒だけ間をおく */
	void StartCurrentSong(float Lead);

	/** エンドレスで次の曲へ移る */
	void AdvanceEndlessSong();

	/**
	 * ★進み損ねた段数を地形から取り除いて、この先の譜面と足場を合わせ直す。
	 *
	 *   呼ぶのは「音符が1つ消えたのに、プレイヤーが進まなかった」ときだけ。
	 *   拍と関係ない空押しで呼んではいけない。譜面は進んでいないので、
	 *   詰めるとかえってずれる。
	 */
	void CollapseRows(int32 Count, bool bClearRedUnderPlayer);

	/** いま乗っている足場を傷める。3回で崩れる */
	void DamageCurrentStep();

	/**
	 * ★音符を叩かずに通り過ぎたとき。
	 *   コンボを切り、足場を傷め、進み損ねたぶんの行を詰める。
	 */
	void OnNoteMissed(int32 Index);

	/** 行を詰めた回数。段数を数え直すのに使う */
	int32 RowShiftTotal = 0;

	/**
	 * ★レベルが切り替わった直後、UIの幕が出るまでの残り時間。
	 *
	 *   幕は UMG なので、レベルの1枚目には間に合わないことがある。
	 *   そのあいだカメラ側でも暗くしておき、
	 *   切り替え直後に素の画面が一瞬映るのを防ぐ。
	 */
	float ScreenFadeHold = 0.f;

	void StartScreenFade(const FLinearColor& Color);
	void TickScreenFade(float DeltaSeconds);

	// ---------------- 譜面づくり ----------------

	/** 譜面編集モードか */
	bool bCharting = false;

	/** 打ち込んだ音符。書き出すとこれが譜面になる */
	TArray<FStairChartNote> ChartNotes;

	/** 譜面編集の操作を受け付ける */
	void UpdateCharting();

	/** いま何番目の音符を待っているか */
	int32 NextNoteIndex = 0;

	/** 音符1つの時刻（秒）を求める */
	float NoteTime(int32 Index) const;

	/**
	 * ★譜面のとおりに跳いだときに通る道を先に計算し、
	 *   その位置に必ず足場があるよう地形へ伝える。
	 *   地形自体はランダムのまま。譜面の道だけを保証する。
	 */
	void BuildChartPath();

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

	/** いまの状態に入ってからの秒数。リザルトへの幕引きに使う */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetStateTime() const { return StateTime; }

	/** Finished から Result までの待ち時間 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetFinishedHold() const { return FinishedHold; }

	/** Finished から Result までの待ち（演出のため） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair")
	float FinishedHold = 1.4f;
};
