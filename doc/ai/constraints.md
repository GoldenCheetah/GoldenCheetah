# Physiological Safety Constraints

**Status**: Non-negotiable. These constraints must override any model output or
optimizer recommendation. No generated training plan may violate these bounds.

## Hard Constraint Table

| Constraint | Definition | Bound | Rationale | Source |
|-----------|-----------|-------|-----------|--------|
| CTL Ramp Rate | Δ 42-day EWMA of daily TSS per week | ≤ 5 TSS/week | Prevents structural tissue breakdown and acute injury | Coggan; Banister recommendation |
| Training Monotony | Weekly mean daily load ÷ SD of daily load | < 1.5 | Enforces intra-week variance; prevents repetitive stress | Foster (1998) |
| Acute:Chronic Workload Ratio (ACWR) | 7-day ATL ÷ 28-day CTL | 0.8 – 1.3 | "Sweet spot" zone; >1.5 dramatically increases injury risk | Gabbett (2016); Hulin et al. |
| Max Daily Load | Single-day TSS ceiling | ≤ 300 TSS (recreational) / ≤ 450 TSS (elite) | Eliminates physiologically impossible single-session loads | Consensus |
| TSB Floor | Training Stress Balance minimum | > -30 | Prevents deep functional overreaching / OTS risk | Empirical coaching guideline |
| Mandatory Rest | Minimum rest days per week | ≥ 1 day/week | Recovery requirement for adaptation consolidation | Consensus |
| Weekly Load Cap | Maximum weekly TSS | ≤ CTL × 1.3 × 7 | Prevents acute spikes beyond chronic capacity | Derived from ACWR |

## Constraint Enforcement Rules

1. **Lexicographic ordering**: Evaluate constraint satisfaction *before* performance
   optimization. Any plan violating a constraint is assigned a fatal penalty score
   regardless of predicted performance.

2. **Hard override**: When the optimizer produces a plan that violates these constraints,
   the constraints win. Never relax constraints to achieve a higher predicted performance.

3. **Conservative defaults**: For athletes with insufficient data, use the more
   conservative bound in each range (e.g., ACWR ≤ 1.3 not 1.5, max daily TSS ≤ 300
   not 450).

## Context-Dependent Adjustments

These constraints may be tightened (never relaxed) based on:

- **Age**: Masters athletes (>50) should use CTL ramp ≤ 3-4 TSS/week
- **Training history**: Novice athletes (<1 year structured training) should use
  max daily TSS ≤ 200 and ACWR ≤ 1.2
- **Wellness signals**: If HRV trending down or subjective fatigue rising, reduce
  max daily load by 20-30%
- **Return from illness/injury**: Reset to 50% of pre-illness CTL target and
  ramp conservatively

## Implementation Notes

In GoldenCheetah, these constraints map to:

- **CTL/ATL/TSB**: Available via `PMCData` (`lts()`, `sts()`, `sb()` methods)
- **Daily stress**: Available via `PMCData::stress()` vector
- **Planned projections**: `PMCData::plannedLts()`, `plannedSts()`, `plannedSb()`
- **Current snapshot**: `WorkoutAthleteSnapshot` provides `ctl`, `atl`, `tsb`

The `WorkoutGenerationService` should check these constraints before returning
any generated plan. The `/athlete/ai/draft` API endpoint should include constraint
violation warnings in the response.
