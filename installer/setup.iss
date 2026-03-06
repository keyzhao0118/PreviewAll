; ================================================================
;  PreviewAll - Inno Setup Installer Script
;  
;  Requirements:
;    1. Default English, auto-detect UI language, support English + Chinese
;    2. Package all files from bin/ directory
;    3. Reserved registry section for HKLM (to be filled later)
; ================================================================

#define MyAppName      "PreviewAll"
#define MyAppVersion   "1.0.0"
#define MyAppPublisher "freedomkey"
#define MyAppURL       "https://github.com/keyzhao0118/PreviewAll"
#define MyAppExeName   "PreviewAll.exe"

; ★ Modify this to point to your actual bin directory (relative to this .iss file)
#define BinDir         "..\out\build\x64-release-user\bin"

[Setup]
; 显式指定源目录为 .iss 文件所在目录（确保相对路径始终正确）
SourceDir=.
; Generate a unique GUID via Inno Setup menu: Tools → Generate GUID
AppId={{BF9C67AD-2D1B-4755-A865-76FDACCD0C29}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
; License file
LicenseFile=..\LICENSE
; Output
OutputDir=.\Output
OutputBaseFilename={#MyAppName}_Setup_{#MyAppVersion}
; Installer icon
SetupIconFile=..\PreviewAll\resources\previewall.ico
; Compression
Compression=lzma2/ultra64
SolidCompression=yes
; 64-bit
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Admin privileges (required for HKLM registry)
PrivilegesRequired=admin
; Uninstall icon
UninstallDisplayIcon={app}\{#MyAppExeName}
; ================================================================
; Language settings:
;   - Default is English (first in [Languages] list)
;   - Auto-detect based on Windows UI language
;   - Show language dialog only when auto-detect is ambiguous
; ================================================================
ShowLanguageDialog=auto
LanguageDetectionMethod=uilanguage
; ...existing settings...
CloseApplications=force
RestartApplications=yes

[Languages]
; English first = default language
Name: "english"; MessagesFile: "compiler:Default.isl"
; Simplified Chinese
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; ================================================================
; ★ Package everything in bin/ recursively
; ================================================================
Source: "{#BinDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Start menu
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
; Desktop shortcut (based on user task selection)
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
; Launch after install
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; \
    Flags: nowait postinstall skipifsilent

[Registry]
; ================================================================
; PreviewAllHandler COM registration — HKLM
; ================================================================
Root: HKLM; Subkey: "Software\Classes\CLSID\{{A26D5A00-AF3F-47B7-B075-A3282DE904E6}"; \
    ValueType: string; ValueName: ""; ValueData: "PreviewAllHandler"; \
    Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Classes\CLSID\{{A26D5A00-AF3F-47B7-B075-A3282DE904E6}"; \
    ValueType: string; ValueName: "AppID"; ValueData: "{{6d2b5079-2f0b-48dd-ab7f-97cec514d30b}"
Root: HKLM; Subkey: "Software\Classes\CLSID\{{A26D5A00-AF3F-47B7-B075-A3282DE904E6}"; \
    ValueType: dword; ValueName: "DisableLowILProcessIsolation"; ValueData: "1"
Root: HKLM; Subkey: "Software\Classes\CLSID\{{A26D5A00-AF3F-47B7-B075-A3282DE904E6}\InProcServer32"; \
    ValueType: string; ValueName: ""; ValueData: "{app}\PreviewAllHandler.dll"; \
    Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Classes\CLSID\{{A26D5A00-AF3F-47B7-B075-A3282DE904E6}\InProcServer32"; \
    ValueType: string; ValueName: "ThreadingModel"; ValueData: "Apartment"
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\PreviewHandlers"; \
    ValueType: string; ValueName: "{{A26D5A00-AF3F-47B7-B075-A3282DE904E6}"; ValueData: "PreviewAllHandler"; \
    Flags: uninsdeletevalue

; ================================================================
; PreviewAllHandler COM registration — HKCU
; ================================================================
Root: HKCU; Subkey: "Software\Classes\CLSID\{{A26D5A00-AF3F-47B7-B075-A3282DE904E6}"; \
    ValueType: string; ValueName: ""; ValueData: "PreviewAllHandler"; \
    Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\CLSID\{{A26D5A00-AF3F-47B7-B075-A3282DE904E6}"; \
    ValueType: string; ValueName: "AppID"; ValueData: "{{6d2b5079-2f0b-48dd-ab7f-97cec514d30b}"
Root: HKCU; Subkey: "Software\Classes\CLSID\{{A26D5A00-AF3F-47B7-B075-A3282DE904E6}"; \
    ValueType: dword; ValueName: "DisableLowILProcessIsolation"; ValueData: "1"
Root: HKCU; Subkey: "Software\Classes\CLSID\{{A26D5A00-AF3F-47B7-B075-A3282DE904E6}\InProcServer32"; \
    ValueType: string; ValueName: ""; ValueData: "{app}\PreviewAllHandler.dll"; \
    Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\CLSID\{{A26D5A00-AF3F-47B7-B075-A3282DE904E6}\InProcServer32"; \
    ValueType: string; ValueName: "ThreadingModel"; ValueData: "Apartment"
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\PreviewHandlers"; \
    ValueType: string; ValueName: "{{A26D5A00-AF3F-47B7-B075-A3282DE904E6}"; ValueData: "PreviewAllHandler"; \
    Flags: uninsdeletevalue

[UninstallDelete]
; Clean up logs/cache on uninstall (if any)
; Type: filesandordirs; Name: "{app}\logs"
; Type: filesandordirs; Name: "{app}\cache"