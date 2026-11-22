# 验收清单与证据包（CADBundle 0.1.0 Unreleased）

> 状态：**部分人工验证已完成（截至 2026-11-22，R5-4 已合并）**；R5-4 默认值变更的复跑证据待补，定版前仍需完成 §5 未验证项。任何未执行项必须写“未验证”，不得写成通过。

## 1. 基线

> 以下为 R5-3 历史基线；R5-4 干净 Rebuild 后替换为当前 HEAD 的真实提交、大小与 SHA。

| 项目 | 值 |
|---|---|
| 分支 | `main` |
| 发布候选提交 | `5f9550c`（merge: R5-3 表格字体回落与性能验证） |
| 构建 | Release x64；R5-3 V-10 全新 `Rebuild` 0 warning/0 error（2026-11-21）；2026-11-22 增量重建复核通过 |
| 产物 | `build/x64/Release/plugins/CADBundle.dlo`，188,928 字节 |
| SHA-256 | `096e4445ac09087b0b4a5e568b889edd77a7d64542af28d5a384ce5e03ec15eb`（V-10 构建）。注意：PE 头 `time date stamp` 使重建哈希变化（2026-11-22 重建为 `863490de…`）；发布证据以源码提交 + 大小 + 导出/依赖校验为准，或改用 `/Brepro` 可复现构建 |
| 导出 | LibDescription / LibNumberClasses / LibClassDesc / LibVersion / LibInitialize / LibShutdown / CanAutoDefer，全部正确 |
| 架构/依赖 | x64；core/Geom/maxutil/ParamBlk2 + CRT；无 Qt |
| V1 阈值 | 屏幕恒定允许偏差 ≤ max(2 px, 5%)（已裁决） |

## 2. 验收判据映射（A1–A13）

| 编号 | 判据 | 方法/测试 | 结果 |
|---|---|---|---|
| A1 | 单 DLL、4 ClassDesc（U11 由 5 降为 4）、7 导出 | 静态 + 批处理 | 通过（静态/批处理：7 导出正确、x64、无 Qt） |
| A2 | 可加载、四类在创建面板可见 | 3dsmaxbatch + plugin.ini；GUI 创建面板 | 通过（批处理四类构造 + 保存重开；2026-11-20 用户复核创建面板按钮 `CAD标注/CAD文字/CAD引线/CAD表格`） |
| A3 | ~~Utility 面板~~（U11 已移除） | —— | 不适用（B-23 随之关闭） |
| A4 | 四类公开可创建（U11 放宽） | 静态 + 批处理构造 | 通过（静态/批处理；创建面板显示经用户复核） |
| A5 | 四类对象可创建 | MAXScript 构造；面板/鼠标创建 T4/T7/T9/T10 | 部分验证：MAXScript 构造通过；创建面板按钮用户复核（2026-11-20）；逐工具鼠标创建步骤未单独归档 |
| A6 | 显示模式：新建默认 sizeMode=1；屏幕恒定（0）需显式设置后验证（箭头/文字/延伸线/表格） | T6 像素测量 1×/2×/4×，3 次，V1 判据；V-07 bbox | 部分验证：V-07 16/16 为 R5-2 旧默认证据；R5-4 复跑后回填 |
| A7 | 文字：字体/字号/颜色生效，回退可见 | T7/T8；V-03b | 部分验证：V-03b 字体切换目视通过（用户，2026-11-20）；BezFont/CJK 回退有实现与回归覆盖；字号/颜色逐项目视未单独归档 |
| A8 | 引线：折线/箭头/注释，坐标 1e-4 一致 | T9；V-05 | 部分验证：V-05 渲染/命中目视通过（用户，2026-11-20）；坐标 1e-4 逐项比对未单独归档 |
| A9 | 表格：网格、编辑器、行列迁移 | T10–T13；V-01 | 部分验证：表格编辑器逐格编辑经用户复核（V-01，2026-11-20）；行列迁移按交集保留有实现；迁移场景未单独归档 |
| A10 | 保存→Reset→重开逐字段一致；2×3 实验 | T14/T15/T16；R5 参数套件 | 部分验证：四类参数与 2×3 cellTexts 批处理通过；sizeMode 三循环 20/20、旧 DLO 场景 13/13 均为旧产物证据（夹具污染/默认值变化），R5-4 复跑后回填；损坏 chunk T16 待人工 |
| A11 | 屏幕恒定公式为完整式 | 静态（与 A12 交叉） | 静态通过 |
| A12 | 连续线经 DrawLineProc/polyline；无 DrawLine/DrawRectangle | 静态扫描 | 静态通过（0 处） |
| A13 | 完整性：无 TODO/占位、资源 ID 一致、行数约束 | 静态 | **部分通过**：无 TODO/占位、资源 ID 一致、`r5_1_static.ps1` fails=0；但 7 个源文件超 500 行（`CADBezFontText.cpp` 581、`CADDimensionRender.cpp` 569、`CADDimension.cpp` 564、`CADLeader.cpp` 520、`CADTable.cpp` 510、`CADText.cpp` 502、`CADTableRender.cpp` 502），行数约束未通过 |

