; Johnny Castaway Reborn - instalador Windows (NSIS)
; Nao empacota RESOURCE.MAP/RESOURCE.001 (copyright Sierra/Dynamix) -
; o passo de recursos corre um script que os extrai a partir de um
; ficheiro que o utilizador ja tem (imagem de disquete original).

!include "MUI2.nsh"

Name "Johnny Castaway Reborn"
OutFile "..\JohnnyCastaway-Setup.exe"
InstallDir "$PROGRAMFILES64\Johnny Castaway Reborn"
InstallDirRegKey HKLM "Software\JohnnyCastawayReborn" "InstallDir"
RequestExecutionLevel admin

!define MUI_ABORTWARNING
!define MUI_LICENSEPAGE_RADIOBUTTONS
!define MUI_LICENSEPAGE_RADIOBUTTONS_TEXT_ACCEPT "Aceito / I accept / Acepto"
!define MUI_LICENSEPAGE_RADIOBUTTONS_TEXT_DECLINE "Nao aceito / I decline / No acepto"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "DISCLAIMER.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_TITLE_3LINES
!define MUI_FINISHPAGE_RUN "$INSTDIR\jc_reborn.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Correr o Johnny Castaway agora (em janela)"
!define MUI_FINISHPAGE_RUN_PARAMETERS "window"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\Tutorial.pdf"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Abrir o Tutorial (PDF)"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "PortugueseBR"

Section "Motor Johnny Castaway (obrigatorio)" SecCore
    SectionIn RO
    SetOutPath "$INSTDIR"
    File "..\dist_publico\jc_reborn.exe"
    File "..\dist_publico\SDL2.dll"
    File "..\dist_publico\deark.exe"
    File "..\dist_publico\LEIA-ME.txt"
    File "DISCLAIMER.txt"
    File "Tutorial.pdf"
    File "Instalar-Recursos.ps1"

    ; Copia com extensao .scr, para poder ser usada diretamente como
    ; protetor de ecra do Windows
    CopyFiles "$INSTDIR\jc_reborn.exe" "$INSTDIR\jc_reborn.scr"

    WriteRegStr HKLM "Software\JohnnyCastawayReborn" "InstallDir" "$INSTDIR"

    ; Entrada em "Aplicativos instalados"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\JohnnyCastawayReborn" "DisplayName" "Johnny Castaway Reborn"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\JohnnyCastawayReborn" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\JohnnyCastawayReborn" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\JohnnyCastawayReborn" "Publisher" "Lucas Yoshiki Amaral"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\JohnnyCastawayReborn" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\JohnnyCastawayReborn" "NoRepair" 1

    WriteUninstaller "$INSTDIR\Uninstall.exe"

    CreateDirectory "$SMPROGRAMS\Johnny Castaway Reborn"
    CreateShortcut "$SMPROGRAMS\Johnny Castaway Reborn\Johnny Castaway.lnk" "$INSTDIR\jc_reborn.exe" "window"
    CreateShortcut "$SMPROGRAMS\Johnny Castaway Reborn\Configurar.lnk" "$INSTDIR\jc_reborn.scr" "/c"
    CreateShortcut "$SMPROGRAMS\Johnny Castaway Reborn\Tutorial (PDF).lnk" "$INSTDIR\Tutorial.pdf"
    CreateShortcut "$SMPROGRAMS\Johnny Castaway Reborn\Obter ficheiros do jogo original.lnk" "powershell.exe" '-NoProfile -ExecutionPolicy Bypass -File "$INSTDIR\Instalar-Recursos.ps1"'
    CreateShortcut "$SMPROGRAMS\Johnny Castaway Reborn\Aviso legal.lnk" "$INSTDIR\DISCLAIMER.txt"
    CreateShortcut "$SMPROGRAMS\Johnny Castaway Reborn\Desinstalar.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Obter ficheiros do jogo original agora" SecResources
    ; RESOURCE.MAP/RESOURCE.001 sao propriedade da Sierra/Dynamix e nao
    ; vem incluidos neste instalador. Este passo pede o ficheiro que o
    ; utilizador ja descarregou (a imagem da disquete original) e
    ; extrai/descomprime tudo automaticamente.
    ExecWait 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$INSTDIR\Instalar-Recursos.ps1" -TargetDir "$INSTDIR"'
