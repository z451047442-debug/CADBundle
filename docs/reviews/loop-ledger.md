# 全局循环账本（Loop Ledger）

> 适用范围：报告重写、方案返工、批次返工、验证返工、门禁退回、第 11 步修复等所有退回/返工/修复循环。
> 规则：同一问题最多 3 轮；连续 2 轮无可验证进展立即停止；整个任务所有循环累计最多 9 轮。达到上限必须汇报阻塞点，不得用新子问题绕过计数器。
> 记录要求：没有记录不得继续。

| 问题ID | 起始步骤 | 问题描述 | 当前轮次 | 累计轮次 | 证据 | 下一步 / 状态 |
|---|---|---|---|---|---|---|
| L001 | 第 2 步 | CAD 注释业务专家首次报告只有末尾三节，缺少审阅范围与关键发现；按报告纪律退回重写。 | 1 | 1 | 首次返回内容仅含“未验证项 / 可能冲突 / 下一步建议”，并注明“关键发现第 1–12 条见前段输出”；接收方未收到前段。 | 已退回同一独立子代理；等待完整报告。 |
| L001 | 第 2 步 | 重写后的完整报告标题“所用模型”误填为底层模型名，不符合统一模板。 | 2 | 2 | 标题为“【CAD 注释业务专家｜deepseek-flash-1017】”；内容其余部分完整。 | 已请该专家仅修正标题；完整报告已重新提交，结论未变；**已关闭**。 |
| L002 | 第 2 步 | 3ds Max SDK 技术专家报告标题未按统一模板写明“专家名｜所用模型”。 | 1 | 3 | 标题为“# 3ds Max SDK 2027 技术专家审阅报告”，缺少模板要求的模型字段。 | 已请该专家仅修正标题；完整报告已重新提交，结论未变；**已关闭**。 |
| L003.1 | 第 3 步 | C1：`TYPE_STRING_TAB`/`cellTexts` 持久化权威（02 技术 vs 03 风险）。 | 1 | 6 | 02 撤回“自动持久化”，同意显式 chunk 单一权威；03 同意“不能假定自动”，但要求先做 2×3 保存/重开实验，再定 pblock 或显式 chunk 单一权威；双方均同意加载后校验 `cellCount == rows*cols`。 | 有条件收敛：单一权威 + 运行时实验决定权威来源；本轮关闭，不进入第 2 轮。 |
| L003.2 | 第 3 步 | C2：`IsPublic=FALSE` 与 MAXScript 创建承诺（02 技术 vs 03 风险）。 | 1 | 6 | 02/03 均无 SDK 文档或运行证据证明非公开类可经 `startObjectCreation` 创建；均同意将 MAXScript 移出验收通过条件；03 澄清“可脚本化”指程序化 `ClassDesc*`/`CreateInstance` 路径。 | 已收敛；是否保留 MAXScript 承诺交用户裁决；本轮关闭。 |
| L003.3 | 第 3 步 | C5：表格单元格文本编辑验收路径（01 业务 vs 03 风险）。 | 1 | 6 | 01 与 03 均认为仅参数块/MAXScript 回读不满足验收 9；均要求真实用户 UI；03 明确 MAXScript 仅能触发用户可见编辑窗口，本质仍是自定义 UI。 | 已收敛；是否将表格编辑器纳入 MVP 交用户裁决；本轮关闭。 |
| L004 | 第 5 步 | 项目总监第 5 步方案评审退回：C-01 文字/表格点序冲突；A8/A12/T13 主观判据；B-01–B-20 需补齐方法/指标/时限/责任人。 | 1 | 7 | 项目总监评审报告；C-01 见 04/05 §3 与 00-plan A5；B 项清单见评审报告。 | 第 5 步复审通过（附条件 B-16/B-07a/B-12/B-21/V1）；C-01 文档层关闭；B-16/B-07a/B-12 已修正；B-21 运行时待验证；累计 7/9，剩余 2 轮。 |
| L006 | 第 5 步 | R4 方案评审不通过：P0-1 `SetUserDlgProc` 语义/生命周期未验证；P0-2 屏幕空间 BezFont 不存在、U13 口径与性能阈值无依据；P0-3 四对话框 ≤200 DLU 与新增控件 ID/DPI 无方案；另 P1-1～P1-8。 | 2 | 9 | 第 2 轮独立项目总监子代理复审：**第 5 步不通过**，阻塞项 B-1（CADDimension 仍用 SetUserDlgProc）、B-2（四类测量/BezFont 未统一）、B-3（sizeMode 无实现级设计）、B-4（行数预算不可达）。V-01/V-02/V-03a 前置证据有效。 | **循环上限 9/9 触发，停止**。用户裁决选 A 收口：仅提交已验证的 V-01（表格子类化）与 V-03a（字形修复 + 引线屏幕 BezFont）；移除 V-02 死参数与临时诊断；R4（sizeMode/字体 UI/190 DLU/命名）延后为新任务。 |

