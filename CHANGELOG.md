# Changelog

本项目遵循“人工验证通过后才定版”的流程：当前为 Unreleased，版本号与 VERSIONINFO 资源在最终验证后统一更新。

## 0.1.0 — Unreleased（待人工运行验证）

### Added

- **R5-1 字体与度量统一**（2026-11-20，merge `6dffbb4`）：四类统一 BezFont 度量入口（`MeasureText`/`CADWorldHeightToPixels`）；引线/标注/表格新增字体下拉，表格新增字号（6–48 px）；四类参数对话框压缩至 ≤190 DLU；新增 `src/CADScreenDraw.cpp`（屏幕空间绘制/命中公共设施）。
- **R5-2 sizeMode**（2026-11-20，merge `23fabbf`）：四类新增 `sizeMode`（0=屏幕恒定默认、1=模型空间）与修改面板下拉；表格模型模式为整表世界单位并逐格生成世界文本盒；新增 `CADAddWorldTextBox`。
- **R5-3 表格字体回落**（2026-11-22，merge `5f9550c`）：表格 BezFont/GDI 渲染回落与滞回（可见格 >480 或像素高 <8 px 进入；≤360 且 ≥10 px 恢复）；`EstimateVisibleCells` 估算可见格；状态切换写 3ds Max 日志（不逐帧）。
- **创建入口**（U11）：4 个对象类 `IsPublic=TRUE`，创建面板 **CAD Tools** 与 MAXScript 构造函数均可创建；Utility 面板已移除。
- **CADDimension**：三点线性标注；ParamBlock2（P0/P1/P2、showXYZ、箭头、文字高度、延伸线、RGB 颜色、精度）；延伸线、闭合三角箭头、BezFont 距离文本、可选坐标标签；退化几何点标记。
- **CADText**：单击放置；ParamBlock2（位置、内容、字号、字体、RGB 颜色）；BezFont 字形轮廓与 billboard；空心标记可选中。
- **CADLeader**：三点引线；ParamBlock2（起点/拐点/文本位置、注释、箭头、文字高度、RGB 颜色）；折线 + 闭合三角箭头 + 注释文本。
- **CADTable**：单击插入；ParamBlock2（插入点、行列 1–50、单元格宽高、RGB 颜色）；网格绘制；Win32 单元格编辑器（ListView、逐格编辑、行列迁移按交集保留）；单元格文本长度上限 256。
- **公共设施**：完整屏幕缩放公式（`NonScalingObjectSize × GetVPWorldWidth / 360`）、`DrawLineProc` 折线/闭合绘制、闭合箭头、BezFont 缓存与 CJK 回退、ParamBlock2 生命周期助手。
- **持久化**：四类对象各含版本 chunk 与数据 chunk（`0x0F01`–`0x0F08`）；表格显式 chunk 单权威；加载校验 `cellCount == rows*cols`、行列与长度上限、有限值。

### Changed

- **R5 四类文字渲染/命中/bbox 统一**：屏幕模式使用 billboard bbox，模型模式使用世界文本盒；标注模型偏移采用 0.6×文字高度（世界单位）；`ShowXYZ` 余量按模式取世界/屏幕单位。
- **CADDimension 移除 `SetUserDlgProc`**，改为窗口子类化（字体下拉仅处理颜色与选择变更）。
- **表格模型模式**：单元格文本按像素字号换算为世界高度，截断使用屏幕缩放后宽度。
- **R1 对齐参考项目（U11–U13）**：`LibNumberClasses` 5→4；四类改为公开；移除 `CADAnnotatePanel` 源文件与工程引用；建立 `3dsmaxbatch + plugin.ini` 独立冒烟验证。
- **B-24 修复**：外部写入 `cellTexts` 镜像时主动同步 `mCells`（`SyncCellsFromMirror`），修复保存后单元格内容丢失。

- **B7 持久化硬化**：四类对象改为在 `Load()` 中仅解析校验 chunk，注册 `ILoad::RegisterPostLoadCallback`（优先级 5），在全部引用就绪后回填 pblock 镜像，使 chunk 权威与系统加载顺序无关。
- 表格单元格文本截断阈值改用屏幕缩放后的宽度。

### Fixed

- **字体缓存**：`EnsureFont` 按请求字体名去重，避免回退字体每帧重开（R5-1 评审 Critical）。
- **撤销同步**：四类 `NotifyRefChanged` 调用 `CADSyncFontComboSelection` / `CADSyncSizeModeCombo`，撤销后下拉恢复正确值。
- **bbox 与渲染不一致**：引线/标注/表格改用 `CADAddScreenTextBox` / `CADAddWorldTextBox`；表格模型 bbox 使用逐格世界文本盒；`ShowXYZ` 余量单位随模式。

### Verification status

- 构建：Release x64 全新 Rebuild 通过，无 warning/error；R5-3 V-10（2026-11-21）0 warning/0 error，诊断残留 0。
- 发布候选：`build/x64/Release/plugins/CADBundle.dlo`，188,928 字节，SHA-256 `096e4445ac09087b0b4a5e568b889edd77a7d64542af28d5a384ce5e03ec15eb`（V-10 构建）。注意：PE 头时间戳使重建哈希变化（2026-11-22 重建为 `863490de…`）；发布证据以源码提交 + 大小 + 导出/依赖校验为准。
- 导出：7 个必需导出全部正确；x64；无 Qt 依赖。
- **自动化回归**：`smoke_r1` 29/29；R5-1 params 11/11 + measure 4/4；R5-2 params 20/20 + mode 4/4 + V-07 16/16；静态断言 `r5_1_static.ps1` fails=0（2026-11-22 复跑）；旧场景 V-02b 13/13。
- **性能（V-06A/B）**：50×50（2500 可见格，回落路径）p50 0.377 ms / p95 1.024 ms；10×10（100 可见格，BezFont）p50 1.139 ms / p95 3.199 ms；均 PASS（帧预算 16.7/33 ms）。
- **回滚（V-08）**：R5-3→R5-2→R5-1 逆序 revert，三次构建 0 error、smoke 29 PASS。
- **人工验证**：R5-1 GUI（V-03b/V-04/V-05）与 R5-2 GUI（四面板切换、旋转/透视命中与 bbox）经用户确认；定版前仍需完成 `docs/release/acceptance-checklist.md` §5 未验证项。
- **批处理验证（R1，2026-11-19）**：`3dsmaxbatch + plugin.ini` 独立加载；四类构造成功；29 项检查全部 PASS（含四类参数与表格 2×3 六个唯一字符串保存→Reset→重开逐格一致）。

### 过程提交（R5，摘要）

- `a1dcdd2`–`414b0c2` R5-1（共享度量 / 字体 UI / 对话框重排）
- `aac4826` R5-2（sizeMode）
- `d3d3b44` R5-3（表格字体回落）
- 合并：`6dffbb4`（R5-1）、`23fabbf`（R5-2）、`5f9550c`（R5-3）

### 过程提交（B0–B7，摘要）

- `5573c51` B0 五类注册骨架
- `79f0780` B1 Utility 面板与创建入口
- `b9b17e5` B2 公共设施（屏幕缩放/绘制/BezFont）
- `d2ca006` B3 CADDimension
- `76a79af` B4 CADText
- `ccb8332` B5 CADLeader
- `890ef3c` B6 CADTable 与单元格编辑器
- `889cc02` B7 持久化硬化