完整 T1–T21 矩阵与回滚模板见 `docs/development/07-test-release.md` §9/§10。

## 3. 证据包模板

每次运行验证建立独立目录（不入库），至少包含：

```
evidence/<日期>/
  environment.txt          # Max 版本、系统版本、DPI、显示器分辨率、显卡
  build.txt                # 构建命令、日志尾部、产物 SHA-256
  screenshots/             # 按测试编号命名：T03-panel.png、T06-1x.png ...
  measurements.csv         # 像素测量原始数据
  fields.csv               # 保存/重开逐字段比较
  notes.md                 # 偏差、失败步骤、复现条件
  test.max                 # 保存的测试场景（非生产数据）
```

**measurements.csv** 表头：

```
test_id,view,zoom,tool,element,expected_px,measured_px,deviation_px,deviation_pct,repetition,pass
```

**fields.csv** 表头：

```
object_type,object_name,field,value_before,value_after,tolerance,pass
```

- 浮点比较容差 1e-4；`cellTexts` 逐格列出完整字符串；2×3 实验使用六个唯一字符串。
- 坐标/角度/箭头方向按 07 §9 B-17 判据（坐标 1e-4；箭头 dot ≥ 0.999；note 精确一致）。

## 4. 运行验证步骤（第 7/11 步）

1. **备份旧插件（条件执行）**：若 `<Max>\plugins\CADDimension.dlo` 存在，复制到 plugins 之外的备份目录并记录 SHA-256/大小；不存在则记录“不存在”。
2. **安装**：复制 `CADBundle.dlo` 到 `<Max>\plugins`。
3. **启动与加载**：验证 A2，截图创建面板。
4. **创建与交互**：按 T4/T7/T9/T10 执行；**B-21 点序**：1 次点击应为 point 0 提交，3 次点击为 point 0/1/2，最后一点 STOP；若实际不同，**立即停止并回报**，不得自行改点序或记为通过。
5. **屏幕恒定**：T6，固定 1280×720、透视+正交、1×/2×/4×，各 3 次，记录原始像素。
6. **保存/重开**：T14（四类对象逐字段）、T15（2×3 表格）、T16（损坏 chunk 不崩溃）。
7. **中止/关闭/撤销/复制**：T17/T18。
8. **恢复旧插件（若备份过）**：删除新 DLO，恢复备份，核对 SHA-256/大小一致（B-06）。
9. **回滚（失败时）**：按 `docs/development/07-test-release.md` §10 三段模板执行；先报告冲突/失败原因，不擅自 `reset --hard`。

## 5. 已知未验证项（不得写成通过）

以下为截至 2026-11-22 仍未归档的验收证据；其余项见 §2 与 §9。

- A5：逐工具面板/鼠标创建 T4/T7/T9/T10 的分步记录（创建面板可见与按钮名称已复核）。
- A6：T6 规定的逐像素测量（V1 判据）；现有证据为 V-07 bbox 套件（16/16）。
- A7：字号/颜色逐项目视；字体切换（V-03b）已通过。
- A8：引线坐标 1e-4 逐项比对；渲染/命中目视（V-05）已通过。
- A9：行列迁移按交集保留的专项场景；表格编辑器逐格编辑已复核（V-01）。
- A10：损坏 chunk（T16）。
- T17/T18：中止/关闭/撤销/复制；仅字体下拉与 sizeMode 下拉撤销同步有专项修复与静态覆盖。
- 完整克隆/复制/撤销/重做、旧 DLO 共存与恢复（B-06）。
- A13：源文件行数约束（7 个文件超 500 行，见 §2 A13）。
- 空文本/长文本逐项目视（截断逻辑有实现，V-07 覆盖表格用例）。
- 表格 GDI 回落的目视外观；回落路径与数值性能已由 V-06A/B 覆盖。
- 旧 `.max` 场景兼容不在 v1 承诺范围；V-02b 旧证据已作废，待干净夹具复跑。
- 旧场景 V-02b：原夹具被 `r5_2_old_load.ms:36` 重存污染；须用 old_stage 旧 DLO 重新生成 pristine 夹具，再以当前 DLO 载入，记录旧/新双哈希。
- 表格 `fontSize=0` 自动字号语义：代码证据为 cellHeight×0.6（屏幕夹取 6–48 px；模型夹取 0.01–10000 单位），运行目视待验证。
- 面板标签：删除“(屏幕)”限定词后核对可读性、截断/换行，覆盖 100/150/200% DPI。
- 测试脚本 SHA：脚本冻结后计算并记录；脚本未入库（J-7），复现依赖本地附件与 SHA。

