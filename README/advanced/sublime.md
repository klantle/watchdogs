# Sublime Text Build System for Watchdogs

## Overview

This setup creates a Sublime Text build system that allows you to run Watchdogs commands with arguments directly within Sublime Text.

## Setup Instructions

### Create a New Build System

1. Open Sublime Text
2. Navigate to: **Tools** → **Build System** → **New Build System**

### Windows Configuration

For Windows systems, add the following configuration:

```json
{
    "cmd": ["windows-native.cmd", "help", "compile"],
    "working_dir": "C:\\Users\\Administrator\\Downloads\\watchdogs"
}
```

## Configuration Details

### Working Directory (working_dir)

Specify the location where your `windows-native.cmd` file is located:

- **Windows example**: If your file is at `C:\Users\your_desktop_name\Downloads\watchdogs`
  - Use: `C:\\Users\\your_desktop_name\\Downloads\\watchdogs`

- **Linux example**: If your file is at `/home/your_desktop_name/Downloads/watchdogs`
  - Use: `/home/your_desktop_name/Downloads/watchdogs`

### Commands (cmd)

The `cmd` array contains the inline commands to execute:

- **Format**: `[<file>] [<command>] [<args>]`
- **Windows examples**:
  - `"windows-native.cmd", "dir", "/s"`
  - `"windows-native.cmd", "help", "compile"`

- **Linux examples**:
  - `"watchdogs", "ls", "-a"`
  - `"watchdogs", "status", "--verbose"`

- **Termux example**:
  - `"watchdogs.tmux", "ls", "-a"`

## Save the Build System

1. Save the file with an appropriate name, such as `Watchdogs.sublime-build`
2. The file will be saved in your Sublime Text user packages directory

## Usage

To use the build system:

1. **Select the build system**:
   - Navigate to: **Tools** → **Build System**
   - Select `Watchdogs` (or whatever you named your build system)

2. **Execute the build**:
   - Press **Ctrl + Shift + B** (Windows/Linux) or **Cmd + Shift + B** (Mac) to execute the build
   - Alternatively, press **Ctrl + B** (Windows/Linux) or **Cmd + B** (Mac) for quick build

3. **View output**: The command output will appear in the build panel at the bottom of Sublime Text

## Tips

- You can create multiple build systems for different Watchdogs commands
- The build system configuration file is stored in JSON format
- Modify the `cmd` array to change the arguments passed to Watchdogs
- Ensure the `working_dir` path uses double backslashes (`\\`) on Windows for proper escaping