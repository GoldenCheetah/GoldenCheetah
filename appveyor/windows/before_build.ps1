# Windows Before Build Script
$ErrorActionPreference = "Stop"

# Define GC version string, only for tagged builds
if ($env:APPVEYOR_REPO_TAG -eq "true") {
    Add-Content src\gcconfig.pri "DEFINES+=GC_VERSION=VERSION_STRING"
}

# Enable CloudDB
Add-Content src\gcconfig.pri "CloudDB=active"

# Add Train Robot
Add-Content src\gcconfig.pri "DEFINES+=GC_WANT_ROBOT"

# Enable TrainerDay Query API
Add-Content src\gcconfig.pri "DEFINES+=GC_WANT_TRAINERDAY_API"
Add-Content src\gcconfig.pri "DEFINES+=GC_TRAINERDAY_API_PAGESIZE=25"

# Avoid macro redefinition warnings
Add-Content src\gcconfig.pri "DEFINES+=_MATH_DEFINES_DEFINED"

# Avoid conflicts between Windows.h min/max macros and limits.h
Add-Content src\gcconfig.pri "DEFINES+=NOMINMAX"

# Secrets.h patching is handled by common/patch_secrets.ps1
