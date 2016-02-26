
;!include "MUI2.nsh"
;!define MUI_ICON "..\images\gc.ico" ;Value

  
Name "Golden Cheetah"

Icon "..\images\gc.ico"

SetCompressor /SOLID lzma

OutFile "GoldenCheetahInstall.exe"

InstallDir $PROGRAMFILES\GoldenCheetah

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\GoldenCheetah" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages

  ;!insertmacro MUI_PAGE_WELCOME
  ;!insertmacro MUI_PAGE_LICENSE "${NSISDIR}\Docs\Modern UI\License.txt"
  ;!insertmacro MUI_PAGE_COMPONENTS
  ;!insertmacro MUI_PAGE_DIRECTORY
  ;!insertmacro MUI_PAGE_INSTFILES
  
  ;!insertmacro MUI_UNPAGE_CONFIRM
  ;!insertmacro MUI_UNPAGE_INSTFILES
  
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------
;Languages
 
  ;!insertmacro MUI_LANGUAGE "English"
;--------------------------------

; The stuff to install
Section "Golden Cheetah (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "..\release\GoldenCheetah.exe"

  ; MingW2 libs 4.8.4 uses MingW32 with GCC 4.4
  ; including all the runtime
  File "..\release\mingwm10.dll"
  File "..\release\libgcc_s_dw2-1.dll"
  File "..\release\libcharset-1.dll"
  File "..\release\libexpat-1.dll"
  File "..\release\libgcc_s_dw2-1.dll"
  File "..\release\libgmp-3.dll"
  File "..\release\libgmpxx-4.dll"
  File "..\release\libgomp-1.dll"
  File "..\release\libiconv-2.dll"
  File "..\release\libmpfr-1.dll"
  File "..\release\libssp-0.dll"
  File "..\release\libstdc++-6.dll"
  File "..\release\mingwm10.dll"
  File "..\release\msvcm90.dll"
  File "..\release\msvcp90.dll"
  File "..\release\msvcr90.dll"
 

  ;
  ; QT Libs
  ;
  File "..\release\QtCore4.dll"
  File "..\release\QtScript4.dll"
  File "..\release\QtGui4.dll"
  File "..\release\QtSql4.dll"
  File "..\release\QtXml4.dll"
  File "..\release\QtWebKit4.dll"
  File "..\release\QtXmlPatterns4.dll"
  File "..\release\phonon4.dll"
  File "..\release\QtNetwork4.dll"
  File "..\release\QtOpenGL4.dll"

  ; ssl
  File "..\release\libeay32.dll"
  File "..\release\ssleay32.dll"

  ; search / data filters
  File "..\release\libclucene-core.dll"
  File "..\release\libclucene-shared.dll"

  ; 3d plotting
  File "..\release\qwtplot3d.dll"
  
  ; USB1 and USB2 Garmin ANT support
  File "..\release\SiUSBXp.dll"
  File "..\release\libusb0.dll"

  ; vlc is a mighty pain in the ass
  File "..\release\axvlc.dll"
  File "..\release\libvlc.dll"
  File "..\release\libvlccore.dll"

  ; SQLite for metric and training DB
  SetOutPath $INSTDIR\sqldrivers
  File "..\release\qsqlite4.dll"

  ; jpegs for Google maps
  SetOutPath $INSTDIR\imageformats
  File "..\release\qjpeg4.dll"

  ; the rest of the VLC libs
  SetOutPath $INSTDIR\plugins
  SetOutPath $INSTDIR\plugins\access
  File "..\release\plugins\access\libaccess_attachment_plugin.dll"
  File "..\release\plugins\access\libaccess_bd_plugin.dll"
  File "..\release\plugins\access\libaccess_ftp_plugin.dll"
  File "..\release\plugins\access\libaccess_http_plugin.dll"
  File "..\release\plugins\access\libaccess_imem_plugin.dll"
  File "..\release\plugins\access\libaccess_mms_plugin.dll"
  File "..\release\plugins\access\libaccess_rar_plugin.dll"
  File "..\release\plugins\access\libaccess_realrtsp_plugin.dll"
  File "..\release\plugins\access\libaccess_smb_plugin.dll"
  File "..\release\plugins\access\libaccess_tcp_plugin.dll"
  File "..\release\plugins\access\libaccess_udp_plugin.dll"
  File "..\release\plugins\access\libaccess_vdr_plugin.dll"
  File "..\release\plugins\access\libcdda_plugin.dll"
  File "..\release\plugins\access\libdshow_plugin.dll"
  File "..\release\plugins\access\libdtv_plugin.dll"
  File "..\release\plugins\access\libdvdnav_plugin.dll"
  File "..\release\plugins\access\libdvdread_plugin.dll"
  File "..\release\plugins\access\libfilesystem_plugin.dll"
  File "..\release\plugins\access\libidummy_plugin.dll"
  File "..\release\plugins\access\liblibbluray_plugin.dll"
  File "..\release\plugins\access\librtp_plugin.dll"
  File "..\release\plugins\access\libscreen_plugin.dll"
  File "..\release\plugins\access\libsdp_plugin.dll"
  File "..\release\plugins\access\libstream_filter_rar_plugin.dll"
  File "..\release\plugins\access\libvcd_plugin.dll"
  File "..\release\plugins\access\libzip_plugin.dll"

  SetOutPath $INSTDIR\plugins\audio_filter
  File "..\release\plugins\audio_filter\liba52tofloat32_plugin.dll"
  File "..\release\plugins\audio_filter\liba52tospdif_plugin.dll"
  File "..\release\plugins\audio_filter\libaudio_format_plugin.dll"
  File "..\release\plugins\audio_filter\libaudiobargraph_a_plugin.dll"
  File "..\release\plugins\audio_filter\libchorus_flanger_plugin.dll"
  File "..\release\plugins\audio_filter\libcompressor_plugin.dll"
  File "..\release\plugins\audio_filter\libconverter_fixed_plugin.dll"
  File "..\release\plugins\audio_filter\libdolby_surround_decoder_plugin.dll"
  File "..\release\plugins\audio_filter\libdtstofloat32_plugin.dll"
  File "..\release\plugins\audio_filter\libdtstospdif_plugin.dll"
  File "..\release\plugins\audio_filter\libequalizer_plugin.dll"
  File "..\release\plugins\audio_filter\libheadphone_channel_mixer_plugin.dll"
  File "..\release\plugins\audio_filter\libkaraoke_plugin.dll"
  File "..\release\plugins\audio_filter\libmono_plugin.dll"
  File "..\release\plugins\audio_filter\libmpgatofixed32_plugin.dll"
  File "..\release\plugins\audio_filter\libnormvol_plugin.dll"
  File "..\release\plugins\audio_filter\libparam_eq_plugin.dll"
  File "..\release\plugins\audio_filter\libsamplerate_plugin.dll"
  File "..\release\plugins\audio_filter\libscaletempo_plugin.dll"
  File "..\release\plugins\audio_filter\libsimple_channel_mixer_plugin.dll"
  File "..\release\plugins\audio_filter\libspatializer_plugin.dll"
  File "..\release\plugins\audio_filter\libspeex_resampler_plugin.dll"
  File "..\release\plugins\audio_filter\libtrivial_channel_mixer_plugin.dll"
  File "..\release\plugins\audio_filter\libugly_resampler_plugin.dll"

  SetOutPath $INSTDIR\plugins\audio_output

  File "..\release\plugins\audio_output\libadummy_plugin.dll"
  File "..\release\plugins\audio_output\libamem_plugin.dll"
  File "..\release\plugins\audio_output\libaout_directx_plugin.dll"
  File "..\release\plugins\audio_output\libaout_file_plugin.dll"
  File "..\release\plugins\audio_output\libwaveout_plugin.dll"

  SetOutPath $INSTDIR\plugins\codec
  File "..\release\plugins\codec\liba52_plugin.dll"
  File "..\release\plugins\codec\libadpcm_plugin.dll"
  File "..\release\plugins\codec\libaes3_plugin.dll"
  File "..\release\plugins\codec\libaraw_plugin.dll"
  File "..\release\plugins\codec\libavcodec_plugin.dll"
  File "..\release\plugins\codec\libcc_plugin.dll"
  File "..\release\plugins\codec\libcdg_plugin.dll"
  File "..\release\plugins\codec\libcrystalhd_plugin.dll"
  File "..\release\plugins\codec\libcvdsub_plugin.dll"
  File "..\release\plugins\codec\libddummy_plugin.dll"
  File "..\release\plugins\codec\libdmo_plugin.dll"
  File "..\release\plugins\codec\libdts_plugin.dll"
  File "..\release\plugins\codec\libdvbsub_plugin.dll"
  File "..\release\plugins\codec\libedummy_plugin.dll"
  File "..\release\plugins\codec\libfaad_plugin.dll"
  File "..\release\plugins\codec\libflac_plugin.dll"
  File "..\release\plugins\codec\libfluidsynth_plugin.dll"
  File "..\release\plugins\codec\libkate_plugin.dll"
  File "..\release\plugins\codec\liblibass_plugin.dll"
  File "..\release\plugins\codec\liblibmpeg2_plugin.dll"
  File "..\release\plugins\codec\liblpcm_plugin.dll"
  File "..\release\plugins\codec\libmpeg_audio_plugin.dll"
  File "..\release\plugins\codec\libopus_plugin.dll"
  File "..\release\plugins\codec\libpng_plugin.dll"
  File "..\release\plugins\codec\libquicktime_plugin.dll"
  File "..\release\plugins\codec\librawvideo_plugin.dll"
  File "..\release\plugins\codec\libschroedinger_plugin.dll"
  File "..\release\plugins\codec\libspeex_plugin.dll"
  File "..\release\plugins\codec\libspudec_plugin.dll"
  File "..\release\plugins\codec\libstl_plugin.dll"
  File "..\release\plugins\codec\libsubsdec_plugin.dll"
  File "..\release\plugins\codec\libsubsusf_plugin.dll"
  File "..\release\plugins\codec\libsvcdsub_plugin.dll"
  File "..\release\plugins\codec\libt140_plugin.dll"
  File "..\release\plugins\codec\libtheora_plugin.dll"
  File "..\release\plugins\codec\libtwolame_plugin.dll"
  File "..\release\plugins\codec\libvorbis_plugin.dll"
  File "..\release\plugins\codec\libx264_plugin.dll"
  File "..\release\plugins\codec\libzvbi_plugin.dll"

  SetOutPath $INSTDIR\plugins\demux
  File "..\release\plugins\demux\libaiff_plugin.dll"
  File "..\release\plugins\demux\libasf_plugin.dll"
  File "..\release\plugins\demux\libau_plugin.dll"
  File "..\release\plugins\demux\libavi_plugin.dll"
  File "..\release\plugins\demux\libdemux_cdg_plugin.dll"
  File "..\release\plugins\demux\libdemux_stl_plugin.dll"
  File "..\release\plugins\demux\libdemuxdump_plugin.dll"
  File "..\release\plugins\demux\libdirac_plugin.dll"
  File "..\release\plugins\demux\libes_plugin.dll"
  File "..\release\plugins\demux\libflacsys_plugin.dll"
  File "..\release\plugins\demux\libgme_plugin.dll"
  File "..\release\plugins\demux\libh264_plugin.dll"
  File "..\release\plugins\demux\libimage_plugin.dll"
  File "..\release\plugins\demux\liblive555_plugin.dll"
  File "..\release\plugins\demux\libmjpeg_plugin.dll"
  File "..\release\plugins\demux\libmkv_plugin.dll"
  File "..\release\plugins\demux\libmod_plugin.dll"
  File "..\release\plugins\demux\libmp4_plugin.dll"
  File "..\release\plugins\demux\libmpc_plugin.dll"
  File "..\release\plugins\demux\libmpgv_plugin.dll"
  File "..\release\plugins\demux\libnsc_plugin.dll"
  File "..\release\plugins\demux\libnsv_plugin.dll"
  File "..\release\plugins\demux\libnuv_plugin.dll"
  File "..\release\plugins\demux\libogg_plugin.dll"
  File "..\release\plugins\demux\libplaylist_plugin.dll"
  File "..\release\plugins\demux\libps_plugin.dll"
  File "..\release\plugins\demux\libpva_plugin.dll"
  File "..\release\plugins\demux\librawaud_plugin.dll"
  File "..\release\plugins\demux\librawdv_plugin.dll"
  File "..\release\plugins\demux\librawvid_plugin.dll"
  File "..\release\plugins\demux\libreal_plugin.dll"
  File "..\release\plugins\demux\libsid_plugin.dll"
  File "..\release\plugins\demux\libsmf_plugin.dll"
  File "..\release\plugins\demux\libsubtitle_plugin.dll"
  File "..\release\plugins\demux\libts_plugin.dll"
  File "..\release\plugins\demux\libtta_plugin.dll"
  File "..\release\plugins\demux\libty_plugin.dll"
  File "..\release\plugins\demux\libvc1_plugin.dll"
  File "..\release\plugins\demux\libvobsub_plugin.dll"
  File "..\release\plugins\demux\libvoc_plugin.dll"
  File "..\release\plugins\demux\libwav_plugin.dll"
  File "..\release\plugins\demux\libxa_plugin.dll"

  SetOutPath $INSTDIR\plugins\video_output
  File "..\release\plugins\video_output\libcaca_plugin.dll"
  File "..\release\plugins\video_output\libdirect2d_plugin.dll"
  File "..\release\plugins\video_output\libdirect3d_plugin.dll"
  File "..\release\plugins\video_output\libdirectx_plugin.dll"
  File "..\release\plugins\video_output\libdrawable_plugin.dll"
  File "..\release\plugins\video_output\libglwin32_plugin.dll"
  File "..\release\plugins\video_output\libvdummy_plugin.dll"
  File "..\release\plugins\video_output\libvmem_plugin.dll"
  File "..\release\plugins\video_output\libwingdi_plugin.dll"
  File "..\release\plugins\video_output\libyuv_plugin.dll"


  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\GoldenCheetah "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GoldenCheetah" "DisplayName" "Golden Cheetah"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GoldenCheetah" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GoldenCheetah" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GoldenCheetah" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\GoldenCheetah"
  CreateShortCut "$SMPROGRAMS\GoldenCheetah\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\GoldenCheetah\Golden Cheetah.lnk" "$INSTDIR\GoldenCheetah.exe" "" "$INSTDIR\GoldenCheetah.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GoldenCheetah"
  DeleteRegKey HKLM SOFTWARE\GoldenCheetah

  ; Remove files and uninstaller
  Delete $INSTDIR\GoldenCheetah.exe

  ; we continue to delete qwt5.dll becvause people don't always uninstall before they install
  ;; so a subsequent uninstall still should remove it if it is there
  Delete $INSTDIR\mingwm10.dll
  Delete $INSTDIR\QtCore4.dll
  Delete $INSTDIR\QtScript4.dll
  Delete $INSTDIR\QtGui4.dll
  Delete $INSTDIR\QtSql4.dll
  Delete $INSTDIR\QtXml4.dll
  Delete $INSTDIR\QtWebKit4.dll
  Delete $INSTDIR\QtXmlPatterns4.dll
  Delete $INSTDIR\phonon4.dll
  Delete $INSTDIR\QtNetwork4.dll
  Delete $INSTDIR\QtOpenGL4.dll
  Delete $INSTDIR\libgcc_s_dw2-1.dll
  Delete $INSTDIR\SiUSBXp.dll
  Delete $INSTDIR\libclucene-core.dll
  Delete $INSTDIR\libclucene-shared.dll
  ;Delete $INSTDIR\libstdc++-6.dll
  Delete $INSTDIR\qwtplot3d.dll
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\sqldrivers\qsqlite4.dll
  Delete $INSTDIR\imageformats\qjpeg4.dll

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\GoldenCheetah\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\GoldenCheetah"
  RMDir "$INSTDIR"

SectionEnd
