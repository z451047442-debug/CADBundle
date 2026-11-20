# 第 4 步修订（2026-11-19）：参考 CADDimension 项目的界面与开发逻辑

> 触发：用户指令“参考 `D:\PycharmProjects\CADDimension` 的界面以及开发逻辑”，并确认三项裁决。
> 影响：本文件记录的 U11–U13 覆盖此前 U3 与验收 A3/A4/A5 的相关口径；其余 U1、U2、U4–U10 仍然有效。

## 1. 参考项目事实（只读勘察）

- 路径 `D:\PycharmProjects\CADDimension`，版本 1.0.0（`dist/CADDimension-1.0.0`）。
- 注册 4 个**公开**对象类（`IsPublic=TRUE`），无 Utility 面板；创建入口为“创建面板 → CAD Tools”，参数在修改面板。
- 基类：CADDimension=GeomObject（含目标节点包围盒模式）、CADText=SimpleObject2、CADLeader/CADTable=HelperObject。
- 屏幕恒定：`GraphicsWindow::wTransPoint` 投影后按**屏幕像素**绘制（`wPolyline`/`wText`），`GWDrawGuard` 保存/恢复变换、限制与文字点大小。
- 自动化：`3dsmaxbatch.exe <smoke.ms> -p <plugin.ini> -dm on -log <log>`，`plugin.ini` 指向独立插件目录；用 `CADDimension()` 构造函数创建对象。
- 交付：`dist/` 打包 + `SHA256SUMS.txt` + README/CHANGELOG；保留 `en-US\*.mui` 卫星资源。

## 2. 裁决

| 编号 | 裁决 | 覆盖/影响 |
|---|---|---|
| U11 | 对齐参考入口：4 类 `IsPublic=TRUE`，创建面板可见；**移除 Utility 面板** | 覆盖 U3（仅面板创建）与验收 A3/A4/A5；A2 改为创建面板/批处理验证 |
| U12 | CADDimension 保持**三点标注**，不引入目标节点/包围盒模式 | 覆盖参考项目的 GeomObject 目标绑定设计；保持现有 pblock 与交互 |
| U13 | 屏幕恒定改用 **wTransPoint 屏幕空间固定像素**（wPolyline/wText） | 覆盖 A11 的“NonScalingObjectSize 公式”表述；A6 验证简化为像素测量 |

## 3. 不照搬的部分（明确不做）

- 不采用 GeomObject + TYPE_INODE 目标绑定、RestoreObj 撤销绑定、旧场景 PostLoad 迁移（U12）。
- 不采用 SimpleObject2/BuildMesh 的 CADText 基类；保持 HelperObject。
- 不恢复 MUI 卫星资源拆分；仍交付单 `.dlo`（资源内嵌）。
- 不复制参考项目的 Class_ID 与类名；CADBundle 的对象 Class_ID 保持冻结（Utility 面板 ID 不再使用）。

## 4. 批次划分

| 批次 | 内容 | 状态 |
|---|---|---|
| R1 | 注册与入口：`LibNumberClasses=4`、四类 `IsPublic=TRUE`、移除 Utility 面板代码/工程引用；`3dsmaxbatch + plugin.ini` 冒烟（构造 + 保存重开） | 已完成（含 B-24 修复） |
| R2 | 四类 Display/HitTest 改为屏幕空间固定像素（wTransPoint/wPolyline/wText + GWDrawGuard + HitSegments） | 待实施 |
| R3 | 验收口径与开发文档重写（A3/A4/A5/A11、U3 引用、README/CHANGELOG/验收清单） | 部分完成（本文档与清单同步中） |

## 5. R1 验证证据（2026-11-19）

- 构建：Release x64，MSBUILD_EXIT=0。
- 批处理：`3dsmaxbatch.exe build/verify/smoke_r1.ms -p build/verify/plugin.ini -dm on` → `BATCH_EXIT=0`。
- 结果：29 项检查全部 PASS，`fails=0`；四类构造成功；四类参数保存→Reset→重开逐项一致；表格 2×3 六个唯一字符串逐格往返一致（C1/T15 核心实验通过）。
- 发现的缺陷与修复（B-24）：外部写入 `cellTexts` 镜像后 `EnsureDimensions` 提前返回，导致 `mCells` 未同步、保存写入空内容。修复：`NotifyRefChanged` 中新增 `SyncCellsFromMirror`，从镜像回流 `mCells` 后再落盘。

## 6. 未验证/边界

