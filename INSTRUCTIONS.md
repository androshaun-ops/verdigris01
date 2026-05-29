# Verdigris Embedded Systems Take-home

## Timebox

Target 2 hours. Please do not exceed 3 hours.

AI tools are allowed and encouraged. We evaluate correctness, validation, and engineering judgment.

## Context

You are working on firmware for an edge device (EDG). In a deployment, there are multiple EDG nodes.

- Every node produces 1 data sample per minute via an ADC interrupt.
- Samples are written to a per-node ring buffer and uploaded to the cloud by a background task.
- A connectivity manager task selects the best uplink (Ethernet, LTE, or Mesh) based on probe results.
- An async probe task refreshes link health snapshots periodically.

The firmware runs on a single-core MCU with priority-based preemptive scheduling:

- **ISR** (highest): ADC sample collection
- **ConnMgr task** (medium-high): Reads probe snapshot, selects active link
- **Probe task** (medium): Updates link health snapshot
- **Upload task** (lowest): Drains ring buffer over the active link

The simulator (`src/sim/simulator.cpp`) models this execution environment by calling
`FirmwareNode` methods in a deterministic phase order each simulated minute. The phase
order reflects the priority-based preemption that would occur on real hardware.

Connectivity options per node:

1. Ethernet
   - Preferred when healthy
   - Can be bandwidth-throttled by firewall policy
   - Probes may be noisy (false negatives)

2. LTE
   - Higher outage frequency
   - Deterministic outage pattern: every 8 minutes, 2 minutes are disconnected

3. Mesh via a gateway node
   - Any node can act as a gateway when it has uplink (Ethernet or LTE)
   - A gateway uploads its own samples first, then may forward for others using remaining capacity
   - Mesh link capacity limits client-to-gateway throughput

## Setup

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

At least one test is expected to fail in the baseline.

## Submission

1. Create a private GitHub repository under your account.
2. Commit the baseline as your first commit.
3. Create a feature branch.
4. Open a Pull Request when complete.
5. Invite reviewers:
   - hongbin@verdigris.co
   - jon@verdigris.co

## Required Tasks

### Task A: Fix connectivity selection and data delivery

Improve the system so that:

- Nodes stay on Ethernet when it is actually usable, even with probe noise.
- Nodes avoid unnecessary link oscillation.
- All produced samples are delivered when link capacity exceeds production rate.
- Nodes keep delivered samples high over time under realistic conditions.

Constraints:

- Keep changes minimal and reviewable.
- Do not weaken existing tests or modify scenario inputs to make failures disappear.
- Do not hardcode scenario-specific logic.
- You may modify FirmwareNode, ConnMgr, and the Simulator.

### Task B: Add one additional verification

Add at least one deterministic test or assertion that increases confidence in the
data path from sample production through delivery.

### Task C: Writeup

Replace `WRITEUP.md` with a short writeup (0.5 to 1 page):

1. What caused the test failures — identify the root causes in the baseline code
2. What you changed and why — summarize your fixes and what invariants they preserve
3. Build, test, and expected output — include commands and explain what passing metrics confirm about your fixes
4. Fleet rollout and observability — how you would deploy this to production (canary gates, rollback triggers)
5. AI usage — which parts did AI help with, which parts required your own judgment, and where did you override or redirect the AI

## Optional Bonus (choose at most one)

### Throughput-aware optimization

Enhance the logic to maximize fleet delivered samples over time while:

- Respecting gateway capacity
- Avoiding oscillation
- Not increasing sample loss

Add one deterministic test demonstrating the improvement.

## Review expectation

Assume this is production firmware for deployed devices. Your PR should be reviewable in under 20 minutes.