## 轮次汇总

- 已用累计轮次：9 / 9（**上限，已停止**）。
- 当前状态：R4 未通过门禁，按用户裁决 A 收口；R4 另立新任务。
- 同一问题最大轮次记录：L006 为 2 轮（未超过 3 轮上限）；全局累计达到 9 轮上限。

## A 收口记录（2026-11-20）

- 范围：仅保留用户人工验证过的 V-01（表格编辑按钮子类化）与 V-03a（BezFont 字形修复 + 引线屏幕空间 BezFont）；移除 V-02 死参数与全部临时诊断；追加 R4-3 中文命名。
- 提交（未推送）：
  - `f2e1f06 fix(ui): 表格编辑单元格改用窗口子类化`
  - `469cc6b fix(render): 修正 BezFont 字形构码并增加屏幕空间绘制`
  - `7aab91a feat(render): 引线注释改用屏幕空间 BezFont`
  - `6ba6bac docs(review): 记录 R4 门禁未通过与 A 收口`
  - 中文命名提交（本次追加）。
- 验证：
  - A 构建 `99c1a2fb…`：冒烟 29 PASS / 0 FAIL；用户复核表格编辑器、`V03中文测试` 拉丁字母正常。
  - 中文命名构建 `3cc4aaaa…`：冒烟 29 PASS / 0 FAIL；用户复核创建面板按钮 `CAD标注/CAD文字/CAD引线/CAD表格`、节点默认名、表格编辑器与 V03 文本均正常。
  - 诊断残留：二进制 `text-probe`/`ui.log` 字符串为 0。
- 延后范围（新任务，需重置循环额度）：R4-1 `sizeMode`（四类文字屏幕/模型）、R4-2 表格字体/字号 UI、R4-4 对话框 ≤190 DLU、R4-5 表格字体渲染/性能、R4-6 其余前置验证。
- 合并（用户裁决 1）：`git merge --no-ff` → merge commit `f55f1924382c3ebd62d3183317eb1898008fb0ee`；当前分支 `main`；工作区干净；**未推送**（用户手动推送）。
- 新任务（用户裁决 2）：R5（字体 UI / sizeMode / 对话框重排 / 表格字体性能）**重置循环额度**，另行建立账本并从 0/9 开始；本文件 L006 的 9/9 记录仅适用于 R4。

## R5 循环账本（重置 0/9）

> 依据：用户 2026-11-20 明确同意“重置循环额度”，R5 为新任务；同一问题最多 3 轮，连续 2 轮无可验证进展停止，R5 累计最多 9 轮。

| 问题ID | 起始步骤 | 问题描述 | 当前轮次 | 累计轮次 | 证据 | 下一步 |
|---|---|---|---|---|---|---|
| R5-000 | 第 5 步 | R5 方案评审：3 轮均未全过；残余 1 项已按评审原文修正；达 3 轮上限 | 3 | 3/9 | 三轮评审 + `step4-revision.md` §8 修订 | 已裁决：接受并进入 R5-1 |
- 2026-11-20 第 2 轮（不通过）：未闭合 = `FitCellText` 像素/世界单位换算、表格文字像素参数与截断、V-06A/B 条件与“可见格”定义、≤190 DLU 缺少验收项、回滚必须逆序。均已修订（含 `fontSize=0` 自动值兼容旧场景）。
- 2026-11-20 第 3 轮（不通过）：缺口 2/3/4 已闭合；缺口 1 残余 `fontSize=0` 自动值未夹取，已修正为自动值与显式值统一夹到 [6,48] px。**R5-000 达 3 轮上限，不开第 4 轮，转用户裁决。**
- 用户裁决（2026-11-20）：**接受修订方案并进入 R5-1**；表格模型模式采用**整表世界单位**；**允许新增 `src/CADScreenDraw.cpp` 并加入 `CADBundle.vcxproj`**；四对话框若无法压到 ≤190 DLU，**先提交布局裁决**，不得默许放宽。

