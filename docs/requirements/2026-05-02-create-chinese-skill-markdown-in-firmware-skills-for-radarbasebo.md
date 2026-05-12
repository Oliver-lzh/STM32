# Create Chinese skill markdown in Firmware skills for RadarBaseboardMCU7 runtime...

## Summary
Create Chinese skill markdown in Firmware skills for RadarBaseboardMCU7 runtime...

## Goal
Create Chinese skill markdown in Firmware skills for RadarBaseboardMCU7 runtime...

## Deliverable
Governed implementation artifacts, verification evidence, and cleanup receipts

## Constraints
- Do not bypass the fixed six-stage governed runtime.
- Do not widen scope silently beyond the frozen requirement document.

## Acceptance Criteria
- Requirement document is frozen before execution.
- Execution plan exists before implementation.
- Verification evidence exists before completion claims.
- Phase cleanup receipt is produced.

## Product Acceptance Criteria
- Requirement document is frozen before execution.
- Execution plan exists before implementation.
- Verification evidence exists before completion claims.
- Phase cleanup receipt is produced.
- The delivered output must satisfy observable behavior implied by the frozen goal and deliverable, not only internal runtime progress.
- Full completion wording is allowed only after downstream delivery truth is passing.

## Manual Spot Checks
- None required beyond automated verification for this task unless the execution scope expands to a user-visible or interactive flow.

## Completion Language Policy
- Full completion wording is allowed only when governance truth, engineering verification truth, workflow completion truth, and product acceptance truth are all passing.
- `completed_with_failures`, degraded execution, or pending manual actions must be reported as non-complete states.
- If manual spot checks remain pending, the run must be described as requiring manual review rather than fully ready.

## Delivery Truth Contract
- Governance truth: requirement, plan, execution, and cleanup artifacts remain traceable and authoritative.
- Engineering verification truth: targeted verification passes or fails explicitly; silence does not count as success.
- Workflow completion truth: planned units, delegated lanes, and specialist outputs reconcile back into the governed plan.
- Product acceptance truth: observable deliverable behavior satisfies frozen acceptance criteria before full completion language is allowed.

## Artifact Review Requirements
No additional artifact review requirements were frozen for this run.

## Code Task TDD Mode
TDD mode: required
Decision source: runtime_inference
Reason: The task includes implementation or defect-correction intent that requires code-task TDD evidence.

## Code Task TDD Evidence Requirements
- Record failing-first evidence for the changed behavior before implementation or defect correction.
- Record the green rerun that proves the targeted behavior passed after implementation.
- Map the changed behavior to targeted verification evidence; generic suite success alone is insufficient.
- If automated failing-first evidence is not appropriate, freeze and honor an explicit code-task TDD exception instead of silently skipping the requirement.

## Code Task TDD Exceptions
No code-task TDD exceptions were frozen for this run.

## Baseline Document Quality Dimensions
No baseline document quality dimensions were frozen for this run.

## Baseline UI Quality Dimensions
No baseline UI quality dimensions were frozen for this run.

## Task-Specific Acceptance Extensions
No additional task-specific acceptance extensions were frozen for this run.

## Research Augmentation Sources
No research augmentation sources were frozen for this run.

> Fill the anti-drift fields once here. Downstream governed plan and completion surfaces should reuse them rather than restate them.

## Primary Objective
Create Chinese skill markdown in Firmware skills for RadarBaseboardMCU7 runtime...

## Non-Objective Proxy Signals
- single sample pass only
- current test green only
- demo success only

## Validation Material Role
validation_only

## Anti-Proxy-Goal-Drift Tier
Tier C

## Intended Scope
scenario_specific

## Abstraction Layer Target
_author_to_declare_

## Completion State
partial

## Generalization Evidence Bundle
- cases: []
- note: add independent evidence before generalized completion claims

## Non-Goals
- Do not treat M/L/XL as user-facing entry branches.
- Do not introduce a second router or control plane.

## Autonomy Mode
interactive_governed

## Assumptions
- Interactive clarification is allowed if unresolved ambiguity materially changes implementation.

## Evidence Inputs
- Source task: Create Chinese skill markdown in Firmware skills for RadarBaseboardMCU7 runtime output mode switch. Filename and body Chinese. Include multi subagent workflow. Future firmware behavior: serial assistant commands slash mode host and slash mode debug switch m_mode at runtime via BoardOutput_setMode. DEBUG_TEXT parses one command line. Switching performs one-time teardown and initialization: debug to host initializes ProtocolHandler RequestHandler Requests_Macro; host to debug stops protocol processing and enables plain text USB CDC send receive. This run should create skill document first, not modify firmware source code.
- Intent contract: intent-contract.json
- Runtime input packet: runtime-input-packet.json

## Runtime Input Truth
- Governance scope: root
- Root run id: 20260502T085004Z-53b7a677
- Entry intent: vibe
- Requested stop stage: requirement_doc
- Requested grade floor: none
- Selected pack: orchestration-core
- Router-selected skill: vibe
- Runtime-selected skill: vibe
- Route mode: pack_overlay
- Route reason: auto_route
- Confirm required: False

