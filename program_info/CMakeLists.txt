# heavily based off https://github.com/PrismLauncher/PrismLauncher/blob/6cf70e75064171c0c7d756d5a111e76a3360d513/program_info/CMakeLists.txt

set(App_CommonName "CaptureCardRelay")
set(App_DisplayName "Capture Card Relay")
set(App_ID "com.coolguy1842.CaptureCardRelay")
set(App_Git "https://github.com/coolguy1842/CaptureCardRelay")

set(App_Name "${App_CommonName}" PARENT_SCOPE)
set(App_DisplayName "${App_DisplayName}" PARENT_SCOPE)
set(App_Description "${App_DisplayName}" PARENT_SCOPE)
set(App_ENVName "CAPTURECARDRELAY" PARENT_SCOPE)
set(App_ID "${App_ID}" PARENT_SCOPE)
set(App_Git "${App_Git}" PARENT_SCOPE)

set(App_SVGFileName "${App_ID}.svg")
set(App_Copyright "© 2025-2026 CaptureCardRelay Contributors")
set(App_Copyright "${App_Copyright}" PARENT_SCOPE)
set(App_SVGFileName "${App_SVGFileName}" PARENT_SCOPE)

set(App_Desktop "program_info/${App_ID}.desktop" PARENT_SCOPE)
set(App_PNG_256 "program_info/${App_ID}_256.png" PARENT_SCOPE)
set(App_SVG "program_info/${App_SVGFileName}" PARENT_SCOPE)
set(App_Branding_ICO "program_info/${App_CommonName}.ico")
set(App_Branding_ICO "${App_Branding_ICO}" PARENT_SCOPE)
set(App_Branding_WindowsRC "program_info/${App_CommonName}.rc" PARENT_SCOPE)
set(App_Authors "coolguy1842")

configure_file(${App_ID}.desktop.in ${App_ID}.desktop)
configure_file(${App_CommonName}.ico ${App_CommonName}.ico COPYONLY)
configure_file(${App_SVGFileName} ${App_SVGFileName} COPYONLY)
configure_file(${App_CommonName}.rc.in ${App_CommonName}.rc @ONLY)
configure_file(${App_CommonName}.manifest.in ${App_CommonName}.manifest @ONLY)

if(MSVC)
    set(App_MSVC_Redist_NSIS_Section [=[
!ifdef haveNScurl
Section "Visual Studio Runtime"
    Var /GLOBAL vc_redist_exe
    ${If} ${IsNativeARM64}
        StrCpy $vc_redist_exe "vc_redist.arm64.exe"
    ${Else}
        StrCpy $vc_redist_exe "vc_redist.x64.exe"
    ${EndIf}
    DetailPrint 'Downloading Microsoft Visual C++ Redistributable...'
    NScurl::http GET "https://aka.ms/vs/17/release/$vc_redist_exe" "$INSTDIR\vc_redist\$vc_redist_exe" /INSIST /CANCEL /Zone.Identifier /END
    Pop $0
    ${If} $0 == "OK"
        DetailPrint "Download successful"
        ExecWait "$INSTDIR\vc_redist\$vc_redist_exe /install /passive /norestart"
    ${Else}
        DetailPrint "Download failed with error $0"
    ${EndIf}
SectionEnd
!endif
]=])
endif()

configure_file(win_install.nsi.in win_install.nsi @ONLY)