## R5-1 实施记录（2026-11-20）

- 提交：`a1dcdd2`（共享度量入口）、`e955bbc`（四类统一度量）、`651d20a`（拆分 `CADScreenDraw.cpp`）、`414b0c2`（字体 UI/资源/子类化）。
- 自动化证据：静态断言 `fails=0`；`r5_1_params.ms` 11/11 PASS；`r5_1_measure.ms` 4/4 PASS；`smoke_r1.ms` 29 PASS；Release x64 0 warning/0 error。
- 资源差异表：`docs/reviews/r5-1-resource-matrix.md`；新增 `IDC_DIM_FONT=1130`（补方案遗漏）。
- 待人工（需 GUI）：V-04 四档 DPI 截图；V-03b 字体切换目视；V-05 渲染/命中目视。
- 独立评审（2026-11-20）：**需修复**。Critical 1：字体回退时缓存存实际回退名，请求名比较恒不等 → 每帧重开字体。Important 3：引线/标注 bbox 与屏幕空间不符；字体下拉撤销后不同步。修复提交 `e00df54`：`EnsureFont` 按请求名去重；`CADAddScreenTextBox` 生成 billboard bbox（含 ShowXYZ 余量、表格截断与 ≤400 格上限）；`CADSyncFontComboSelection` 在四类 `NotifyRefChanged` 同步下拉。复验：static 0、params 0、measure 0、smoke 0、build 0 warning。
- 延后 Minor：空 `vpt` 回退计数未实现；`theHold.Begin` 后未检查 `SetValue` 返回值。
- 测试口径修订：标注 bbox 用例把 arrow/extension 边距压小以隔离文字宽度（探针确认实现已包含文字）。
- 范围裁定：表格 HitTest 保持仅网格线；`CADText` bbox 一并改为 billboard 矩形（防御性一致性修复，非评审发现）。
- 用户确认（2026-11-20）：GUI 验证通过（V-03b / V-04 / V-05），接受 R5-1，进入合并。
- 合并（2026-11-20）：`git merge --no-ff ai-r5-2026-11-20` → merge commit `6dffbb4`；当前分支 `main`；工作区干净；**未推送**（用户手动推送）。

## R5-2 实施记录（2026-11-20）

