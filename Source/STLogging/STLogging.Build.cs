using UnrealBuildTool;

public class STLogging : ModuleRules
{
	public STLogging(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject" });
	}
}
