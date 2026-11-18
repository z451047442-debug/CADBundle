# 架构与文件清单

## 1. 总体架构

- 单 DLL：`CADBundle.dlo`，x64，Plugin Bundle。
- `LibNumberClasses() = 5`；`LibClassDesc(0)` = `CADAnnotatePanel`（UTILITY_CLASS_ID，IsPublic=TRUE）；1–4 = CADDimension/CADText/CADLeader/CADTable（HELPER_CLASS_ID，IsPublic=FALSE）。
- Utility 面板是唯一用户入口；按钮调用 `GetCOREInterface7()->StartCreatingObject(desc)`；关闭/切换/中止用 `StopCreating` 清理。
- 四个对象均继承 `HelperObject`；公共逻辑通过自由函数/小助手模块复用，不改变继承关系。
- 参数 UI：Win32 `P_AUTO_UI`；不使用 Qt/`P_AUTO_UI_QT`。
- 文本：BezFont 字形轮廓，面向视图绘制；表格单元格用 `GraphicsWindow::text`。
- 持久化：pblock 保存参数；几何关键点与 `cellTexts` 使用显式自定义 chunk 为权威；pblock 中的点/字符串表为编辑镜像。

## 2. Class_ID（本地生成，冻结候选）

| 类 | Class_ID |
|---|---|
| CADAnnotatePanel（Utility） | `Class_ID(0xb8266181, 0x1d05484b)` |
| CADDimension | `Class_ID(0xec922077, 0xb1bf4a4a)` |
| CADText | `Class_ID(0xf2b7067c, 0x62a54963)` |
| CADLeader | `Class_ID(0x530b0886, 0xf00a4baa)` |
| CADTable | `Class_ID(0xf36d1d67, 0x86164c67)` |

- 写入 `src/CADBundleIDs.h`，全工程只引用该头文件；实现过程中不得再改。
- 旧 DLO 不复用上述 ID；v1 不迁移旧场景（U8）。
- B0 构建时用静态脚本检查 5 个 ID 互不重复且与旧工程宏不重复。

## 3. 目录与文件清单

> 现有根目录向导文件在 B0 通过 `git mv` 迁移到 `src/`（保留历史，不删除内容）；如用户要求保留原件，先备份再迁移。所有文件目标 <500 行。

| 文件 | 职责 | 预估行数 |
|---|---|---|
| `src/CADBundle.h` | 公共 include、`extern HINSTANCE hInstance`、`GetString`、资源头引用 | <80 |
| `src/CADBundleIDs.h` | 5 个 Class_ID 常量、pblock ID/参数 ID/chunk ID 常量 | <120 |
| `src/DllEntry.cpp` | DllMain、LibDescription、LibNumberClasses=5、LibClassDesc、LibVersion、LibInitialize/Shutdown、CanAutoDefer、GetString | <180 |
| `src/CADAnnotatePanel.h/.cpp` | UtilityObj、ClassDesc2、Win32 浮动面板、4 按钮、状态机、创建/清理 | <450 |
| `src/CADAnnotationCommon.h/.cpp` | 屏幕缩放、DrawLineProc 助手、闭合箭头、命中/包围盒助手、格式化、GW 状态 RAII | <400 |
| `src/CADParamBlockHost.h/.cpp` | pblock 构造/引用/Clone/NotifyRefChanged/BeginEditParams/EndEditParams 的公共助手（自由函数，不改变继承） | <300 |
| `src/CADBezFontText.h/.cpp` | BezFont 打开/关闭、字形缓存、billboard 变换、绘制、字体回退、文本包围盒 | <450 |
| `src/CADDimension.h/.cpp` | CADDimension 全部实现、pblock/UI、三点交互、显示、命中、Save/Load | <500 |
| `src/CADText.h/.cpp` | CADText 全部实现、pblock/UI、单击放置、BezFont 文本、Save/Load | <450 |
| `src/CADLeader.h/.cpp` | CADLeader 全部实现、pblock/UI、三段交互、折线/箭头/文本、Save/Load | <450 |
| `src/CADTable.h/.cpp` | CADTable 全部实现、pblock/UI、网格、行优先 cellTexts、行列迁移、Save/Load | <500 |
| `src/CADTableEditor.h/.cpp` | Win32 单元格编辑对话框（ListView/CustEdit 或等价网格）、行列调整、确定/取消 | <450 |
| `src/CADPersistence.h/.cpp` | chunk 版本、点/字符串表写入读取、校验、加载优先级、错误路径 | <350 |
| `res/resource.h` | 全部 IDS_/IDC_/IDD_ 唯一 ID | <250 |
| `res/CADBundle.rc` | 中文/英文回退字符串、Utility 面板、4 个参数对话框、表格编辑器对话框、版本信息 | <450 |
| `CADBundle.vcxproj` | x64/Release 等配置、源文件/资源、`/utf-8`、C++20、v143、SDK 路径、输出目录 | <180 |
| `CADBundle.def` | 7 个导出（含 CanAutoDefer） | <20 |
| `docs/development/*` | 本开发文档 | — |

## 4. 构建配置