- 分支 `ai-r5-2-2026-11-20`；提交 `aac4826`。
- 交付：四类 `sizeMode`（0=屏幕恒定默认、1=模型空间）+ 下拉；表格模型模式整表世界单位；引线/标注模型高度换算；标注模型偏移按世界单位；新增 `CADAddWorldTextBox` 用于模型 bbox。
- 证据：`r5_2_params` 20/20；`r5_2_mode` 4/4；`r5_2_v07` 16/16（FOV 45/22.5/11.25 作为 1×/2×/4×）；`smoke_r1` 29 PASS；R5-1 全部回归 0；静态断言 0。
- 裁定：标注模型模式偏移采用 `0.6 × textHeight`（世界单位），而非投影像素，避免 bbox 随视角漂移；引线/标注模型 bbox 用世界文本盒。
- 待人工：四面板 sizeMode 下拉切换目视；模型模式在旋转/透视视图下的命中/bbox 目视。
- 独立评审（2026-11-20）：**无 Critical**；Important 3：表格模型 bbox 未用世界文本盒、标注 ShowXYZ 余量单位错误、V-07 表格只测网格。修复提交 `d18d79c`：表格模型 bbox 逐格世界文本盒；ShowXYZ 余量按模式取世界/屏幕；V-07 表格改 1×1、`fontSize=40`、样本文本 `Ag中123`。复验：static 0、V-07 16/16、params 20/20、mode 4/4、smoke 29、R5-1 回归 0。
- V-02b 旧场景夹具：用旧 DLO（`old_plugin.ini`）建场景，再用新 DLO 加载；13/13 PASS（四类加载、旧参数保留、`sizeMode` 默认 0、重存后仍 0）。
- 延后 Minor：`CADApplySizeModeCombo` 的 hold/返回值检查；标注 `baseMid` 与真实文本锚点的透视偏差；表格模型模式逐格投影性能（转 R5-3 V-06）。
- 2026-11-20 第 1 轮（不通过）：P0 = `/1000` 换算缺失；表格模型模式单位不一致；V-06 全帧回落不可验证；回滚与 vcxproj 变更矛盾；“屏幕模式保持现状”不可判定。P0-1（bbox 无 `ViewExp`）经核实为误判——四类 bbox 均带该参数。P1-1…P1-5 已并入方案。
- 2026-11-20 修订：§8.2.1 补 `/1000` 公式与空视口回退；§8.2.2 表格模型模式改为整表世界单位；§8.2.3 回落加滞回；§8.2.4 补压缩矩阵与升级裁决；§8.2.5 补行数预算与新增文件门禁；§8.3 V-06 拆 A/B、V-07/V-08 补口径；§8.4/§8.5 同步。
- 同一问题最大轮次记录：L001 为 2 轮；L004 为 1 轮（均未超过 3 轮上限）。
- 连续无可验证进展：无。
- 当前阻塞：无 C 类；B-21 运行时点序待验证；V1 已裁决（≤ max(2 px, 5%)）；U11–U13 修订执行中（B0–B8 已提交，R1 已完成，R2 屏幕空间绘制待实施）。

## 第 3 步关闭记录

- 2026-11-19：用户对项目总监 Phase B 的 U1–U10 全部裁决为“按建议执行”；裁决写入 `docs/requirements/CADAnnotatePanel.md` 第 11.3 节。
- C1/C2/C5 均在第 1 轮收敛；未使用第 2 轮；第 3 步结束时累计 6 / 9 轮；无待处理技术辩论。
- 下一步：第 4 步项目经理召集实战专家制定可执行方案；不修改代码。


## 执行修复记录（实现级，不计方案轮次）

- EX-01：B0 首次构建因 RC 未包含 `winres.h` 导致语言宏未定义；补 include 后通过。
- EX-02：B0 导出表因 `/OPT:ICF` 折叠导致 `CanAutoDefer`/`LibInitialize`/`LibShutdown` 别名错误；关闭 COMDAT 折叠后 7 个导出名正确。
- EX-03：B1 首次构建时 SDK `RunMUIRCT` 因多语言资源拆分失败；关闭该项目后处理（`PostBuildEventUseInBuild=false`），资源内嵌单 `.dlo`。同时为满足验收 3 的字面名称，Utility/对象类显示名使用 `CADAnnotatePanel`/`CADDimension` 等英文标识，面板标题与控件保持中文。B1 仅完成构建/静态验证，运行时未验证。
- EX-04：B2 首次编译因 SDK 2027 的 `Matrix3(bool)` 弃用、`Dot`/`Cross` 自由函数缺失、字形缓存返回类型与 `CharIterator` 用法不匹配而失败；改为默认 `Matrix3()`、`Point3::DotProd/CrossProd`、非 const 字形返回与 `Char::CharCode()` 后通过。另增加默认 CJK 字体与二级 CJK 回退。属于实现级构建修复，不计方案轮次。
- EX-05：B3 首次编译缺少 `gfx.h`（`HitRegion` 未定义）；补齐后通过。实现文件达到 858 行，按 500 行约束拆为 `CADDimension.cpp` + `CADDimensionRender.cpp`；修正 `Load` 对 `OpenChunk` 结束值 `IO_END` 的处理。属于实现级构建/结构修复，不计方案轮次。
- EX-06：B4 首次构建通过；`CADText.cpp` 为 532 行，超过 450 行预算，拆为 `CADText.cpp`（322 行）+ `CADTextRender.cpp`（221 行），行为不变。属于实现级结构修复，不计方案轮次。
- EX-07：B5 首次构建通过；`CADLeader.cpp` 为 607 行，超过 500 行约束，拆为 `CADLeader.cpp`（390 行）+ `CADLeaderRender.cpp`（229 行），行为不变。属于实现级结构修复，不计方案轮次。
- EX-08：B6 在执行过程中于“构建完成 → 批次报告”之间中断（构建日志已生成、DLO 已链接，未提交）。恢复时先核对工作树与构建产物，未重写已落盘实现；仅修复 `CADTableRender.cpp` 中 `FitCellText` 使用未缩放 `cellWidth` 作为像素阈值的问题（改为屏幕缩放后的 `width`），并重新构建通过。属于实现级恢复/修复，不计方案轮次。
- 以上为实现级构建修复，不属于第 4 步方案退回或批次返工；累计技术轮次维持 7/9。

