# 验收清单与证据包（CADBundle 0.1.1）

> 状态：**0.1.0 人工验证于 2026-11-22 由用户确认通过（未提供逐项明细）；0.1.1 标签裁切修复经用户目视确认（未提供新截图）**；R5-4 复跑证据见 §9。§5 所列未归档项仍按“未验证”保留，不得写成通过。

## 1. 基线

| 项目 | 值 |
|---|---|
| 分支 | `chore/release-0.1.1` |
| 发布候选提交 | 0.1.1 发布分支定版提交（含 VERSIONINFO）；源码逻辑绑定 `db44e06`；tag `v0.1.1` 将指向合并后的最终 HEAD |
| 构建 | Release x64 全新 `Rebuild`（2026-11-22 22:28，含 VERSIONINFO 0.1.1），0 warning/0 error；日志 `build/verify/r5_4_release_0.1.1_build.log` |
| 产物 | `build/x64/Release/plugins/CADBundle.dlo`，193,024 字节 |
| SHA-256 | `8d0f019414450394c493f666e495e29071b56aa7acfe2ca73974a6f8142c674f`；stage `build/verify/stage/CADBundle.dlo` 同哈希，进程模块路径已核。PE 头时间戳使重建哈希变化，以本次记录为准 |

> 产物绑定说明：0.1.1 发布 DLO 构建于发布分支定版提交（`src/` 与 `db44e06` 相同，新增 VERSIONINFO 0.1.1 与 Text 标签宽度修复）；其后的文档提交不改变二进制。tag `v0.1.1` 指向含文档的最终 HEAD；验收以产物 SHA 与构建日志为准。
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
| A6 | 显示模式：新建默认 sizeMode=1；屏幕恒定（0）需显式设置后验证（箭头/文字/延伸线/表格） | T6 像素测量 1×/2×/4×，3 次，V1 判据；V-07 bbox | 部分验证：V-07 16/16 为 R5-2 旧默认证据；T6 逐像素测量仍待人工（本次未复跑） |
| A7 | 文字：字体/字号/颜色生效，回退可见 | T7/T8；V-03b | 部分验证：V-03b 字体切换目视通过（用户，2026-11-20）；BezFont/CJK 回退有实现与回归覆盖；字号/颜色逐项目视未单独归档 |
| A8 | 引线：折线/箭头/注释，坐标 1e-4 一致 | T9；V-05 | 部分验证：V-05 渲染/命中目视通过（用户，2026-11-20）；坐标 1e-4 逐项比对未单独归档 |
| A9 | 表格：网格、编辑器、行列迁移 | T10–T13；V-01 | 部分验证：表格编辑器逐格编辑经用户复核（V-01，2026-11-20）；行列迁移按交集保留有实现；迁移场景未单独归档 |
| A10 | 保存→Reset→重开逐字段一致；2×3 实验 | T14/T15/T16；R5 参数套件 | 部分验证：R5-4 复跑 r5_2_params 25/25；旧场景 pristine 夹具 13/13，四类加载 sizeMode=0 且重存后仍 0；损坏 chunk T16 待人工 |
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
- 旧 `.max` 场景兼容不在 v1 承诺范围；V-02b 已用 pristine 夹具复跑（见下）。
- 旧场景 V-02b（R5-4 已复跑）：用 old_stage 旧 DLO 重新生成 pristine 夹具，再以当前 DLO 载入；四类加载 sizeMode=0，重存后仍 0（13/13）；记录旧/新双哈希（旧 `1cd24051…`，pristine `2e189f0f…`，当前 `f789f42a…`）。r5_2_old_load.ms 已改为重存到 resaved 路径，不覆盖源夹具。
- 字体字面诊断（2026-11-22）：默认“宋体”实际字面=宋体（W/i=500/500，拉丁等宽）；SimSun/NSimSun/Microsoft YaHei 在 BezFont 下落到 Arial。默认/UI 使用本地化名称；英文别名不得替代。
- CAD文字面板“显示模式”标签裁切（0.1.0）：0.1.1 已修复（标签宽 40、下拉 x=44/宽 142）；用户目视确认，README 截图保留 0.1.0 版本。
- 表格 `fontSize=0` 自动字号语义：代码证据为 cellHeight×0.6（屏幕夹取 6–48 px；模型夹取 0.01–10000 单位），运行目视待验证。
- 面板标签：删除“(屏幕)”限定词后核对可读性、截断/换行，覆盖 100/150/200% DPI。
- 测试脚本 SHA（2026-11-22 冻结，未入库，J-7）：`r5_2_params.ms` `5efd9c32868602ceb5f25c35a81aa11bc2d7a7f5aaf3cb4572b81501f49df10f`；`v02_verify.ms` `f10c601d724a18029a480722c10a88eb43c838f59e623875a60ea1027a3c21fa`；`r5_2_old_load.ms` `9ba4464b3adc8647c07c272e5a59c5b1ff5f5d52c805d1d5f9ae715f2bee61f5`；`r5_4_size_mode_test.ms` `f42405a766af72552325adc5b7c3bc6e2c7a982f9a4e5a6373da6ac1dfcff462`；`r5_1_measure.ms` `d15c88428778ddf0630b64840e4795c8045f80f5836be5c02bc7680b8f2849cb`。复现依赖本地附件与上述 SHA。

