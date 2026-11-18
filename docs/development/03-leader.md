# CADLeader 设计

## 1. 基本架构

- 直接继承 `HelperObject`；`IsPublic()=FALSE`；Class_ID `0x530b0886, 0xf00a4baa`；Category “CAD Tools”。
- ClassDesc2 完整实现 `IsPublic/Create/ClassName/NonLocalizedClassName/SuperClassID/ClassID/Category/InternalName/HInstance`。
- 构造函数 `MakeAutoParamBlocks(this)`；`NumRefs=1`，ref 0 为 pblock；`GetReference/SetReference` 仅处理 ref 0。
- Win32 `P_AUTO_UI`；几何点为对象空间；创建时 `local = Inverse(mat) * SnapPoint`。
- 屏幕缩放 `s = ScreenScale(vpt, inode->GetObjectTM(t))`；箭头/文本高度乘 `s`。
- 折线 `DrawLineProc`；箭头闭合三角直接 `gw->polyline(..., 1, ...)`；文本 BezFont billboard；颜色 RGB。

## 2. 参数块（BlockID `kLeaderParams=0x1003`）

| 索引 | 内部名 | 类型 | 默认/范围 | UI |
|---|---|---|---|---|
| 0 | `start` | TYPE_POINT3 | (0,0,0) | `IDC_LEAD_STARTX/Y/Z` + spins |
| 1 | `elbow` | TYPE_POINT3 | (50,0,0) | `IDC_LEAD_ELBOWX/Y/Z` |
| 2 | `textPos` | TYPE_POINT3 | (100,20,0) | `IDC_LEAD_TEXTPOSX/Y/Z` |
| 3 | `note` | TYPE_STRING | “注释” | CustEdit `IDC_LEAD_NOTE` |
| 4 | `arrowSize` | TYPE_FLOAT | 8.0；0.1–1000 | `IDC_LEAD_ARROW`，EDITTYPE_FLOAT |
| 5 | `textHeight` | TYPE_FLOAT | 12.0；1–1000 | `IDC_LEAD_TEXTH`，EDITTYPE_FLOAT |
| 6 | `lineColor` | TYPE_RGBA | 0.1,0.8,0.1 | ColorSwatch `IDC_LEAD_LINECOL` |
| 7 | `textColor` | TYPE_RGBA | 1,1,1 | ColorSwatch `IDC_LEAD_TEXTCOL` |

- `P_AUTO_CONSTRUCT + P_AUTO_UI`，ref 0，`P_VERSION 1`；点参数为 U9 的编辑镜像。
- 颜色无 alpha；文本空串时不绘制。

## 3. 鼠标交互

- `point==0`：start = world→local；`elbow=textPos=start`；返回 `CREATE_CONTINUE`。
- `point==1`：elbow = world→local；`textPos=elbow`；返回 `CREATE_CONTINUE`。
- `point==2`：textPos = world→local；`NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE)`；返回 `CREATE_STOP`。
- `MOUSE_MOVE`：`SnapPreview`，更新预览，`NotifyDependents` + `RedrawViews(REDRAW_INTERACTIVE)`。
- `MOUSE_ABORT`：`CREATE_ABORT`；不在回调内调用 `StopCreating`。
- 退化：零长度段跳过绘制；不除零。

## 4. Display

- `tm=inode->GetObjectTM(t)`；`gw->setTransform(tm)`；`s=ScreenScale`。
- 折线 `start→elbow→textPos`：`DrawLineProc`；颜色 `lineColor`。
- 箭头在 `start`：方向 `elbow→start`，尖端在 start，长度 `arrowSize*s`，宽约 `0.35*arrowSize*s`，闭合三角；颜色 `lineColor`。
- 文本 `note` 在 `textPos`：BezFont billboard，高度 `textHeight*s`，颜色 `textColor`；空串不绘制。
- 选中/冻结颜色按 SDK 惯例；保存/恢复 GW 状态。

## 5. HitTest 与包围盒

- `GWRndLimitsGuard`；`MakeHitRegion(...,4,p)`；`GW_PICK`；绘制折线/箭头，文本近似盒；`checkHitCode()`。
- Local/World bbox 含三点、箭头长度、文本高度近似；`vp==nullptr` 时 `s=1` + 裕量；`HasViewDependentBoundingBox()=TRUE`。

## 6. 持久化与引用

- pblock 自动保存 note/参数与点镜像；`Save` 写 `kChunkLeaderVersion=0x0F03`、`kChunkLeaderPoints=0x0F04`：版本、点数 3、start/elbow/textPos。
- `Load` 校验并回填 pblock；缺失用 pblock；损坏 `IO_ERROR`。
- `Clone`/`NotifyRefChanged`/`BeginEditParams`/`EndEditParams`/`Eval` 与 CADDimension 同模式。
- 必需方法齐全；`NumSubs` 与 `SubAnim/SubAnimName` 一致。

## 7. 行数预算

`CADLeader.h <160`；`CADLeader.cpp <450`。超出时拆创建回调/绘制助手到公共模块。