- Create 面板可见性、鼠标创建、修改面板 UI、屏幕显示与命中（R2/R3 后人工验证）。
- 屏幕空间固定像素的像素测量（A6）待 R2 完成后执行。
- 批处理验证使用 MAXScript 构造与属性赋值，不覆盖鼠标交互（B-21）。

## 7. R4 方案（2026-11-20）：修改面板适配、字体补全与命名（待第 5 步评审）

> 触发：用户 2026-11-19 人工复核确认——原“选中已绘制对象无法修改”已闭环（根因为安装 DLO 与 HEAD 构建漂移，`ui.log` 与人工复核为证）；遗留 CADLeader 缺字体下拉、CADTable 参数不全/编辑按钮无效。
> 用户 2026-11-20 裁决：A–E 全范围执行；表格字体采用“屏幕空间 BezFont + 可见单元格裁剪”；临时诊断日志移除；四类显示名统一为“CAD+中文”。

### 7.1 范围（A–F）

| 编号 | 内容 | 类型 |
|---|---|---|
| A | 修复表格“编辑单元格…”按钮：`BeginEditParams` 后用 `ClassDesc2::SetUserDlgProc(&gTableParamBlock, proc)` 注册到已存在的参数 map，Begin/End 成对注销；CADDimension 主题 proc 同法修正 | 实现缺陷 |
| B | CADLeader 增加 `fontName`（TYPE_STRING）参数 + 修改面板字体下拉；注释文本改用 BezFont billboard 渲染；HitTest 按字体宽度计算 | 需求新增 |
| C | CADTable 增加 `fontName`、`fontSize` 参数 + UI；单元格文本改用屏幕空间 BezFont；可见单元格裁剪 + 字形缓存；单帧可见单元格 >400 或单元格像素高度 <8 px 时该帧回落固定屏幕字体（仅渲染层，不改存储） | 需求新增 |
| D | 四个参数对话框收窄到 ≤ 200 DLU 并重排，保证默认命令面板宽度下控件完整可见；控件 ID 与 `p_ui` 绑定不变 | 界面适配 |
| E | 移除临时诊断：`CADParamBlockHost` 的 Begin/End 文件日志、`CADText` 的检查点日志（R4-4 最终构建前执行） | 清理 |
| F | 命名：`IDS_*_CLASS` 与 `InitNodeName` 统一为“CAD+中文”——`CAD标注`/`CAD文字`/`CAD引线`/`CAD表格`；`NonLocalizedClassName`/`InternalName` 保持英文以兼容 MAXScript；Category 不变 | 命名 |

### 7.2 技术要点与约束

- **表格 DlgProc**：SDK 文档明确 `ClassDesc2::SetUserDlgProc` 返回“是否注册到参数 map”；`ParamBlockDesc2::SetUserDlgProc` 只影响之后创建的 map。参考项目 `CADDimension::BeginEditParams` 使用前者且在 map 创建后调用，作为已验证模式。
- **引线字体**：pblock 末尾追加参数，不改 chunk；旧场景缺省为 `Microsoft YaHei`。复用 `CADDrawBezFontText`（与 CADText 相同）；空文本不绘制；缺字体回退沿用 `CADBezFontCache` 逻辑。
- **表格字体**：在 `CADBezFontText` 新增屏幕空间绘制函数（基准单位 1000 → 像素高度），复用字形缓存与 `DrawGlyphOutline`；不改 `cellTexts` 存储与行列迁移。性能阈值：单帧可见单元格超过 **400** 或单元格像素高度 < **8 px** 时，该帧单元格文本回落固定屏幕字体（仅渲染层回落，不改变存储/参数）。
- **对话框重排**：宽度 ≤ 200 DLU；控件 ID 与 p_ui 不变；仅调整坐标/尺寸；保持中文标签。
- **pblock 兼容**：追加参数不提升 `P_VERSION`（现有参数顺序不变，旧场景新增参数取默认值）；R4 冒烟需加入“旧场景无新参数加载”用例。
- **行数约束**：`CADTable.cpp` 当前 434 行，新增逻辑优先放入 `CADTableRender.cpp`/`CADTableEditor.cpp`；`CADLeader.cpp` 当前 482 行，字体 UI/绘制优先放入 `CADLeaderRender.cpp` 或公共字体 helper；单文件 ≤ 500 行。

### 7.3 明确不做

