# Unreal Engine Plugin

## Features

## System Requirements

- **Unreal Engine**: 5.6 or later
- **Platform**: Windows (x64)
- **Graphics**: Any graphics card supporting Unreal Engine's default rendering pipeline
- **Development Tools**: Visual Studio 2019 or later (for building from source)

## Installation

### Method 1: Plugin Installation (Recommended)

1. **Copy the Plugin**:
   - Copy the `GrammarEditor` folder from `MyProject/Plugins/` to your Unreal project's `Plugins/` directory
   - Ensure the following structure exists:
     ```
     YourProject/Plugins/GrammarEditor/
     ├── GrammarEditor.uplugin
     ├── Binaries/Win64/
     │   ├── pmugg release.dll
     │   ├── pmugg debug.dll
     │   └── UnrealEditor-GrammarEditor.dll
     └── Source/GrammarEditor/
     ```

2. **Enable the Plugin**:
   - Open your Unreal project
   - Go to **Edit > Plugins**
   - Search for "Graph Grammar Editor"
   - Enable the plugin and restart the editor

### Method 2: Project Integration

1. **Copy the Entire Project**:
   - Copy the `MyProject` folder as a starting point
   - Rename it to your desired project name
   - Open the `.uproject` file with Unreal Engine

2. **Modify Project Settings**:
   - Update the project name in `MyProject.uproject`
   - Ensure the GrammarEditor plugin is enabled in the project settings

## Setup

### 1. Open the Grammar Editor

1. In Unreal Engine, go to **Tools > Grammar Editor**
2. The Grammar Editor window will open in your Unreal editor

### 2. Load Grammar Files

To load a grammar:
1. Click **Load Grammar File** in the Grammar Editor
2. Navigate to the `grammar data` folder
3. Select a `.json` grammar file
4. The grammar will be loaded and ready for generation

## Usage

- **Load a Grammar**: Select a grammar file from the available options.
- **Set Parameters**:
  - **Size**: Adjust the generation bounds (X, Y, Z).
  - **Seed**: Set a random seed for reproducible results.
- **Play/Pause**: Toggle generation on or off.
- **Step Controls**: Use the step buttons to advance generation one step at a time
- **Reset**: Start over with a new generation

## Troubleshooting

### Engine Version Compatibility Issues

If you encounter errors like:
- "The following modules are built with a different engine version: GrammarEditor"
- "Engine modules cannot be compiled at runtime. Please build through your IDE"
- "MyProject could not be compiled. Try rebuilding from source manually"

**Solution: Rebuild the Plugin from Source**

1. **Ensure Visual Studio is Installed**:
   - Install Visual Studio 2019 or later with C++ development tools
   - Make sure you have the "Game development with C++" workload installed

2. **Right-Click the Project File**:
   - Navigate to your project folder
   - Right-click on `MyProject.uproject`
   - Select "Generate Visual Studio project files"
   - Wait for the generation to complete

3. **Open in Visual Studio**:
   - Open the generated `MyProject.sln` file in Visual Studio
   - Set the solution configuration to "Development Editor" and "Win64"
   - Build the solution (Build > Build Solution or Ctrl+Shift+B)

4. **Alternative: Use Unreal's Build Tool**:
   ```bash
   # Navigate to your project directory
   cd path/to/your/project
   
   # Generate project files
   "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="MyProject.uproject" -game -rocket -progress
   
   # Build the project
   "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" MyProject Win64 Development -Project="MyProject.uproject" -WaitMutex -FromMsBuild
   ```

5. **Verify Engine Version**:
   - Check that your Unreal Engine version matches the one specified in `MyProject.uproject` (EngineAssociation: "5.6")
   - If using a different version, update the EngineAssociation field in the .uproject file

6. **Clean and Rebuild**:
   - Delete the `Intermediate/` and `Binaries/` folders in your project
   - Delete the `Intermediate/` and `Binaries/` folders in the GrammarEditor plugin
   - Rebuild the project from step 3

### Common Issues and Solutions

**Issue**: "Missing required module" errors
- **Solution**: Ensure all dependencies are properly installed and the plugin is enabled

**Issue**: Plugin doesn't appear in Tools menu
- **Solution**: Restart the Unreal Editor after enabling the plugin

**Issue**: DLL loading errors
- **Solution**: Ensure the PMUGG DLL files are in the correct location and match your build configuration

**Note**: This plugin is compiled for Windows x64 platforms. For other platforms, the C++ backend would need to be recompiled for the target platform. 