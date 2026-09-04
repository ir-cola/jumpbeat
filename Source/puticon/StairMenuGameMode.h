#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StairMenuGameMode.generated.h"

class UUserWidget;

/** メニュー画面用の共通GameMode。指定のWidgetを出してマウス操作にするだけ */
UCLASS(Abstract)
class PUTICON_API AStairMenuGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair")
	TSubclassOf<UUserWidget> MenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	TObjectPtr<UUserWidget> MenuWidget;
};

/**
 * タイトル（L_Title）。
 * ★背景でステージを自動生成し、カメラをゆっくり登らせて流す。
 */
UCLASS()
class PUTICON_API AStairTitleGameMode : public AStairMenuGameModeBase
{
	GENERATED_BODY()

public:
	AStairTitleGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

protected:
	/** BGMが鳴り終わったら頭から鳴らし直す */
	UFUNCTION()
	void HandleBGMFinished();

	/** 背景のステージ生成 */
	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	TObjectPtr<class UStairTerrain> Terrain;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair")
	TObjectPtr<class UStairConfig> Config;

	/** カメラが1秒あたり何段ぶん進むか */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	float ScrollRowsPerSecond = 0.55f;

	/**
	 * ★どの段から見せ始めるか。
	 *   0 から始めると階段の始まり（何も無い縁）が映るので、少し進めておく。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	float StartScrollRow = 30.f;

	/** カメラの位置（段からの相対） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	FVector CameraOffset = FVector(-900.f, 260.f, 620.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	FRotator CameraRotation = FRotator(-22.f, 0.f, 0.f);

	UPROPERTY()
	TObjectPtr<class ACameraActor> ViewCamera;

	/** タイトルで流すループBGM */
	UPROPERTY()
	TObjectPtr<class UAudioComponent> BGMComp;

	// ---------------- 見せ玉のキャラクター ----------------

	/**
	 * 見せ玉に使うクラス。
	 * 未設定なら BP_StairCharacter を読み込む。
	 * C++ の AStairCharacter には見た目が無いので、そのままでは映らない。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	TSubclassOf<class AStairCharacter> DemoCharClass;

	/**
	 * 何体走らせるか。
	 * 0 以下なら「全レーンを1つ飛ばしで埋める数」を自動で使う。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	int32 DemoCount = 0;

	/** 1回の跳躍にかかる時間（秒）。体ごとに少しばらす */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	float DemoHopTime = 0.46f;

	/** ★着地してから次に跳ぶまで、この範囲でひと呼吸おく（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	float DemoWaitMin = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	float DemoWaitMax = 0.2f;

	/** ★横に避けられる回数。これを超えたら正面が穴でも落ちる */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	int32 DemoMaxDodges = 3;

	/**
	 * ★出現の間隔（秒）。
	 *   まとめて湧かせると起動直後に重なって見えるので、1体ずつ順に出す。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	float DemoSpawnInterval = 0.16f;

	/**
	 * ★出現させる段の範囲。カメラのいる段からの相対。
	 *   カメラは約3.9段後ろにいるので、-4 あたりが画面下端。
	 *   これより後ろに出すと画面に入るまで何十秒もかかる。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	int32 DemoSpawnRowMin = -4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	int32 DemoSpawnRowMax = 1;

	/** 跳ぶ高さ */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	float DemoHopHeight = 150.f;

	/**
	 * 出現させる横方向の範囲（レーン）。
	 * ★0 以下なら Config の LaneRadius をそのまま使い、全レーンに配る。
	 *   レーンは1つ飛ばしで使うので、隣り合って並ぶことはない。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	int32 DemoLaneSpread = 0;

	/** 横に避けるかどうかを決める確率。残りはそのまま落ちる */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DemoDodgeChance = 0.5f;

	/** 背景の穴の密度。ゲーム本編より少なくする */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景",
		meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float TitleHoleDensity = 0.07f;

	/**
	 * ★見せ玉に再生させるアニメーション。
	 *   CharacterMovement を止めて手で動かしているため、
	 *   AnimBP からは「止まって接地している」ようにしか見えず待機モーションになる。
	 *   そこで AnimBP を使わず、この2つを直接再生する。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	TObjectPtr<class UAnimSequence> DemoJumpAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	TObjectPtr<class UAnimSequence> DemoFallAnim;

	/** ひと呼吸おいているあいだの待機モーション */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|背景")
	TObjectPtr<class UAnimSequence> DemoIdleAnim;

	void UpdateDemoRunners(float DeltaSeconds);

	/** 段の中心からキャラ中心までの高さ */
	float GetStandZOffset() const;

	/** いまカメラがいる段（小数） */
	float ScrollRow = 0.f;

	/** 生成した本体。GC に回収されないよう UPROPERTY で持つ */
	UPROPERTY()
	TArray<TObjectPtr<class AStairCharacter>> DemoActors;

private:
	/** 1体ぶんの状態。跳んでいるか落ちているかを持つ */
	struct FDemoRunner
	{
		int32 FromRow = 0, FromLane = 0;
		int32 ToRow = 0, ToLane = 0;

		float HopT = 0.f;        // 0〜1 の進み具合
		float HopTime = 0.5f;
		float Yaw = 0.f;

		/** 着地後の休み。0 になったら次を跳ぶ */
		float WaitLeft = 0.f;

		/** 何回横に避けたか。上限を超えたらもう避けない */
		int32 Dodges = 0;

		/** 出現までの待ち。0 より大きいあいだは隠しておく */
		float SpawnDelay = 0.f;
		bool bActive = false;

		bool bFalling = false;
		FVector FallPos = FVector::ZeroVector;
		FVector FallVel = FVector::ZeroVector;
	};

	/** DemoActors と同じ添字で対応する */
	TArray<FDemoRunner> Runners;

	/**
	 * ★次に使うレーン。端から順に一巡させる。
	 *   ランダムに選ぶと偏るので、全レーンに1体ずつ行き渡らせる。
	 */
	int32 NextSpawnLane = 0;

	/** 画面外の手前から出し直す */
	void RespawnRunner(int32 Index);

	/** そのマスに既に他の個体がいるか。重なりを防ぐ */
	bool IsCellTaken(int32 Row, int32 Lane, int32 SkipIndex) const;

	/** しばらく隠してから出し直す */
	void ScheduleRespawn(int32 Index, float Delay);

	/** 次の跳び先を決める。無ければ落下に切り替える */
	void PickNextHop(int32 Index);

	/** 待機／ジャンプ／落下のアニメーションを再生する */
	void PlayDemoAnim(int32 Index, int32 Kind);   // 0=待機 1=跳躍 2=落下
};

/** BGM選択（L_BGMSelect） */
UCLASS()
class PUTICON_API AStairBGMSelectGameMode : public AStairMenuGameModeBase
{
	GENERATED_BODY()

public:
	AStairBGMSelectGameMode();
};
