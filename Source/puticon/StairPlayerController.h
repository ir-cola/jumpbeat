#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
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
	void OnLeftReleased(const FInputActionValue& Value);
	void OnRightPressed(const FInputActionValue& Value);
	void OnRightReleased(const FInputActionValue& Value);
	void OnJump(const FInputActionValue& Value);

	/**
	 * ★撃つ。Enter と左クリックの両方から呼ばれる。
	 *   入力アセットを増やさずに済むよう、キーを直接束ねている。
	 */
	UFUNCTION()
	void OnFireKey();

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
