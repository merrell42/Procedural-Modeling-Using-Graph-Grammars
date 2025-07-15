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

**Note**: This plugin is compiled for Windows x64 platforms. For other platforms, the C++ backend would need to be recompiled for the target platform. 