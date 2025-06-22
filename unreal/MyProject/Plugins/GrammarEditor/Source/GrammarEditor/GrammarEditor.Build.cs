using UnrealBuildTool;

public class GrammarEditor : ModuleRules
{
	public GrammarEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Slate",
				"SlateCore",
				"InputCore"
			}
			);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"WorkspaceMenuStructure",
				"ToolMenus",
				"LevelEditor"
			}
			);
	}
} 