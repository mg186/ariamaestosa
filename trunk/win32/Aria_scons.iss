; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=Aria Maestosa
AppVerName=Aria Maestosa 1.4.4
VersionInfoVersion=1.4.4
AppPublisher=
AppPublisherURL=http://ariamaestosa.sourceforge.net/
AppSupportURL=
AppUpdatesURL=
DefaultDirName={pf}\Aria Maestosa
DefaultGroupName=Aria Maestosa
WizardImageFile=WizardImage.bmp
WizardSmallImageFile=WizardSmallImage.bmp
AllowNoIcons=yes
OutputDir=.
Uninstallable=yes
WindowVisible=no
AppCopyright=Copyright Auria
OutputBaseFilename=AriaMaestosaSetup
UninstallDisplayIcon={app}\Aria.exe
LicenseFile=..\license.txt
DisableStartupPrompt=yes
ChangesAssociations=yes






[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}";
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\Aria.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Resources\*"; Excludes: "*.icns"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs
Source: "..\international\*.mo"; DestDir: "{app}\Languages"; Flags: ignoreversion recursesubdirs
Source: "..\license.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\MinGW-4.5.0\bin\libgcc_s_dw2-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\MinGW-4.5.0\bin\mingwm10.dll"; DestDir: "{app}"; Flags: ignoreversion


[Icons]
Name: "{group}\Aria Maestosa"; Filename: "{app}\Aria.exe"
Name: "{group}\{cm:ProgramOnTheWeb,Aria Maestosa}"; Filename: "http://ariamaestosa.sourceforge.net/"
Name: "{group}\Manual"; Filename: "http://ariamaestosa.sourceforge.net/man.html"
Name: "{group}\{cm:UninstallProgram,Aria Maestosa}"; Filename: "{app}\unins000.exe"
Name: "{userdesktop}\Aria Maestosa"; Filename: "{app}\Aria.exe";  Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\Aria"; Filename: "{app}\Aria.exe"; Tasks: quicklaunchicon

[Registry]
Root: HKCR; Subkey: ".aria"; ValueType: string; ValueName: ""; ValueData: "Aria.Document"; Flags: uninsdeletevalue
;".myp" is the extension we're associating. "MyProgramFile" is the internal name for the file type as stored in the registry.
;Make sure you use a unique name for this so you don't inadvertently overwrite another application's registry key.

Root: HKCR; Subkey: "Aria.Document"; ValueType: string; ValueName: ""; ValueData: "Document Aria"; Flags: uninsdeletekey
;"My Program File" above is the name for the file type as shown in Explorer.

Root: HKCR; Subkey: "Aria.Document\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\Aria.exe,1"
;"DefaultIcon" is the registry key that specifies the filename containing the icon to associate with the file type.
; ",0" tells Explorer to use the first icon from MYPROG.EXE. (",1" would mean the second icon.)

Root: HKCR; Subkey: "Aria.Document\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\Aria.exe"" ""%1"""
;"shell\open\command" is the registry key that specifies the program to execute when a file of the type is double-clicked in Explorer.
; The surrounding quotes are in the command line so it handles long filenames correctly.



[Run]
Filename: "{app}\Aria.exe"; Description: "{cm:LaunchProgram,Aria Maestosa}";  Flags: nowait postinstall skipifsilent



