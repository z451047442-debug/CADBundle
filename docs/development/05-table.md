# CADTable 设计

## 1. 基本架构

- 直接继承 `HelperObject`；`IsPublic()=FALSE`；Class_ID `0xf36d1d67, 0x86164c67`；Category “CAD Tools”。
- ClassDesc2 完整实现 `IsPublic/Create/ClassName/NonLocalizedClassName/SuperClassID/ClassID/Category/InternalName/HInstance`。
- 构造函数 `MakeAutoParamBlocks(this)`；`NumRefs=1`，ref 0 为 pblock。
- Win32 `P_AUTO_UI`；网格在对象 XY 平面；`cellWidth/cellHeight` 完全屏幕恒定（U7）。
- 网格线 `DrawLineProc`；单元格文本 `GraphicsWindow::text`（屏幕固定字体）；颜色 RGB。
- 表格编辑器：用户可见 Win32 网格/逐格编辑对话框（U6）。

## 2. 参数块（BlockID `kTableParams=0x1004`）

| 索引 | 内部名 | 类型 | 默认/范围 | UI |
|---|---|---|---|---|
| 0 | `insertPos` | TYPE_POINT3 | (0,0,0) | `IDC_TABLE_POSX/Y/Z` + spins |
| 1 | `rows` | TYPE_INT | 2；1–50 | `IDC_TABLE_ROWS`，EDITTYPE_INT |
| 2 | `cols` | TYPE_INT | 2；1–50 | `IDC_TABLE_COLS`，EDITTYPE_INT |
| 3 | `cellWidth` | TYPE_FLOAT | 60.0；1–1000 | `IDC_TABLE_CW`，EDITTYPE_FLOAT |
| 4 | `cellHeight` | TYPE_FLOAT | 20.0；1–1000 | `IDC_TABLE_CH`，EDITTYPE_FLOAT |
| 5 | `cellTexts` | TYPE_STRING_TAB | 空/行优先，P_VARIABLE_SIZE | 无自动 p_ui；“编辑单元格…”按钮 `IDC_TABLE_EDIT` 走 ParamMap2UserDlgProc |
| 6 | `lineColor` | TYPE_RGBA | 0.1,0.8,0.1 | ColorSwatch `IDC_TABLE_LINECOL` |
| 7 | `textColor` | TYPE_RGBA | 1,1,1 | ColorSwatch `IDC_TABLE_TEXTCOL` |

- `P_AUTO_CONSTRUCT + P_AUTO_UI`，ref 0，`P_VERSION 1`；`insertPos` 为 U9 Point3 参数。
- `cellTexts` 以显式 chunk 为持久化权威（U5），pblock 字符串表为 UI/运行时镜像；不允许无优先级双写。
- `rows/cols` 使用 `PBAccessor`：变化时按 `(row,col)` 交集迁移 `cellTexts`；新增空串，超出丢弃；用 `theHold` 包裹一次多步修改；`SetCount` 后重新取指针再填充。

## 3. 鼠标交互

- 实现 `int override(int mode) override { return CLICK_MOVE_CLICK; }`：一次用户单击（按下+释放）生成一个 `MOUSE_POINT`，`point==0` 即该次单击。
- `point==0`：insertPos = world→local；`NotifyDependents`；返回 `CREATE_STOP`。
- `MOUSE_MOVE`：`SnapPreview`，更新 insertPos，`NotifyDependents` + `RedrawViews`；不提交。
- `MOUSE_ABORT`：`CREATE_ABORT`；不在回调内调用 `StopCreating`。
- 证据：`maxapi.h:1940-2031`、`mouseman.h:143-191`；默认模式样例 `pthelp.cpp`/`text.cpp` 使用 point 0/1，本方案用 `CLICK_MOVE_CLICK` 统一“用户点击次数=point 索引”。

## 4. Display