## Specialist Decision
- Governed `vibe` must explicitly record whether specialist execution is happening, stayed advisory, or remained unresolved before closeout.
- Decision state: approved_dispatch
- Resolution mode: approved_dispatch
- Notes: Bounded specialist recommendations were surfaced and promoted into effective approved dispatch.

## Specialist Recommendations
Raw router candidates remain in `runtime-input-packet.json` for audit and are not frozen as user-facing requirements.
Only host-adopted or effective approved specialist dispatch is shown here; non-adopted candidates and stage assistants stay out of the requirement surface.
- Adopted Skill: tdd-guide
  Role: specialist_assist; native usage required: True; preserve workflow: True
  Binding: profile=default; phase=in_execution; lane policy=inherit_grade; parallel in XL=True
  Write scope: specialist:tdd-guide; review mode: native_contract; execution priority: 50
  Reason: top ranked specialist candidate from pack 'code-quality' via fallback_task_default
  Required inputs: bounded specialist subtask contract, frozen requirement context, relevant source files or domain artifacts
  Expected outputs: bounded specialist findings or code changes, verification notes aligned with the specialist skill
  Verification expectation: Preserve the specialist skill's native workflow, boundaries, and validation style.
- Adopted Skill: fred-economic-data
  Role: specialist_assist; native usage required: True; preserve workflow: True
  Binding: profile=implementation; phase=in_execution; lane policy=bounded_parallel; parallel in XL=True
  Write scope: specialist:execution:fred-economic-data; review mode: native_contract; execution priority: 50
  Reason: top ranked specialist candidate from pack 'finance-edgar-macro' via keyword_ranked
  Required inputs: bounded specialist subtask contract, frozen requirement context, relevant source files or domain artifacts
  Expected outputs: bounded specialist findings or code changes, verification notes aligned with the specialist skill
  Verification expectation: Preserve the specialist skill's native workflow, boundaries, and validation style.

## Specialist Consultation
These are specialists resolved for discussion-time handling under governed `vibe` before this requirement doc was frozen. Depending on policy, they may be consulted live or routed for direct current-session loading.
- Consulted Skill: tdd-guide
  Why now: top ranked specialist candidate from pack 'code-quality' via fallback_task_default
  Loaded from: C:\Users\Oliver\.codex\skills\vibe\bundled\skills\tdd-guide\SKILL.runtime-mirror.md
- Consulted Skill: fred-economic-data
  Why now: top ranked specialist candidate from pack 'finance-edgar-macro' via keyword_ranked
  Loaded from: C:\Users\Oliver\.codex\skills\vibe\bundled\skills\fred-economic-data\SKILL.runtime-mirror.md

## Unified Specialist Lifecycle Disclosure This unified disclosure keeps routing truth, consultation truth, and execution truth separate while showing one user-readable specialist timeline.  ### discussion_routing - Skill: tdd-guide   State: routed   Why now: top ranked specialist candidate from pack 'code-quality' via fallback_task_default   Loaded from: C:\Users\Oliver\.codex\skills\vibe\bundled\skills\tdd-guide\SKILL.runtime-mirror.md - Skill: fred-economic-data   State: routed   Why now: top ranked specialist candidate from pack 'finance-edgar-macro' via keyword_ranked   Loaded from: C:\Users\Oliver\.codex\skills\vibe\bundled\skills\fred-economic-data\SKILL.runtime-mirror.md  ### discussion_consultation - Skill: tdd-guide   State: routed_pending_current_session   Why now: top ranked specialist candidate from pack 'code-quality' via fallback_task_default   Loaded from: C:\Users\Oliver\.codex\skills\vibe\bundled\skills\tdd-guide\SKILL.runtime-mirror.md - Skill: fred-economic-data   State: routed_pending_current_session   Why now: top ranked specialist candidate from pack 'finance-edgar-macro' via keyword_ranked   Loaded from: C:\Users\Oliver\.codex\skills\vibe\bundled\skills\fred-economic-data\SKILL.runtime-mirror.md

## Memory Context
Bounded stage-aware memory context injected into requirement freezing:
- Disclosure level: decision_focused
- Capsule [d6d5bf031c6f3e9f] Cognee relation: continue-vibe-continue-vibe-implement-radarbaseboardmcu7-first-f specified_by 2026-04-30-continue-vibe-continue-vibe-implem...
  Owner: Cognee
  Why now: Matched Cognee memory for requirement_doc.
  Expansion Ref: C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\outputs\runtime\vibe-sessions\20260502T085004Z-53b7a677\memory-backend\cognee-read-response.json#d6d5bf031c6f3e9f
  Summary: Cognee relation: continue-vibe-continue-vibe-implement-radarbaseboardmcu7-first-f specified_by 2026-04-30-continue-vibe-continue-vibe-implement-radarbaseboardmcu7-first-f.md
  Summary: specified_by
