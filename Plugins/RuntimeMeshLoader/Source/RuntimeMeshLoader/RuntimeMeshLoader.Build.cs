// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class RuntimeMeshLoader : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
    }

    private string BinFolder(ReadOnlyTargetRules Target)
    {
        if(Target.Platform == UnrealTargetPlatform.Mac)
            return Path.GetFullPath(Path.Combine(ModulePath, "../../../../Binaries/Mac/"));
        else if(Target.Platform == UnrealTargetPlatform.IOS)
            return Path.GetFullPath(Path.Combine(ModulePath, "../../../../Binaries/IOS/"));
        if(Target.Platform == UnrealTargetPlatform.Win64)
            return Path.GetFullPath(Path.Combine(ModulePath, "../../../../Binaries/Win64/"));
        if(Target.Platform == UnrealTargetPlatform.Android)
            return Path.GetFullPath(Path.Combine(ModulePath, "../../../../Binaries/Android/"));
        return "";
}

    public RuntimeMeshLoader(ReadOnlyTargetRules Target) : base(Target)
	{
	    PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
			new string[] {
				"RuntimeMeshLoader/Public",
                Path.Combine(ThirdPartyPath, "assimp/include")
				// ... add public include paths required here ...
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"RuntimeMeshLoader/Private",
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "CoreUObject",
                "Engine",
                "ProceduralMeshComponent",
                "ImageCore",
                "Json"

                // ... add other public dependencies that you statically link with here ...
			}
			);


        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
                "ImageWrapper"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
            }
            );

        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "x86";
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp/lib",PlatformString, "assimp-vc140-mt.lib"));

            RuntimeDependencies.Add(new RuntimeDependency(Path.Combine(ThirdPartyPath, "assimp/bin",PlatformString, "assimp-vc140-mt.dll")));

            string AssimpDll = ThirdPartyPath + "assimp/bin/" + PlatformString + "/assimp-vc140-mt.dll";

            string thp = BinFolder(Target) + "assimp-vc140-mt.dll";

            CopyFile(AssimpDll, thp);
        }

        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
           // PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp/bin","Mac", "libassimp.4.1.0.dylib"));

          //  RuntimeDependencies.Add(new RuntimeDependency(Path.Combine(ThirdPartyPath, "assimp/bin", "Mac", "libassimp.4.1.0.dylib")));

           // string AssimpDll = ThirdPartyPath + "assimp/bin/" + "Mac" + "/libassimp.4.1.0.dylib";

          //  string thp = BinFolder(Target) + "libassimp.4.1.0.dylib";

           // CopyFile(AssimpDll, thp);
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp/lib","iOS", "libassimp.a"));
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp/lib","iOS", "libIrrXML.a"));
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp/lib","iOS", "libzlibstatic.a"));

        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            //PrivateIncludePaths.Add(Path.Combine(ThirdPartyPath, "assimp.dir"));
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp/lib", "Android", "libIrrXML.a"));
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp/lib", "Android", "libzlibstatic.a"));
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp/lib", "Android", "libassimp.a"));

            PrivateDependencyModuleNames.AddRange(new string[] { "Launch" });
            AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "RuntimeMeshLoader._APL.xml"));
        }
    }

    public void CopyFile(string Source, string Dest)
    {
        System.Console.WriteLine("Copying {0} to {1}", Source, Dest);
        if (System.IO.File.Exists(Dest))
        {
            System.IO.File.SetAttributes(Dest, System.IO.File.GetAttributes(Dest) & ~System.IO.FileAttributes.ReadOnly);
        }
        try
        {
            System.IO.File.Copy(Source, Dest, true);
        }
        catch (System.Exception ex)
        {
            System.Console.WriteLine("Failed to copy file: {0}", ex.Message);
        }
    }
}
