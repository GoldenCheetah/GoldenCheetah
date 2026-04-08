# AI Training Optimizer — Changelog

## 2026-04-07: HRV Readiness Integration

**Phase 1b — complete.**

Integrated heart rate variability (HRV) data from GoldenCheetah's Measures system
into the training simulator as a recovery-aware constraint and scoring modifier.

### Changes

**Snapshot enrichment** (`WorkoutGenerationService`)
- Added `hrvRMSSD`, `hrvBaseline`, `hrvRatio`, `hrvAvailable` to `WorkoutAthleteSnapshot`
- `athleteSnapshot()` reads today's RMSSD from `hrvmeasures.json` via Measures system
- Computes 7-day rolling baseline (requires >= 3 days of data)
- API responses include `hrv` object in snapshot when data available

**Constraint checker** (`TrainingConstraintChecker`)
- Added `checkHRV()` static method with two thresholds:
  - Hard (ratio < 0.85): autonomic recovery suppressed
  - Warning (ratio < 0.93): consider reducing intensity
- Added `hrvHardThreshold` and `hrvWarningThreshold` to `TrainingConstraintBounds`

**Simulator scoring** (`TrainingSimulator`)
- `simulateCandidate()` runs HRV constraints; marks high-intensity (IF >= 0.90) infeasible on hard violation
- `scoreForGoal()` applies additive penalty scaling with suppression depth and workout intensity
- Penalty is additive (not multiplicative) to work correctly with negative scores

**Tests**
- 6 new constraint tests: no-data passthrough, normal ratio, mild/severe suppression, boundary cases
- 5 new simulator tests: no-data no-effect, normal ratio, severe blocks high-intensity, mild penalizes score, constraint violation presence
- All 49 tests pass (24 constraint + 21 simulator + 4 MCP)

**Documentation**
- `constraints.md`: Added HRV suppression row to hard constraint table
- `metrics.md`: Added HRV readiness fields and `hrvSuppressed` constraint
- `architecture.md`: Added Phase 1b section

### Design decisions

- **Algorithm-agnostic**: Uses RMSSD ratio (today/baseline), which self-calibrates to any device or app
- **Graceful degradation**: No HRV data = no effect on scoring or constraints
- **Additive penalty**: Avoids sign issues with negative PMC-based scores
- **3-day minimum baseline**: Prevents noisy single-day readings from dominating

## 2026-04-06: AI Workout Dashboard Card

Added `SimulationOverviewItem` to the Overview dashboard — shows today's recommended
workout type, estimated TSS, ACWR status, constraint summary, and alternative candidates.

### Changes

- Added `SIMULATION` to `OverviewItemType` enum
- `SimulationOverviewItem` class with `setData()`, `itemPaint()`, config widget
- Registered in factory: appears as "AI Workout" in Add Tile wizard
- Renders: top recommendation, TSS estimate, ACWR with color coding, constraint
  violations, two alternative workout types

## 2026-04-06: Metrics Reference Documentation

Created `doc/ai/metrics.md` — complete catalog of all computed metrics across the
AI training optimizer with API reference and visualization gap analysis.

## 2026-04-05: MCP Write Operations Routed Through GC HTTP API

Fixed three classes of bugs where MCP tools bypassed GoldenCheetah's internal state
management, causing crashes and stale UI.

### Changes

**Delete planned activity crash fix**
- `gc_delete_planned_activity` was calling `path.unlink()` directly, leaving dangling
  `RideItem*` pointers in RideCache. Caused segfault on next click.
- Fixed: Added `DELETE /ai/plan` endpoint routing through `RideCache::removeRide()`

**Update planned activity stale UI fix**
- `gc_update_planned_activity` wrote JSON directly, bypassing RideCache. Date changes
  didn't rename the file (filename encodes datetime).
- Fixed: Added `PUT /ai/plan` endpoint routing through `RideCache::moveActivity()` and
  `saveActivity()`

**Delete workout orphaned TrainDB fix**
- `gc_delete_workout` deleted .erg file without calling `trainDB->deleteWorkout()`,
  leaving ghost entries in the workout library.
- Fixed: Added `DELETE /ai/save` endpoint calling both TrainDB and file removal

**MCP server updated**
- All write operations now call GC HTTP API instead of direct file I/O
- Removed `_validate_gc_path()` (validation on C++ side)
- Added `do_PUT`/`do_DELETE` to test mock handler

## 2026-04-04: Simulator Feasibility Fix

- Skip historical constraint violations when evaluating forward projections
- Handle low-CTL ACWR: below CTL=30, all ACWR violations are warnings (ratio
  unreliable with small denominator)
- Added `checkFromDay` parameter to constraint checker
- Added `preExistingHardViolations` to `SimulationResult`
