# AI Training Optimizer — Knowledge Base

This directory contains research, architecture decisions, and reference data for
GoldenCheetah's AI-assisted training optimization system.

## Structure

```
doc/ai/
├── README.md                  ← You are here
├── CHANGELOG.md               ← What changed and when
├── architecture.md            ← System design, hybrid model, phased roadmap
├── constraints.md             ← Hard physiological safety constraints (reference table)
├── metrics.md                 ← Complete catalog of computed metrics + API reference
├── research/
│   ├── banister-forward-simulation.md   ← Literature review: using IR models prospectively
│   └── computational-optimization.md    ← Algorithms, nonlinear models, 3D-IR, hybrid LLM
└── (future)
    ├── energy-systems.md      ← Aerobic / glycolytic / alactic adaptation timelines
    ├── periodization.md       ← Polarized, pyramidal, block, reverse periodization
    ├── recovery-science.md    ← Sleep, nutrition, HRV, deload protocols
    └── taper-protocols.md     ← Event-specific taper strategies
```

## For AI Agents

If you are an AI agent working on GoldenCheetah's training features:

1. **Always read `constraints.md` first** — these are non-negotiable safety bounds.
2. **Read `architecture.md`** to understand the hybrid math-model + LLM design.
3. **Read `metrics.md`** for the complete catalog of computed metrics and API endpoints.
4. **Consult `research/`** for evidence behind design decisions.
5. **Never optimize the linear Banister model directly** — it produces pathological
   "train maximally" prescriptions. Use nonlinear extensions or constrained heuristics.

## For Humans

These documents synthesize sports science research specifically as it applies to
GoldenCheetah's codebase. They are written to be useful both as developer documentation
and as context for AI-assisted development.

Research documents include citations. Architecture documents reference specific files
and classes in the GoldenCheetah source tree.
