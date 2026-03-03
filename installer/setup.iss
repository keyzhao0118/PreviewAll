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
#define MyAppPublisher "zhaokai3"
#define MyAppURL       "https://github.com/keyzhao0118/PreviewAll"
#define MyAppExeName   "PreviewAll.exe"

; ★ Modify this to point to your actual bin directory (relative to this .iss file)
#define BinDir         "..\bin"

[Setup]
; Generate a unique GUID via Inno Setup menu: Tools → Generate GUID
AppId={{PUT-YOUR-GUID-HERE}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
; License file
LicenseFile=..\LICENSE
; Output
OutputDir=.\Output
OutputBaseFilename={#MyAppName}_Setup_{#MyAppVersion}
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

[Languages]
; English first = default language
Name: "english"; MessagesFile: "compiler:Default.isl"
; Simplified Chinese
Name: "chinese"; MessagesFile: "compiler:Languages\Chinese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; ================================================================
; ★ Package everything in bin/ recursively
; ================================================================
Source: "{#BinDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Start menu
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
; Desktop shortcut (based on user task selection)
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
; Launch after install
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; \
    Flags: nowait postinstall skipifsilent

[Registry]
; ================================================================
; ★ RESERVED: Fill in your HKLM registry entries below
;
; Examples:
;
; Root: HKLM; Subkey: "SOFTWARE\{#MyAppPublisher}\{#MyAppName}"; \
;     ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; \
;     Flags: uninsdeletekey
;
; Root: HKLM; Subkey: "SOFTWARE\{#MyAppPublisher}\{#MyAppName}"; \
;     ValueType: string; ValueName: "Version"; ValueData: "{#MyAppVersion}"; \
;     Flags: uninsdeletekey
;
; ================================================================

[UninstallDelete]
; Clean up logs/cache on uninstall (if any)
Type: filesandordirs; Name: "{app}\logs"
Type: filesandordirs; Name: "{app}\cache"