- 不改四类 Class_ID、不改持久化 chunk 格式、不改创建鼠标点序、不改行列迁移规则。
- 不为表格引入模型空间尺寸模式（仍屏幕恒定）。
- 不新增第三方依赖（继续使用 SDK `theBezFontManager`）。
- 不改 `NonLocalizedClassName`/`InternalName`（MAXScript 与批处理兼容）。

### 7.4 验收标准（可量化）

1. 四个修改面板在默认命令面板宽度下，控件**全部可见且可编辑**（逐项截图；与 R4 前裁切截图对比）。
2. 表格“编辑单元格…”点击后打开 `IDD_CAD_TABLE_EDITOR`；编辑 A1/B2 → 确定 → 屏幕回读一致。
3. 引线字体下拉可枚举（>0 项）、切换后视口注释立即变化；保存→重开字体保持（MAXScript 属性回读相等）。
4. 表格字体/字号切换后单元格文本立即变化；50×50 表格在默认缩放下滚动/缩放无崩溃；回落阈值生效可观察（日志或计时证据）。
5. 命名：创建面板与修改面板显示 `CAD标注/CAD文字/CAD引线/CAD表格`；MAXScript 仍可用英文类名构造四类对象。
6. Release x64 构建 0 warning/0 error；`smoke_r1.ms` 29 项仍全 PASS；旧场景（无新参数）加载不崩溃、新参数取默认。

### 7.5 失败条件与最低可行验证

- 失败条件：追加参数导致旧场景加载失败；表格 BezFont 性能不可接受；对话框重排后控件绑定错位；表格 DlgProc 注册后出现重复释放/崩溃。
- 最低验证：构建 → `smoke_r1.ms` 29 项 → 人工逐项检查四类面板 → 保存/重开 → 50×50 表格一次缩放/滚动 → 打开一次表格编辑器。

### 7.6 回滚

- 代码：按 R4-1…R4-4 分批提交，异常时 `git revert` 对应提交；配置无变更；数据无格式变更（新增参数为增量，旧场景不受影响）。
- 若表格字体方案不达标：降级为“保留字号、字体固定”，不影响存储与其它三类。

### 7.7 批次

| 批次 | 内容 |
|---|---|
| R4-1 | 命名（F）+ 表格 DlgProc 修复（A）+ 四对话框重排（D） |
| R4-2 | 引线字体参数/UI/渲染（B） |
| R4-3 | 表格字体/字号参数/UI/屏幕 BezFont（C） |
| R4-4 | 诊断清理（E）+ 文档/验收清单同步 + 全量构建与冒烟 |

## 7.8 修订与前置验证结果（2026-11-20，供第 5 步复审）

> 本节补充并修正 §7.1–7.7 的实现口径；用户裁决全文见 `docs/requirements/CADAnnotatePanel.md` §11.4（R4-1～R4-6）。

### 7.8.1 用户裁决（框架级覆盖）

- **R4-1（覆盖 U13 文字类条款）**：CADText、CADLeader 注释、CADDimension 文本、CADTable 单元格文本新增 `sizeMode`（屏幕恒定/模型空间），**默认屏幕恒定**；模型空间下文字随视口/世界缩放。线/箭头/网格/单元格宽高仍按现有屏幕规则。
- **R4-2（覆盖 U10 表格固定字体条款）**：表格新增 `fontName`/`fontSize`，默认保持现有视觉；存储/行列迁移不变。
- **R4-3**：显示名统一 `CAD标注`/`CAD文字`/`CAD引线`/`CAD表格`（无空格），`InitNodeName` 同步；英文 `NonLocalizedClassName`/`InternalName` 不变。
- **R4-4**：四对话框宽度 **≤190 DLU**；新控件 ID 从既有区间尾部分配。
- **R4-5**：表格字体使用屏幕空间 BezFont + 可见单元格裁剪；回落初值 400 格/8 px，实测调整；验收 p95 ≤ 33 ms（50×50、1920×1080、默认缩放）。
- **R4-6**：V-01/V-02/V-03a 前置验证通过；诊断日志在最终批次移除。

### 7.8.2 前置验证证据

