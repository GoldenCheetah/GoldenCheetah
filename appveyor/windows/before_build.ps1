Copy-Item "qwt\qwtconfig.pri.in" "qwt\qwtconfig.pri"

$gcconfig = "src\gcconfig.pri"
Copy-Item "src\gcconfig.pri.in" $gcconfig

# Function to replace text in a file
function Replace-InFile {
    param (
        [string]$File,
        [string]$Pattern,
        [string]$Replacement
    )
    (Get-Content $File) -replace $Pattern, $Replacement | Set-Content $File
}

# 1. App Name
Replace-InFile $gcconfig "#APP_NAME = " "APP_NAME = GoldenCheetah"

# 2. Config Release
Replace-InFile $gcconfig "#CONFIG \+= release" "CONFIG += release"

# 3. Disable strerror deprecation warning
Add-Content $gcconfig "`nDEFINES += _CRT_SECURE_NO_WARNINGS"

# 4. lrelease
Replace-InFile $gcconfig "#QMAKE_LRELEASE = /usr/bin/lrelease" "QMAKE_LRELEASE = lrelease"

# 5. Lex/Yacc
Replace-InFile $gcconfig "CONFIG \+= lex" "CONFIG += lex"
Replace-InFile $gcconfig "CONFIG \+= yacc" "CONFIG += yacc"
Replace-InFile $gcconfig "#QMAKE_LEX  = win_flex --wincompat" "QMAKE_LEX  = c:\libs\win_flex --wincompat"
Replace-InFile $gcconfig "#QMAKE_YACC = win_bison --file-prefix=y -t" "QMAKE_YACC = c:\libs\win_bison --file-prefix=y -t"

# Add target_predeps for lex/yacc
Add-Content $gcconfig "lex.CONFIG += target_predeps"
Add-Content $gcconfig "yacc_impl.CONFIG += target_predeps"
Add-Content $gcconfig "yacc_decl.CONFIG += target_predeps"

# 6. D2XX
Replace-InFile $gcconfig "#D2XX_INCLUDE =" "D2XX_INCLUDE = c:\libs\10_Precompiled_DLL\D2XX\CDM"
Replace-InFile $gcconfig "#D2XX_LIBS =" "D2XX_LIBS = -Lc:\libs\10_Precompiled_DLL\D2XX\CDM\Static\amd64 -lftd2xx"

# 7. ICAL
Replace-InFile $gcconfig "#ICAL_INSTALL =" "ICAL_INSTALL = c:\libs\10_Precompiled_DLL\libical64"
Replace-InFile $gcconfig "#ICAL_INCLUDE =" "ICAL_INCLUDE = c:\libs\10_Precompiled_DLL\libical64\include"
Replace-InFile $gcconfig "#ICAL_LIBS    =" "ICAL_LIBS = -Lc:\libs\10_Precompiled_DLL\libical64\lib-release -llibical-static"

# 8. USBXPRESS
Replace-InFile $gcconfig "#USBXPRESS_INSTALL =" "USBXPRESS_INSTALL = c:\libs\10_Precompiled_DLL\usbexpress_3.5.1\USBXpress\USBXpress_API\Host"
Replace-InFile $gcconfig "#USBXPRESS_INCLUDE =" "USBXPRESS_INCLUDE = c:\libs\10_Precompiled_DLL\usbexpress_3.5.1\USBXpress\USBXpress_API\Host"
Replace-InFile $gcconfig "#USBXPRESS_LIBS    =" "USBXPRESS_LIBS = -Lc:\libs\10_Precompiled_DLL\usbexpress_3.5.1\USBXpress\USBXpress_API\Host\x64 -lSiUSBXp"

# 9. LIBUSB
Replace-InFile $gcconfig "#LIBUSB_INSTALL = /usr/local" "LIBUSB_INSTALL = c:\libs\10_Precompiled_DLL\libusb-win32-bin-1.2.6.0"
Replace-InFile $gcconfig "#LIBUSB_INCLUDE =" "LIBUSB_INCLUDE = c:\libs\10_Precompiled_DLL\libusb-win32-bin-1.2.6.0\include"
Replace-InFile $gcconfig "#LIBUSB_LIBS    =" "LIBUSB_LIBS = -Lc:\libs\10_Precompiled_DLL\libusb-win32-bin-1.2.6.0\lib\msvc_x64 -llibusb"

# 10. SAMPLERATE
Replace-InFile $gcconfig "#SAMPLERATE_INSTALL = /usr/local" "SAMPLERATE_INSTALL = c:\libs\10_Precompiled_DLL\libsamplerate64"
Replace-InFile $gcconfig "#SAMPLERATE_INCLUDE = /usr/local/include" "SAMPLERATE_INCLUDE = c:\libs\10_Precompiled_DLL\libsamplerate64\include"
Replace-InFile $gcconfig "#SAMPLERATE_LIBS = /usr/local/lib/libsamplerate.a" "SAMPLERATE_LIBS = -Lc:\libs\10_Precompiled_DLL\libsamplerate64\lib -llibsamplerate-0"

# 11. HTPATH
Replace-InFile $gcconfig "#HTPATH = ../httpserver" "HTPATH = ../httpserver"

# 12. Video
Replace-InFile $gcconfig "DEFINES \+= GC_VIDEO_NONE" "#DEFINES += GC_VIDEO_NONE"
Replace-InFile $gcconfig "#DEFINES \+= GC_VIDEO_QT6" "DEFINES += GC_VIDEO_QT6"

# 13. R Support
Replace-InFile $gcconfig "#DEFINES \+= GC_WANT_R" "DEFINES += GC_WANT_R"

# 14. Python Support
Replace-InFile $gcconfig "#DEFINES \+= GC_WANT_PYTHON" "DEFINES += GC_WANT_PYTHON"
Replace-InFile $gcconfig "#PYTHONINCLUDES =" "PYTHONINCLUDES = -ICore -I`"c:\python311-x64\include`""
Replace-InFile $gcconfig "#PYTHONLIBS =" "PYTHONLIBS = -L`"c:\python311-x64\libs`" -lpython311"

# 15. GSL Support
Replace-InFile $gcconfig "#  GSL_INCLUDES = c:\\vcpkg\\installed\\x64-windows\\include" "GSL_INCLUDES = c:\tools\vcpkg\installed\x64-windows\include"
Replace-InFile $gcconfig "#  GSL_LIBS = -LC:\\vcpkg\\installed\\x64-windows\\lib -lgsl -lgslcblas" "GSL_LIBS = -Lc:\tools\vcpkg\installed\x64-windows\lib -lgsl -lgslcblas"

# 16. CloudDB
Replace-InFile $gcconfig "#CloudDB = active" "CloudDB = active"

# 17. Train Robot
Replace-InFile $gcconfig "#DEFINES \+= GC_WANT_ROBOT" "DEFINES += GC_WANT_ROBOT"

# 18. TrainerDay
Replace-InFile $gcconfig "#DEFINES \+= GC_WANT_TRAINERDAY_API" "DEFINES += GC_WANT_TRAINERDAY_API"
Replace-InFile $gcconfig "#DEFINES \+= GC_TRAINERDAY_API_PAGESIZE=25" "DEFINES += GC_TRAINERDAY_API_PAGESIZE=25"

# 19. Math Defines and Nominmax
Add-Content $gcconfig "DEFINES += _MATH_DEFINES_DEFINED"
Replace-InFile $gcconfig "#DEFINES \+= NOMINMAX" "DEFINES += NOMINMAX"

Add-Content $gcconfig "CONFIG += lex"
Add-Content $gcconfig "CONFIG += yacc"
