ReadMe - GoldenCheetah Build - 64 Bit Version
---------------------------------------------

This is a build of GoldenCheetah for Windows.

We *strongly* recommend that you back up your data before 
using this or any other version of GoldenCheetah.


Installation
------------

Pre-requisite is to have the 

- Visual C++ Redistributable for Visual Studio 2019 -

installed on you PC. For your convenience we have added
a copy of them 

- "vcredist.x64.exe" (for the 64bit Version)
- "vcredist.x86.exe" (for the 32bit Version)

to this ZIP archive.

In case you want to be sure on the version, please download the 
latest version of:
-  Microsoft Visual C++ Redistributable for Visual Studio 2015, 2017 and 2019
from https://www.microsoft.com


Adding "R" Support to GoldenCheetah
-----------------------------------

To be able to create "R" based charts in GoldenCheetah you need
to do the following additional steps:

1. Install "R" on machine - you can use the same version with
   which GoldenCheetah has been build - which is 4.0, but also
   the more recent versions - both can be downloaded here:  
   https://cran.r-project.org/bin/windows/base/
   
   Note: Do NOT use the proposed standard installation path, but
         install "R" into a directory which does NOT contain any 
         blanks/spaces in it's path.

2. Open GoldenCheetah, goto "Options"->"General", mark the 
   Checkbox "Enable R" so that the input field for the 
   "R Installation Directory" gets visible. Enter the directory
   you have used when installing "R" and "Save" the changes.

3. Close GoldenCheetah and restart - with this the "R" libraries should
   be available and you can create you first "R" based GoldenCheetah
   chart.

For more details on how to use "R", please check the GoldenCheetah
wiki and mailing list.
