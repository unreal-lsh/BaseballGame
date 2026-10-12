using UnrealBuildTool;
using System.Collections.Generic;

public class NumberBaseballEditorTarget : TargetRules
{
	public NumberBaseballEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("NumberBaseball");
	}
}
