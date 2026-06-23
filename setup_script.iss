[Setup]
AppName=SonarPractice
AppVersion=0.0.1
OutputBaseFilename=SonarPractice_0.0.1_Alpha_Setup
DefaultDirName={autopf}\SonarPractice
ArchitecturesInstallIn64BitMode=x64compatible

[Files]
Source: "deploy\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs
