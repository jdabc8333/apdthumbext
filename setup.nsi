; Script generated by the HM NIS Edit Script Wizard.

!include Library.nsh
!include x64.nsh

RequestExecutionLevel admin

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "ApdThumbExt"
!define PRODUCT_VERSION "1.0"
!define PRODUCT_PUBLISHER "teraapi"
!define PRODUCT_WEB_SITE "http://teraapi.com/"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\ApdThumbExt"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\classic-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\classic-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
;!insertmacro MUI_PAGE_LICENSE "${NSISDIR}\License.txt"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Japanese"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${PRODUCT_NAME}-${PRODUCT_VERSION}.exe"
!ifdef DngrDir
 InstallDir ${DngrDir}
!else ifdef DngrUnDir
 InstallDir ${DngrUnDir}
!else
 InstallDir "$PROGRAMFILES\ApdThumbExt"
!endif
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite on
  CreateDirectory "$SMPROGRAMS\ApdThumbExt"

${If} ${RunningX64}
!define LIBRARY_X64
!define LIBRARY_SHELL_EXTENSION
!define LIBRARY_COM
!insertmacro InstallLib REGDLL NOTSHARED REBOOT_PROTECTED "..\x64\Release64\ApdThumbExt64.dll" "$INSTDIR\ApdThumbExt64.dll" "$INSTDIR"
!undef LIBRARY_X64
!undef LIBRARY_SHELL_EXTENSION
!undef LIBRARY_COM

${Else}
!define LIBRARY_SHELL_EXTENSION
!define LIBRARY_COM
  !insertmacro InstallLib REGDLL NOTSHARED REBOOT_PROTECTED "..\Release\ApdThumbExt.dll" "$INSTDIR\ApdThumbExt.dll" "$INSTDIR"
!undef LIBRARY_SHELL_EXTENSION
!undef LIBRARY_COM
${Endif}
SectionEnd

Section -AdditionalIcons
  CreateShortCut "$SMPROGRAMS\ApdThumbExt\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\ApdThumbExt.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\ApdThumbExt.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd


Function .onInit
${If} ${RunningX64}
  SetRegView 64
  !define DngrDir "$PROGRAMFILES64\ApdThumbExt"
  StrCpy $INSTDIR "$PROGRAMFILES64\ApdThumbExt"
${Else}
  SetRegView 32
  StrCpy $INSTDIR "$PROGRAMFILES\ApdThumbExt"
${Endif}

  ReadRegStr $R0 HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" \
  "UninstallString"
  StrCmp $R0 "" done
 
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to remove the \
  previous version or `Cancel` to cancel this upgrade." \
  IDOK uninst
  Abort
  
;Run the uninstaller
uninst:
  ClearErrors
  ExecWait '$R0 _?=$INSTDIR' ;Do not copy the uninstaller to a temp file
;  Exec $INSTDIR\uninst.exe ; instead of the ExecWait line
 
  IfErrors no_remove_uninstaller
    ;You can either use Delete /REBOOTOK in the uninstaller or add some code
    ;here to remove the uninstaller. Use a registry key to check
    ;whether the user has chosen to uninstall. If you are using an uninstaller
    ;components page, make sure all sections are uninstalled.
  no_remove_uninstaller:
  
done:
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
${If} ${RunningX64}
  !define DngrUnDir "$PROGRAMFILES64\ApdThumbExt"
  StrCpy $INSTDIR "$PROGRAMFILES64\ApdThumbExt"
  SetRegView 64
${Else}
  SetRegView 32
  StrCpy $INSTDIR "$PROGRAMFILES\ApdThumbExt"
${Endif}
FunctionEnd

Section Uninstall
${If} ${RunningX64}
  !define LIBRARY_X64
  !define LIBRARY_SHELL_EXTENSION
  !define LIBRARY_COM
  !insertmacro UninstallLib REGDLL NOTSHARED REBOOT_PROTECTED "$INSTDIR\ApdThumbExt64.dll"
  !undef LIBRARY_X64
  !undef LIBRARY_SHELL_EXTENSION
  !undef LIBRARY_COM
${Else}
  !define LIBRARY_SHELL_EXTENSION
  !define LIBRARY_COM
  !insertmacro UninstallLib REGDLL NOTSHARED REBOOT_PROTECTED "$INSTDIR\ApdThumbExt.dll"
  !undef LIBRARY_SHELL_EXTENSION
  !undef LIBRARY_COM
${Endif}
  Delete "$INSTDIR\uninst.exe"
${If} ${RunningX64}
  Delete "$INSTDIR\ApdThumbExt64.dll"
${Else}
  Delete "$INSTDIR\ApdThumbExt.dll"
${Endif}

  Delete "$SMPROGRAMS\ApdThumbExt\Uninstall.lnk"

  RMDir "$SMPROGRAMS\ApdThumbExt"
  RMDir "$INSTDIR"

${If} ${RunningX64}
  DeleteRegKey HKCU "Software\Classes\CLSID\{0A18F5AA-0057-4B78-8DD9-8EAFDB078393}"
  DeleteRegKey HKCU "Software\Classes\.apd\ShellEx\{e357fccd-a995-4576-b01f-234630154e96}"
${Endif}

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
