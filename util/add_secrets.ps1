# Patch Secrets.h (Windows / Linux / macOS) used from CI scripts
param(
  [Parameter(Position=0)] [string]$f = "src/Core/Secrets.h"
)
(Get-Content $f) -replace 'OPENDATA_DISABLE', 'OPENDATA_ENABLE' | Set-Content $f
(Get-Content $f) -replace '__GC_CLOUD_OPENDATA_SECRET__', $env:GC_CLOUD_OPENDATA_SECRET | Set-Content $f
(Get-Content $f) -replace '__GC_NOKIA_CLIENT_SECRET__', $env:GC_NOKIA_CLIENT_SECRET | Set-Content $f
(Get-Content $f) -replace '__GC_DROPBOX_CLIENT_SECRET__', $env:GC_DROPBOX_CLIENT_SECRET | Set-Content $f
(Get-Content $f) -replace '__GC_STRAVA_CLIENT_SECRET__', $env:GC_STRAVA_CLIENT_SECRET | Set-Content $f
(Get-Content $f) -replace '__GC_CYCLINGANALYTICS_CLIENT_SECRET__', $env:GC_CYCLINGANALYTICS_CLIENT_SECRET | Set-Content $f
(Get-Content $f) -replace '__GC_CLOUD_DB_BASIC_AUTH__', $env:GC_CLOUD_DB_BASIC_AUTH | Set-Content $f
(Get-Content $f) -replace '__GC_CLOUD_DB_APP_NAME__', $env:GC_CLOUD_DB_APP_NAME | Set-Content $f
(Get-Content $f) -replace '__GC_POLARFLOW_CLIENT_SECRET__', $env:GC_POLARFLOW_CLIENT_SECRET | Set-Content $f
(Get-Content $f) -replace '__GC_SPORTTRACKS_CLIENT_SECRET__', $env:GC_SPORTTRACKS_CLIENT_SECRET | Set-Content $f
(Get-Content $f) -replace '__GC_RWGPS_API_KEY__', $env:GC_RWGPS_API_KEY | Set-Content $f
(Get-Content $f) -replace '__GC_NOLIO_CLIENT_ID__', $env:GC_NOLIO_CLIENT_ID | Set-Content $f
(Get-Content $f) -replace '__GC_NOLIO_SECRET__', $env:GC_NOLIO_SECRET | Set-Content $f
(Get-Content $f) -replace '__GC_XERT_CLIENT_SECRET__', $env:GC_XERT_CLIENT_SECRET | Set-Content $f
(Get-Content $f) -replace '__GC_AZUM_CLIENT_SECRET__', $env:GC_AZUM_CLIENT_SECRET | Set-Content $f
(Get-Content $f) -replace '__GC_TRAINERDAY_API_KEY__', $env:GC_TRAINERDAY_API_KEY | Set-Content $f
