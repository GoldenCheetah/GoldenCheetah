# AI Metrics Reference

Complete catalog of metrics computed by the AI training optimizer, their sources,
API availability, and visualization status.

## Athlete Snapshot

Captured per-request from the athlete's current state. Available in every AI API
response as the `snapshot` object.

**Source:** `WorkoutAthleteSnapshot` in `src/Train/WorkoutGenerationService.h`

| Field | Type | Description | Visualized |
|-------|------|-------------|------------|
| `athleteName` | string | Athlete identifier | — |
| `date` | date | Snapshot date (defaults to today) | — |
| `ftp` | int | Functional Threshold Power (watts) | Zones chart |
| `cp` | int | Critical Power (watts) | CP chart |
| `ctl` | double | Chronic Training Load (42-day EWMA of daily TSS) | PMC chart |
| `atl` | double | Acute Training Load (7-day EWMA of daily TSS) | PMC chart |
| `tsb` | double | Training Stress Balance (CTL - ATL) | PMC chart |
| `tsbTrend` | double | 7-day TSB delta; positive = freshening | **No** |
| `ctlRampRate` | double | CTL change per week over last 14 days | **No** |
| `daysToEvent` | int | Days until next SeasonEvent (-1 = none) | **No** |
| `nextEventName` | string | Name of next event | **No** |
| `recentStress` | double[] | Last 7 days of daily TSS (index 0 = oldest) | **No** |

### HRV Readiness (populated from Measures system)

| Field | JSON path | Type | Description | Visualized |
|-------|-----------|------|-------------|------------|
| `hrvAvailable` | `snapshot.hrvAvailable` | bool | True if HRV data exists for today + baseline | **No** |
| `rmssd` | `snapshot.hrv.rmssd` | double | Today's RMSSD reading (msec) | **No** |
| `baseline` | `snapshot.hrv.baseline` | double | 7-day rolling average RMSSD (msec) | **No** |
| `ratio` | `snapshot.hrv.ratio` | double | Today / baseline (1.0 = normal) | **No** |

HRV data is sourced from GoldenCheetah's Measures system (`hrvmeasures.json`).
Requires at least 3 days of baseline data within the prior 7 days to activate.
When available, HRV ratio is used by the constraint checker and scoring modifier.

**API:** All `/ai/*` endpoints include `snapshot` in their response.

## Simulation Metrics (Phase 1)

Per-candidate workout evaluation using PMC forward projection.

**Source:** `TrainingSimulator` in `src/Train/TrainingSimulator.h`

### Per Candidate (7 workout types)

| Field | Type | Description | Visualized |
|-------|------|-------------|------------|
| `workoutType` | string | One of: recovery, endurance, sweetspot, threshold, vo2max, anaerobic, mixed | **No** |
| `estimatedTSS` | double | Predicted TSS for this workout type at given duration and FTP | **No** |
| `score` | double | Ranking score for the selected training goal | **No** |
| `feasible` | bool | True if workout doesn't cause more hard violations than rest | **No** |
| `preExistingHardViolations` | int | Hard violations that also occur with TSS=0 (unavoidable) | **No** |
| `projCtl[]` | double[] | Projected CTL for 7 days post-workout | Partial (LTM planned curves) |
| `projAtl[]` | double[] | Projected ATL for 7 days post-workout | Partial (LTM planned curves) |
| `projTsb[]` | double[] | Projected TSB for 7 days post-workout | Partial (LTM planned curves) |

### Projection Summary (per candidate)

Derived from the projection arrays for quick comparison:

| Field | JSON path | Description |
|-------|-----------|-------------|
| CTL endpoint | `projection.ctlEnd` | CTL at end of 7-day projection |
| ATL endpoint | `projection.atlEnd` | ATL at end of 7-day projection |
| TSB endpoint | `projection.tsbEnd` | TSB at end of 7-day projection |
| TSB minimum | `projection.tsbMin` | Lowest TSB during projection (fatigue peak) |
| TSB maximum | `projection.tsbMax` | Highest TSB during projection |

### Scoring by Goal

The `score` field is computed differently per training goal:

