#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StairMusicClock.generated.h"

class UAudioComponent;
class USoundBase;
class USoundWave;

/**
 * 曲の再生位置を基準にした時計。
 *
 * ★リズム判定にフレーム時間を使ってはいけない。
 *   フレーム落ちすると判定が音とズレていくため。
 *
 * 実装:
 *   ・オーディオの再生率コールバック(OnAudioPlaybackPercent)で真の位置を受け取る
 *   ・コールバックは毎フレーム来るとは限らないので、その間はプラットフォーム時計で
 *     補間する（時間拡張の影響を受けない実時間を使う）
 *   ・コールバックが来たら、急に飛ばないよう滑らかに寄せる
 */
UCLASS(ClassGroup = (Stair), meta = (BlueprintSpawnableComponent))
class JUMPBEAT_API UStairMusicClock : public UActorComponent
{
	GENERATED_BODY()

public:
	UStairMusicClock();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** 曲を鳴らし始める。StartDelay 秒後に鳴る（カウントダウンぶん） */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void StartSong(USoundBase* Sound, float InBPM, float InBeatOffset, float StartDelay);

	/**
	 * ★曲を一時停止する。
	 *   ゲームを止めると Tick が来なくなり時計は勝手に止まるが、
	 *   音だけは鳴り続けてしまうので明示的に止める。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void SetPaused(bool bPause);

	/** 環境ごとの音の遅れ補正（秒）を設定する */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void SetGlobalOffset(float Seconds) { GlobalOffset = Seconds; }

	/** いまの補正値をミリ秒で返す */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetGlobalOffsetMs() const { return GlobalOffset * 1000.f; }

	/** 補正値をミリ秒ぶん動かす */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void NudgeGlobalOffsetMs(float DeltaMs)
	{
		GlobalOffset = FMath::Clamp(GlobalOffset + DeltaMs * 0.001f, -0.3f, 0.3f);
	}

	UFUNCTION(BlueprintCallable, Category = "Stair")
	void StopSong();

	/** いまの再生位置（秒）。カウントダウン中は負の値になる */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetSongTime() const { return SongTime; }

	/** 曲の長さ（秒） */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetSongLength() const { return SongLength; }

	/** ★全システムが参照する進行度 t（0→1）。1箇所でしか計算しない */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetT() const;

	/** 曲が鳴り終わったか */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsSongFinished() const;

	/** 実際に音が鳴り始めたか（カウントダウンが終わったか） */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool HasStarted() const { return SongTime >= 0.f; }

	/**
	 * 判定1回ぶんの長さ（秒）。
	 * ★BeatScale で musical な1拍の何個ぶんかを決める。
	 *   2 にするとゲージが半分の速さになり、着地してすぐ次を狙える。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetBeatDuration() const
	{
		const float Raw = (BPM > 0.f) ? (60.f / BPM) : 0.5f;
		return Raw * FMath::Max(1.f, BeatScale);
	}

	/** 判定1回に使う拍数を設定する */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void SetBeatScale(float Scale) { BeatScale = FMath::Max(1.f, Scale); }

	/** いま何拍目か（小数） */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetBeatPosition() const;

	/**
	 * 直近の拍からのズレ（秒）。負なら拍より早い、正なら遅い。
	 * 絶対値が判定に使われる。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetOffsetFromNearestBeat() const;

	/** 拍と拍のあいだのどこにいるか 0〜1。ゲージ描画用 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetBeatPhase() const;

	/** カウントダウンの残り秒。0以下なら開始済み */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetCountdownRemaining() const { return FMath::Max(0.f, -SongTime); }

protected:
	UFUNCTION()
	void HandlePlaybackPercent(const USoundWave* PlayingSoundWave, const float PlaybackPercent);

	UPROPERTY()
	TObjectPtr<UAudioComponent> AudioComp;

	/** 再生位置（秒）。負の値＝カウントダウン中 */
	float SongTime = 0.f;

	float SongLength = 0.f;
	float BPM = 140.f;
	float BeatOffset = 0.f;

	/** 環境ごとの音の遅れ補正（秒）。Config の AudioOffsetMs から入る */
	float GlobalOffset = 0.f;

	/** 判定1回に使う拍数。Config の BeatsPerJudge から入る */
	float BeatScale = 1.f;

	bool bPlaying = false;
	bool bAudioStarted = false;

	/** 音の再生開始までの残り秒 */
	float PendingDelay = 0.f;

	/** コールバックで受け取った真の位置。次のTickで寄せる */
	float AuthoritativeTime = -1.f;
};
