# RadarBaseboardMCU7 第一项功能：USB CDC 输出模式切换需求文档

## 摘要
本轮目标是在 RadarBaseboardMCU7 固件中实现第一项二次开发功能：为 USB CDC 输出增加清晰的模式边界，支持 `HOST_PROTOCOL` 和 `DEBUG_TEXT` 两种互斥输出模式。

## 目标
在保持官方上位机协议兼容的前提下，为后续调试和二次开发增加普通串口文本输出能力。默认模式必须继续使用 `HOST_PROTOCOL`，避免普通文本混入 vendor protocol 数据流。

## 交付物
- 受 `vibe` 治理的实现产物。
- 可追踪的验证证据。
- 阶段清理记录。

## 约束
- 不绕过固定的六阶段 `vibe` 治理流程。
- 不在冻结需求之外静默扩大范围。
- 不修改底层 HAL、ASF/CMSIS、SPI/DMA 中断路径。
- 不破坏官方上位机通信协议。
- 默认固件行为必须保持官方兼容。

## 验收标准
- 实现前必须存在冻结需求文档。
- 实现前必须存在执行计划。
- 声明完成前必须有验证证据。
- 必须生成阶段清理记录。
- `HOST_PROTOCOL` 模式下继续走原厂协议路径。
- `DEBUG_TEXT` 模式下允许 USB CDC 输出普通文本调试信息。
- 两种模式互斥，不能在同一个 CDC 数据流里混合 vendor protocol 和普通文本。

## 产品验收标准
- 需求文档在执行前冻结。
- 执行计划在实现前存在。
- 完成声明前存在验证证据。
- 生成阶段清理记录。
- 交付结果必须满足冻结目标和交付物所要求的可观察行为，不能只满足内部流程进度。
- 只有当下游交付真实性通过后，才允许使用完整完成措辞。

## 人工抽查
除自动化或命令行验证外，本任务不要求额外人工抽查。若实现范围扩大到需要用户可见交互或硬件烧录验证，则必须单独标记为人工验证项。

## 完成措辞策略
- 只有治理真实性、工程验证真实性、工作流完成真实性和产品验收真实性全部通过时，才允许说“完成”。
- 如果存在 `completed_with_failures`、降级执行或待人工确认事项，必须说明为未完全完成状态。
- 如果人工抽查仍未完成，本轮只能描述为需要人工复核，不能描述为完全可交付。

## 交付真实性契约
- 治理真实性：需求、计划、执行和清理产物必须可追踪且权威。
- 工程验证真实性：目标验证必须显式通过或失败，沉默不算通过。
- 工作流完成真实性：计划单元、委派通道和专家输出必须回归到治理计划中。
- 产品验收真实性：可观察交付行为必须满足冻结验收标准，之后才允许完整完成措辞。

## 产物审查要求
本轮未冻结额外产物审查要求。

## 代码任务 TDD 模式
TDD 模式：需要。

决策来源：`runtime_inference`。

原因：本任务包含实现或缺陷修正意图，需要记录代码任务的 TDD 证据。

## 代码任务 TDD 证据要求
- 实现或修正前，记录目标行为的失败优先证据。
- 实现后，记录目标行为转绿的重新验证证据。
- 将变更行为映射到具体的验证证据；只说“整个套件通过”不够。
- 如果自动化失败优先证据不适合本固件任务，必须冻结并遵守明确的 TDD 例外原因，不能静默跳过。

## 代码任务 TDD 例外
本轮未冻结代码任务 TDD 例外。

## 基线文档质量维度
本轮未冻结基线文档质量维度。

## 基线 UI 质量维度
本轮未冻结基线 UI 质量维度。

## 任务特定验收扩展
本轮未冻结额外任务特定验收扩展。

## 研究增强来源
本轮未冻结研究增强来源。

> 防目标漂移字段只在这里填写一次。后续治理计划和完成产物应复用这些字段，而不是重新解释。

## 主要目标
实现 RadarBaseboardMCU7 第一项功能：USB CDC 输出模式切换，支持 `HOST_PROTOCOL` 与 `DEBUG_TEXT`，默认保持 `HOST_PROTOCOL`。

## 非目标代理信号
- 只通过单个样例。
- 只保证当前测试为绿色。
- 只完成演示效果。

## 验证材料角色
`validation_only`

## 防目标漂移等级
Tier C

## 预期范围
`scenario_specific`

