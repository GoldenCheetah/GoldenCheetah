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
┌─────────────────────────────────────────────────────────┐
│                    REST API Layer                        │
│  /athlete/ai/snapshot  — enriched athlete context        │
│  /athlete/ai/simulate  — compare candidate plans (NEW)   │
│  /athlete/ai/draft     — generate single workout         │
│  /athlete/ai/save      — persist to disk                 │
│  /athlete/ai/plan      — create planned activity         │
└────────────────────────┬────────────────────────────────┘
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

### Phase 1: PMC-Based Plan Comparator (Minimum Viable Simulation)

**Goal:** Replace blind heuristic selection with model-informed workout type choice.

**Approach:** For a given day, simulate what happens if the athlete does each of the
7 workout types (recovery, endurance, sweetspot, threshold, vo2max, anaerobic, mixed).
Project the resulting CTL/ATL/TSB trajectory forward. Select the type that best serves
the athlete's target (build fitness, maintain, taper, recover).

**Key insight:** This does NOT require Banister performance prediction. It only needs
the existing PMC forward projection machinery, plus constraint checking.

**What changes:**
- Enrich `WorkoutAthleteSnapshot` with TSB trend, days-to-event, recent load pattern
- New `simulateDay()` function: given a workout type + duration, estimate its TSS and
  project PMC forward 7-14 days
- New constraint checker: verify projected trajectory stays within bounds
- Modify `/athlete/ai/draft` to accept a `goal` parameter (build/maintain/taper/recover)
  and auto-select workout type via simulation

**What stays the same:**
- Existing heuristic workout structure (interval patterns, warmup/cooldown)
- WorkoutDraft format
- ErgFile generation and TrainDB import

**No performance tests required. Works for every athlete with CTL data.**

### Phase 2: Banister Forward Prediction

**Goal:** Use individually fitted Banister parameters to predict performance outcomes
for candidate multi-week plans.

**Approach:**
1. Generate 3-5 heuristic plan templates (polarized, pyramidal, threshold-heavy, etc.)
2. For each, populate a daily TSS array spanning current date to target event
3. Run Banister forward simulation using athlete's fitted k1, k2, τ1, τ2
4. Rank plans by predicted performance at target date
5. Select winner, subject to constraint validation

**Prerequisites:**
- Athlete must have sufficient performance test data (≥5 tests recommended)
- Banister model must be fitted with acceptable RMSE
- Must use nonlinear model variant (Busso VDR) or heavy constraints to prevent
  "train maximally" pathology

**Key additions:**
- `BanisterSimulator` class: takes fitted params + daily TSS array → performance curve
- Nonlinear fatigue extension (Busso 2003: k2 becomes load-dependent)
- `/athlete/ai/simulate` endpoint returning ranked plan comparisons with projections
- Graceful degradation: fall back to Phase 1 if insufficient test data

**Open-source references:**
- Turner's nonlinear GA optimizer: github.com/jturner314/nl_perf_model_opt (Python)
- dorem R package for cross-validation

### Phase 3: Bayesian Estimation + Uncertainty

**Goal:** Solve cold-start, quantify prediction confidence, enable rolling updates.

**Approach:**
- Implement Bayesian parameter estimation with population priors compiled from
  ~57 published parameter sets (per Peng et al., 2024)
- Reparameterize model: replace k2 with α·k1 where α > 1 for identifiability
- Provide posterior predictive intervals on all predictions
- Sequential Bayesian updating as new data arrives (no batch refitting needed)

**Cold-start behavior:**
- <5 performance observations: use population priors only → wide confidence intervals
- 5-15 observations: blend priors with individual data → narrowing intervals
- >15 observations: individual parameters dominate → tight predictions

**Open-source reference:**
- Peng's Bayesian IR model: github.com/KenKPeng/IR-model (R/JAGS)

### Phase 4: Multi-Component (3D) Model

**Goal:** Distinguish between training stimuli targeting different energy systems.

**Approach:** Separate IR filters for:
- Aerobic (oxidative) → maps to CP; τ1 ≈ 40-60 days, τ2 ≈ 11-15 days
- Glycolytic (lactic) → maps to W'; τ1 ≈ 5-12 days, τ2 ≈ 3-5 days
- Alactic (phosphocreatine) → maps to Pmax; τ1 ≈ 15-30 days, τ2 ≈ 2-4 days

GoldenCheetah already tracks CP, W', and Pmax via `PDEstimate`. The optimizer can
then target specific energy systems (e.g., maximize CP while permitting W' decline
for a long TT) by adjusting intensity distribution.

**This is the long-term vision. Empirically unvalidated as of 2025.**

### Phase 5: LLM Tactical Executor

**Goal:** Translate mathematical load targets into structured, context-aware workouts.

**The math model outputs:** "Day 42 needs Aerobic Impulse = 85, Glycolytic = 15"

**The LLM translates to:** A complete WorkoutDraft with specific intervals, rest
periods, progression patterns, adapted for subjective context (fatigue, time
constraints, equipment availability).

**Architecture:** The LLM receives model outputs as rigid constraints in its system
prompt — not as suggestions. It cannot override the macro plan. It can only choose
*how* to structure the session within the prescribed energy system targets.

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
