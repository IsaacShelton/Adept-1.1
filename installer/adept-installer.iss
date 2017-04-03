; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Adept"
#define MyAppVersion "1.0"
#define MyAppPublisher "Isaac Shelton"
#define MyAppURL "http://www.dockysoft.com/adept"
#define MyAppExeName "adept.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{C6A7A7DF-AF3A-4D0C-B815-D1CBDB460159}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName=C:/Users/{username}/.adept
DisableDirPage=yes
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=C:\Users\isaac\Google Drive\Adept\LICENSE
OutputDir=C:\Users\isaac\Google Drive\Adept\installer
OutputBaseFilename=adept-installer-64
SetupIconFile=C:\Users\isaac\Google Drive\Adept\installer\favicon.ico
Compression=lzma
SolidCompression=yes
UninstallDisplayName=Adept
ChangesEnvironment=yes

[CustomMessages]
AppAddPath=Add application directory to path environment variable

[Tasks]
Name: modifypath; Description:{cm:AppAddPath}; 

[Code]

function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
begin
  if not IsTaskSelected('modifypath')
  then begin
      Result := False;
      exit;
  end;
  
  if not RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
    'Path', OrigPath)
  then begin
    Result := True;
    exit;
  end;
  // look for the path with leading and trailing semicolon
  // Pos() returns 0 if not found
  // Result := Pos(';' + Param + ';', ';' + OrigPath + ';') = 0;
  Result := False;
  exit;
end;

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; \
    ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; \
    Check: NeedsAddPath('{app}')

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "C:\Users\isaac\Google Drive\Adept\bin\Release\adept.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\isaac\Google Drive\Adept\installer\adept_files\import\*"; DestDir: "{app}\import"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\Users\isaac\Google Drive\Adept\installer\adept_files\lib\*"; DestDir: "{app}\lib"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\Users\isaac\Google Drive\Adept\installer\adept_files\mingw64\*"; DestDir: "{app}\mingw64"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\Users\isaac\Google Drive\Adept\installer\adept_files\obj\core\*"; DestDir: "{app}\obj\core"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\Users\isaac\Google Drive\Adept\installer\adept_files\libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\Users\isaac\Google Drive\Adept\installer\adept_files\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\Users\isaac\Google Drive\Adept\installer\adept_files\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Dirs]
Name: "{app}\obj\module_cache"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"