| 编号 | 方法 | 结果 | 证据 |
|---|---|---|---|
| V-01 | 表格“编辑单元格…”改用 `SetWindowSubclass`（替代 `SetUserDlgProc`）；构建后用户 GUI 验证 | 对话框打开 ✅；单元格编辑并确定生效 ✅；连续开关 3 次正常 ✅ | 用户人工验证结论（2026-11-20）+ 工作区代码 `src/CADTable.cpp` |
| V-02 | 旧构建（`build/verify/CADBundle.prev.dlo`, SHA `1cd2405…`）保存四类场景；新构建（追加 `fontName`/`fontSize`）加载并保存重开 3 次 | 22 项检查全 PASS，`fails=0`；旧参数保持、新参数取默认、3 次往返稳定 | `build/verify/v02_verify_result.txt`；`build/verify/smoke_before.max`；`v02_old.log`/`v02_new.log` |
| V-03a | 新增 `CADDrawScreenBezFontText`（屏幕空间 BezFont，逐段 `CADDrawScreenSegment`）；引线注释切换该路径；修正 `BuildGlyph` 的 `ggo_native` 用法 | 注释显示/编辑生效/屏幕固定 ✅；表格编辑器仍正常 ✅；拉丁字母由 `sMP` 修复为 `V03` ✅（引线与 CADText 均正常） | 用户人工验证结论 + `src/CADBezFontText.cpp`（`CADDrawScreenBezFontText`、`BuildGlyph`） |

### 7.8.3 实现口径修正

- **P0-1 关闭**：不再使用 `SetUserDlgProc` 注册；表格按钮与主题色统一走**窗口子类化**（与 CADText 已验证路径一致），子类在 `EndEditParams` 先移除再销毁 map。
- **P0-2 关闭**：`CADDrawScreenBezFontText` 已实现并实机验证；字形构码修正为字符码统一走 `ggo_native=true`（CJK 加 `0x10000000`）。引线/表格/标注/文字共用该函数与 `CADBezFontCache`；文字测量统一走 `CADBezFontCache::TextWidth`，Display/HitTest/bbox 使用同一入口。
- **P0-3 关闭（设计层）**：四对话框 ≤190 DLU；新控件 ID：
  - `IDC_DIM_SIZEMODE` 1129
  - `IDC_TEXT_SIZEMODE` 1211
  - `IDC_LEAD_FONT` 1325、`IDC_LEAD_SIZEMODE` 1326
  - `IDC_TABLE_FONT` 1417、`IDC_TABLE_FONTSIZE` 1418、`IDC_TABLE_FONTSIZESPIN` 1419、`IDC_TABLE_SIZEMODE` 1420
  既有控件 ID 与 `p_ui` 绑定不变；仅新增/重排控件坐标。验收：在用户环境与 100%/150% DPI 截图中，全部控件位于面板客户区内（边距 ≥2 px、无重叠）。
- **P1-1 关闭**：V-02 证明同版本追加参数兼容；无需 `P_VERSION` 升级或 PostLoad 迁移。
- **P1-2 关闭**：删除 `CADLeaderThemeProc` 死代码；引线主题/字体统一走子类化。
- **P1-5 调整**：诊断日志保留至 V-06 性能证据采集完成，再于 R4-4 移除；增加 DLO 备份/恢复步骤（旧备份 `CADBundle.prev.dlo`，诊断备份 `CADBundle.diag.dlo`）。
- **P1-8**：不新增源文件；新函数置于 `CADBezFontText.{h,cpp}`，UI 置于既有文件，`CADBundle.vcxproj` 不变；各文件 ≤500 行。

### 7.8.4 更新后的验收标准

在 §7.4 基础上增加/修订：

1. **尺寸模式**：四类各自切换屏幕/模型模式；屏幕模式下 1×/2×/4× 文字像素尺寸不变（V1 容差 ≤ max(2 px, 5%)）；模型模式下文字像素尺寸随视口缩放成比例变化（同容差）；保存→重开模式保持；旧场景默认屏幕模式。
2. **表格字体/字号**：默认值保持现有视觉；切换字体/字号后单元格文本立即变化；50×50 默认缩放下 p50 ≤ 16.7 ms、p95 ≤ 33 ms；回落阈值可观察。
3. **引线字体**：下拉可枚举、切换生效、保存重开保持；多字体（YaHei/Arial/CJK）下注释与命中框一致。
4. **命名/布局/表格编辑器**：`CAD标注/CAD文字/CAD引线/CAD表格`；四对话框控件完整可见；表格编辑器可打开并回读。
5. **回归**：Release x64 0 warning/0 error；`smoke_r1.ms` 29 项 PASS；旧场景（无新参数）加载不崩溃、新参数取默认；诊断日志在 R4-4 后 0 处残留。

### 7.8.5 批次调整

