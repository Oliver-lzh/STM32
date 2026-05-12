# RadarBaseboardMCU7 第一项功能：USB CDC 输出模式切换执行计划

## 执行摘要
这是 `vibe` 在 `interactive_governed` 模式下生成的治理执行计划。目标是在 RadarBaseboardMCU7 固件中实现 `HOST_PROTOCOL` 与 `DEBUG_TEXT` 两种 USB CDC 输出模式，并保持默认官方协议兼容。

## 冻结输入
- Requirement doc: `C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\docs\requirements\2026-04-30-continue-vibe-implement-radarbaseboardmcu7-first-feature-usb-cdc.md`
- Runtime input packet: `C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\outputs\runtime\vibe-sessions\20260430T113216Z-97b501b5\runtime-input-packet.json`
- Source task: continue-vibe Implement RadarBaseboardMCU7 first feature USB CDC HOST_PROTOCOL DEBUG_TEXT output switching.
- Governance scope: `root`
- Root run id: `20260430T113216Z-97b501b5`
- Entry intent: `vibe`
- Requested stop stage: `xl_plan`
- Requested grade floor: none
- Frozen route pack: `orchestration-core`
- Frozen route skill: `vibe`
- Frozen route mode: `pack_overlay`
- Router/runtime skill mismatch: `False`
- Execution topology companion: `C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\outputs\runtime\vibe-sessions\20260430T113216Z-97b501b5\execution-topology.json`

## 防目标漂移控制
优先复用冻结需求文档中的字段。只有在有明确理由时才允许偏离。

### 主要目标
实现 RadarBaseboardMCU7 第一项功能：USB CDC 输出模式切换，支持 `HOST_PROTOCOL` 与 `DEBUG_TEXT`，默认保持 `HOST_PROTOCOL`。

### 非目标代理信号
- 只通过单个样例。
- 只保证当前测试为绿色。
- 只完成演示效果。

### 验证材料角色
`validation_only`

### 声明等级
Tier C

### 预期范围
`scenario_specific`

### 抽象层目标
`Board.c` 附近的应用层输出模式框架，不下沉到 HAL/SPI/DMA。

### 完成状态目标
`partial`

### 泛化证据计划
- 复用需求文档声明的证明边界作为起点。
- cases: []
- note: 在声明更广泛完成之前，需要补充独立证据。

## 内部等级决策
- Grade: `L`
- 用户可见 runtime 保持固定；该等级仅为内部信息。
- `vibe` 仍是执行流的治理者和最终权威。

## 波次计划
- Wave 1: 设计确认与实现准备。
- Wave 2: 实现与目标验证。
- Wave 3: 清理与残余风险复查。

## 交付验收计划
- 将下游产品验收冻结在治理需求文档内，后续复用它，而不是在结束时临时发明验收口径。
- 在 `phase_cleanup` 阶段生成每次 run 的 delivery-acceptance report，将 runtime/process 成功与项目交付成功分开。
- Delivery-acceptance report: `C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\outputs\runtime\vibe-sessions\20260430T113216Z-97b501b5\delivery-acceptance-report.json`
- 如果需求文档声明了人工抽查，最终完成措辞必须保持 blocked，直到人工抽查清除，或明确降级为 manual review。
- Release truth aggregation 属于外层 gate；本轮只输出单次 run 的 delivery-truth report。

## 产物审查策略
- 如果冻结需求文档声明了 `Artifact Review Requirements`，执行必须留下明确的产物审查证据，不能只依赖通用完成措辞。
- 产物审查可以记录在 `phase-execute.json` 中，也可以通过专门的 `artifact-review.json` sidecar 记录；只要存在直接产物审查要求，就必须至少有一个治理表面承载证据。
- 当必需的产物审查缺失、部分完成、降级或仅人工复核时，产品验收保持 blocked。

## 代码任务 TDD 证据计划
- 复用需求文档中冻结的 `Code Task TDD Evidence Requirements`，不要在结束时重新发明证据要求。
- 如果严格失败优先流程被明确豁免，复用需求文档中冻结的 `Code Task TDD Exceptions`。
- 将每个冻结需求或例外映射到实现步骤、目标验证命令和证明产物。
- 如果严格失败优先证据受限，执行阶段必须记录有界原因和替代证据。

