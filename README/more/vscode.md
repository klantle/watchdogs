# What are VSCode Tasks?

VSCode tasks allow you to run watchdogs or other automated processes directly within Visual Studio Code.

## Setup Instructions

### Create Directory Structure

1. In your project's root directory, create a folder named `.vscode`
2. Inside the `.vscode` folder, create a file named `tasks.json`

### Windows Configuration

For Windows systems, add the following configuration to `tasks.json`:

```json
{
    "label": "Run a bat file",
    "type": "shell",
    "command": "cmd.exe",
    "args": ["/c", "${workspaceFolder}/windows-native.cmd"],
    "group": {
        "kind": "build",
        "isDefault": false
    },
    "presentation": {
        "echo": true,
        "reveal": "always",
        "focus": false,
        "panel": "shared",
        "showReuseMessage": true,
        "clear": true
    },
    "problemMatcher": [
        {
            "owner": "custom",
            "fileLocation": ["relative", "${workspaceFolder}"],
            "pattern": {
                "regexp": "^(.*):(\\d+):(\\d+):\\s+(error|warning|info):\\s+(.*)$",
                "file": 1,
                "line": 2,
                "column": 3,
                "severity": 4,
                "message": 5
            }
        }
    ]
}
```

### Linux/Mac Configuration

For Linux/Mac systems, add the following configuration to `tasks.json`:

```json
{
    "label": "Run a shell file",
    "type": "shell",
    "command": "./watchdogs",
    "args": ["${workspaceFolder}/watchdogs"],
    "group": {
        "kind": "build",
        "isDefault": false
    },
    "presentation": {
        "echo": true,
        "reveal": "always",
        "focus": false,
        "panel": "shared",
        "showReuseMessage": true,
        "clear": true
    },
    "problemMatcher": [
        {
            "owner": "custom",
            "fileLocation": ["relative", "${workspaceFolder}"],
            "pattern": {
                "regexp": "^(.*):(\\d+):(\\d+):\\s+(error|warning|info):\\s+(.*)$",
                "file": 1,
                "line": 2,
                "column": 3,
                "severity": 4,
                "message": 5
            }
        }
    ]
}
```

## Save the File

After adding the appropriate configuration, save the `tasks.json` file.

## Usage

To run the configured task:

1. Press **Ctrl + Shift + B** (Windows/Linux) or **Cmd + Shift + B** (Mac)
2. This will execute the shell script or batch file defined in your configuration

**Note**: The `problemMatcher` configuration helps VSCode identify and display errors, warnings, and informational messages from your build/output in the Problems panel.