| 批次 | 内容 |
|---|---|
| R4-1 | 命名（R4-3）+ 表格子类化（V-01 定稿）+ 四对话框 ≤190 DLU/新 ID（R4-4）+ 删除死代码 |
| R4-2 | 四类 `sizeMode` 参数/UI/渲染与共享文本度量（R4-1 裁决） |
| R4-3 | 引线字体 UI；表格字体/字号 UI + 屏幕 BezFont 渲染 + 可见裁剪/回落；V-06 性能证据采集 |
| R4-4 | 诊断清理（E）+ 文档/验收清单同步 + 全量构建/冒烟 + 回滚演练 |

## 7.9 门禁结论与延后（2026-11-20）

- 最终复审（同一问题第 2 轮，全局累计 9/9）：**第 5 步不通过**。阻塞项 B-1～B-4：CADDimension 仍用 `SetUserDlgProc`；四类 BezFont/测量未统一；`sizeMode` 缺实现级设计；行数预算不可达。
- 用户裁决：选 **A 收口**——仅提交已由用户人工验证的 V-01（表格编辑按钮子类化）与 V-03a（BezFont 字形修复 + 引线屏幕空间 BezFont）；移除 V-02 追加参数与临时诊断。
- **R4 延后**：`sizeMode`、引线/表格字体 UI、四对话框 ≤190 DLU、中文命名、190 DLU/DPI/性能采样等，转入新任务并按新循环额度重新走第 4/5 步；本 §7.8 保留为后续设计参考，**不代表已通过门禁**。

## 8. R5 方案（2026-11-20；循环额度重置 0/9）

> 前置：A 收口已合并（merge `f55f192`）；V-01/V-02/V-03a 已由用户人工验证。本方案承接延后的 R4-1/R4-2/R4-4/R4-5/R4-6，并落实上一轮项目总监 6.1 的六条修正。

### 8.1 范围与不做

- **交付**：
  1. 四类文字测量统一（Display/HitTest/bbox 共用同一 `CADBezFontCache::TextWidth` + 模式换算入口）；
  2. 四类 `sizeMode`：屏幕恒定（默认）/模型空间；
  3. 引线、标注、表格的字体 UI（下拉/字号），表格单元格字体渲染；
  4. 四参数对话框 ≤190 DLU + 新控件 ID/字符串；
  5. 表格字体性能采样与回落阈值；
  6. 文档、验收清单、回滚演练同步。
- **不做**：不改 Class_ID、不改持久化 chunk 格式、不改创建点序、不改行列迁移规则；不引入第三方依赖；不改 `NonLocalizedClassName`/`InternalName`；不把未验证项写成通过。

### 8.2 设计

#### 8.2.1 统一文字度量

- **单位公式（强制）**：`CADBezFontCache::TextWidth` 返回 1000 参考字高单位；入口统一写为
  `widthPx = TextWidth(text) * pixelHeight / 1000.0f`（与现有 `CADTextRender.cpp:81-82、227-228` 一致）。漏除 1000 将使宽度放大千倍，V-07 必失败。
- 新增共享入口（放在 `CADBezFontText`，避免 `CADAnnotationCommon.cpp` 继续膨胀）：
  - `CADTextMetrics MeasureText(CADBezFontCache&, const MCHAR* text, float pixelHeight)` → `{widthPx, heightPx}`，内部执行上述 `/1000`；
  - `float CADWorldHeightToPixels(ViewExp*, const Point3& worldPos, float worldHeight)`：投影 `p` 与 `p + up*worldHeight` 量取像素高度（透视/正交一致）。
  - 空视口回退：`vpt == nullptr || !vpt->IsAlive()` 时与 `CADScreenScale` 保持同一口径（`CADAnnotationCommon.cpp:16-19` 返回 1.0），`CADWorldHeightToPixels` 返回屏幕模式高度；bbox/HitTest 不得崩溃，并记录回退计数。
- Display、HitTest、`GetLocalBoundBox/GetWorldBoundBox` 三处必须调用同一入口；四类 bbox 均已带 `ViewExp* vpt`（`CADText.h:48`、`CADLeader.h:48`、`CADTable.h:49`、`CADDimension.h:48`），禁止再保留 `0.6×字数×字高` 近似（引线/标注/表格均改为实测宽度）。
- 兼容口径：屏幕模式像素**高度**不变；宽度与命中框按实测重算（见 8.4-3）。

#### 8.2.2 sizeMode（四类文字）