- `tm=inode->GetObjectTM(t)`；`gw->setTransform(tm)`；`s=ScreenScale(vpt, tm)`。
- 网格：以 insertPos 为左上/原点，`w=cellWidth*s`，`h=cellHeight*s`；画 `rows+1` 条水平线（长度 `cols*w`）和 `cols+1` 条垂直线（长度 `rows*h`），用 `DrawLineProc`；颜色 `lineColor`。
- 单元格文本：`cellTexts` 行优先；非空文本用 `gw->text` 在单元格内偏移（约 `0.1*w, 0.1*h`）绘制；颜色 `textColor`；单元格文本最大 256 字符，显示超宽时省略号、存储完整；空表仅网格。
- 尺寸语义：表格完全屏幕恒定是 U7 裁决；与 CAD 模型空间表格习惯不同，README/交付说明需注明；后续可扩展模型尺寸模式。
- 行列变化后按新行列重绘；`NotifyRefChanged` 失效/重绘；不崩溃。
- 选中/冻结颜色按 SDK 惯例；保存/恢复 GW 状态。
- 50×50：避免每帧重建大量字符串；缓存字符串指针/文本布局，仅在参数变化时重建；性能阈值在第 7 步实测。

## 5. HitTest 与包围盒

- 网格线/表格区域命中；`GWRndLimitsGuard`；`MakeHitRegion(...,4,p)`；`GW_PICK`；绘制网格；`checkHitCode()`；单元格文本可不在命中范围内。
- Local bbox 为 `(0,0,0)` 到 `(cols*w, rows*h, 0)`；World bbox 乘对象 TM；`vp==nullptr` 时 `s=1` + 裕量；`HasViewDependentBoundingBox()=TRUE`。

## 6. 表格编辑对话框

- 对话框 `IDD_CAD_TABLE_EDITOR`；由对象修改面板的“编辑单元格…”按钮打开；模态；单实例。
- 控件：`ListView`（report 模式，列数=cols，行数=rows）或等价网格；`CustEdit` 编辑框覆盖当前选中单元格；`IDC_TABLE_EDIT_ROWS/COLS` spinner 可调整行列；`IDC_TABLE_EDIT_APPLY`/`CANCEL`。
- 交互：双击/选中单元格→编辑框出现→输入中文/长文本/空串→Enter/焦点离开提交；滚动/调整行列后保留交集文本；确定写回 pblock（先写镜像）并标记需要保存 chunk；取消不修改。
- 最低验收证据（A9/C5）：打开编辑器→创建/选中表格→编辑 A1/B2→确定→屏幕回读→保存/重开逐格一致；截图 + `cellTexts` 数据 + 命令日志。
- 不使用 `TYPE_STRINGLISTBOX` 作为唯一编辑路径；`TYPE_STRING_TAB` 的 pblock 镜像不作为“用户可编辑”证据。
- 文本编码：Unicode/UTF-8 安全；长度上限实现阶段定；空串允许。

## 7. 持久化与引用

- pblock 自动保存 rows/cols/cellWidth/cellHeight/颜色/insertPos。
- `Save` 写 `kChunkTableVersion=0x0F07`、`kChunkTableData=0x0F08`：版本、rows、cols、cellCount、每个字符串长度前缀+字符数据；`insertPos` 可写入同一 chunk 或独立 chunk。
- `Load` 校验 `cellCount==rows*cols`、rows/cols 1–50、字符串长度上限；先清空/SetCount，再填充 pblock 镜像；chunk 为权威，pblock 为镜像；损坏返回 `IO_ERROR`，保持默认状态不崩溃。
- `Clone`：`BaseClone` + `remap.CloneRef(pblock)`；复制 cellTexts 和 insertPos；不复制编辑器状态。
- `NotifyRefChanged`：pblock 变化 -> 缓存失效 + `NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE)`；返回 `REF_SUCCEED`。
- 2×3 保存/Reset/重开实验（U5/C1）作为验收 10 的前置验证。

## 8. 必需方法

`GetClassName`、`ClassID`、`SuperClassID`、`DeleteThis`、`NumRefs/SetReference/GetReference`、`NotifyRefChanged`、`GetLocalBoundBox/GetWorldBoundBox`、`Display`、`HitTest`、`GetCreateMouseCallBack`、`BeginEditParams/EndEditParams`、`Save/Load`、`Clone`、`Eval`、`GetObjectName/InitNodeName`。`NumSubs` 与 `SubAnim/SubAnimName` 一致。

## 9. 行数预算

`CADTable.h <180`；`CADTable.cpp <500`；`CADTableEditor.h <120`；`CADTableEditor.cpp <450`。超出时拆出行列迁移/网格绘制助手。
