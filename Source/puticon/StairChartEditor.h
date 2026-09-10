#pragma once

#include "CoreMinimal.h"
#include "StairWidgets.h"
#include "StairTypes.h"
#include "StairChartEditor.generated.h"

/**
 * 譜面エディタ。ピアノロール方式。
 *
 *   縦軸＝時間（上が早い、下が遅い）
 *   横軸＝跳ぶ方向（左 / 正面 / 右 / 赤マス）
 *
 * ★セルは1つずつボタンにせず、格子全体でクリック位置を拾って
 *   行と列に換算する。数百個のボタンを並べずに済む。
 *
 * ★見えている範囲だけ描く。曲全体だと1500マス以上になるため、
 *   表示するのは常に一定数で、スクロール位置だけを変える。
 *
 * 製品版では丸ごと外す前提の開発用ツール。
 */
UCLASS()
class PUTICON_API UStairChartEditWidget : public UStairWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geo, float DeltaSeconds) override;

	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& Geo, const FPointerEvent& Ev) override;
	virtual FReply NativeOnMouseWheel(
		const FGeometry& Geo, const FPointerEvent& Ev) override;
	virtual FReply NativeOnKeyDown(
		const FGeometry& Geo, const FKeyEvent& Ev) override;

	/** どの曲を編集するか */
	void SetSongIndex(int32 Index) { SongIndex = Index; }

	/** 閉じたときに操作を返す相手。タイトル画面を渡す */
	void SetReturnWidget(UUserWidget* W) { ReturnWidget = W; }

protected:
	virtual void BuildUI(UCanvasPanel* Canvas) override;

	UFUNCTION() void OnPlayClicked();
	UFUNCTION() void OnSaveClicked();
	UFUNCTION() void OnBackClicked();

	/** 表示している行のセルを塗り直す */
	void RefreshGrid();

	/** 曲を鳴らす／止める */
	void TogglePlay();

	/** その位置に音符があるか探す。無ければ INDEX_NONE */
	int32 FindNote(int32 InSlot, EStairNote Type) const;

	/** 位置と種類から音符を置く／消す */
	void ToggleNote(int32 InSlot, EStairNote Type);

	/** 譜面をテキストに書き出す。押したことが分かるよう画面にも出す */
	void ExportChart();

	/** 書き出したファイル、無ければ Config から読む */
	void LoadChart();

	/** いまの中身をファイルへ残す */
	bool SaveChart();

	/** 編集する曲を切り替える。打ちかけの譜面は覚えておく */
	void SwitchSong(int32 Delta);

	// ---- 部品 ----

	UPROPERTY() TArray<TObjectPtr<UBorder>> Cells;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> RowLabels;
	UPROPERTY() TObjectPtr<UBorder> PlayHead;

	UPROPERTY() TObjectPtr<UTextBlock> TitleText;
	UPROPERTY() TObjectPtr<UTextBlock> InfoText;
	UPROPERTY() TObjectPtr<UButton> PlayButton;
	UPROPERTY() TObjectPtr<UButton> SaveButton;
	UPROPERTY() TObjectPtr<UButton> BackButton;

	UPROPERTY() TObjectPtr<class UAudioComponent> Audio;

	// ---- 状態 ----

	int32 SongIndex = 0;

	/**
	 * 編集中の譜面。
	 * ★閉じるときに必ず Saved/Charts へ書く。Config には触らない。
	 *   製品版へ焼き込むのは Tools/import_chart.py の役目。
	 */
	TArray<FStairChartNote> Notes;

	/** 閉じたあと操作を返す相手 */
	TWeakObjectPtr<UUserWidget> ReturnWidget;

	/** 画面のいちばん上に出ている位置（1拍 ÷ 分割 の単位） */
	int32 TopSlot = 0;

	/** 再生位置（秒）。再生していないときも保持する */
	float PlayTime = 0.f;
	bool bPlaying = false;

	// ---- 見た目の設定 ----

	/** 縦に何行ぶん見せるか */
	static constexpr int32 VisibleRows = 24;

	/** 横の列。左・正面・右・赤 */
	static constexpr int32 NumCols = 4;

	float GridLeft = 420.f;
	float GridTop = 150.f;
	float CellW = 130.f;
	float CellH = 34.f;
};