- 参数：每类追加 `sizeMode`（`TYPE_INT`，0=屏幕恒定、1=模型空间），默认 0；MAXScript 属性名 `sizeMode`；pblock 末尾追加（V-02 已验证追加兼容，V-02b 覆盖新参数）。
- UI：每类 `COMBOBOX`（`IDC_*_SIZEMODE`），两项“屏幕恒定/模型空间”；选择写 pblock 并触发 `RedrawViews`。
- 语义：
  - **CADText**：屏幕模式 `heightWorld = fontSize * CADScreenScale`；模型模式 `heightWorld = fontSize`（世界单位）；billboard 朝向不变；Display/HitTest/bbox 共用。
  - **引线注释**：屏幕模式 `pixelHeight = textHeight`；模型模式 `pixelHeight = CADWorldHeightToPixels(vpt, textPos*objectTM, textHeight)`；使用屏幕空间 BezFont。
  - **标注文本**：屏幕模式沿用现状；模型模式按世界高度换算像素；文本偏移 `0.6 * 当前像素高度`，与模式一致；HitTest/bbox 同步。
  - **表格（单位随模式切换）**：
    - 屏幕模式：`cellWidth/cellHeight` 为**像素**；`widthWorld = cellWidth * CADScreenScale`（现状，`CADTableRender.cpp:172-174`）；文字像素高度按 `fontSize` 解析：`fontSize=0` 表示自动（`cellHeight*0.6`），**自动值与显式值统一夹到 [6,48] px**（与旧实现 `ClampInt(...,6,48)` 的 `CADTableRender.cpp:190-192` 完全一致，保证旧场景字号不变）。
    - 模型模式：`cellWidth/cellHeight` 为**世界单位**；`widthWorld = cellWidth`（不再乘 `CADScreenScale`）；文字世界高度 `worldTextHeight = (fontSize>0 ? fontSize : cellHeight*0.6)`（世界单位，夹到 [0.01,10000]），`pixelHeight = CADWorldHeightToPixels(vpt, anchor*objectTM, worldTextHeight)`；**不得**再对 `pixelHeight` 套用旧的 6–48 px 截断，否则模型模式失去缩放。
    - 单格裁剪：`FitCellText` 的 `maxPixels` 必须传**像素宽**——屏幕模式传 `cellWidth`；模型模式传 `cellWidth / CADScreenScale`（`CADTableRender.cpp:20-32` 以 `getTextExtents().cx` 比较）。`cellTexts` 存储不变。
- 覆盖关系：U7 的“网格/单元格屏幕恒定”仅适用于**屏幕模式**；模型模式对整表覆盖，失败条件“模型模式表格溢出不可读”只允许由单格 `FitCellText` 截断触发。
- 模式切换：UI 控件写 pblock → `NotifyRefChanged` 清字形缓存并 `RedrawViews`；验收要求切换后立即变化。
- V-02b：旧场景（无 `sizeMode`/字体新参数）加载后默认 0/默认字体，3 次保存重开 `fails=0`。

#### 8.2.3 字体 UI

- **引线**：新增 `fontName`（TYPE_STRING，默认 Microsoft YaHei）+ COMBOBOX；子类化处理 `CBN_SELCHANGE`；渲染 `CADDrawScreenBezFontText` + `cache.SetFont(fontName)`。
- **标注**：新增 `fontName` + COMBOBOX；距离文本改用 BezFont（屏幕空间，模型模式按 8.2.2 换算）；缺字体回退。
- **表格**：新增 `fontName`、`fontSize` + COMBOBOX/SPINNER；单元格文本用屏幕 BezFont；可见单元格裁剪；**回落带滞回**：用 8.2.2 换算后的最终 `pixelHeight` 判定，可见单元格 >480 或像素高度 <8 px 进入固定字体回落，≤360 且 ≥10 px 恢复；每帧记录实际路径与回落次数（初值由 V-06 校准）。
- **CADText**：已有字体下拉，保持；纳入统一度量与 sizeMode。
- 所有字体 UI 使用与 CADText 相同的窗口子类化模式；禁止再使用 `SetUserDlgProc` 注册时序（V-01 已验证）。

#### 8.2.4 对话框与资源

