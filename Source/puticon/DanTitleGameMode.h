#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DanTitleGameMode.generated.h"

class UUserWidget;

/**
 * タイトル画面（L_Title）用の GameMode。
 * WBP_Title を出してマウス操作を有効にするだけ。
 */
UCLASS()
class PUTICON_API ADanTitleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	/** BP_TitleGameMode の詳細で WBP_Title を指定する */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dan")
	TSubclassOf<UUserWidget> TitleWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	TObjectPtr<UUserWidget> TitleWidget;
};
