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