- 四对话框宽度 ≤190 DLU；控件 ID 与既有 `p_ui` 绑定不变（仅新增/重排）。
- **可行性前置**：现有控件右边界已达 205–220 DLU（评审实测；`res/CADBundle.rc` 对应对话框内的控件），≤190 需横向压缩约 15–30 DLU（评审估计 30–40）。R5-1 编码前必须产出**压缩矩阵**：标签缩短、两列分组、纵向增高（≤+40 DLU）、不得删控件或重叠；若某对话框确实无法在 ≤190 内保持可读，提交用户裁决，**不得默许放宽**。
- 新增控件 ID（已核对 `res/resource.h` 无占用）：
  - `IDC_DIM_SIZEMODE` 1129（DIM 现有止于 1128）
  - `IDC_TEXT_SIZEMODE` 1211（TEXT 现有止于 1210）
  - `IDC_LEAD_FONT` 1325、`IDC_LEAD_SIZEMODE` 1326（LEADER 现有止于 1324）
  - `IDC_TABLE_FONT` 1417、`IDC_TABLE_FONTSIZE` 1418、`IDC_TABLE_FONTSIZESPIN` 1419、`IDC_TABLE_SIZEMODE` 1420（TABLE 现有止于 1416）
- 新增字符串 ID（已核对 `res/resource.h`：现有已用到 162，原写 132–138 与 `IDS_DIM_*` 冲突）：`IDS_SIZEMODE` 163、`IDS_SIZEMODE_SCREEN` 164、`IDS_SIZEMODE_MODEL` 165、`IDS_LEAD_FONT` 166、`IDS_DIM_FONT` 167、`IDS_TABLE_FONT` 168、`IDS_TABLE_FONTSIZE` 169。
- R5-1 编码前先产出**资源差异表**（每个控件的 ID、坐标、尺寸、DLU→像素、DPI 100/125/150/200% 预期），作为 V-04 的证据基线。

#### 8.2.5 行数拆分

- `CADAnnotationCommon.cpp` 当前 555 行：将屏幕空间绘制/命中实现（`CADDrawScreenSegment/Polyline/Arrow/Text`、`CADHitScreenSegments` 等）拆到新文件 `src/CADScreenDraw.cpp`，声明保留在 `CADAnnotationCommon.h`；新增文件加入 `CADBundle.vcxproj`。
- **行数预算**：`CADScreenDraw.cpp` 预计 120–180 行；`CADAnnotationCommon.cpp` 目标 ≤450 行；两文件均 ≤500。
- `CADText.cpp`（468）与 `CADLeader.cpp`（482）：字体 UI/sizeMode 的新增代码优先放入 `Render`/`Editor`/公共 helper，编码前核对行数；不得突破 500。
- 新增文件属工程配置变更，不新增第三方依赖。**R5-1 开工前必须取得用户对新增 `CADScreenDraw.cpp` 的确认**，作为出口前置门禁；若未获批准，改为在既有文件内拆分并优先迁出非文本绘制代码。

### 8.3 验证与门禁

| 编号 | 内容 | 指标 |
|---|---|---|
| V-01b | CADDimension 主题改子类化、删除 `CADLeaderThemeProc` | `grep -r SetUserDlgProc src` = 0；三种面板各开关 3 次正常 |
| V-02b | 旧场景 + 新参数（sizeMode/fontName/fontSize）加载/保存/重开 3 次 | 默认值精确；`fails=0` |
| V-03b | 引线/标注/表格多字体（YaHei/Arial/CJK）字体切换 | 立即生效；缺字体回退可观察 |
| V-04 | 100/125/150/200% DPI 截图 + 四对话框宽度实测 | 每对话框宽度 ≤190 DLU（资源字段与 100% 截图换算一致）；控件在客户区、边距 ≥2 px、无重叠；压缩矩阵逐项对照 |
| V-05 | 四类渲染范围 vs HitTest/bbox（有效视口） | 偏差 ≤ max(2 px, 5%)；空 `vpt` 回退路径单独记录且不崩溃 |
| V-06A | **屏幕模式**、50×50 全非空、1920×1080、默认缩放（同基线相机）、≥300 帧 | 可见格 ≈1600 → 断言实际路径为 Fallback 且 `fallbackCount≥1`；p50 ≤16.7 ms、p95 ≤33 ms |
| V-06B | **屏幕模式**、`fontSize≥10 px`、可见格 <400（如缩放至 10×10 格占满视口）、≥300 帧 | 断言实际路径为 BezFont；p50 ≤16.7 ms、p95 ≤33 ms |
| V-07 | 四类 sizeMode，视口缩放 1×/2×/4×（同相机、世界宽度比），样本文本 "Ag中123"、YaHei | 屏幕模式高度误差 ≤ max(2 px, 5%)；模型模式像素高度与缩放成反比（比例误差 ≤5%）；保存重开保持 |
| V-08 | 新场景 → `f55f192` 构建的旧 DLO 加载；**逆序** `git revert`（R5-3→R5-2→R5-1） | 不崩溃、忽略新增参数；每批回滚后 `smoke_r1.ms` 29 PASS；vcxproj 恢复 |
| V-10 | 全量 Release + `smoke_r1.ms` + 诊断残留检查 | 0 warning/0 error；29 PASS；0 残留 |

