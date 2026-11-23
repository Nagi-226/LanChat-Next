#ifndef PayloadDirectory
  #error PayloadDirectory is required
#endif
#ifndef OutputDirectory
  #error OutputDirectory is required
#endif
#ifndef ReleaseVersion
  #define ReleaseVersion "2026.11.23"
#endif

[Setup]
AppId={{8AA962BF-0263-4A45-9EA1-89D1DAFF07F1}
AppName=LanChat-Next
AppVersion={#ReleaseVersion}
VersionInfoVersion={#ReleaseVersion}.0
VersionInfoProductName=LanChat-Next
VersionInfoDescription=LanChat-Next Windows installer
AppPublisher=Nagi-226
AppPublisherURL=https://github.com/Nagi-226/LanChat-Next
AppSupportURL=https://github.com/Nagi-226/LanChat-Next/issues
DefaultDirName={localappdata}\LanChat-Qt
DefaultGroupName=LanChat-Next
PrivilegesRequired=lowest
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0
OutputDir={#OutputDirectory}
OutputBaseFilename=LanChat-Next-Setup-{#ReleaseVersion}-x64
SetupIconFile={#PayloadDirectory}\lanchat-rounded-v2.ico
UninstallDisplayIcon={app}\lanchat_qt.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
DisableProgramGroupPage=yes
UsePreviousTasks=yes
CloseApplications=yes
RestartApplications=no
InfoAfterFile={#PayloadDirectory}\联机使用说明.txt

[Languages]
Name: "chinesesimp"; MessagesFile: "ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; Only the verified payload is staged here. Account data and local profiles stay outside it.
Source: "{#PayloadDirectory}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\LanChat-Next"; Filename: "{app}\lanchat_qt.exe"; WorkingDir: "{app}"; IconFilename: "{app}\lanchat-rounded-v2.ico"
Name: "{group}\联机使用说明"; Filename: "{app}\联机使用说明.txt"
Name: "{group}\{cm:UninstallProgram,LanChat-Next}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\LanChat-Next"; Filename: "{app}\lanchat_qt.exe"; WorkingDir: "{app}"; IconFilename: "{app}\lanchat-rounded-v2.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\lanchat_qt.exe"; Description: "{cm:LaunchProgram,LanChat-Next}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

; There is no recursive uninstall deletion. Personal data and settings are preserved.
