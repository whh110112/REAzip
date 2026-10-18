#ifndef Stage
  #error Stage must be provided
#endif
[Setup]
AppId=REAzip.Native.{#Arch}
AppName=ZENzip
AppVersion={#Version}
AppPublisher=whh110112
AppPublisherURL=https://github.com/whh110112/ZENzip
DefaultDirName={localappdata}\Programs\REAzip-{#Arch}
DefaultGroupName=ZENzip
PrivilegesRequired=lowest
UninstallDisplayIcon={app}\zenzip.exe
OutputDir={#Output}
OutputBaseFilename=ZENzip-{#Version}-windows-{#Arch}-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
#if Arch == "arm64"
ArchitecturesAllowed=arm64
ArchitecturesInstallIn64BitMode=arm64
#else
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
#endif
[Files]
Source: "{#Stage}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "portable.flag"
[InstallDelete]
Type: files; Name: "{app}\reazip.exe"
Type: files; Name: "{app}\reazip-worker.exe"
Type: files; Name: "{autoprograms}\REAzip\REAzip.lnk"

[Icons]
Name: "{autoprograms}\ZENzip"; Filename: "{app}\zenzip.exe"
Name: "{group}\Uninstall ZENzip"; Filename: "{uninstallexe}"
[Tasks]
Name: "register"; Description: "Register archive file types and the classic context menu (recommended)"
[Run]
Filename: "{app}\zenzip.exe"; Parameters: "--register"; Tasks: register; Flags: runhidden waituntilterminated
[UninstallRun]
Filename: "{app}\zenzip.exe"; Parameters: "--unregister"; Flags: runhidden waituntilterminated; RunOnceId: "ZENzipOwnedRegistration"
