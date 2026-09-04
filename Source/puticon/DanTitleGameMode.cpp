#include "DanTitleGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

void ADanTitleGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	if (TitleWidgetClass)
	{
		TitleWidget = CreateWidget<UUserWidget>(PC, TitleWidgetClass);
		if (TitleWidget)
		{
			TitleWidget->AddToViewport();
		}
	}

	// タイトルはマウスでUIを操作する
	UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PC, TitleWidget, EMouseLockMode::DoNotLock);
	PC->bShowMouseCursor = true;
}
