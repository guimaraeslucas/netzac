#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef AppVersionNumeric
  #define AppVersionNumeric "1.0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\build-windows\dist"
#endif
#ifndef LicenseFile
  #define LicenseFile "..\LICENCE"
#endif
#ifndef IconFile
  #define IconFile "..\share\icons\netzac.ico"
#endif
#ifndef OutputDir
  #define OutputDir ".."
#endif
#ifndef OutputBaseFilename
  #define OutputBaseFilename "zacos9-netzac_" + AppVersion + "_windows-x64-setup"
#endif

#define AppName "Netzac Navigator"
#define AppDescription "Netzac web browser"
#define AppPublisher "G3Pix Ltda"
#define AppURL "https://github.com/guimaraeslucas/netzac"
#define AppExeName "zacos9-netzac.exe"
#define AppCopyright "Copyright (C) 2026 Lucas Guimarães (@guimaraeslucas)"

[Setup]
AppId={{7B6E4A1D-9C3F-4E2A-B854-2F1E6A9C3D08}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}/issues
AppUpdatesURL={#AppURL}/releases
AppCopyright={#AppCopyright}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
LicenseFile={#LicenseFile}
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
OutputDir={#OutputDir}
OutputBaseFilename={#OutputBaseFilename}
SetupIconFile={#IconFile}
UninstallDisplayIcon={app}\{#AppExeName}
UninstallDisplayName={#AppName}
#if VER >= 0x06030000
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
#else
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
#endif
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ChangesAssociations=yes
CloseApplications=yes
RestartApplications=no
VersionInfoVersion={#AppVersionNumeric}
VersionInfoProductName={#AppName}
VersionInfoProductTextVersion={#AppVersion}
VersionInfoDescription={#AppName} Setup
VersionInfoCompany={#AppPublisher}
VersionInfoCopyright={#AppCopyright}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
#if FileExists(CompilerPath + "Languages\Arabic.isl")
Name: "arabic"; MessagesFile: "compiler:Languages\Arabic.isl"
#endif

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\*"; DestDir: "{app}"; Excludes: "{#AppExeName},vc_redist*.exe"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#LicenseFile}"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Registry]
Root: HKA; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\{#AppExeName}"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName}"; Flags: uninsdeletekey
Root: HKA; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\{#AppExeName}"; ValueType: string; ValueName: "Path"; ValueData: "{app}"

Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator"; ValueType: string; ValueName: ""; ValueData: "{#AppName}"; Flags: uninsdeletekey
Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "{#AppName}"
Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "{#AppDescription}"
Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator\Capabilities"; ValueType: string; ValueName: "ApplicationIcon"; ValueData: "{app}\{#AppExeName},0"
Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator\Capabilities\Startmenu"; ValueType: string; ValueName: "StartMenuInternet"; ValueData: "Netzac Navigator"
Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator\Capabilities\FileAssociations"; ValueType: string; ValueName: ".htm"; ValueData: "NetzacHTML"
Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator\Capabilities\FileAssociations"; ValueType: string; ValueName: ".html"; ValueData: "NetzacHTML"
Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator\Capabilities\FileAssociations"; ValueType: string; ValueName: ".shtml"; ValueData: "NetzacHTML"
Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator\Capabilities\FileAssociations"; ValueType: string; ValueName: ".xhtml"; ValueData: "NetzacHTML"
Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator\Capabilities\URLAssociations"; ValueType: string; ValueName: "http"; ValueData: "NetzacHTML"
Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator\Capabilities\URLAssociations"; ValueType: string; ValueName: "https"; ValueData: "NetzacHTML"
Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"
Root: HKA; Subkey: "SOFTWARE\Clients\StartMenuInternet\Netzac Navigator\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"""
Root: HKA; Subkey: "SOFTWARE\RegisteredApplications"; ValueType: string; ValueName: "Netzac Navigator"; ValueData: "Software\Clients\StartMenuInternet\Netzac Navigator\Capabilities"; Flags: uninsdeletevalue

Root: HKA; Subkey: "SOFTWARE\Classes\NetzacHTML"; ValueType: string; ValueName: ""; ValueData: "Netzac HTML Document"; Flags: uninsdeletekey
Root: HKA; Subkey: "SOFTWARE\Classes\NetzacHTML\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"
Root: HKA; Subkey: "SOFTWARE\Classes\NetzacHTML\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
