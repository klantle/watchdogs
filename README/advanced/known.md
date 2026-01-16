| Platform        | Issues                                     | Fix                                                                 |
|-----------------|--------------------------------------------|---------------------------------------------------------------------|
| Linux/Docker    | Permissions&nbsp;denied&nbsp;on&nbsp;apt&nbsp;lock&nbsp;file | sudo&nbsp;make&nbsp;atau&nbsp;run0&nbsp;make                       |
| MSYS2           | GDB:&nbsp;no&nbsp;module&nbsp;named&nbsp;'libstdcxx'           | sed -i&nbsp;'/libstdcxx/d'&nbsp;/etc/gdbinit&nbsp;2>/dev/null; echo&nbsp;"set auto-load safe-path /"&nbsp;>&nbsp;~/.gdbinit |
| MSYS2           | GDB:&nbsp;gets&nbsp;stuck&nbsp;at&nbsp;startup               | pacman&nbsp;-S&nbsp;winpty * winpty&nbsp;gdb&nbsp;./watchdogs.debug.win |
| Android/Termux  | Storage&nbsp;setup&nbsp;fails&nbsp;(Android&nbsp;11+)        | pkg&nbsp;install&nbsp;termux-am                                      |
| Android/Termux  | Partial&nbsp;package&nbsp;download&nbsp;errors               | apt&nbsp;clean&nbsp;&amp;&amp;&nbsp;apt&nbsp;update * (opt): apt&nbsp;upgrade |
| Android/Termux  | Repository/Clearsigned&nbsp;file&nbsp;error                  | termux-change-repo                                                   |
