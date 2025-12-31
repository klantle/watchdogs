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

| **Virus/Malware Detection** | **Why is Watchdogs' binary detected as a virus or malware?** |
| :--- | :--- |
| **1. Use of UPX (The Ultimate Packer for eXecutables)** | The main binary of Watchdogs is compressed using **UPX** (`https://upx.github.io/`) to reduce file size. Many antivirus and antimalware engines heuristically flag **UPX-packed executables** as suspicious, because malware authors also commonly use UPX to obfuscate their code. This compression method alone can trigger false-positive detections. |
| **2. UPX on Library Files** | The same UPX compression is also applied to **library files (*.dll)** used by Watchdogs (e.g., `libdog.dll`). Packed DLLs are similarly viewed as suspicious by security software, amplifying the detection. |
| **3. Absence of Standard Metadata** | Watchdogs binaries do not include standard **Windows metadata** (such as version information, company name, or digital signatures) for **compile-time efficiency**. The lack of this identifying information makes the file appear more "generic" and anonymous, which antivirus heuristics often associate with potentially malicious software. |

**Summary:** The combination of **UPX compression** (on both EXE and DLL files) and the **lack of standard Windows metadata** causes many antivirus programs to heuristically classify the Watchdogs binaries as suspicious or malicious, even though they are legitimate. This is a common issue for many lean, efficiency-focused applications that use packers and minimize optional metadata.