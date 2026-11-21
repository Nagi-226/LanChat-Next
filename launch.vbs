' LanChat-Next silent launcher — no console windows
' Double-click this file to launch the app without any terminal UI

Set fso = CreateObject("Scripting.FileSystemObject")
Set ws = CreateObject("Wscript.Shell")
exe = ws.ExpandEnvironmentStrings("%LOCALAPPDATA%\LanChat-Qt\lanchat_qt.exe")
If Not fso.FileExists(exe) Then
  MsgBox "Please deploy the Qt client using scripts\deploy-qt.ps1 first.", 48, "LanChat-Next"
  WScript.Quit 1
End If
ws.CurrentDirectory = fso.GetParentFolderName(exe)
ws.Run """" & exe & """", 1, False
