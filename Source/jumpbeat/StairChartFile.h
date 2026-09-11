#pragma once

#include "CoreMinimal.h"
#include "StairTypes.h"

/**
 * 譜面をテキストファイルとして読み書きする。
 *
 * ★譜面エディタで打ったものは、閉じた時点で必ずここへ書く。
 *   ウィジェットを閉じると中身は消えるので、覚えておく場所が要る。
 *
 * ★ゲーム側も起動時にここを読む。
 *   打ってすぐ遊べるようにするため、Python での取り込みは
 *   「製品版の Config に焼き込むとき」だけでよくなる。
 *
 * 置き場所は Saved/Charts/Chart_<曲の番号>.txt。
 * 中身は人が読める形にしてある。Tools/import_chart.py が同じ形を読む。
 */
namespace StairChartFile
{
	/** その曲の譜面ファイルの場所 */
	JUMPBEAT_API FString PathFor(int32 SongIndex);

	/** 読み込む。ファイルが無ければ false を返し、OutNotes は触らない */
	JUMPBEAT_API bool Load(int32 SongIndex, TArray<FStairChartNote>& OutNotes);

	/** 書き出す。音符が0個でも「空の譜面」として書く */
	JUMPBEAT_API bool Save(int32 SongIndex, const TArray<FStairChartNote>& Notes,
		const FString& Title, float BPM, int32 Subdivision);
}
