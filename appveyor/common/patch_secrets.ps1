# Patch Secrets.h
$ErrorActionPreference = "Stop"

$SecretsPath = "src/Core/Secrets.h"

# Helper function to replace content
function Patch-Secret {
    param($Token, $Value)
    if ($Value) {
        (Get-Content $SecretsPath) -replace $Token, $Value | Set-Content $SecretsPath
    }
}

Patch-Secret '__GC_GOOGLE_CALENDAR_CLIENT_SECRET__' $env:GC_GOOGLE_CALENDAR_CLIENT_SECRET
Patch-Secret '__GC_GOOGLE_DRIVE_CLIENT_ID__' $env:GC_GOOGLE_DRIVE_CLIENT_ID
Patch-Secret '__GC_GOOGLE_DRIVE_CLIENT_SECRET__' $env:GC_GOOGLE_DRIVE_CLIENT_SECRET
Patch-Secret '__GC_GOOGLE_DRIVE_API_KEY__' $env:GC_GOOGLE_DRIVE_API_KEY

(Get-Content $SecretsPath) -replace 'OPENDATA_DISABLE', 'OPENDATA_ENABLE' | Set-Content $SecretsPath

Patch-Secret '__GC_CLOUD_OPENDATA_SECRET__' $env:GC_CLOUD_OPENDATA_SECRET
Patch-Secret '__GC_WITHINGS_CONSUMER_SECRET__' $env:GC_WITHINGS_CONSUMER_SECRET
Patch-Secret '__GC_NOKIA_CLIENT_SECRET__' $env:GC_NOKIA_CLIENT_SECRET
Patch-Secret '__GC_DROPBOX_CLIENT_SECRET__' $env:GC_DROPBOX_CLIENT_SECRET
Patch-Secret '__GC_STRAVA_CLIENT_SECRET__' $env:GC_STRAVA_CLIENT_SECRET
Patch-Secret '__GC_CYCLINGANALYTICS_CLIENT_SECRET__' $env:GC_CYCLINGANALYTICS_CLIENT_SECRET
Patch-Secret '__GC_CLOUD_DB_BASIC_AUTH__' $env:GC_CLOUD_DB_BASIC_AUTH
Patch-Secret '__GC_CLOUD_DB_APP_NAME__' $env:GC_CLOUD_DB_APP_NAME
Patch-Secret '__GC_POLARFLOW_CLIENT_SECRET__' $env:GC_POLARFLOW_CLIENT_SECRET
Patch-Secret '__GC_SPORTTRACKS_CLIENT_SECRET__' $env:GC_SPORTTRACKS_CLIENT_SECRET
Patch-Secret '__GC_RWGPS_API_KEY__' $env:GC_RWGPS_API_KEY
Patch-Secret '__GC_NOLIO_CLIENT_ID__' $env:GC_NOLIO_CLIENT_ID
Patch-Secret '__GC_NOLIO_SECRET__' $env:GC_NOLIO_SECRET
Patch-Secret '__GC_XERT_CLIENT_SECRET__' $env:GC_XERT_CLIENT_SECRET
Patch-Secret '__GC_AZUM_CLIENT_SECRET__' $env:GC_AZUM_CLIENT_SECRET
Patch-Secret '__GC_TRAINERDAY_API_KEY__' $env:GC_TRAINERDAY_API_KEY
