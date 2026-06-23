#ifndef AppVersion
  #define AppVersion "0.0.1"
#endif
#ifndef OutputBaseFilename
  #define OutputBaseFilename "SonarPractice_0.0.1_Alpha_Setup"
#endif

[Setup]
AppName=SonarPractice
AppVersion={#AppVersion}
OutputBaseFilename={#OutputBaseFilename}
DefaultDirName={autopf}\SonarPractice
ArchitecturesInstallIn64BitMode=x64compatible

[Files]
Source: "deploy\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs

[Tasks]
Name: "desktopicon"; Description: "Create a desktop icon"; GroupDescription: "Additional icons:"; Flags: checkedonce

[Icons]
Name: "{userdesktop}\SonarPractice"; Filename: "{app}\SonarPractice.exe"; Tasks: desktopicon

Name: "{autoprograms}\SonarPractice"; Filename: "{app}\SonarPractice.exe"; WorkingDir: "{app}"
