#include "StairMusicClock.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "Kismet/GameplayStatics.h"

UStairMusicClock::UStairMusicClock()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UStairMusicClock::StartSong(USoundBase* Sound, float InBPM, float InBeatOffset,
	float StartDelay)
{
	StopSong();

	BPM = (InBPM > 0.f) ? InBPM : 140.f;
	BeatOffset = InBeatOffset;
	PendingDelay = FMath::Max(0.f, StartDelay);
	SongTime = -PendingDelay;
	AuthoritativeTime = -1.f;
	bAudioStarted = false;
	bPlaying = true;

	SongLength = 0.f;
	if (Sound)
	{
		SongLength = Sound->GetDuration();
		// ループ音源など無限長のものは安全な値に丸める
		if (!FMath::IsFinite(SongLength) || SongLength > 3600.f)
		{
			SongLength = 120.f;
		}
	}
	if (SongLength <= 0.f)
	{
		// 音源が無い場合でも遊べるように既定の長さを入れる
		SongLength = 90.f;
	}

	if (Sound)
	{
		// ★SpawnSound2D は生成と同時に鳴り始めてしまう。
		//   直後に Stop() しても曲の頭が一瞬鳴り、再生開始のタイミングが
		//   不安定になる（リトライでズレる原因）。
		//   CreateSound2D は鳴らさずに作るだけなので、こちらを使う。
		AudioComp = UGameplayStatics::CreateSound2D(this, Sound, 1.f, 1.f, 0.f,
			nullptr, false, false);
		if (AudioComp)
		{
			AudioComp->OnAudioPlaybackPercent.AddDynamic(
				this, &UStairMusicClock::HandlePlaybackPercent);
		}
	}
}

void UStairMusicClock::SetPaused(bool bPause)
{
	if (AudioComp)
	{
		AudioComp->SetPaused(bPause);
	}
}

void UStairMusicClock::StopSong()
{
	if (AudioComp)
	{
		AudioComp->OnAudioPlaybackPercent.RemoveDynamic(
			this, &UStairMusicClock::HandlePlaybackPercent);
		AudioComp->Stop();
		AudioComp->DestroyComponent();
		AudioComp = nullptr;
	}
	bPlaying = false;
	bAudioStarted = false;
}

void UStairMusicClock::HandlePlaybackPercent(const USoundWave* PlayingSoundWave,
	const float PlaybackPercent)
{
	if (!PlayingSoundWave)
	{
		return;
	}

	const float Dur = PlayingSoundWave->Duration;
	if (Dur > 0.f && FMath::IsFinite(Dur) && Dur < 3600.f)
	{
		SongLength = Dur;
	}

	// これが「真の再生位置」。次のTickでここへ寄せる
	AuthoritativeTime = PlaybackPercent * SongLength;
}

void UStairMusicClock::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bPlaying)
	{
		return;
	}

	// 時間拡張の影響を受けない実時間で進める
	const float RealDelta = FApp::GetDeltaTime();

	if (!bAudioStarted)
	{
		// ★カウントダウン中は1フレームの進みを抑える。
		//   レベル読み込み直後は1フレームで0.5秒進むことがあり、
		//   そのぶん「3」の再生が遅れて 3→2 の間隔だけ詰まってしまう。
		//   まだ音は鳴っていないので、少し伸びても害はない。
		SongTime += FMath::Min(RealDelta, 0.05f);
		if (SongTime >= 0.f)
		{
			if (AudioComp)
			{
				AudioComp->Play(FMath::Max(0.f, SongTime));
			}
			// ★音源が無くても必ず開始状態にする。
			//   ここで止めると時計が進まず、ゲームが始まらない
			bAudioStarted = true;
		}
		return;
	}

	SongTime += RealDelta;

	// コールバックが来ていたら真の位置へ滑らかに寄せる
	if (AuthoritativeTime >= 0.f)
	{
		const float Diff = AuthoritativeTime - SongTime;

		// 大きくズレていたら即座に合わせる（シーク・停止からの復帰など）
		if (FMath::Abs(Diff) > 0.25f)
		{
			SongTime = AuthoritativeTime;
		}
		else
		{
			// 小さなズレは少しずつ。カクつきを防ぐ
			SongTime += Diff * FMath::Min(1.f, RealDelta * 6.f);
		}
		AuthoritativeTime = -1.f;
	}
}

float UStairMusicClock::GetT() const
{
	if (SongLength <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(SongTime / SongLength, 0.f, 1.f);
}

bool UStairMusicClock::IsSongFinished() const
{
	return bAudioStarted && SongLength > 0.f && SongTime >= SongLength;
}

float UStairMusicClock::GetBeatPosition() const
{
	const float Beat = GetBeatDuration();
	if (Beat <= 0.f)
	{
		return 0.f;
	}
	// GlobalOffset は環境ごとの音の遅れを吸収するための補正
	return (SongTime - GlobalOffset - BeatOffset) / Beat;
}

float UStairMusicClock::GetOffsetFromNearestBeat() const
{
	const float Beat = GetBeatDuration();
	if (Beat <= 0.f)
	{
		return 0.f;
	}

	const float Pos = GetBeatPosition();
	const float Nearest = FMath::RoundToFloat(Pos);
	return (Pos - Nearest) * Beat;
}

float UStairMusicClock::GetBeatPhase() const
{
	const float Pos = GetBeatPosition();
	return Pos - FMath::FloorToFloat(Pos);
}
