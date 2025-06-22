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
				"InputCore",
				"Engine",
				"CoreUObject"
			}
			);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"WorkspaceMenuStructure",
				"ToolMenus",
				"LevelEditor",
				"DesktopPlatform",
				"MeshDescription",
				"StaticMeshDescription",
				"RenderCore",
				"RHI",
				"AssetRegistry",
				"Projects",
				"ProceduralMeshComponent"
			}
			);
	}
} 