## 抽象层目标
`Board.c` 附近的应用层输出模式框架，不下沉到 HAL/SPI/DMA。

## 完成状态
`partial`

## 泛化证据包
- cases: []
- note: 在声明更广泛完成之前，需要补充独立证据。

## 非目标
- 不把 M/L/XL 当作用户可见入口分支。
- 不引入第二套路由器或控制平面。
- 不实现雷达算法完整功能。
- 不改变官方上位机协议格式。

## 自主模式
`interactive_governed`

## 假设
- 如果未解决的歧义会实质改变实现，可以进行交互式澄清。
- 当前第一阶段优先实现调试输出模式框架。
- `Board_dataCallback()` 是后续算法接入的主要入口，但本轮不强行实现完整算法。

## 证据输入
- Source task: continue-vibe Implement RadarBaseboardMCU7 first feature USB CDC HOST_PROTOCOL DEBUG_TEXT output switching.
- Intent contract: `intent-contract.json`
- Runtime input packet: `runtime-input-packet.json`

## Runtime 输入真实性
- Governance scope: `root`
- Root run id: `20260430T113216Z-97b501b5`
- Entry intent: `vibe`
- Requested stop stage: `xl_plan`
- Requested grade floor: none
- Selected pack: `orchestration-core`
- Router-selected skill: `vibe`
- Runtime-selected skill: `vibe`
- Route mode: `pack_overlay`
- Route reason: `auto_route`
- Confirm required: `False`

## 专家决策
- 受治理的 `vibe` 必须在结束前明确记录专家执行是已经发生、仅作为建议保留，还是仍未解决。
- Decision state: `approved_dispatch`
- Resolution mode: `approved_dispatch`
- Notes: 有界专家建议已被展示，并提升为有效批准的分派。

## 专家建议
原始路由候选保留在 `runtime-input-packet.json` 里用于审计，不冻结为用户可见需求。
这里只列出主控采纳或有效批准的专家分派；未采纳候选和阶段助手不进入需求表面。

- Adopted Skill: `tdd-guide`
  Role: `specialist_assist`; native usage required: `True`; preserve workflow: `True`
  Binding: profile=`default`; phase=`in_execution`; lane policy=`inherit_grade`; parallel in XL=`True`
  Write scope: `specialist:tdd-guide`; review mode: `native_contract`; execution priority: `50`
  Reason: 由 `code-quality` 包通过 `fallback_task_default` 排名最高的专家候选。
  Required inputs: 有界专家子任务契约、冻结需求上下文、相关源文件或领域产物。
  Expected outputs: 有界专家发现或代码变更，以及符合专家技能的验证说明。
  Verification expectation: 保留专家技能原生工作流、边界和验证风格。

- Adopted Skill: `scikit-learn`
  Role: `specialist_assist`; native usage required: `True`; preserve workflow: `True`
  Binding: profile=`default`; phase=`in_execution`; lane policy=`inherit_grade`; parallel in XL=`True`
  Write scope: `specialist:scikit-learn`; review mode: `native_contract`; execution priority: `50`
  Reason: 由 `data-ml` 包通过 `fallback_task_default` 排名最高的专家候选。
  Required inputs: 有界专家子任务契约、冻结需求上下文、相关源文件或领域产物。
  Expected outputs: 有界专家发现或代码变更，以及符合专家技能的验证说明。
  Verification expectation: 保留专家技能原生工作流、边界和验证风格。

## 专家咨询
以下专家是在该需求文档冻结前，由受治理的 `vibe` 在讨论阶段解析出来的。根据策略，它们可能被实时咨询，也可能被路由为当前会话直接加载。

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

## 记忆上下文
有界、阶段感知的记忆上下文已注入需求冻结阶段：

- Disclosure level: `decision_focused`
- Capsule [`dc89086b565d9ad1`] Task focus: RadarBaseboardMCU7 USB CDC `HOST_PROTOCOL` / `DEBUG_TEXT` 输出模式切换。
  Owner: `state_store`
  Why now: 与 `requirement_doc` 阶段的 state_store 记忆匹配。
  Expansion Ref: `C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\outputs\runtime\vibe-sessions\20260430T113216Z-97b501b5\memory-activation\skeleton-local-digest.json#dc89086b565d9ad1`
  Summary: 任务聚焦于 RadarBaseboardMCU7 的 USB CDC 输出模式切换、受治理实现产物、验证证据和清理记录。
  Summary: Git branch: 
  Summary: 所有必需的治理 runtime 前置路径均存在。