| Goal | Formula | Interpretation |
|------|---------|---------------|
| `build` | `ctlEnd - snapshot.ctl` | Maximize fitness gain |
| `maintain` | `-abs(ctlEnd - snapshot.ctl)` | Minimize fitness change |
| `taper` | `tsbEnd - 0.5 * abs(ctlEnd - snapshot.ctl)` | Maximize freshness, preserve fitness |
| `recover` | `tsbEnd` | Maximize freshness |

**API:** `POST /athlete/{name}/ai/simulate`

```
Request:  {"goal": "build", "durationMin": 60}
Response: {"snapshot": {...}, "ranking": {"goal": "build", "candidates": [...]}}
```

## Constraint Metrics (Phase 1)

Six physiological safety constraints evaluated against projected training load.
See `constraints.md` for rationale and bounds.

**Source:** `TrainingConstraintChecker` in `src/Train/TrainingConstraintChecker.h`

| Constraint ID | Metric | Safe Range | Severity | Visualized |
|---------------|--------|------------|----------|------------|
| `acwr` | ATL / CTL (acute:chronic workload ratio) | 0.8 – 1.3 | Hard when CTL ≥ 30, Warning below | **No** |
| `monotony` | mean(daily load) / SD(daily load) over 7 days | < 1.5 | Warning | **No** |
| `ctlRamp` | Weekly CTL change | ≤ 5 TSS/week | Hard | **No** |
| `maxDailyTSS` | Single-day TSS | ≤ 300 (recreational) / ≤ 450 (elite) | Hard | **No** |
| `tsbFloor` | TSB minimum | > -30 | Hard | **No** |
| `restDays` | Rest days (TSS < 20) per week | ≥ 1/week | Hard | **No** |
| `hrvSuppressed` | Today's RMSSD / 7-day baseline RMSSD | ≥ 0.85 (hard) / ≥ 0.93 (warning) | Hard/Warning | **No** |

### Violation Detail

Each violation includes:

```json
{
  "constraintId": "acwr",
  "severity": "warning",
  "date": "2026-04-07",
  "actual": 1.76,
  "limit": 1.3,
  "message": "ACWR 1.76 outside safe range [0.8, 1.3]"
}
```

### `checkFromDay` Parameter

The constraint checker accepts a `checkFromDay` parameter to skip historical days
when evaluating forward projections. Historical stress values seed the PMC state
(CTL/ATL) but don't generate violations — the athlete can't change the past.

**API:** Constraint results are nested in each simulation candidate:
```
candidate.constraints.passed   — true if no hard violations beyond baseline
candidate.constraints.violations[] — array of violation detail objects
```

## Banister Model Metrics (Phase 2)

Individually fitted impulse-response model for performance prediction.

**Source:** `BanisterSimulator` in `src/Train/BanisterSimulator.h`

### Fitted Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `k1` | 0.2 | Positive Training Effect (PTE) coefficient |
| `k2` | 0.2 | Negative Training Effect (NTE) coefficient |
| `p0` | 0.0 | Baseline performance intercept |
| `t1` | 50.0 | PTE decay constant (days); slow-decaying fitness |
| `t2` | 11.0 | NTE decay constant (days); fast-decaying fatigue |
| `g` | 0.0 | Current PTE accumulator (carried from history) |
| `h` | 0.0 | Current NTE accumulator (carried from history) |
| `isFitted` | false | True if fitted to individual athlete data |

### Forward Projection (per plan)

| Field | Type | Description | Visualized |
|-------|------|-------------|------------|
| `dailyPerf[]` | double[] | Predicted performance per day: `p0 + k1*g(t) - k2*h(t)` | **No** |
| `dailyPTE[]` | double[] | Positive training effect per day: `k1*g(t)` | **No** |
| `dailyNTE[]` | double[] | Negative training effect per day: `k2*h(t)` | **No** |
| `peakPerf` | double | Maximum predicted performance in projection window | **No** |
| `peakDay` | int | Day index of peak performance | **No** |

### Plan Templates

Five heuristic multi-week load distributions used as comparison candidates:

| Template | Pattern | Description |
|----------|---------|-------------|
| `3-1` | 3 build + 1 recovery | Classic periodization: 90%, 100%, 110% then 50% |
| `2-1` | 2 build + 1 recovery | Aggressive: 95%, 110% then 50% |
| `progressive` | Linear ramp | 0.8x to 1.2x weekly target, no dedicated recovery |
| `polarized` | 80/20 distribution | 2 hard days (2.0x, 1.8x), rest at 0.5x daily avg |
| `flat` | Steady state | Constant daily load with 1 rest day/week |

### Plan Comparison

| Field | Type | Description |
|-------|------|-------------|
| `planAName` / `planBName` | string | Template names being compared |
| `projA` / `projB` | BanisterProjection | Full projection for each plan |
| `perfDelta` | double | `projA.peakPerf - projB.peakPerf` |

**API:** `POST /athlete/{name}/ai/banister`

```
Request:  {"weeklyTSS": 300, "days": 28}
Response: {"params": {...}, "planComparisons": [...]}
```

## Training Plan Model

Structured multi-phase plan stored as JSON.

**Source:** `TrainingPlan` in `src/Train/TrainingPlan.h`

| Field | Type | Description |
|-------|------|-------------|
| `id` | UUID | Plan identifier |
| `name`, `description`, `goal` | string | Plan metadata |
| `startDate`, `endDate` | date | Plan timeline |
| `status` | enum | Draft / Active / Completed / Archived |
| `phases[]` | array | Ordered list of `TrainingPlanPhase` |
| `startingCTL`, `startingATL`, `startingTSB` | double | Athlete state at plan creation |
| `athleteFTP` | int | FTP at creation time |
| `banisterSnapshot` | BanisterParams | Model parameters at creation (audit trail) |
| `constraintBounds` | bounds | Constraint thresholds used (audit trail) |

Each phase contains:

| Field | Type | Description |
|-------|------|-------------|
| `durationWeeks` | int | Phase length |
| `weeklyTSS` | double | Target weekly load |
| `templateName` | string | Load distribution template (3-1, polarized, etc.) |
| `goal` | string | Phase-specific goal (build, maintain, taper, recover) |

**API:** `GET/POST /athlete/{name}/ai/plans`, `GET/PUT/DELETE /athlete/{name}/ai/plans/{id}`

## API Quick Reference

All endpoints live under `/api/athlete/{name}/ai/`.

| Method | Path | Description |
|--------|------|-------------|
| GET | `/ai/snapshot` | Enriched athlete snapshot |
| POST | `/ai/draft` | Generate heuristic workout draft |
| POST | `/ai/save` | Save workout to library + TrainDB |
| DELETE | `/ai/save` | Delete workout from library + TrainDB |
| POST | `/ai/plan` | Create planned activity on calendar |
| PUT | `/ai/plan` | Update planned activity (date/time/metadata) |
| DELETE | `/ai/plan` | Delete planned activity from calendar |
| POST | `/ai/simulate` | Rank 7 workout types by training goal |
| POST | `/ai/banister` | Compare multi-week plan templates |
| GET | `/ai/plans` | List training plans |
| POST | `/ai/plans` | Create training plan |
| GET | `/ai/plans/{id}` | Get training plan |
| PUT | `/ai/plans/{id}` | Update training plan |
| DELETE | `/ai/plans/{id}` | Delete training plan |

## Visualization Gaps

The following computed metrics have no GoldenCheetah UI representation:

**High priority** (directly actionable by the athlete):
- Simulation candidate ranking (which workout type is best today?)
- Constraint violations (what safety thresholds are at risk?)
- ACWR trend (daily acute:chronic ratio over time)

**Medium priority** (planning and analysis):
- Banister forward projections (PTE/NTE/performance curves)
- Plan template comparison (which periodization peaks highest?)
- TSB trend and CTL ramp rate (snapshot enrichments)

**Low priority** (advanced/future):
- Bayesian uncertainty bands (Phase 3, stub only)
- Multi-component projections (Phase 4, interface only)

**Existing infrastructure that can be extended:**
- `LTMPlot` (Qwt curves) — add ACWR, monotony, CTL ramp overlays
- `OverviewItems` — add constraint status / simulation summary cards
- `PMCData` — extend with `simulatedCtl`, `simulatedAtl` arrays
- `PlanningWindow` — currently a stub, natural home for plan comparison charts
