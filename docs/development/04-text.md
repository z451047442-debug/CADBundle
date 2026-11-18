# CADText 设计

## 1. 基本架构

- 直接继承 `HelperObject`；**不得继承 `ITextObject`**；`IsPublic()=FALSE`；Class_ID `0xf2b7067c, 0x62a54963`；Category “CAD Tools”。
- ClassDesc2 完整实现 `IsPublic/Create/ClassName/NonLocalizedClassName/SuperClassID/ClassID/Category/InternalName/HInstance`。
- 构造函数 `MakeAutoParamBlocks(this)`；`NumRefs=1`，ref 0 为 pblock。
- Win32 `P_AUTO_UI`；几何位置为对象空间；创建时 `local = Inverse(mat) * SnapPoint`。
- 文本用 BezFont 字形轮廓，面向视图（billboard）；不是 `GraphicsWindow::setFont`（SDK 无此 API）。
- 颜色 RGB；无 alpha；`fontSize` 真实影响字形高度。

## 2. 参数块（BlockID `kTextParams=0x1002`）

| 索引 | 内部名 | 类型 | 默认/范围 | UI |
|---|---|---|---|---|
| 0 | `textPosition` | TYPE_POINT3 | (0,0,0) | `IDC_TEXT_POSX/Y/Z` + spins |
| 1 | `textString` | TYPE_STRING | “文字” | CustEdit `IDC_TEXT_STRING` |
| 2 | `fontSize` | TYPE_FLOAT | 12.0；1–1000 | `IDC_TEXT_SIZE`，EDITTYPE_FLOAT |
| 3 | `fontName` | TYPE_STRING | 默认字体（如 “Arial”） | CustEdit `IDC_TEXT_FONT` |
| 4 | `textColor` | TYPE_RGBA | 1,1,1 | ColorSwatch `IDC_TEXT_COLOR` |

- `P_AUTO_CONSTRUCT + P_AUTO_UI`，ref 0，`P_VERSION 1`；`textPosition` 为 U9 Point3 参数，也是对象空间唯一权威（U10）。
- `fontSize` 为屏幕基准高度；绘制时乘 `s`。
- `textString`/`fontName` 为空时的行为见第 4 节。

## 3. 鼠标交互

- 单击放置；默认 `CLICK_DRAG_CLICK` 下：
  - `point==0`：textPosition = world→local；返回 `CREATE_CONTINUE`。
  - `point==1 && MOUSE_POINT`：确认位置；`NotifyDependents`；返回 `CREATE_STOP`。
- `MOUSE_MOVE`：`SnapPreview`，更新 textPosition，`NotifyDependents` + `RedrawViews(REDRAW_INTERACTIVE)`。
- `MOUSE_ABORT`：`CREATE_ABORT`；不在回调内调用 `StopCreating`。
- 不依赖 `ITextObject`；不修改对象 TM 作为位置权威。

## 4. Display

- `tm=inode->GetObjectTM(t)`；`gw->setTransform(tm)`；`s=ScreenScale(vpt, tm)`。
- 位置 `p = textPosition`；字体高度 `fontSize*s`；颜色 `textColor`。
- `textString` 为空/纯空白：不绘制字形，但保留一个最小包围盒/点标记用于选中；不崩溃。
- `fontName` 打开失败：回退默认字体，再回退 CJK 字体（如 “Microsoft YaHei”）；一次性状态提示/日志；不阻塞创建。
- BezFont：`theBezFontManager.OpenFont(fontName,0)`；`BuildCharacterEx(handle, charCode, 1000.0f, shape, width, 1, false)`；按 `fontSize*s/1000` 缩放；billboard 使用 `ViewExp::GetAffineTM` 的视图轴，保证面向视图。
- 多行/超长文本：MVP 支持单行；换行符按空格/截断处理；最大字符数在实现阶段定（V10），不得崩溃。
- 修改 `textString`/`fontSize`/`fontName`/`textColor`/`textPosition` 后，`NotifyRefChanged` 失效字形缓存并重绘；500 ms 内可见（A7）。
- 选中/冻结颜色按 SDK 惯例；文本颜色可覆盖选择色？选择态优先显示选择色或加亮；文档注明。

## 5. HitTest 与包围盒

- 字形缓存提供近似文本包围盒（按字形宽度/高度、`s` 缩放）；HitTest 用该盒 + 4 px 容差；空文本用最小盒/点标记。
- `GetLocalBoundBox` 含文本盒；`GetWorldBoundBox` 用对象 TM；`vp==nullptr` 时 `s=1` + 裕量；`HasViewDependentBoundingBox()=TRUE`。
- `GWRndLimitsGuard` 恢复所有状态。

## 6. 持久化与引用

- pblock 自动保存 `textString/fontSize/fontName/textColor/textPosition`。
- `Save` 写 `kChunkTextVersion=0x0F05`、`kChunkTextPosition=0x0F06`：版本、textPosition；`Load` 校验并回填 pblock；chunk 缺失用 pblock；损坏 `IO_ERROR`。
- `textPosition` 以 chunk 为持久化权威，pblock 为编辑镜像；加载后 chunk 值覆盖 pblock 镜像。
- `Clone`：`BaseClone` + `remap.CloneRef(pblock)`；复制 textPosition；字体缓存不复制，按新对象重建。
- `NotifyRefChanged`：pblock 变化 -> 失效字形缓存 + `NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE)`；返回 `REF_SUCCEED`。
- 对象析构关闭 BezFont 句柄；`LibShutdown` 不强制关闭存活对象字体。

## 7. 必需方法

`GetClassName`、`ClassID`、`SuperClassID`、`DeleteThis`、`NumRefs/SetReference/GetReference`、`NotifyRefChanged`、`GetLocalBoundBox/GetWorldBoundBox`、`Display`、`HitTest`、`GetCreateMouseCallBack`、`BeginEditParams/EndEditParams`、`Save/Load`、`Clone`、`Eval`、`GetObjectName/InitNodeName`。`NumSubs` 与 `SubAnim/SubAnimName` 一致；类声明不得出现 `public ITextObject`。

## 8. 行数预算

`CADText.h <150`；`CADText.cpp <450`。BezFont 细节放 `CADBezFontText.cpp`。
