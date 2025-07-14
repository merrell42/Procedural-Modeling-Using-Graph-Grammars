# Unity Plugin

## System Requirements

- **Unity**: 2021.3 LTS or later
- **Platform**: Windows (x64)
- **Graphics**: Any graphics card supporting Unity's default rendering pipeline

## Installation

### Method 1: Import Package

1. Download the PMUGG Unity package
2. In Unity, go to **Assets > Import Package > Custom Package**
3. Select the downloaded `.unitypackage` file
4. Click **Import** to install the plugin

### Method 2: Manual Installation

1. Copy the `Assets` folder from this repository to your Unity project
2. Ensure the following files are present:
   - `Assets/Plugins/pmugg release.dll` (or `pmugg debug.dll` for development)
   - `Assets/GraphGrammar/Editor/GrammarEditorWindow.cs`
   - `Assets/GraphGrammar/Runtime/GrammarCreatorGenerator.cs`

## Setup

### 1. Open the Grammar Editor

1. In Unity, go to **Window > Graph Grammar Generator**
2. The Grammar Editor window will open in your Unity editor

### 2. Load Grammar Files

To load a grammar:
1. Click **Load Grammar File** in the Grammar Editor
2. Navigate to the `grammar data` folder
3. Select a `.json` grammar file
4. The grammar will be loaded and ready for generation

## Usage

### Basic Generation

- **Load a Grammar**: Select a grammar file from the available options.
- **Set Parameters**:
   - **Size**: Adjust the generation bounds (X, Y, Z).
   - **Seed**: Set a random seed for reproducible results.
- **Play/Pause**: Toggle generation on or off.
- **Step Controls**: Use the step buttons to advance generation one step at a time
- **Reset**: Start over with a new generation

### Keyboard Shortcuts

- **Space**: Toggle animation play/pause
- **0 (Keypad/Alpha)**: Reset generation
- **1-3 (Keypad/Alpha)**: Step 1, 10, or 100 iterations

**Note**: This plugin is compiled for Windows x64 platforms. For other platforms, the C++ backend would need to be recompiled for the target platform. 