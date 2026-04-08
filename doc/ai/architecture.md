# Architecture: AI Training Optimizer

## Design Philosophy

**Math for macro, LLM for micro, constraints for safety.**

The training optimizer uses a hybrid architecture where:
- **Mathematical models** (Banister IR, PMC) handle load distribution over time
- **An LLM agent** translates daily load targets into structured workout sessions
- **Hard physiological constraints** override all model outputs (see `constraints.md`)

The system is a decision-support tool, not an autonomous coach.

## Current State (Baseline)

The workout generator uses static heuristics with minimal personalization:

```
WorkoutAthleteSnapshot {ftp, cp, ctl, atl, tsb}
         ↓
scaledIntervalCount(base, cycle, available, ctl)
  → ctlFactor = clamp(0.6, CTL / 70.0, 1.5)
         ↓
Fixed power zones per workout type (e.g., sweet spot = always 90% FTP)
         ↓
WorkoutDraft → ErgFile → TrainDB
```

**Key files:**
- `src/Train/WorkoutGenerationService.h/.cpp` — Heuristic generator
- `src/Train/WorkoutDraft.h/.cpp` — Portable workout format
- `src/Core/APIWebService.cpp` — REST API (`/athlete/ai/*` endpoints)

**Unused athlete data:** The `Athlete` object contains rich data not currently
exposed to the workout generator:
- `QMap<QString, Banister*> banisterData` — Fitted k1, k2, τ1, τ2 per metric
- `PMCData` with full `planned_lts_`, `planned_sts_`, `planned_sb_` projections
- `PDEstimate` with CP, W', FTP, PMax per date range
- `Measures` — body weight, HRV data
- `Seasons` — race/event targets
- Complete ride history via `rideCache`

## Target Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     REST API Layer                           │
│  GET  /ai/snapshot  — enriched athlete context               │
│  POST /ai/simulate  — rank 7 workout types by goal           │
│  POST /ai/banister   — compare multi-week plan templates     │
│  POST /ai/draft      — generate heuristic workout            │
│  POST|DELETE /ai/save — save or delete workout               │
│  POST|PUT|DELETE /ai/plan — create, update, delete planned   │
│  CRUD /ai/plans[/:id] — training plan management             │
│  (see metrics.md for full API reference)                     │
└────────────────────────┬────────────────────────────────────┘
                         │
         ┌───────────────┼───────────────┐
         ▼               ▼               ▼
┌─────────────┐  ┌──────────────┐  ┌──────────────┐
│  Constraint │  │   Forward    │  │   Workout    │
│   Checker   │  │  Simulator   │  │  Generator   │
│             │  │              │  │  (existing   │
│ ACWR, mono- │  │ PMC project- │  │  heuristic   │
│ tony, CTL   │  │ ion, Banister│  │  + LLM)      │
│ ramp, TSB   │  │ IR forward   │  │              │
│ floor       │  │ eval         │  │              │
└─────────────┘  └──────────────┘  └──────────────┘
         │               │               │
         └───────────────┼───────────────┘
                         ▼
              ┌──────────────────┐
              │   Athlete Data   │
              │                  │
              │ Banister params  │
              │ PMC time series  │
              │ PD estimates     │
              │ Ride history     │
              │ Zones, Measures  │
              │ Seasons/Goals    │
              └──────────────────┘