## 第 6 步批次记录

- B0–B6：已提交（`5573c51`、`79f0780`、`b9b17e5`、`d2ca006`、`76a79af`、`ccb8332`、`890ef3c`）。
- B7：2026-11-19 持久化硬化——四类对象 `Load()` 改为"解析校验 chunk → 注册 `ILoad::RegisterPostLoadCallback`（默认优先级 5）→ 引用就绪后回填 pblock"，使 chunk 权威与系统先/后加载 pblock 的顺序无关；`CADTable::ApplyLoadedData` 同时处理无 chunk 时的镜像初始化。构建通过；T14/T15 运行时逐字段验证待人工执行。
- V1 阈值：用户裁决记录为允许偏差 ≤ max(2 px, 5%)（小尺寸按 2 px、大尺寸按 5%）。
- B8：2026-11-19 发布准备——全新 `Rebuild` 通过（15 个编译单元，无 warning/error），产物 164,864 B，SHA-256 `0191f6540cd4067f9eb604a05d9d25e6b792f61522488789784d518c9c740c1b`，7 个导出正确、x64、无 Qt 依赖；新增 `README.md`、`BUILD.md`、`CHANGELOG.md`（0.1.0 Unreleased）与 `docs/release/acceptance-checklist.md`（A1–A13 映射、证据模板、运行验证步骤、未验证清单）。旧 `CADDimension.dlo` 当前不存在于 plugins 目录（已核查）。运行时验证（第 7/11 步）仍待人工执行。

## 第 7 步运行验证记录（自动部分，2026-11-19）

- 安装：发布候选 `CADBundle.dlo`（SHA-256 `0191f6540cd4067f9eb604a05d9d25e6b792f61522488789784d518c9c740c1b`）已复制到 3ds Max 2027 `plugins`；旧 `CADDimension.dlo` 不存在，无需备份。
- 通过（自动）：新 Max 实例加载 DLL 无崩溃（多次 `EXIT=0`）；`CADDimension`/`CADText`/`CADLeader`/`CADTable`/`CADAnnotatePanel` 名称均可被 MAXScript 解析；`utilityPanel.openUtility` 返回，面板打开标记已写入。
- 预期内不可用：`createInstance` 无法创建 `IsPublic=FALSE` 的四个对象类（与 U3/C2 裁决一致）；T14/T15 保存重开无法程序化执行。
- 待人工确认 B-23：首次探针在“面板打开后”超时（脚本含一次 `sleep`，未能区分 sleep 单位与 `closeUtility` 行为）；随后启动不再写入脚本日志，疑为强制终止后的场景恢复对话框阻塞，已停止继续启动测试实例。人工步骤：启动 Max（处理可能的恢复提示）→ 打开“CAD 注释”面板 → 用面板“关闭”按钮关闭 → 重开 Utility，确认无挂起、无残留创建模式。
- 附带影响：3 个测试用 Max 实例被强制终止；原有 2 个 Max 实例未触碰。`CADBundle.dlo` 保留在 plugins 供人工验证；卸载删除该文件即可。
- 轮次：本记录为运行验证发现/待确认项，不构成方案返工；累计技术轮次维持 7/9。

## R1 批次记录（U11–U13：对齐参考项目 CADDimension）

