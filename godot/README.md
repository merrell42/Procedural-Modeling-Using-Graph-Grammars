# Godot Plugin

This directory is to get PMUGG working in Godot. To get it setup, these tutorials were used:

https://www.youtube.com/watch?v=02KJouOjQ0c
https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/gdextension_cpp_example.html#

## Installation

### Manual Installation

I'm not 100% sure this is right. I'd love for someone other than me to test this.

1. **Clone the Repository**
   ```bash
   git clone --recurse-submodules <repository-url>
   cd PMUGG/godot
   ```

If the repository is already cloned without the necessary Godot submodule, you can
clone the submodule with the following command:

  ```bash
  git submodule update --init --recursive
  ```

2. **Build the Plugin**
   ```bash
   # Windows
   scons platform=windows target=template_debug
   scons platform=windows target=template_release
   
   # Linux
   scons platform=linux target=template_debug
   scons platform=linux target=template_release
   
   # macOS
   scons platform=macos target=template_debug
   scons platform=macos target=template_release
   ```

3. **Install in Godot Project**
   - Copy the `demo/` folder to your Godot project
   - Or copy the generated `.gdextension` and `.dll/.so/.dylib` files to your project's `addons/` directory

### Using the Demo Project

1. Open the `demo/` folder in Godot
2. The plugin will be automatically loaded
3. Open the Graph Grammar Editor dock from the editor

## Setup

### Project Configuration

1. Right click on the Main node add click "Add Child Node... (Ctrl+A)"
2. Search for "Grammar Editor".

### File Structure

```
godot/
├── src/                    # Source code
│   ├── grammar_editor.h    # Main editor header
│   ├── grammar_editor.cpp  # Main editor implementation
│   ├── register_types.h    # Type registration
│   └── register_types.cpp  # Type registration implementation
├── demo/                   # Demo project
│   ├── bin/               # Built plugin files
│   ├── project.godot      # Project configuration
│   └── main.tscn          # Main scene
├── godot-cpp/             # Godot C++ bindings
├── SConstruct             # Build configuration
└── README.md              # This file
```

## Usage

### Basic Workflow

1. **Load a Grammar File**
   - Click "Load Grammar..." in the editor dock
   - Navigate to a `.json` grammar file
   - The file will be loaded and initialized

2. **Generate Models**
   - Use "Step" to generate one iteration at a time
   - Use "Play" for continuous generation
   - Use "Reset" to start over with a new seed

3. **Adjust Parameters**
   - Set the random seed for reproducible results
   - Adjust size parameters (X, Y, Z) to control model dimensions

### Advanced Features

#### Batch Processing
- Click "Load Folder..." to process all JSON files in a directory
- The plugin will automatically cycle through files during generation

#### Animation Control
- The "Play" button starts continuous generation
- Use "Step" to iterate one step at a time.

**Note**: This plugin is compiled for Windows x64 platforms. For other platforms, the C++ backend would need to be recompiled for the target platform.