```

## Phased Implementation

### Phase 1: PMC-Based Plan Comparator — COMPLETE

**Goal:** Replace blind heuristic selection with model-informed workout type choice.

**Approach:** For a given day, simulate what happens if the athlete does each of the
7 workout types (recovery, endurance, sweetspot, threshold, vo2max, anaerobic, mixed).
Project the resulting CTL/ATL/TSB trajectory forward. Select the type that best serves
the athlete's target (build fitness, maintain, taper, recover).

**Implementation:**
- `TrainingSimulator` — simulates all 7 workout types, ranks by goal (build/maintain/taper/recover)
- `TrainingConstraintChecker` — ACWR, monotony, CTL ramp rate, TSB floor enforcement
- `/athlete/ai/simulate` REST endpoint + `gc_simulate_workout` MCP tool
- `WorkoutAthleteSnapshot` enriched with TSB trend, days-to-event, recent load pattern

Works for every athlete with CTL data. No performance tests required.

### Phase 2: Banister Forward Prediction — COMPLETE

**Goal:** Use individually fitted Banister parameters to predict performance outcomes
for candidate multi-week plans.

**Implementation:**
- `BanisterSimulator` — extracts fitted k1, k2, τ1, τ2 from athlete's Banister model
- `projectForward()` — runs IR model forward given a daily TSS array
- `buildPlanTemplate()` — generates heuristic plan templates (3-1, 2-1, progressive, polarized, flat)
- `comparePlans()` — ranks templates by predicted peak performance
- `/athlete/ai/banister` REST endpoint + `gc_banister_compare` MCP tool
- Graceful degradation to population priors when no fitted model exists

### Phase 1b: HRV Readiness Integration — COMPLETE

**Goal:** Use heart rate variability data as a recovery signal to modulate workout recommendations.

**Implementation:**
- `WorkoutAthleteSnapshot` extended with `hrvRMSSD`, `hrvBaseline`, `hrvRatio`, `hrvAvailable`
- `athleteSnapshot()` pulls today's RMSSD from Measures system, computes 7-day rolling baseline
- `TrainingConstraintChecker::checkHRV()` — hard violation at ratio < 0.85, warning at < 0.93
- `TrainingSimulator::scoreForGoal()` — additive penalty on high-intensity scores when HRV suppressed
- `simulateCandidate()` — marks high-intensity workouts (IF ≥ 0.90) infeasible on hard HRV violation
- API responses include `hrv` object in snapshot when data available
- Graceful degradation: no HRV data = no effect (requires ≥3 days baseline within prior 7)
- Algorithm-agnostic: works with any HRV source (Kubios, HRV4Training, Oura, Garmin, etc.)

**Data path:** HRV app → CSV/API → `hrvmeasures.json` → Measures system → snapshot → simulator

### Phase 3: Bayesian Estimation + Uncertainty — PARTIAL

**Goal:** Solve cold-start, quantify prediction confidence, enable rolling updates.

**Implemented:**
- `BayesianEstimator` — bootstrap-based approximate Bayesian computation (ABC)
- `estimateUncertainty()` — parameter distributions via bootstrap resampling
- `ensembleProject()` — forward projection with 50% and 95% confidence bands
- `planDifferenceSignificance()` — tests if two plans are meaningfully different
- `dataConfidenceLevel()` — qualitative confidence based on data availability
- Cold-start tiers: <2 tests (population priors), 2-5 (wide CI), >5 (individual params)

**Not yet implemented:**
- Full MCMC via Stan/JAGS (currently using bootstrap approximation)
- Reparameterized model (k2 = α·k1 for identifiability)
- Sequential Bayesian updating (currently requires batch refit)

**Open-source reference:**
- Peng's Bayesian IR model: github.com/KenKPeng/IR-model (R/JAGS)

### Phase 4: Multi-Component (3D) Model — INTERFACE ONLY

**Goal:** Distinguish between training stimuli targeting different energy systems.

**Implemented:**
- `MultiComponentModel` — class with full interface design
- `DecomposedImpulse` — aerobic / glycolytic / neuromuscular impulse split
- `ComponentParams` — per-energy-system Banister parameters
- `decomposeByIF()` — heuristic decomposition when zone data unavailable
- `projectForward()` — independent per-component projection with composite weighting

**Not yet implemented:**
- Actual decomposition math (zone time-in-zone extraction from rides)
- Per-component model fitting (requires component-specific performance tests)
- Zone boundary calibration (VT1/VT2 thresholds)
- Empirical validation — this remains unvalidated as of 2026

**Approach (when implemented):** Separate IR filters for:
- Aerobic (oxidative) → maps to CP; τ1 ≈ 40-60 days, τ2 ≈ 11-15 days
- Glycolytic (lactic) → maps to W'; τ1 ≈ 5-12 days, τ2 ≈ 3-5 days
- Alactic (phosphocreatine) → maps to Pmax; τ1 ≈ 15-30 days, τ2 ≈ 2-4 days

### Phase 5: LLM Tactical Executor — COMPLETE (architecture evolved)

**Goal:** Translate mathematical load targets into structured, context-aware workouts.

**Original design:** Embedded LLM call within the C++ pipeline.

**Actual implementation:** The architecture inverted — the LLM (Claude via MCP) is
the external caller, and GoldenCheetah provides the structured context and validation
layer. This is a better design: the LLM orchestrates the full workflow rather than
being a single step in a rigid pipeline.

**Implementation:**
- `LLMWorkoutBridge` — packages simulation outputs into structured LLM context
- `buildContext()` — bundles snapshot + recommendation + constraints for the LLM
- `generatePrompt()` — structured JSON prompt with safety bounds and output schema
- `validateDraft()` — validates LLM-generated WorkoutDraft against TSS target (±20%)
- `clampToSafety()` — reduces any power values exceeding safe bounds
- `estimateDraftTSS()` — TSS estimation for generated workouts
- MCP server (`goldencheetah-mcp`) exposes all tools for external LLM orchestration

**Architecture:** The LLM receives model outputs as rigid constraints — not as
suggestions. It cannot override the macro plan. It can only choose *how* to structure
the session within the prescribed energy system targets.

## Key Principles

1. **Models predict, constraints protect.** Never trust model output without
   constraint checking.

2. **Degrade gracefully.** Insufficient data → fall back to simpler method,
   never to unconstrained optimization.

3. **Show uncertainty.** Every prediction should communicate confidence bounds.
   When the 95% CI for peak timing spans 43 days, the user must see this.

4. **Recalibrate regularly.** Parameters drift. Refit every 60-90 days minimum,
   or use sequential Bayesian updating.

5. **Decision support, not autopilot.** The system recommends; the athlete/coach
   decides.
