# Agent Guidance For This Fork

This fork is being developed for a single-user setup. Do not assume the full
upstream GoldenCheetah surface is equally important.

## Product Priorities

Treat these paths as highest priority for bug fixing, testing, and UX polish:

- `ANT+` trainer mode, especially `FE-C` control and workout execution
- Heart-rate and training-session workflows used with a `Polar H10`
- Strava import and export
- Cloud services in general: keep them available for exploration, do not strip
  them out preemptively
- Standard workout and training file workflows likely to be used in practice,
  such as `ERG`, `MRC`, `ZWO`, `FIT`, `TCX`, and `GPX`
- Workout planning, generated workouts, calendar handoff into Train mode, and
  local workout library behavior

## Known Prioritization Rules

- Do not spend significant time hardening obscure legacy trainers, dead serial
  device paths, or proprietary importers unless they affect shared code, break
  the build, or are needed for a concrete current task.
- Do not aggressively delete large subsystems just because they are unused in
  this fork. Prefer to deprioritize them rather than removing them, because the
  codebase is flat and removals make future merges harder.
- If choosing between a bug in a niche importer and a bug in `ANT+`, Strava,
  Train mode, planning, or standard workout flows, prefer the latter.

## Strava Status

The current expected behavior for this fork is:

- The upstream AppImage build can import from and export to Strava successfully.
- The locally compiled build currently does not reliably pull or upload workouts
  to Strava.

Treat this as a real fork-specific bug, not as user error.

When working on Strava issues:

- Use the upstream AppImage behavior as the reference baseline.
- Assume the problem may be build- or packaging-specific, not only API logic.
- Compare runtime differences before making broad OAuth or networking changes.
- Likely investigation areas include OAuth callback handling, embedded web auth,
  secrets/config packaging, SSL or CA bundle behavior, and app-path/runtime
  assumptions that differ between AppImage and local builds.
- The local build now supports runtime Strava credential overrides via
  `GC_STRAVA_CLIENT_ID` and `GC_STRAVA_CLIENT_SECRET`. Prefer this route for
  personal fork testing when the official packaged secret is not available.

## Practical Scope For Future Passes

High ROI:

- `src/ANT`
- `src/Train`
- Strava and cloud integration code under `src/Cloud`
- Planning and Train handoff code under `src/Gui`, `src/Charts`, and `src/Core`
- Standard workout and activity import/export paths under `src/FileIO`

Lower ROI unless directly implicated:

- Legacy trainer integrations that are not part of the current setup
- Rare proprietary file formats with no current user value
- Broad cleanup or refactoring that does not improve the active workflows above

## Change Strategy

- Keep the working tree focused and tactical.
- Prefer small, testable extractions over broad refactors.
- Add regression tests where practical for real bugs in the active workflows.
- If a code path is unused in this fork, default to leaving it alone unless it
  is causing collateral damage.
