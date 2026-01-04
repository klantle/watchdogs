| Platform | Issues | Fix |
|---|---|---|
| **Linux/Docker** | Permission denied on apt lock file | `sudo make` or using run0 (if exists) `run0 make` |
| **Windows (MSYS)** | GDB: No module named 'libstdcxx' | Run: `sed -i '/libstdcxx/d' /etc/gdbinit 2>/dev/null; echo "set auto-load safe-path /" > ~/.gdbinit` |
| **Windows (MSYS)** | GDB gets stuck at startup | Install winpty: `pacman -S winpty`<br>Then use: `winpty gdb ./watchdogs.debug.win` |
| **Android/Termux** | Storage setup fails (Android 11+) | Install: `pkg install termux-am` |
| **Android/Termux** | Partial package download errors | Clean cache: `apt clean && apt update` |
| **Android/Termux** | Repository/Clearsigned file error | Change repo: `termux-change-repo` |
| **Android** | "Problem parsing the package" error | Try different APK from GitHub releases<br>Or use installer: **SAI (Split APKs Installer)**<br>Or use **GitHub Codespaces** in browser |
| **Android** | APK installation blocked by OEM | Use: **Shizuku** (non-root) or **Magisk/KingoRoot** (root)<br>Or alternative installers: APKMirror, APKPure, MT Manager |

