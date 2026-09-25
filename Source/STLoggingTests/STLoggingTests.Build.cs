using UnrealBuildTool;

// Editor-only: scripted ST_LOG scenarios run through commandlets (-run=STLogTest, -run=STSmoke).
public class STLoggingTests : ModuleRules
{
	public STLoggingTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "STLogging" });
	}
}