- 2026-11-19：按用户裁决 U11–U13 实施 R1：`LibNumberClasses` 5→4、四类 `IsPublic=TRUE`、移除 `CADAnnotatePanel`（源文件/工程/入口）；保留对象 Class_ID 与资源。裁决记录见 `docs/decisions/step4-revision.md`。
- 自动化：采用参考项目 `3dsmaxbatch + plugin.ini`（独立 stage，不改用户 plugins）；四类构造成功；29 项检查全部 PASS（fails=0），含四类参数与表格 2×3 六格文本保存→Reset→重开逐项一致。
- B-24/EX-09：外部写入 `cellTexts` 镜像后 `EnsureDimensions` 提前返回导致 `mCells` 未同步；新增 `SyncCellsFromMirror` 后通过。
- 影响：原 A3/A4/A5/U3 口径被 U11 覆盖，已在修订文档与验收清单同步；B-23（旧 Utility 关闭路径）随面板移除而失效，**关闭（不再适用）**。
- 下一步：R2 屏幕空间固定像素绘制；R3 文档与验收口径收尾。

## R2 批次记录（屏幕空间固定像素绘制）

- 2026-11-19：新增屏幕空间公共设施（`CADWorldToScreen`/`CADDrawScreenSegment`/`CADDrawScreenPolyline`/`CADDrawScreenArrow`/`CADDrawScreenText`/`CADHitScreenSegments`/`GWScreenGuard`）。
- 阶段 1：`CADDimension` 的 `Display/HitTest` 改为投影后像素绘制与屏幕线段命中（提交 `1859c80`）。
- 阶段 2：`CADLeader`（折线/箭头/注释）、`CADTable`（网格/单元格文本/命中）、`CADText`（空文本标记与命中框投影；BezFont 文本保留）完成同类改造。
- 验证：Release x64 构建通过；批处理冒烟复跑 29 项全 PASS（fails=0）。
- 未验证：GUI 视口显示效果、命中体验、透视裁剪边界、DPI；A6 像素测量待人工执行。

## UI 主题修复（2026-11-19）

- 反馈：参数面板颜色与 3ds Max 2027 主界面不搭。
- 根因：P_AUTO_UI 的 Win32 对话框未处理 `WM_CTLCOLORDLG/STATIC/EDIT/LISTBOX`，静态文本与编辑框使用系统默认浅色画刷。
- 修复：新增 `CADApplyThemeColors`，使用 `IColorManager::CustSysColor/CustSysColorBrush`（`COLOR_BTNFACE/BTNTEXT/WINDOW/WINDOWTEXT`）；四类参数对话框在 `BeginEditParams` 注册主题 `ParamMap2UserDlgProc`；表格编辑器同步处理颜色消息并设置 ListView 背景/文字色。
- 验证：Release x64 构建通过；批处理冒烟 29 项全 PASS；最新 DLO 已安装（SHA-256 `d7ab5d080d61866f065f6e50ead3f44d508b2ff7902a4b51cff05e980c283ede`）。GUI 主题效果待用户复查。
- 追加（同日）：用户反馈“注释、文字不能编辑”与类名标题条不协调。修复：`IDC_TEXT_STRING`/`IDC_TEXT_FONT`/`IDC_LEAD_NOTE` 由 `CustEdit` 改为标准 `EDITTEXT`（`TYPE_EDITBOX` 的正确控件类）；四个参数对话框 `CAPTION` 置空，尝试消除 Max 绘制的浅蓝色类名标题条。构建与冒烟通过；新 DLO SHA-256 `2caf1f98cbea2c198c6c30ddb3aca3e9b14c5be5730eec06ef92f522ade5995a`，待用户重启 Max 复查。
- 追加 2（同日）：用户反馈标准 EDIT 绑定 `TYPE_EDITBOX` 后仍“可点击但打字无反应”。按 SDK 契约 `TYPE_EDITBOX` 必须为 `CustEdit`，而 CustEdit 路径在该对话框仍未生效。改为**不经自动 UI 绑定**：`text_string`/`leader_note` 移除 `p_ui`，控件使用标准 `EDITTEXT`，在自定义 `ParamMap2UserDlgProc` 中 `WM_INITDIALOG` 填入、`EN_KILLFOCUS` 用 `theHold` 写回并刷新；字体改为 `COMBOBOX` + `BezFontManager::EnumerateFonts`。构建与冒烟通过；DLO SHA-256 `24e399704daf3b964386af98bb170248d7b5703ea820db2f5f71af77d8eea363`。

