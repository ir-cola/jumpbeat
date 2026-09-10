#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "StairGameInstance.generated.h"

class UStairConfig;

/**
 * レベルをまたいで持ち越す情報。
 * BGM選択画面で選んだ曲を Game レベルへ渡すために使う。
 */
UCLASS()
class PUTICON_API UStairGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** 選ばれた曲の番号（Config->Songs の添字） */
	UPROPERTY(BlueprintReadWrite, Category = "Stair")
	int32 SelectedSongIndex = 0;

	/**
	 * ★タイトルを開いたとき、いきなり曲選択パネルを出すか。
	 *   リザルトの「曲をえらぶ」から戻ってきたときに使う。
	 *   一度読んだら false に戻すこと。
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Stair")
	bool bOpenSongSelectOnTitle = false;

	/** 直近のスコア。リザルトからタイトルへ戻っても保持する */
	UPROPERTY(BlueprintReadWrite, Category = "Stair")
	int32 LastScore = 0;

	/**
	 * ★エンドレスモードか。
	 *   曲を順番に流し続け、死ぬまで終わらない。
	 *   譜面は曲ごとのものを使うが、地形は完全ランダムにする。
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Stair")
	bool bEndlessMode = false;

	/**
	 * ★起動直後の1回だけ true→false になる。
	 *   最初にタイトルを出すときは白からのフェードイン、
	 *   ゲームから戻ってきたときはサークルワイプ、と出し分けるために使う。
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Stair")
	bool bBootDone = false;

	/** 曲リストなどの設定。BGM選択画面が参照する */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair")
	TObjectPtr<UStairConfig> Config;
};
