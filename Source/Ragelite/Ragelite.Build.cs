// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Ragelite : ModuleRules
{
	public Ragelite(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Paper2D", "Sockets", "Networking", "UMG", "Slate", "SlateCore" });

        PrivateDependencyModuleNames.AddRange(new string[] { "Paper2D"/*, "OpenSSL"*/ });

        //AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
    }
}
