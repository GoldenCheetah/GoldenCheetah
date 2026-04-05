# GoldenCheetah — Copilot Instructions

## Project Overview

GoldenCheetah is an open-source cycling and endurance sports performance analysis
tool written in C++ with Qt. It provides training load tracking (PMC/CTL/ATL/TSB),
power-duration modeling (CP, W', Pmax), Banister impulse-response performance
modeling, and structured workout generation.

## AI Training Optimizer

When working on AI-assisted training features, follow these rules:

### Critical Safety Rules

1. **Never optimize the linear Banister model directly.** The model is linear in
   training load — unconstrained optimization produces "train maximally every day"
   prescriptions that would injure the athlete. Always use nonlinear extensions
   (Busso VDR) or impose hard constraints.

2. **Hard constraints override model outputs.** See `doc/ai/constraints.md` for the
   full table. Key bounds:
   - CTL ramp rate ≤ 5 TSS/week
   - Training monotony < 1.5
   - ACWR between 0.8 and 1.3
   - TSB floor > -30
   - Max daily TSS ≤ 300 (recreational) / ≤ 450 (elite)
   - ≥ 1 rest day per week

3. **Degrade gracefully.** If an athlete lacks sufficient data for a given method,
   fall back to a simpler approach — never to unconstrained optimization.

### Architecture Pattern

The system uses a hybrid design: **mathematical models for macro load planning,
LLM for micro session design, constraints for safety.**

- Math models (PMC, Banister) decide *what type* of workout and *how much* load
- The LLM translates load targets into structured interval sessions
- The constraint checker validates everything before output

See `doc/ai/architecture.md` for the full phased design.

### Key Source Files

- `src/Metrics/Banister.h/.cpp` — IR model implementation (Morton/Clarke/Skiba)
- `src/Metrics/PMCData.h` — CTL/ATL/TSB with planned/expected projections
- `src/Train/WorkoutGenerationService.h/.cpp` — Heuristic workout generator
- `src/Train/WorkoutDraft.h/.cpp` — Portable workout interchange format
- `src/Core/APIWebService.cpp` — REST API (`/athlete/ai/*` endpoints)
- `src/Core/Athlete.h` — Athlete data: zones, Banister params, PD estimates, measures

### Coding Conventions

- C++ with Qt (QObject, signals/slots, QVector, QMap, QString)
- Static methods on service classes (e.g., `WorkoutGenerationService::method()`)
- JSON serialization via `QJsonObject` / `QJsonDocument`
- Use `generatorId` / `modelId` / `modelVersion` fields for audit trail on all
  generated content
