# Microsoft Developer Studio Project File - Name="scidisasm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=scidisasm - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "scidisasm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "scidisasm.mak" CFG="scidisasm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "scidisasm - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "scidisasm - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "scidisasm - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "scidisasm_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\win32\getopt" /I "..\include" /I "..\..\..\glib" /I "\cygnus\cygwin-b20\src" /I "\cygnus\cygwin-b20\src\include" /I "..\..\..\libpng" /I "..\..\..\zlib" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_STRING_H" /D "HAVE_LIBPNG" /D PACKAGE=\"freesci\" /D VERSION=\"0.2.6\" /D "HAVE_OBSTACK_H" /D "HAVE_GETOPT_H" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "scidisasm - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "scidisasm___Win32_Debug"
# PROP BASE Intermediate_Dir "scidisasm___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "scidisasm_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\win32" /I "..\include" /I "..\..\..\glib" /I "\cygnus\cygwin-b20\src" /I "\cygnus\cygwin-b20\src\include" /I "..\..\..\libpng" /I "..\..\..\zlib" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_STRING_H" /D "HAVE_LIBPNG" /D PACKAGE=\"freesci\" /D VERSION=\"0.2.6\" /D "HAVE_OBSTACK_H" /D "HAVE_GETOPT_H" /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "scidisasm - Win32 Release"
# Name "scidisasm - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\console\console.c
# End Source File
# Begin Source File

SOURCE=..\console\console_font.c
# End Source File
# Begin Source File

SOURCE=..\core\decompress0.c
# End Source File
# Begin Source File

SOURCE=..\core\decompress1.c
# End Source File
# Begin Source File

SOURCE=..\core\resource.c
# End Source File
# Begin Source File

SOURCE=..\tools\scidisasm.c
# End Source File
# Begin Source File

SOURCE=..\core\script.c
# End Source File
# Begin Source File

SOURCE=..\core\vocab.c
# End Source File
# Begin Source File

SOURCE=..\core\vocab_debug.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\config.h
# End Source File
# Begin Source File

SOURCE=..\include\console.h
# End Source File
# Begin Source File

SOURCE=..\include\engine.h
# End Source File
# Begin Source File

SOURCE=..\include\graphics.h
# End Source File
# Begin Source File

SOURCE=..\include\graphics_png.h
# End Source File
# Begin Source File

SOURCE=..\include\heap.h
# End Source File
# Begin Source File

SOURCE=..\include\menubar.h
# End Source File
# Begin Source File

SOURCE=..\include\resource.h
# End Source File
# Begin Source File

SOURCE=..\include\script.h
# End Source File
# Begin Source File

SOURCE=..\include\sound.h
# End Source File
# Begin Source File

SOURCE=..\include\uinput.h
# End Source File
# Begin Source File

SOURCE=..\include\versions.h
# End Source File
# Begin Source File

SOURCE=..\include\vm.h
# End Source File
# Begin Source File

SOURCE=..\include\vocabulary.h
# End Source File
# End Group
# Begin Group "Libs"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\libiberty\getopt.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\include\getopt.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\libiberty\getopt1.c"
# End Source File
# Begin Source File

SOURCE=..\..\..\glib\glib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\glib\glibconfig.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\libiberty\obstack.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\include\obstack.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\libpng\png.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libpng\pngconf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE="..\..\..\glib\glib-1.3.lib"
# End Source File
# Begin Source File

SOURCE=..\..\..\libpng\libpng.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\zlib\zlib.lib
# End Source File
# End Group
# End Target
# End Project