- “可见格”定义：裁剪到视口后、与视口相交的单元格数量；屏幕外单元格不计入。
- “默认缩放”定义：与 R4 基线相同的默认相机（未人为 Zoom），记录 `GetVPWorldWidth` 值备查。

### 8.4 验收标准（可签字）

1. 四个修改面板在默认宽度与 4 档 DPI 下控件完整可见、可编辑；**资源字段实测宽度 ≤190 DLU** 且与 100% 截图换算一致（截图 + 边距/重叠测量 + 压缩矩阵）。
2. 引线/标注/表格字体下拉可枚举、切换生效；缺字体回退可见；保存重开保持。
3. `sizeMode` 四类可切换；屏幕模式**像素高度不变**，宽度/命中框按实测重算且满足 V-05；模型模式文字与表格均随视口缩放（像素尺寸成反比），切换即时刷新。
4. 表格 V-06A 与 V-06B 均达标；回落路径、次数与滞回有证据。
5. Display/HitTest/bbox 一致（V-05）；表格编辑器可打开、回读一致。
6. 旧场景加载不崩溃、新参数默认；`smoke_r1.ms` 29 PASS；无诊断残留。

### 8.5 失败条件、最低验证、回滚

- 失败条件：旧场景加载失败；模型模式表格系统性溢出不可读（单格截断除外）；性能不达标；DPI 裁切；子类化生命周期异常；字体回退不可见。
- 最低验证：构建 → 29 项 smoke → 四面板 → 字体/模式切换 → 保存重开 → V-06A/B 采样 → 回滚演练。
- 回滚：R5-1…R5-3 为叠加提交，**必须逆序** `git revert`（R5-3→R5-2→R5-1）；**R5-1 含新增 `CADScreenDraw.cpp` 与 `CADBundle.vcxproj` 变更，revert 时一并恢复工程配置**；若走 8.2.5 的“不新增文件”分支，同步删除本句的 `CADScreenDraw.cpp` 表述；数据为追加参数（不改 chunk）；DLO 备份/恢复；旧场景夹具测试。

### 8.6 批次

| 批次 | 内容 | 出口证据 |
|---|---|---|
| R5-1 | 度量统一 + `CADScreenDraw.cpp` 拆分 + 四对话框 ≤190 DLU/新 ID + 引线/标注/表格字体 UI + CADDimension 子类化 | 用户确认新增文件、资源差异表、压缩矩阵、V-01b、V-03b、V-04、V-05、构建 0 warning |
| R5-2 | 四类 `sizeMode` + 模型模式整表/命中/bbox + 切换刷新 | V-02b、V-07 |
| R5-3 | 表格字体性能采样 + 回落阈值/滞回 + 文档/验收 + 回滚演练 | V-06A、V-06B、V-08、V-10 |

### 8.7 R5 循环账本

- 重置后 **0/9**；记录见 `docs/reviews/loop-ledger.md` 的 R5 表。同一问题最多 3 轮；达到 9 轮即停止。
- **2026-11-20 第 1 轮评审：不通过**（1/9）。处理：P0-2/3/4/5/6 属实并已按上文修订；P0-1 经代码核实不成立（四类 bbox 均带 `ViewExp*`），降级为空视口回退要求；P1-1/2/3/4/5 已并入 8.2.4/8.2.5/8.3/8.4。进入第 2 轮评审。
- **2026-11-20 第 2 轮评审：不通过**（2/9）。未闭合项：表格 `FitCellText` 像素/世界单位换算、表格文字像素参数与截断、V-06A/B 条件与“可见格”定义、≤190 DLU 缺少验收项、回滚必须逆序。已全部修订；进入第 3 轮评审（该问题最后一轮）。
- **2026-11-20 第 3 轮评审：不通过**（3/9）。缺口 2/3/4 已闭合；缺口 1 残余：`fontSize=0` 自动值未与显式值一起夹到 [6,48] px，旧场景极端 `cellHeight` 会变字号。已按评审原文修正；**R5-000 达 3 轮上限，不再开第 4 轮**，转用户裁决（接受修订进入 R5-1 / 调整范围）。