## 6. 定版条件（流程 11.6）

人工运行验证通过后，才在独立发布分支完成：

1. 更新版本号与 VERSIONINFO；
2. 更新 `CHANGELOG.md`（去除 Unreleased，记录验证证据路径与产物 SHA-256）；
3. 经合并流程进入主干。
4. 核对 Commit B 的 rc 标签与最终 VERSIONINFO 一并生效。
5. tag 指向最终 HEAD；产物再变须重做；tag 与合并前经用户确认。

## 7. R1 批处理验证（2026-11-19）

- 命令：`3dsmaxbatch.exe build/verify/smoke_r1.ms -p build/verify/plugin.ini -dm on -log build/verify/smoke_r1.log -v 2`
- 结果：`BATCH_EXIT=0`，29 项检查全部 PASS（`fails=0`）。
- 覆盖：四类构造；Dimension P0/P1/P2/箭头/文字/延伸线/精度；Text 位置/内容/字号/字体；Leader 三点/注释/箭头/文字；Table 行列/插入点/单元格宽高/**2×3 六格文本逐格往返**。
- 修复：B-24（`SyncCellsFromMirror`）。
- 不覆盖：鼠标创建与点序（B-21）、修改面板 UI、显示/命中、屏幕恒定像素（R2 后）、损坏 chunk、DPI、旧插件共存。


## 8. 自动运行验证记录（历史，2026-11-19）

> 以下为 Utility 面板移除（U11）之前的记录，保留供追溯；其面板相关步骤已不适用。

- 安装：发布候选 `CADBundle.dlo`（SHA-256 `0191f654…740c1b`）复制到 3ds Max 2027 `plugins`；旧 `CADDimension.dlo` 不存在，无需备份。
- 已通过（自动）：
  - 新 Max 实例加载 DLL 无崩溃（多次 `EXIT=0`）；
  - `CADDimension` / `CADText` / `CADLeader` / `CADTable` / `CADAnnotatePanel` 名称均可被 MAXScript 解析（ClassDesc 已注册）；
  - `utilityPanel.openUtility` 返回，面板打开标记写入成功。
- 预期内不可用：`createInstance` 无法创建 `IsPublic=FALSE` 的四类对象（与 U3/C2 裁决一致）；因此 T14/T15 保存重开无法程序化执行。
- 待人工确认（B-23）：
  1. 启动 Max（如出现场景恢复提示，按提示处理）；
  2. 打开 Utilities → “CAD 注释”；
  3. 用面板“关闭”按钮关闭，再次打开，确认无挂起、无残留创建模式；
  4. 从面板分别创建四类对象，按 §4 执行 T14/T15。
- 环境说明：自动验证强制终止过 3 个测试用 Max 实例（原有实例未触碰）；若后续启动出现恢复提示属预期现象。
- **B-23 关闭**：U11 移除 Utility 面板后，本项不再适用（见 `docs/reviews/loop-ledger.md` R1 记录）。

## 9. R5 验证记录（2026-11-20 ～ 2026-11-22）

> 以下 R5-1～R5-3 数字为历史记录；R5-4 默认值变更后，20/20、13/13、r5_4 12 PASS 均不得作为当前放行证据。

- R5-1 合并 `6dffbb4`：四类字体 UI、共享度量、`CADScreenDraw.cpp` 拆分、四对话框 ≤190 DLU；用户 GUI 确认 V-03b/V-04/V-05。
- R5-2 合并 `23fabbf`：四类 sizeMode（屏幕/模型）与撤销同步；params 20/20、mode 4/4、V-07 16/16、旧场景 V-02b 13/13；用户 GUI 确认四面板切换与旋转/透视命中/bbox。
- R5-3 合并 `5f9550c`：表格 GDI 回落滞回；V-06A/B（50×50 p50 0.377 ms / p95 1.024 ms；10×10 p50 1.139 ms / p95 3.199 ms，均 PASS）；V-08 回滚演练三次构建 0 error + smoke 29 PASS；V-10 干净重建 0 warning/0 error、诊断残留 0。
- 回归：`smoke_r1` 29/29；R5-1 params 11/11 + measure 4/4；R5-2 params 20/20 + mode 4/4 + V-07 16/16；`r5_1_static.ps1` fails=0（2026-11-22 复跑）。
- 证据：`docs/reviews/loop-ledger.md`、`build/verify/*_result.txt`（其中 `r5_3_perf_gui_result.txt` 为已作废探针，文件头已标注）。
- R5-4 复跑：待回填（干净 Rebuild + pristine 夹具 + 更新后断言）。
