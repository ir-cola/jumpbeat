#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "StairTypes.h"
#include "StairCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class AStairStep;
class AStairGameMode;

/**
 * 階段を登るプレイヤー。
 *
 * 移動は「正面・斜め左・斜め右」の3方向のみ。
 * ジャンプは任意タイミングで、見送りもできる。
 * 段数は物理の初速倍率ではなく「N段上へ確定移動」として実装する。
 */
UCLASS()
class PUTICON_API AStairCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AStairCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;

	// ---------------- 入力 ----------------

	/**
	 * ★A / D は押しっぱなしで方向が決まる。
	 *   押した瞬間だけでなく、ジャンプする時点で押されていれば効く。
	 *   事前に長押ししておく遊び方に対応するため。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void SetHeldLeft(bool bHeld);

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void SetHeldRight(bool bHeld);

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void TryJump();

	// ---------------- 状態 ----------------

	UFUNCTION(BlueprintPure, Category = "Stair")
	EStairDir GetDir() const { return Dir; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetCurrentRow() const { return CurrentRow; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetCurrentLane() const { return CurrentLane; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsAirborne() const { return bAirborne; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	EStairJudge GetLastJudge() const { return LastJudge; }

	/** 直前の判定からの経過秒。UIの表示消しに使う */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetTimeSinceJudge() const { return TimeSinceJudge; }

	UFUNCTION(BlueprintPure, Category = "Stair")
	int32 GetLastSteps() const { return LastSteps; }

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void SetControlEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsControlEnabled() const { return bControlEnabled; }

	/** 指定の段の上に置き直す */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void PlaceOnStep(int32 Row, int32 Lane, const FVector& StepLocation);

	/** カメラの引き具合。5段ジャンプ中は一時的に引く */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void SetCameraWide(bool bWide);

	// ---- 演出フック ----

	UFUNCTION(BlueprintImplementableEvent, Category = "Stair")
	void OnStairJumped(EStairJudge Judge, int32 StepsUp, bool bFromRed);

	UFUNCTION(BlueprintImplementableEvent, Category = "Stair")
	void OnLandedOnStep(int32 NewRow);

	UFUNCTION(BlueprintImplementableEvent, Category = "Stair")
	void OnDirChanged(EStairDir NewDir);

protected:
	AStairGameMode* GetStairGameMode() const;

	/**
	 * ★目標地点へ確実に飛ばす。
	 *
	 *   物理の速度で跳ばすと、壁に当たった瞬間に軌道が崩れて
	 *   足場の中央に立てなくなる。段の途中で止まると次へ進めない。
	 *   そこで速度は使わず、位置を毎フレーム直接置く。
	 *
	 *   ・滞空時間は段数によらず一定
	 *   ・着地点は必ず狙ったマスの中央
	 *   ・跳んでいるあいだは壁をすり抜ける（跳び越えられる）
	 *
	 * @param bFallAfter 着地点に足場が無い場合。着いたあと落下させる
	 */
	void StartScriptedJump(const FVector& Target, float FlightTime,
		float ApexClearance, bool bFallAfter);

	/** 跳んでいる最中の位置を進める */
	void TickScriptedJump(float DeltaSeconds);

	/** 跳び終わった。着地の処理を行う */
	void FinishScriptedJump();

	/** ジャンプ音を鳴らす。判定でピッチを変える */
	void PlayJumpSound(EStairJudge Judge);

public:
	/**
	 * 段の中心座標から、キャラの中心をどれだけ上に置くべきか。
	 * ★段の厚み(Cube 100 × ScaleZ)の半分 ＋ カプセルの半分。
	 *   ここを間違えるとカプセルが段にめり込んで引っかかる。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetStandZOffset() const;

	/** 指定の段に立つときのワールド座標 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	FVector GetStandLocation(int32 Row, int32 Lane) const;

protected:

	// ---------------- カメラ ----------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stair|Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stair|Camera")
	TObjectPtr<UCameraComponent> Camera;

	/** カメラ距離の追従速度 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|Camera")
	float CameraZoomSpeed = 4.5f;

	float TargetArmLength = 720.f;

	// ---------------- 内部状態 ----------------

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	EStairDir Dir = EStairDir::Forward;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	int32 CurrentRow = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	int32 CurrentLane = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	bool bAirborne = false;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	bool bControlEnabled = false;

	/** A / D の押しっぱなし状態 */
	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	bool bHeldLeft = false;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	bool bHeldRight = false;

	// ---- 位置を直接動かすジャンプ ----

	/** いま跳んでいる最中か */
	bool bScriptedJump = false;

	FVector JumpFrom = FVector::ZeroVector;
	FVector JumpTo = FVector::ZeroVector;

	float JumpElapsed = 0.f;
	float JumpDuration = 0.42f;
	float JumpArcHeight = 140.f;

	/** 着地点に足場が無い。着いたら落とす */
	bool bFallOnLand = false;

	/**
	 * ★跳んでいるあいだのアニメーション。
	 *   位置を手で動かすと AnimBP からは「止まって接地している」ようにしか
	 *   見えず、待機モーションになってしまう。跳躍中だけ直接再生し、
	 *   着地したら AnimBP に戻す。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|アニメ")
	TObjectPtr<class UAnimSequence> JumpAnim;

	/** 元の AnimBP。着地時にここへ戻す */
	UPROPERTY()
	TSubclassOf<class UAnimInstance> DefaultAnimClass;

	/** 跳躍アニメを再生する／AnimBP に戻す */
	void PlayJumpAnim();
	void RestoreAnimBlueprint();

	/** 押されているキーから進行方向を決める */
	void UpdateDirFromHeld();

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	EStairJudge LastJudge = EStairJudge::Miss;

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	int32 LastSteps = 0;

	float TimeSinceJudge = 99.f;

	/** ジャンプ開始時のZ。落下判定に使う */
	float LaunchBaseZ = 0.f;

	/** 目標の座標。着地判定の裏取りに使う */
	int32 TargetRow = 0;
	int32 TargetLane = 0;
};
