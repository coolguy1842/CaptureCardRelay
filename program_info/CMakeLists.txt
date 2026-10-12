# heavily based off https://github.com/PrismLauncher/PrismLauncher/blob/6cf70e75064171c0c7d756d5a111e76a3360d513/program_info/CMakeLists.txt

set(App_CommonName "CaptureCardRelay")
set(App_DisplayName "Capture Card Relay")
set(App_ID "com.coolguy1842.CaptureCardRelay")
set(App_Git "https://github.com/coolguy1842/CaptureCardRelay")

set(App_Name "${App_CommonName}" PARENT_SCOPE)
set(App_DisplayName "${App_DisplayName}" PARENT_SCOPE)
set(App_ENVName "CAPTURECARDRELAY" PARENT_SCOPE)
set(App_ID "${App_ID}" PARENT_SCOPE)
set(App_Git "${App_Git}" PARENT_SCOPE)

set(App_Desktop "program_info/${App_ID}.desktop")
set(App_PNGFileName "${App_ID}.png")
set(App_PNG "program_info/${App_PNGFileName}")

set(App_Desktop "${App_Desktop}" PARENT_SCOPE)
set(App_PNG "${App_PNG}" PARENT_SCOPE)

configure_file(${App_ID}.desktop.in ${App_ID}.desktop)
configure_file(${App_PNGFileName} ${App_PNGFileName} COPYONLY)