SectionEnd

Section "Protetor de ecra do Windows" SecScr
    ; Copia para System32 para aparecer na lista de protetores de ecra
    ; do Windows (Definicoes > Personalizacao > Ecra de bloqueio). So
    ; aparece quando o PC fica inativo.
    SetOutPath "$WINDIR\System32"
    CopyFiles "$INSTDIR\jc_reborn.scr" "$WINDIR\System32\JohnnyCastawayReborn.scr"
    CopyFiles "$INSTDIR\SDL2.dll" "$WINDIR\System32\SDL2.dll"
    IfFileExists "$INSTDIR\RESOURCE.MAP" 0 +2
        CopyFiles "$INSTDIR\RESOURCE.MAP" "$WINDIR\System32\RESOURCE.MAP"
    IfFileExists "$INSTDIR\RESOURCE.001" 0 +2
        CopyFiles "$INSTDIR\RESOURCE.001" "$WINDIR\System32\RESOURCE.001"

    WriteRegStr HKCU "Control Panel\Desktop" "SCRNSAVE.EXE" "$WINDIR\System32\JohnnyCastawayReborn.scr"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "O programa em si. Sempre instalado."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecResources} "Pede a imagem da disquete original e extrai os ficheiros automaticamente. Podes fazer isto depois, pelo Menu Iniciar."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecScr} "PROTETOR DE ECRA: so aparece quando o PC fica inativo. Desaparece ao mexer no rato/teclado. Para live wallpaper (fundo animado enquanto trabalhas), ver Tutorial.pdf - secao Wallpaper Engine."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Uninstall"
    Delete "$INSTDIR\jc_reborn.exe"
    Delete "$INSTDIR\jc_reborn.scr"
    Delete "$INSTDIR\SDL2.dll"
    Delete "$INSTDIR\deark.exe"
    Delete "$INSTDIR\LEIA-ME.txt"
    Delete "$INSTDIR\DISCLAIMER.txt"
    Delete "$INSTDIR\Tutorial.pdf"
    Delete "$INSTDIR\Instalar-Recursos.ps1"
    Delete "$INSTDIR\RESOURCE.MAP"
    Delete "$INSTDIR\RESOURCE.001"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"

    Delete "$WINDIR\System32\JohnnyCastawayReborn.scr"
    Delete "$WINDIR\System32\RESOURCE.MAP"
    Delete "$WINDIR\System32\RESOURCE.001"

    Delete "$SMPROGRAMS\Johnny Castaway Reborn\Johnny Castaway.lnk"
    Delete "$SMPROGRAMS\Johnny Castaway Reborn\Configurar.lnk"
    Delete "$SMPROGRAMS\Johnny Castaway Reborn\Tutorial (PDF).lnk"
    Delete "$SMPROGRAMS\Johnny Castaway Reborn\Obter ficheiros do jogo original.lnk"
    Delete "$SMPROGRAMS\Johnny Castaway Reborn\Aviso legal.lnk"
    Delete "$SMPROGRAMS\Johnny Castaway Reborn\Desinstalar.lnk"
    RMDir "$SMPROGRAMS\Johnny Castaway Reborn"

    ; Versoes antigas criavam estes - remover se ainda existirem de uma
    ; instalacao anterior.
    Delete "$SMPROGRAMS\Johnny Castaway Reborn\Live Wallpaper (nativo)\Iniciar Live Wallpaper.lnk"
    Delete "$SMPROGRAMS\Johnny Castaway Reborn\Live Wallpaper (nativo)\Parar Live Wallpaper.lnk"
    Delete "$SMPROGRAMS\Johnny Castaway Reborn\Live Wallpaper (nativo)\Diagnostico do Live Wallpaper.lnk"
    RMDir "$SMPROGRAMS\Johnny Castaway Reborn\Live Wallpaper (nativo)"
    Delete "$DESKTOP\Iniciar Live Wallpaper (Johnny Castaway).lnk"
    DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "JohnnyCastawayRebornWallpaper"

    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\JohnnyCastawayReborn"
    DeleteRegKey HKLM "Software\JohnnyCastawayReborn"
SectionEnd
