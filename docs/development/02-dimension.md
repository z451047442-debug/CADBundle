# CADDimension 设计

## 1. 基本架构

- 直接继承 `HelperObject`；`IsPublic()=FALSE`；Class_ID `0xec922077, 0xb1bf4a4a`；Category “CAD Tools”。
- ClassDesc2 完整实现 `IsPublic/Create/ClassName/NonLocalizedClassName/SuperClassID/ClassID/Category/InternalName/HInstance`。
- 构造函数 `MakeAutoParamBlocks(this)`；`NumRefs=1`，ref 0 为 pblock；`GetReference/SetReference` 仅处理 ref 0。
- 参数 UI 使用 Win32 `P_AUTO_UI`；不使用 Qt。
- 几何点为对象空间；创建时 `local = Inverse(mat) * vpt->SnapPoint(...)`。
- 屏幕缩放 `float s = ScreenScale(vpt, inode->GetObjectTM(t))`；箭头/文字/延伸线超长乘 `s`。
- 连续线 `DrawLineProc`；箭头闭合三角形直接 `gw->polyline(3, pts, nullptr, nullptr, 1, nullptr)`。
- 文本 BezFont billboard；颜色 RGB。

## 2. 参数块（BlockID `kDimParams=0x1001`）

| 索引 | 内部名 | 类型 | 默认/范围 | UI |
|---|---|---|---|---|
| 0 | `p0` | TYPE_POINT3 | (0,0,0) | `IDC_DIM_P0X/Y/Z` + 3 spins |
| 1 | `p1` | TYPE_POINT3 | (100,0,0) | `IDC_DIM_P1X/Y/Z` |
| 2 | `p2` | TYPE_POINT3 | (100,50,0) | `IDC_DIM_P2X/Y/Z` |
| 3 | `showXYZ` | TYPE_BOOL | FALSE | checkbox `IDC_DIM_SHOWXYZ` |
| 4 | `arrowSize` | TYPE_FLOAT | 8.0；0.1–1000 | `IDC_DIM_ARROW`，EDITTYPE_FLOAT |
| 5 | `textHeight` | TYPE_FLOAT | 12.0；1–1000 | `IDC_DIM_TEXTH`，EDITTYPE_FLOAT |
| 6 | `extensionOffset` | TYPE_FLOAT | 5.0；0–1000 | `IDC_DIM_EXTOFF`，EDITTYPE_FLOAT |
| 7 | `lineColor` | TYPE_RGBA | 0.1,0.8,0.1 | ColorSwatch `IDC_DIM_LINECOL` |
| 8 | `textColor` | TYPE_RGBA | 1,1,1 | ColorSwatch `IDC_DIM_TEXTCOL` |
| 9 | `textPrecision` | TYPE_INT | 2；0–6 | `IDC_DIM_PREC`，EDITTYPE_INT |

- `P_AUTO_CONSTRUCT + P_AUTO_UI`，ref 0，`P_VERSION 1`；点参数为 U9 的编辑镜像。
- `arrowSize`/`textHeight`/`extensionOffset` 是屏幕基准值，使用 `EDITTYPE_FLOAT`，不按场景单位显示。
- 颜色无 alpha；`showXYZ` 默认关闭。

## 3. 鼠标交互

- `point==0`：p0 = world→local；p1=p2=p0；返回 `CREATE_CONTINUE`。
- `point==1`：p1 = world→local；p2=p1；返回 `CREATE_CONTINUE`。
- `point==2`：p2 = world→local；`NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE)`；返回 `CREATE_STOP`。
- `MOUSE_MOVE`：`SnapPreview`，更新 pblock 预览点，`NotifyDependents` + `RedrawViews(REDRAW_INTERACTIVE)`。
- `MOUSE_ABORT`：返回 `CREATE_ABORT`；不在回调内调用 `StopCreating`；确保无持久 snap 状态。
- 默认 `CLICK_DRAG_CLICK` 下，三点工具在 point 2 提交；单点工具的点序见 CADText/Table 文档。

## 4. Display

```
tm = inode->GetObjectTM(t); gw->setTransform(tm); s = ScreenScale(vpt, tm);
dir = Normalize(p1-p0); normal = Normalize(Cross(dir, Point3(0,0,1)));
offset = DotProd(p2-p0, normal);
dimP0 = p0 + normal*offset; dimP1 = p1 + normal*offset;
```

- 延伸线：`p0 -> dimP0 + normal*(extensionOffset*s)`；`p1 -> dimP1 + normal*(extensionOffset*s)`；`DrawLineProc`。
- 尺寸线：`dimP0 -> dimP1`；`DrawLineProc`。
- 箭头：在 dimP0/dimP1 处闭合三角，尖端指向内侧；长度 `arrowSize*s`，宽约 `0.35*arrowSize*s`；`lineColor`。
- 距离文本：`Length(p1-p0)` 按 `textPrecision` 格式化（区域无关）；位置 `(dimP0+dimP1)*0.5 + normal*(textHeight*s*0.6)`；BezFont billboard，高度 `textHeight*s`；`textColor`。
- `showXYZ`：在 p0/p1/p2 法线方向偏移 `textHeight*s*0.8` 处绘制标签；高度约 `0.6*textHeight*s`；格式 “P0 (x, y, z)”。
- 退化：`Length(p1-p0)<1e-4` 时不画尺寸线/箭头/文本，仅点标记；不除零。
- 选中/冻结颜色按 SDK 惯例；保存/恢复 GW 状态；Display 不修改对象数据。

## 5. HitTest 与包围盒

- `GWRndLimitsGuard` 保存/恢复；`MakeHitRegion(hitRegion,type,crossing,4,p)`；`GW_PICK`；setTransform(tm)；绘制延伸线/尺寸线/箭头轮廓；`checkHitCode()`；文本用近似盒参与命中。
- `GetLocalBoundBox`：包含 p0/p1/p2、dimP0/dimP1、箭头、文本位置；屏幕恒定元素用 `s`；`vp==nullptr` 时 `s=1` + 裕量。
- `GetWorldBoundBox`：本地盒 8 点乘对象 TM；空 vp 用保守盒。
- `HasViewDependentBoundingBox()=TRUE`。

## 6. 持久化与引用

- pblock 自动保存参数与点镜像；`Save` 写 `kChunkDimVersion=0x0F01`、`kChunkDimPoints=0x0F02`：版本、点数 3、P0/P1/P2。
- `Load` 校验有限值并回填 pblock；chunk 缺失用 pblock；损坏返回 `IO_ERROR` 不崩溃。
- `Clone`：`BaseClone` + `remap.CloneRef(pblock)`；复制几何点；不复制临时缓存。
- `NotifyRefChanged`：pblock `REFMSG_CHANGE` -> `NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE)`；返回 `REF_SUCCEED`。
- 不重复写 pblock 参数；显式 chunk 只写几何点。

## 7. 必需方法

`GetClassName`、`ClassID`、`SuperClassID`、`DeleteThis`、`NumRefs/SetReference/GetReference`、`NotifyRefChanged`、`GetLocalBoundBox/GetWorldBoundBox`、`Display`、`HitTest`、`GetCreateMouseCallBack`、`BeginEditParams/EndEditParams`、`Save/Load`、`Clone`、`Eval`、`GetObjectName/InitNodeName`。`NumSubs` 与 `SubAnim/SubAnimName` 保持一致。

## 8. 行数预算

`CADDimension.h <180` 行；`CADDimension.cpp <500` 行。超出时把创建回调或绘制助手移到 `CADAnnotationCommon.cpp`。
