[Setup]
AppName=Frequalizer
AppVersion=1.1.0
Uninstallable=no
SourceDir="{#BuildDir}"
DefaultDirName="{commoncf64}\VST3"
DefaultGroupName=Frequalizer
OutputBaseFilename=Frequalizer_WIN
[Files]
Source: "VST3\Frequalizer.vst3"; DestDir: {app}; Flags: recursesubdirs
;Source: "VST\Frequalizer.dll"; DestDir: {cf64}\VST
;Source: "AAX\Frequalizer.aaxplugin\*"; DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\Frequalizer.aaxplugin"; Flags: recursesubdirs
