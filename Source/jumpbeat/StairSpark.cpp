#include "StairSpark.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AStairSpark::AStairSpark()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// ★粒は球。立方体だと角が見えて花火に見えない
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(
		TEXT("/Engine/BasicShapes/Cube.Cube"));

	UStaticMesh* Shape = SphereAsset.Succeeded()
		? SphereAsset.Object
		: (CubeAsset.Succeeded() ? CubeAsset.Object : nullptr);

	// ★光る粒には専用のマテリアルを使う。
	//   BasicShapeMaterial はライトの影響を受けるので、
	//   暗い場面では沈んでしまい「光っている」ように見えない。
	//   M_Spark は Unlit + Additive で、Bloom が効いて滲む。
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatAsset(
		TEXT("/Game/Stair/UI/M_Spark.M_Spark"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackMat(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UMaterialInterface* SparkMat = MatAsset.Succeeded()
		? MatAsset.Object
		: (FallbackMat.Succeeded() ? FallbackMat.Object : nullptr);

	// 粒はコンストラクタで作っておく。実行中の生成コストを避ける
	for (int32 i = 0; i < MaxParticles; ++i)
	{
		UStaticMeshComponent* M = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Spark%d"), i));
		M->SetupAttachment(Root);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetCastShadow(false);
		if (Shape)    { M->SetStaticMesh(Shape); }
		if (SparkMat) { M->SetMaterial(0, SparkMat); }
		M->SetVisibility(false);
		Meshes.Add(M);
	}
}

void AStairSpark::BeginPlay()
{
	Super::BeginPlay();

	// ★寿命はここでは決めない。
	//   Life は Burst のときに決まるので、BeginPlay の時点では既定値のまま。
	//   ここで SetLifeSpan すると、演出が終わる前に消えてしまう。
}

void AStairSpark::BurstRainbow(float Power)
{
	// 粒ごとに色相を変える。派手さは色数で出す
	bRainbow = true;
	NumParticles = Meshes.Num();      // 全部使う
	ParticleSize = 0.5f;
	Life = 1.5f;
	Burst(FLinearColor::White, Power);
}

void AStairSpark::Burst(const FLinearColor& Color, float Power)
{
	SparkColor = Color;
	Age = 0.f;
	bBurst = true;

	// ★Life が確定したこの時点で寿命を決める。
	//   Tick 側でも消すが、取りこぼしたときの保険として少し長めに取る。
	SetLifeSpan(Life + 0.5f);

	const int32 N = FMath::Clamp(NumParticles, 1, Meshes.Num());

	Velocities.Reset();
	Starts.Reset();
	Colors.Reset();
	Mats.Reset();
	Sizes.Reset();
	Phases.Reset();

	for (int32 i = 0; i < Meshes.Num(); ++i)
	{
		UStaticMeshComponent* M = Meshes[i];
		if (!M) { continue; }

		if (i >= N)
		{
			M->SetVisibility(false);
			continue;
		}

		M->SetVisibility(true);
		M->SetRelativeLocation(FVector::ZeroVector);

		// ★大きさを粒ごとにばらす。全部同じだと造花のように見える
		const float MySize = ParticleSize * FMath::FRandRange(0.55f, 1.35f);
		Sizes.Add(MySize);
		Phases.Add(FMath::FRandRange(0.f, 2.f * PI));
		M->SetRelativeScale3D(FVector(MySize));

		// ★虹色モードなら粒ごとに色相をずらす
		FLinearColor MyColor = SparkColor;
		if (bRainbow)
		{
			// ぐるっと一周させず、隣り合う粒の色が近くなるよう2周ぶんにする
			const float Hue = FMath::Fmod(720.f * float(i) / float(FMath::Max(1, N)), 360.f);
			MyColor = FLinearColor::MakeFromHSV8(
				uint8(Hue / 360.f * 255.f), 210, 255);
		}
		Colors.Add(MyColor);

		if (UMaterialInstanceDynamic* D = M->CreateAndSetMaterialInstanceDynamic(0))
		{
			D->SetVectorParameterValue(TEXT("Color"), MyColor);
			D->SetScalarParameterValue(TEXT("Intensity"), Glow);
			D->SetScalarParameterValue(TEXT("Alpha"), 1.f);
			// 光らないマテリアルに落ちたときのため。無い名前は無視される
			D->SetVectorParameterValue(TEXT("BaseColor"), MyColor);
			Mats.Add(D);
		}
		else
		{
			Mats.Add(nullptr);
		}

		// ★球状にばらまく。半球だと打ち上げ花火というより噴水に見える。
		//   少しだけ上に寄せて、開いてから落ちる形にする。
		const float Ang = FMath::FRandRange(0.f, 2.f * PI);
		const float Z = FMath::FRandRange(-0.35f, 1.f);
		const float R = FMath::Sqrt(FMath::Max(0.f, 1.f - Z * Z));
		FVector Dir(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, Z);
		Dir.Normalize();

		// 速さもばらす。揃っていると輪が広がるだけに見える
		const float Speed = FMath::FRandRange(320.f, 900.f) * Power;
		Velocities.Add(Dir * Speed);
		Starts.Add(FVector::ZeroVector);
	}
}

void AStairSpark::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bBurst)
	{
		return;
	}

	Age += DeltaSeconds;
	const float T = FMath::Clamp(Age / FMath::Max(0.01f, Life), 0.f, 1.f);

	for (int32 i = 0; i < Velocities.Num() && i < Meshes.Num(); ++i)
	{
		UStaticMeshComponent* M = Meshes[i];
		if (!M) { continue; }

		// ★空気抵抗をかける。等速で飛び続けると花火に見えない。
		//   勢いよく開いて、すっと減速してから落ちる。
		Velocities[i] *= FMath::Pow(0.12f, DeltaSeconds);
		Velocities[i].Z -= Gravity * DeltaSeconds;
		Starts[i] += Velocities[i] * DeltaSeconds;
		M->SetRelativeLocation(Starts[i]);

		const float Base = Sizes.IsValidIndex(i) ? Sizes[i] : ParticleSize;

		// ★飛ぶ向きへ伸ばして尾を引かせる。
		//   速いほど長い筋になり、止まると点に戻る。
		const float Speed = Velocities[i].Size();
		if (Speed > 1.f)
		{
			M->SetRelativeRotation(Velocities[i].Rotation());
		}
		const float Stretch = 1.f + Speed / 260.f;

		// 縮めながら消す。最後まで細く残るよう2乗で落とす
		const float S = Base * FMath::Max(0.f, 1.f - T * T);
		M->SetRelativeScale3D(FVector(S * Stretch, S, S));

		if (Mats.IsValidIndex(i) && Mats[i])
		{
			FLinearColor C = Colors.IsValidIndex(i) ? Colors[i] : SparkColor;

			// 開いた直後は白く飛ばし、そのあと本来の色に落ち着く
			C = FMath::Lerp(FLinearColor::White, C, FMath::Min(1.f, T * 4.f));

			// ★粒ごとに位相をずらして瞬かせる。
			//   揃っていると全体が点滅しているようにしか見えない。
			const float Ph = Phases.IsValidIndex(i) ? Phases[i] : 0.f;
			const float Flick = 0.65f + 0.35f * FMath::Sin(Age * 38.f + Ph);

			// 消え際は一度強く光ってから落とす
			const float Fade = FMath::Sin(FMath::Min(1.f, T) * PI * 0.5f + PI * 0.5f);

			Mats[i]->SetVectorParameterValue(TEXT("Color"), C);
			Mats[i]->SetScalarParameterValue(TEXT("Intensity"), Glow * Flick);
			Mats[i]->SetScalarParameterValue(TEXT("Alpha"), FMath::Max(0.f, Fade));

			C.A = FMath::Max(0.f, Fade);
			Mats[i]->SetVectorParameterValue(TEXT("BaseColor"), C);
		}
	}

	if (T >= 1.f)
	{
		Destroy();
	}
}