## 基线文档质量映射
- 使用需求文档中冻结的 `Baseline Document Quality Dimensions` 作为文档产物质量维度权威列表。
- 通过产物审查注释追踪每个文档维度，使 delivery-acceptance report 能显示结构、格式、完整性、引用完整性、布局稳定性和输出保真度是否已检查。
- 缺失文档维度覆盖时，视为需要人工复核，并与 UI 基线和代码任务 TDD 证据分开。

## 基线 UI 质量映射
- 使用需求文档中冻结的 `Baseline UI Quality Dimensions` 作为 UI 交付前必须覆盖的维度列表。
- 通过执行和产物审查注释追踪每个维度，使 delivery-acceptance report 能显示结构、交互、状态、一致性、响应性和保真度是否已检查。
- 缺失维度覆盖时，视为需要人工复核，并加入明确映射步骤或目标验证单元来收集证据。

## 任务特定验收映射
- 复用需求文档中冻结的任务特定验收扩展，不在结束阶段临时增加验收标准。
- 将基础交付真实性与任务特定期望分开，以便审查时独立检查。

## 研究增强计划
- 保留需求文档中冻结的研究增强来源，使后续审查者能看到哪些外部标准增强了任务说明。
- 研究增强可以强化粗略需求，但不能替代用户拥有的需求表面。

## 执行拓扑快照
- Delegation mode: `serial_child_lanes`
- Review mode: `two_stage_after_unit`
- Specialist execution mode: `native_bounded_units`
- Max parallel units: `1`
- Wave `wave-1` 有 4 个可执行步骤。
- Step `wave-1-step-1` -> mode `sequential`, units `1`。
- Step `wave-1-step-2` -> mode `sequential`, units `1`。
- Step `wave-1-specialist-in_execution-group-1-serial-1` -> mode `sequential`, units `1`。
- Step `wave-1-specialist-in_execution-group-1-serial-2` -> mode `sequential`, units `1`。

## 专家决策计划
- 治理 runtime 必须从冻结到交付验收，全程保留一个明确的专家决策表面。
- Frozen decision state: `approved_dispatch`
- Frozen resolution mode: `approved_dispatch`
- Frozen decision notes: 有界专家建议已被展示，并提升为有效批准的分派。

## 专家技能分派计划
- 专家路由在受治理的 `vibe` 内是强制且有界的；它不会把 runtime 权威转移出 `vibe`。
- 本节只列出有效批准的分派；未采纳路由候选和本地建议保留在 packet/audit 数据中，不作为用户可见执行要求。
- 专家执行开始前，受治理的 `vibe` 会基于每个技能真实的 `native_skill_entrypoint`，对有效 `approved_dispatch` 集合发出统一披露。
- 每个专家必须通过其原生工作流、输入契约和验证风格被调用。
- 专家输出从属于冻结需求和治理计划。

- Dispatch `tdd-guide` as `specialist_assist`.
  Binding profile: `default`; dispatch phase: `in_execution`; lane policy: `inherit_grade`; parallel in XL: `True`
  Write scope: `specialist:tdd-guide`; review mode: `native_contract`; execution priority: `50`
  Reason: 由 `code-quality` 包通过 `fallback_task_default` 排名最高的专家候选。
  Required inputs: 有界专家子任务契约、冻结需求上下文、相关源文件或领域产物。
  Expected outputs: 有界专家发现或代码变更，以及符合专家技能的验证说明。
  Verification: 保留专家技能原生工作流、边界和验证风格。

- Dispatch `scikit-learn` as `specialist_assist`.
  Binding profile: `default`; dispatch phase: `in_execution`; lane policy: `inherit_grade`; parallel in XL: `True`
  Write scope: `specialist:scikit-learn`; review mode: `native_contract`; execution priority: `50`
  Reason: 由 `data-ml` 包通过 `fallback_task_default` 排名最高的专家候选。
  Required inputs: 有界专家子任务契约、冻结需求上下文、相关源文件或领域产物。
  Expected outputs: 有界专家发现或代码变更，以及符合专家技能的验证说明。
  Verification: 保留专家技能原生工作流、边界和验证风格。

