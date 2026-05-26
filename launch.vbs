' LanChat-Next silent launcher — no console windows
' Double-click this file to launch the app without any terminal UI

Set fso = CreateObject("Scripting.FileSystemObject")
Set ws = CreateObject("Wscript.Shell")
ws.CurrentDirectory = fso.GetParentFolderName(WScript.ScriptFullName)
ws.Run "powershell -ExecutionPolicy Bypass -WindowStyle Hidden -File ""scripts\launch.ps1""", 0, True
