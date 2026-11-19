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