## 6. 定版条件（流程 11.6）

人工运行验证通过后，才在独立发布分支完成：

1. 版本号与 VERSIONINFO：已完成（0.1.1.0）。
2. `CHANGELOG.md`：已记录 0.1.1 修复与发布产物 SHA-256。
3. 经合并流程进入主干：待执行（--no-ff）。
4. rc 标签与 VERSIONINFO 一并生效：发布构建已核对（0.1.1.0）。
5. tag `v0.1.1` 指向最终 HEAD；产物再变须重做。

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

> 以下 R5-1～R5-3 数字为历史记录；当前放行证据见本节末尾 R5-4 复跑。

- R5-1 合并 `6dffbb4`：四类字体 UI、共享度量、`CADScreenDraw.cpp` 拆分、四对话框 ≤190 DLU；用户 GUI 确认 V-03b/V-04/V-05。
- R5-2 合并 `23fabbf`：四类 sizeMode（屏幕/模型）与撤销同步；params 20/20、mode 4/4、V-07 16/16、旧场景 V-02b 13/13；用户 GUI 确认四面板切换与旋转/透视命中/bbox。
- R5-3 合并 `5f9550c`：表格 GDI 回落滞回；V-06A/B（50×50 p50 0.377 ms / p95 1.024 ms；10×10 p50 1.139 ms / p95 3.199 ms，均 PASS）；V-08 回滚演练三次构建 0 error + smoke 29 PASS；V-10 干净重建 0 warning/0 error、诊断残留 0。
- 回归：`smoke_r1` 29/29；R5-1 params 11/11 + measure 4/4；R5-2 params 20/20 + mode 4/4 + V-07 16/16；`r5_1_static.ps1` fails=0（2026-11-22 复跑）。
- 证据：`docs/reviews/loop-ledger.md`、`build/verify/*_result.txt`（其中 `r5_3_perf_gui_result.txt` 为已作废探针，文件头已标注）。
- **R5-4 复跑（2026-11-22，HEAD `88a92b7`，DLO `f789f42a…`，192,000 B）**：static 77/0；smoke 29/29；r5_2_params 25/25；r5_2_old_load 13/13（四类加载 sizeMode=0，重存后仍 0）；r5_4 12/12；v02 24/24；r5_1_params 11/11；r5_1_measure 6/6；r5_2_mode 4/4；r5_2_v07 16/16。
- **字体字面诊断**：宋体→宋体（face/logfont=宋体，W/i=500/500）；SimSun/NSimSun/Microsoft YaHei→Arial。r5_1_measure 修正为显式 Arial + 默认宋体等宽/高于缺字回退断言。
- **人工验证（2026-11-22）**：用户确认通过（未提供逐项明细）；未归档项见 §5。
- **发布构建（含 VERSIONINFO，2026-11-22）**：`87a966e1…`，193,024 B；全量套件复跑通过（static 77/0、smoke 29/29、params 25/25、old 13/13、r5_4 12/12、v02 24/24、r5_1 params 11/11、measure 6/6、mode 4/4、v07 16/16）。
- **0.1.1 修复与验证（2026-11-22）**：Text 标签宽 26→40、下拉 x=32→44/宽=154→142；构建 `8d0f0194…`，193,024 B，VERSIONINFO 0.1.1.0；全量套件通过；用户在 GUI 目视确认标签完整（未提供新截图）。
