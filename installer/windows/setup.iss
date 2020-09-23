#define AppSetupName 'Conceal Desktop'
#define AppVersion '6.3.1'
#define AppPublisher 'Conceal Network'
#define AppCopyright 'Copyright (C) Conceal Network 2020'
#define AppURL 'https://conceal.network'
#define UpdateURL 'https://github.com/ConcealNetwork/conceal-desktop/releases'
#define AppExe 'conceal-desktop.exe'
#define AppComments 'Conceal Wallet and Banking App'
#define LicenseFile 'license.txt'

[Setup]
AppName={#AppSetupName}
AppVersion={#AppVersion}
AppVerName={#AppSetupName} {#AppVersion}
AppCopyright={#AppCopyright}
VersionInfoVersion={#AppVersion}
VersionInfoCompany={#AppPublisher}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#UpdateURL}
AppComments={#AppComments}
OutputBaseFilename={#AppSetupName}-{#AppVersion}
DefaultGroupName={#AppSetupName}
#if VER < EncodeVer(6,0,0)
DefaultDirName={pf}\{#AppSetupName}
#else
DefaultDirName={autopf}\{#AppSetupName}
#endif
UninstallDisplayIcon={app}\{#AppExe}
OutputDir=bin
SourceDir=.
AllowNoIcons=yes
WizardImageFile=Image.bmp
WizardSmallImageFile=SmallImage.bmp

PrivilegesRequired=admin
ArchitecturesAllowed=x86 x64 ia64
ArchitecturesInstallIn64BitMode=x64 ia64

LicenseFile={#LicenseFile}

// downloading and installing dependencies will only work if the memo/ready page is enabled (default and current behaviour)
DisableReadyPage=no
DisableReadyMemo=no

#define use_msiproduct
#define use_vc2019

// supported languages
#include "scripts\lang\english.iss"
#include "scripts\lang\german.iss"
#include "scripts\lang\french.iss"
#include "scripts\lang\italian.iss"
#include "scripts\lang\dutch.iss"

#ifdef UNICODE
#include "scripts\lang\chinese.iss"
#include "scripts\lang\polish.iss"
#include "scripts\lang\russian.iss"
#include "scripts\lang\japanese.iss"
#endif

// shared code for installing the products
#include "scripts\products.iss"

// helper functions
#include "scripts\products\stringversion.iss"

#ifdef use_msiproduct
#include "scripts\products\msiproduct.iss"
#endif

#ifdef use_vc2019
#include "scripts\products\vcredist2019.iss"
#endif

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"

[Registry]
// Remove the registry entries left by the old installer
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\CONCEAL-GUI.exe"; Flags: deletekey
Root: HKLM; Subkey: "Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Conceal Wallet";  Flags: deletekey

[Files]
Source: "{#LicenseFile}"; DestDir: "{app}";
Source: "build\{#AppExe}"; DestDir: "{app}"; DestName: "{#AppExe}";
Source: "build\Qt5Core.dll"; DestDir: "{app}";
Source: "build\Qt5Gui.dll"; DestDir: "{app}";
Source: "build\Qt5Network.dll"; DestDir: "{app}";
Source: "build\Qt5Widgets.dll"; DestDir: "{app}";
Source: "build\platforms\qwindows.dll"; DestDir: "{app}/platforms";


[Icons]
Name: "{group}\{#AppSetupName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon
Name: "{group}\{cm:UninstallProgram,{#AppSetupName}}"; Filename: "{uninstallexe}"; Tasks: desktopicon
Name: "{commondesktop}\{#AppSetupName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppSetupName}}"; Flags: nowait postinstall skipifsilent

[InstallDelete]
// Uninstall the old version (Remove the files installed by the old installer)
Type: filesandordirs; Name: "{code:OldVersionPath}\bearer"
Type: filesandordirs; Name: "{code:OldVersionPath}\iconengines"
Type: filesandordirs; Name: "{code:OldVersionPath}\imageformats"
Type: filesandordirs; Name: "{code:OldVersionPath}\languages"
Type: filesandordirs; Name: "{code:OldVersionPath}\platforms"
Type: filesandordirs; Name: "{code:OldVersionPath}\styles"
Type: files; Name: "{code:OldVersionPath}\Conceal Wallet website.url"
Type: files; Name: "{code:OldVersionPath}\CONCEAL-GUI.exe"
Type: files; Name: "{code:OldVersionPath}\D3Dcompiler_47.dll"
Type: files; Name: "{code:OldVersionPath}\lib*.dll"
Type: files; Name: "{code:OldVersionPath}\Qt5*.dll"
Type: files; Name: "{code:OldVersionPath}\uninstall.exe"
Type: dirifempty; Name: "{code:OldVersionPath}"

// Uninstall the old version (Remove the links installed by the old installer)
Type: files; Name: "{commondesktop}\Conceal Wallet.lnk"
Type: files; Name: "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Conceal Wallet\Conceal Wallet.lnk"
Type: files; Name: "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Conceal Wallet\Conceal Wallet Website.lnk"
Type: files; Name: "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Conceal Wallet\Uninstall Conceal Wallet.lnk"
Type: dirifempty; Name: "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Conceal Wallet"

[CustomMessages]
DependenciesDir={AppSetupName} Dependencies

[Code]
var path: string;

function OldVersionPath(Param: String): String;
begin
  Result := path;
end;

function InitializeSetup(): Boolean;
var
	V: string;
begin
	#ifdef use_vc2019
		vcredist2019('14.20'); // install if version < 14.20
	#endif

	Result := true;

	if RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\App Paths\CONCEAL-GUI.exe', '', V) then
	begin
    	if MsgBox('An old version of {#AppSetupName} is installed on this computer.' + #13#10 +
		'This installer will uninstall the old version and install the new one.' + #13#10 + #13#10 +
		'Would you like to continue? ', mbConfirmation, MB_YESNO or MB_DEFBUTTON1) = IDYES then
		begin
			path := Copy (V, 0, (length(V)-16));
		end
		else
		begin
			MsgBox('Installation cancelled. You can run the installer again to install the program.', mbInformation, MB_OK);
			Result := false;
		end;
	end;
end;
