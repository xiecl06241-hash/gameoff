using UnrealBuildTool;

public class ShiftStoreTarget : TargetRules
{
	public ShiftStoreTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ShiftStore");
	}
}
