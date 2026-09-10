#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "StairTypes.h"
#include "StairPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * 入力は A / D / SPACE の3つだけ。
 */
UCLASS()
class PUTICON_API AStairPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** ★Playing 以外では false。Result中の入力を完全に遮断する */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsInputAllowed() const;

protected:
	void OnLeftPressed(const FInputActionValue& Value);
	void OnRightPressed(const FInputActionValue& Value);
	void OnJump(const FInputActionValue& Value);

	/** 3つのキーから共通で呼ぶ。状態に応じて跳ぶ／数える */
	void DoJump(EStairDir InDir);

	/** 譜面編集中に音符を置く（左クリック） */
	UFUNCTION()
	void OnFireKey();

	/** Esc。ポーズの出入り */
	UFUNCTION()
	void OnPauseKey();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|Input")
	TObjectPtr<UInputMappingContext> StairMappingContext;

	/** A キー */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|Input")
	TObjectPtr<UInputAction> LeftAction;

	/** D キー */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|Input")
	TObjectPtr<UInputAction> RightAction;

	/** SPACE キー */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|Input")
	TObjectPtr<UInputAction> JumpAction;
};
