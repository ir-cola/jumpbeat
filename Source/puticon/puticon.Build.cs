// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class puticon : ModuleRules
{
	public puticon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"puticon",
			"puticon/Variant_Platforming",
			"puticon/Variant_Platforming/Animation",
			"puticon/Variant_Combat",
			"puticon/Variant_Combat/AI",
			"puticon/Variant_Combat/Animation",
			"puticon/Variant_Combat/Gameplay",
			"puticon/Variant_Combat/Interfaces",
			"puticon/Variant_Combat/UI",
			"puticon/Variant_SideScrolling",
			"puticon/Variant_SideScrolling/AI",
			"puticon/Variant_SideScrolling/Gameplay",
			"puticon/Variant_SideScrolling/Interfaces",
			"puticon/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