## 专家咨询
以下专家是在该执行计划冻结前，由受治理的 `vibe` 在计划阶段解析出来的。根据策略，它们可能被实时咨询，也可能被路由为当前会话直接加载。

- Consulted Skill: `tdd-guide`
  Why now: 由 `code-quality` 包通过 `fallback_task_default` 排名最高的专家候选。
  Loaded from: `C:\Users\Oliver\.codex\skills\vibe\bundled\skills\tdd-guide\SKILL.runtime-mirror.md`

- Consulted Skill: `scikit-learn`
  Why now: 由 `data-ml` 包通过 `fallback_task_default` 排名最高的专家候选。
  Loaded from: `C:\Users\Oliver\.codex\skills\vibe\bundled\skills\scikit-learn\SKILL.runtime-mirror.md`

## 统一专家生命周期披露
此披露将路由真实性、咨询真实性和执行真实性分开，同时提供一条用户可读的专家时间线。

### discussion_routing
- Skill: `tdd-guide`
  State: `routed`
  Why now: 由 `code-quality` 包通过 `fallback_task_default` 排名最高的专家候选。
  Loaded from: `C:\Users\Oliver\.codex\skills\vibe\bundled\skills\tdd-guide\SKILL.runtime-mirror.md`

- Skill: `scikit-learn`
  State: `routed`
  Why now: 由 `data-ml` 包通过 `fallback_task_default` 排名最高的专家候选。
  Loaded from: `C:\Users\Oliver\.codex\skills\vibe\bundled\skills\scikit-learn\SKILL.runtime-mirror.md`

### discussion_consultation
- Skill: `tdd-guide`
  State: `routed_pending_current_session`
  Why now: 由 `code-quality` 包通过 `fallback_task_default` 排名最高的专家候选。
  Loaded from: `C:\Users\Oliver\.codex\skills\vibe\bundled\skills\tdd-guide\SKILL.runtime-mirror.md`

- Skill: `scikit-learn`
  State: `routed_pending_current_session`
  Why now: 由 `data-ml` 包通过 `fallback_task_default` 排名最高的专家候选。
  Loaded from: `C:\Users\Oliver\.codex\skills\vibe\bundled\skills\scikit-learn\SKILL.runtime-mirror.md`

### planning_consultation
- Skill: `tdd-guide`
  State: `routed_pending_current_session`
  Why now: 由 `code-quality` 包通过 `fallback_task_default` 排名最高的专家候选。
  Loaded from: `C:\Users\Oliver\.codex\skills\vibe\bundled\skills\tdd-guide\SKILL.runtime-mirror.md`

- Skill: `scikit-learn`
  State: `routed_pending_current_session`
  Why now: 由 `data-ml` 包通过 `fallback_task_default` 排名最高的专家候选。
  Loaded from: `C:\Users\Oliver\.codex\skills\vibe\bundled\skills\scikit-learn\SKILL.runtime-mirror.md`

## 完成措辞规则
- 除非 delivery-acceptance report 返回 `PASS`，否则不能把 runtime 完成描述为下游项目交付完成。
- `completed_with_failures`、降级执行或待人工操作必须降低完成措辞。
- 子治理完成只在本地范围内有效，不能支撑 root 级完成措辞。

## 所有权边界
- 每组产物只有一个 owner。
- 并行工作必须使用互不重叠的写入范围。
- Subagent prompt 必须以 `$vibe` 结尾。
- 专家帮助保持有界和原生模式；不能变成第二个 planner 或第二套 runtime。

## 验证命令
- 对变更表面运行目标仓库验证。
- 在声明完成前运行 runtime contract gate。
- 使用完整完成措辞前，审查 `phase_cleanup` 期间生成的 delivery-acceptance report。
- 在发布声明前，重新运行 mirror sync 和 parity validation。

## 回滚计划
- 如果验证失败，只回滚治理 runtime 变更集。
- 不回滚无关的用户变更。

## 阶段清理契约
- 删除本波次创建的临时产物。
- 需要时运行 node audit 和 cleanup。
- 完成前写入 cleanup receipt。