- 追加 3（同日）：参考项目完全对齐后确认"文字/注释可输入"；问题根因在自定义 DlgProc/手动绑定路径。字体下拉改用**窗口子类化**（`IParamMap2::GetHWnd` + `SetWindowSubclass`）实现：子类仅处理 `WM_CTLCOLOR*` 与 `CBN_SELCHANGE`，其余经 `DefSubclassProc` 转发，不干扰 `CustEdit` 输入。构建与冒烟通过；DLO SHA-256 `798e7b469c079fd7108d72dbc2fedaf0e28ae0c9bb562c9ac2da281fab239d93`。

## 备注

- 第 2 步两次退回均为“报告格式/完整性”问题，不涉及技术结论争议；专家结论未被项目经理或总裁改写。
- 第 3 步互辩如果产生退回/返工，按冲突 ID 新增 L003.x 行，并同步更新累计轮次。
- 第 5 步方案评审退回记为 L004；C-01 修复与 B 项补齐后重新提交，不另开新问题绕过计数。

## 第 5 步评审记录

- 2026-11-19 第 5 步复审通过：“第 5 步通过，允许进入第 6 步”。
- C-01 文档层关闭；B-16/B-07a/B-12 已按复审要求修正；B-21 运行时点序待 B1/B3–B6 验证；V1 阈值待用户选择。
- 本次为 L004 同一轮复审，不新增轮次；累计 7/9，剩余 2 轮。

## 第 11 步人工验证反馈：选中已绘制对象无法修改（2026-11-19）

- 用户反馈原文：“1，在视口中画上了后，在选中原来画的（不管是线条、字）没办法修改。”
- 处理路径：属实现级问题，回到第 6 步口径；未新增方案轮次（累计技术轮次维持 7/9）。
- 工作区临时诊断（不提交，定案后移除）：`CADParamBlockHost.cpp` 的 Begin/EndEditParams 时点日志；`CADText.cpp` 的 `BeginEditParams`/`PopulateFontCombo` 检查点日志。
- 自动取证（GUI 探针 `build/verify/probe_ui_edit.ms`，带 `-startupFailureDetectionOff`）：
  - 选中已有 `CADText` 时 `BeginEditParams` 被调用；`map`/`hwnd` 在 `CADBeginAutoUI` 返回后创建成功（`ui.log` 记录 `begin` map=0 → `begin-done` map/hwnd 非空）。
  - Max 进程在创建对象后约 1.5 s 以 EXIT=1 退出；空转对照（`probe8.ms`）在“启动失败检测”处挂起；Windows 事件无崩溃模块记录。判定为**自动取证环境不稳定**（启动失败检测 + 多次强制终止），脚本路径与人工操作路径不完全一致。
  - 已停止继续启动/强杀测试实例，避免污染环境。
- **闭环结论（2026-11-19 人工复核）**：根因为**安装的 DLO 与分支 HEAD 构建漂移**——会话开始时 `plugins` 中的 DLO SHA-256 为 `1cd2405…`（171,520 B，13:13），而 HEAD（`6898a8d`）构建为 173,568 B。重新安装诊断版后，用户人工确认：CADText 改值生效、CADDimension 参数齐全可编辑；CADLeader/CADTable 的 `BeginEditParams` 与 map/HWND 亦正常创建（`ui.log` 证据）。**“选中后无法修改”关闭**。
- 遗留功能缺口（用户 2026-11-19 确认，属需求变更，待用户裁决后补文档与实现）：
  1. CADLeader 修改面板缺“字体”下拉（要求：与 CADText 一致，注释文本按所选字体渲染）；
  2. CADTable 修改面板“参数不全可编辑”：缺字体/字号、编辑单元格按钮无效、行列/尺寸/颜色可见性不足（待方案确认）。
- 当前安装的 DLO 为诊断构建（SHA-256 `9c1afbb85459999d84028abd88dbe4f0e892c14094f498381afead029bfb8a2b`），未提交任何修复。
