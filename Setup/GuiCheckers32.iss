; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=GuiCheckers
AppVerName=GuiCheckers version 1.11
AppPublisher=Ed Gilbert
DefaultDirName={pf}\CheckerBoard\engines
AppendDefaultDirName=no
DefaultGroupName=GuiCheckers
DisableProgramGroupPage=yes
SourceDir=Source
OutputDir=..\Output
OutputBaseFilename=GuiCheckers32Setup.111
Compression=lzma/ultra
SolidCompression=yes
Uninstallable=no
DirExistsWarning=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "GuiCheckers.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "opening.gbk"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\db_dtw\*.*"; DestDir: "{app}\..\db_dtw"; Flags: ignoreversion