- 平台：x64；配置至少 Release；保留 Debug/Hybrid 但按配置区分 `OutDir`，避免覆盖 Release。
- 工具集：v143（14.38.33130）；C++20；`/utf-8`；警告级别 Level4；不把警告一律当错误（SDK 头警告多），但本项目代码新增警告需修复。
- `TargetExt=.dlo`；`TargetName=CADBundle`；`OutDir` 建议 `$(ProjectDir)build\$(Platform)\$(Configuration)\plugins\`。
- 包含路径：`$(MaxSdkInc)`；资源编译使用 UTF-8（`#pragma code_page(65001)` + `/c65001` 视 RC 行为验证）。
- 链接库：沿用 SDK 属性表；按 B0 实际链接结果补齐（现有列表含 bmm/core/Geom/gfx/mesh/maxutil/maxscrpt/paramblk2；如 BezFont/BezierShape 需要，追加对应 SDK 库，不引入第三方）。
- 不链接 Qt；不引用 `MaxQtBridge.lib`。
- `CADBundle.slnx` 若仍不可用，明确以 `CADBundle.vcxproj` 为构建入口；或生成有效解决方案。

## 5. DLL 入口与注册

- `DllMain`：保存 `hInstance`、`UseLanguagePackLocale()`、`DisableThreadLibraryCalls`；不做重初始化。
- `LibDescription`：本地化“CAD 注释工具集”。
- `LibNumberClasses`：返回 5。
- `LibClassDesc(i)`：0 Utility，1 CADDimension，2 CADText，3 CADLeader，4 CADTable；其他返回 nullptr。
- `LibVersion`：`VERSION_3DSMAX`。
- `LibInitialize`/`LibShutdown`：初始化/清理 DLL 级资源；不强制关闭仍被对象持有的 BezFont 句柄；`GetString` 使用安全缓冲/`GetResourceStringAsMSTR` 方案。
- `CanAutoDefer`：导出；默认返回 0（不延迟加载），确保初始化顺序。
- `CADBundle.def` 导出：LibDescription、LibNumberClasses、LibClassDesc、LibVersion、LibInitialize、LibShutdown、CanAutoDefer。

## 6. 公共基础设施

### 6.1 屏幕缩放

```cpp
float ScreenScale(ViewExp* vpt, const Matrix3& tm) {
    if (!vpt || !vpt->IsAlive()) return 1.0f;
    return vpt->NonScalingObjectSize()
         * vpt->GetVPWorldWidth(tm.GetTrans()) / 360.0f;
}
```
- 所有 Display 先取 `s`；箭头、文字高度、延伸线超长、表格 cellWidth/cellHeight、线宽替代偏移均乘 `s`。
- 参考点固定为对象变换平移 `tm.GetTrans()`（U4）；文字 billboard 使用文字位置处的缩放。
- 1×/2×/4× 像素测量按 A6/A11。

### 6.2 绘制助手

- `DrawPolyline(GraphicsWindow*, const Point3*, int)` 内部用 `DrawLineProc`；连续线统一走此助手。
- 闭合箭头：`gw->polyline(3, pts, nullptr, nullptr, 1, nullptr)` 或派生专用 `ClosedLineProc`；文档记录 `DrawLineProc::proc` 硬编码 `closed=0` 的例外。
- `GWRndLimitsGuard`：构造保存 `getRndLimits()`，析构恢复；避免样例 `HitTest` 早退不恢复的缺陷。
- 颜色：`gw->setColor(LINE_COLOR, Color)`；文本用 `TEXT_COLOR`；RGB 无 alpha。

### 6.3 引用与 pblock 助手

- 每个对象构造函数 `MakeAutoParamBlocks(this)`；`NumRefs=1`；ref 0 为 pblock；`GetReference/SetReference` 只处理 ref 0；`Clone` 用 `BaseClone` + `remap.CloneRef(pblock)`。
- `NotifyRefChanged`：pblock `REFMSG_CHANGE` 时失效字形/几何缓存、`NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE)` 并 `RedrawViews`。
- `BeginEditParams`/`EndEditParams` 调用 ClassDesc2 自动 UI；Utility 面板单独实现。

### 6.4 BezFont 文本

- `theBezFontManager.OpenFont(fontName, 0)`；失败回退默认/CJK 字体并记录。
- `BuildCharacterEx(handle, charCode, 1000.0f, shape, width, version, false)` 生成字形；按文本/字体/字号缓存。
- 字形按 `fontSize * s / 1000` 缩放；面向视图绘制（billboard），保证透视图可读。
- 表格单元格使用 `gw->text`（屏幕固定字体），不经过 BezFont。

### 6.5 持久化

- 自定义 chunk ID 按类分配；版本号 + 点数 + 点坐标；表格维度 + cellCount + 长度前缀字符串（UTF-8/Unicode 安全写入）。
- 单权威：点/cellTexts 以 chunk 为准；pblock 点/字符串表为镜像；加载后清空/SetCount 再回填；禁止无优先级双写。
- 加载校验：`cellCount==rows*cols`、rows/cols 1–50、字符串长度上限、点非 NaN/Inf、未知 chunk 跳过、损坏返回 `IO_ERROR` 不崩溃。
- pblock 参数自动持久化；版本迁移用 `P_VERSION`/`ParamBlock2PLCB`（如需要）。

## 7. 构建顺序与门禁

1. B0：迁移/整理文件、IDs、资源骨架、5 个 ClassDesc 最小实现、构建通过。
2. B1：Utility 面板 + 创建链路 + 启动验证。
3. B2：公共基础设施（scale/draw/BezFont/pblock）。
4. B3–B6：四类对象逐类实现，每类单独提交。
5. B7：持久化与字段比较。
6. B8：集成、文档、发布准备。

> 本文件只定义架构与文件职责；各对象参数、交互、资源 ID、验收细节见后续文档。
