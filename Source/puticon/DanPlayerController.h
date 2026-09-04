#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DanPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * 入力の受け口。設計書 6-2。
 *
 * ★Input Action は2つだけ。
 *   チャージフェーズと防衛フェーズは絶対に同時に来ないので、
 *   ボタンを分ける必要がない。状態による振り分けは GameMode 側で行う。
 */
UCLASS()
class PUTICON_API ADanPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADanPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

protected:
	void OnActionPressed(const FInputActionValue& Value);
	void OnActionReleased(const FInputActionValue& Value);
	void OnPointerMoved(const FInputActionValue& Value);

	/** BP_DanPlayerController の詳細で IMC_Dan を指定する */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dan|Input")
	TObjectPtr<UInputMappingContext> DanMappingContext;

	/** 押す/離す。左マウスボタン と スペースキー を割り当てる */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dan|Input")
	TObjectPtr<UInputAction> ActionInput;

	/** マウス移動量。回転ミニゲーム用（Tier 3。今は未使用でよい） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dan|Input")
	TObjectPtr<UInputAction> PointerInput;
};
