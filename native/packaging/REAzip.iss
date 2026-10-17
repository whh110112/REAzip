#ifndef Stage
  #error Stage must be provided
#endif
[Setup]
AppId=REAzip.Native.{#Arch}
AppName=REAzip
AppVersion={#Version}
AppPublisher=whh110112
AppPublisherURL=https://github.com/whh110112/REAzip
DefaultDirName={localappdata}\Programs\REAzip-{#Arch}
DefaultGroupName=REAzip
PrivilegesRequired=lowest
UninstallDisplayIcon={app}\reazip.exe
OutputDir={#Output}
OutputBaseFilename=REAzip-{#Version}-windows-{#Arch}-Setup
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
[Icons]
Name: "{group}\REAzip"; Filename: "{app}\reazip.exe"
Name: "{group}\Uninstall REAzip"; Filename: "{uninstallexe}"
[Tasks]
Name: "register"; Description: "Register archive file types and classic context menu"; Flags: unchecked
[Run]
Filename: "{app}\reazip.exe"; Parameters: "--register"; Tasks: register; Flags: runhidden waituntilterminated
[UninstallRun]
Filename: "{app}\reazip.exe"; Parameters: "--unregister"; Flags: runhidden waituntilterminated
