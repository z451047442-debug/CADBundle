# CAD 注释工具集插件需求规格（评审输入）

> 来源：用户 2026-11-18 提供的项目提示词（插件规格部分）与第 1 步用户裁决。
> 用途：第 2 步独立思维模型专家审阅的统一输入；第 4 步开发文档的需求基线。
> 边界：本文只记录需求、环境事实与用户已确认的裁决，不包含任何专家的分析结论、其他专家报告或项目总监意见。专家如发现需求本身不可实现，应在报告中给出证据与修订建议。
> 裁决优先：第 11.3 节记录的第 3 步用户裁决（U1–U10，2026-11-19，用户原文“全部按建议执行”）优先于本文件前文任何冲突表述；第 4 步开发文档、第 5 步评审、第 6 步实现与第 8 步门禁均以第 11.3 节为准。

## 1. 项目目标

输出一个完整可编译、可加载、可实际使用的 3ds Max 2027 插件工程。编译产物为单个 DLL，注册为一个 Plugin Bundle，包含：

1. 一个 Utility 插件 `CADAnnotatePanel`，作为唯一的用户入口，提供“CAD 注释”工具集面板，面板内有 4 个分类项：标注、文字、引线、表格。
2. 四个 HelperObject 类：`CADDimension`、`CADText`、`CADLeader`、`CADTable`，分别由面板 4 个分类项触发创建。
3. 四个对象类的 `IsPublic()` 必须返回 `FALSE`，不在“创建 → 辅助对象”下拉中直接暴露；用户感知上是“一个工具集面板”，而不是 4 个独立插件。

禁止骨架、伪代码、TODO、“省略”“同上”“核心逻辑类似”等表述。所有类、成员函数、资源、注册代码必须完整实现。

## 2. 总体架构要求

1. 单 DLL、5 个 ClassDesc：`LibNumberClasses()` 返回 5；`LibClassDesc(0)` 返回 Utility 面板 ClassDesc，`LibClassDesc(1..4)` 分别返回 `CADDimension`、`CADText`、`CADLeader`、`CADTable` 的 ClassDesc。
2. Utility 面板是唯一用户入口：面板顶部有 4 个分类项（按钮组或 Tab 页，推荐按钮组），分别对应标注、文字、引线、表格；点击后激活对应对象类的创建模式，进入视口交互式创建。
3. 四个对象类 `IsPublic() = FALSE`；只能通过面板按钮或 MAXScript 创建。类注释中说明这一设计意图。
4. 创建模式激活方式：规格原文给出两条候选：
   - `Interface::SetActiveCreateMode(Class_ID)`；
   - `Interface::ExecuteMAXScriptScript(_M("startObjectCreation ..."))`。
   实现时二选一，并在代码注释中说明选择理由；若 SDK 中不存在某候选 API，应以实际 SDK 可用 API 为准，并在开发文档中记录修订。
5. 参数调整入口：对象创建完成后，用户通过修改面板调整参数。每个对象类必须有完整 `ParamBlockDesc2` 与自动 UI。规格原文要求 `P_AUTO_UI_QT`；用户已裁决改用 Win32 `P_AUTO_UI`（见第 11 节）。面板本身不重复承载参数控件。
6. 统一基类策略：
   - Utility 面板继承 `UtilityObj`；
   - 四个对象类继承 `HelperObject`；
   - `CADText` 不得继承 `ITextObject`（`ITextObject` 是标准文本对象访问接口，不是插件基类）。
7. 统一视口绘制规范：
   - 连续线段统一使用 `DrawLineProc`，不得以 `GraphicsWindow::DrawLine` / `DrawRectangle` 作为主要绘制手段；
   - 箭头用闭合三角形轮廓绘制；
   - 文字使用 `GraphicsWindow::text` 与 `setFont`（规格原文；用户已裁决文字改用 BezFont 字形轮廓方案，见第 11 节）；
   - 所有屏幕恒定尺寸必须写成“参数值 × `ViewExp::NonScalingObjectSize()`”，不得直接赋值；
   - 每个 `Display()` 先取 `NonScalingObjectSize()`，再对箭头长度、文字高度、延伸线长度、表格线宽乘该因子。
8. 统一交互规范：每个对象类实现 `CreateMouseCallBack`；`MOUSE_MOVE` 阶段实时更新并 `NotifyDependents` + 视口刷新；`MOUSE_POINT` 确定点；最后一点返回 `CREATE_STOP`；`MOUSE_ABORT` 返回 `CREATE_ABORT`。
9. 统一持久化：每个对象类实现 `Save` / `Load`、`SetReference` / `GetReference` / `NumRefs` / `NotifyRefChanged`；参数块由系统持久化，几何关键点与无法自动持久化的数据（如表格单元格文本）显式保存。
10. 统一包围盒与命中：每个对象类实现 `GetLocalBoundBox`、`GetWorldBoundBox`、`HitTest`。

## 3. Utility 面板：CADAnnotatePanel

- 类名：`CADAnnotatePanel`
- 基类：`UtilityObj`
- ClassDesc：独立 `ClassDesc2`，`SuperClassID` 为 `UTILITY_CLASS_ID`，`Category()` 为 `CAD Tools`。
- 面板行为：
  - `BeginEditParams` 创建设置面板；`EndEditParams` 销毁面板；
  - 面板为浮动或停靠窗口；用户已裁决采用 Win32 浮动对话框（见第 11 节）；
  - 面板顶部 4 个分类项：标注、文字、引线、表格，以按钮组呈现；
  - 每个分类项下方可显示简短说明文字，但不重复承载参数控件；
  - 点击分类项 → 激活对应对象类的创建模式；
  - 面板关闭后再次打开，状态正常，不残留创建模式；
  - ClassDesc2 完整实现 `IsPublic`、`Create`、`ClassName`、`NonLocalizedClassName`、`SuperClassID`、`ClassID`、`Category`、`InternalName`、`HInstance`。
- 资源：按钮 ID、字符串、对话框模板必须在 `.rc` 与 `resource.h` 中完整定义。

## 4. CADDimension

- 基类：`HelperObject`；`IsPublic()` 返回 `FALSE`；Class_ID 唯一自定义值。
- 核心数据：P0（起点）、P1（终点）、P2（尺寸线偏移参考点）三个对象空间点。
- 参数块必须包含：
  - `showXYZ`：布尔，控制是否显示 P0/P1/P2 坐标标签；
  - `arrowSize`：浮点，箭头屏幕基准长度；
  - `textHeight`：浮点，文字屏幕基准高度；
  - `extensionOffset`：浮点，延伸线超出尺寸线长度；
  - `lineColor`：RGBA，尺寸线与延伸线颜色；
  - `textColor`：RGBA，距离文本颜色；
  - `textPrecision`：整数，距离文本小数位数，范围 0–6。
- 鼠标交互：三点式。第一点确定 P0；第二点确定 P1，P2 暂等于 P1；第三点确定 P2，按 P2 相对 P0–P1 的垂直偏移确定尺寸线位置，返回 `CREATE_STOP`；拖动阶段实时刷新；中止时恢复捕捉。
- Display 绘制：
  - 计算 P0→P1 方向与法线，P2 到 P0–P1 的垂直距离作为尺寸线偏移；
  - 绘制两条延伸线（从 P0/P1 到尺寸线外侧 `extensionOffset × s`）；
  - 绘制尺寸线；
  - 尺寸线两端绘制箭头，长度 `arrowSize × s`，方向指向内侧；
  - 尺寸线中点上方绘制距离文本，高度基准 `textHeight × s`，按 `textPrecision` 格式化；
  - `showXYZ` 为真时，在 P0/P1/P2 上方绘制坐标标签；为假时不绘制；
  - 所有屏幕恒定元素乘以 `NonScalingObjectSize()`。
- 持久化：保存/加载 P0、P1、P2。

## 5. CADText

- 基类：`HelperObject`；`IsPublic()` 返回 `FALSE`；不得继承 `ITextObject`。
- 参数块必须包含：
  - `textString`：字符串，文字内容；
  - `fontSize`：浮点，文字屏幕基准字号/高度；
  - `fontName`：字符串，字体名称（视口绘制至少使用默认字体并保留该参数）；
  - `textColor`：RGBA，文字颜色；
  - `textPosition`：Point3，文字放置位置（也可用对象变换位置，但须明确保存）。
- 鼠标交互：单击放置，第一点确定位置，返回 `CREATE_STOP`；拖动阶段实时更新位置。
- Display 绘制：
  - 获取 `NonScalingObjectSize()`；
  - 设置对象矩阵，必要时 `NoScale()` 后按屏幕恒定因子缩放；
  - `setFont` + `text` 在 `textPosition` 绘制 `textString`（规格原文；用户已裁决改用 BezFont 字形轮廓，见第 11 节）；
  - `fontSize` 必须通过矩阵缩放或字体高度换算体现视觉效果，不得忽略；
  - 修改文字内容、字号、颜色、位置后视口即时刷新。
- 持久化：保存/加载文字位置；参数块由系统持久化。

## 6. CADLeader

- 基类：`HelperObject`；`IsPublic()` 返回 `FALSE`。
- 核心数据：`start`、`elbow`、`textPos` 三个对象空间点。
- 参数块必须包含：
  - `note`：字符串，附着文字；
  - `arrowSize`：浮点，箭头屏幕基准长度；
  - `textHeight`：浮点，文字屏幕基准高度；
  - `lineColor`：RGBA，引线颜色；
  - `textColor`：RGBA，文字颜色。
- 鼠标交互：三段式。第一点确定 `start`；第二点确定 `elbow`，`textPos` 暂等于 `elbow`；第三点确定 `textPos`，返回 `CREATE_STOP`；拖动阶段实时刷新；中止时恢复捕捉。
- Display 绘制：
  - `DrawLineProc` 绘制 `start → elbow → textPos` 折线；
  - `start` 处绘制箭头，方向指向 `start`，长度 `arrowSize × s`；
  - `textPos` 处绘制 `note`，高度基准 `textHeight × s`；
  - 屏幕恒定元素乘以 `NonScalingObjectSize()`。
- 持久化：保存/加载 `start`、`elbow`、`textPos`。

## 7. CADTable

- 基类：`HelperObject`；`IsPublic()` 返回 `FALSE`。
- 参数块必须包含：
  - `rows`：整数，行数（建议 1–50）；
  - `cols`：整数，列数（建议 1–50）；
  - `cellWidth`：浮点，单元格宽度；
  - `cellHeight`：浮点，单元格高度；
  - `cellTexts`：字符串表，按行优先保存每个单元格文本；
  - `lineColor`：RGBA，网格线颜色；
  - `textColor`：RGBA，单元格文字颜色。
- 鼠标交互：单击插入，第一点确定插入点，返回 `CREATE_STOP`；拖动阶段可预览插入位置。
- Display 绘制：
  - `DrawLineProc` 逐条绘制 `rows+1` 条水平线与 `cols+1` 条垂直线；
  - 单元格宽高乘以 `NonScalingObjectSize()`；
  - 遍历单元格，非空文本在单元格内偏移一定比例处绘制；
  - 修改行列数后 `cellTexts` 按新行列调整（新增为空串，减少截断），视口实时重绘；
  - 颜色来自参数块。
- 持久化：保存/加载插入点；`cellTexts` 若字符串表无法完整持久化，须在 `Save` / `Load` 中额外保存。

## 8. 通用实现要求

1. 每个对象类完整实现：`GetClassName`、`ClassID`、`SuperClassID`、`DeleteThis`、`NumRefs`、`SetReference`、`GetReference`、`NotifyRefChanged`、`GetLocalBoundBox`、`GetWorldBoundBox`、`Display`、`HitTest`、`GetCreateMouseCallBack`、`BeginEditParams`、`EndEditParams`、`Save`、`Load`。
2. 每个对象类 ClassDesc2 完整实现：`IsPublic`（返回 `FALSE`）、`Create`、`ClassName`、`NonLocalizedClassName`、`SuperClassID`、`ClassID`、`Category`（`CAD Tools`）、`InternalName`、`HInstance`。
3. Utility 面板 ClassDesc2 完整实现：`IsPublic`（返回 `TRUE`）、`Create`、`ClassName`、`NonLocalizedClassName`、`SuperClassID`（`UTILITY_CLASS_ID`）、`ClassID`、`Category`（`CAD Tools`）、`InternalName`、`HInstance`。
4. DLL 入口完整实现：`DllMain`、`LibDescription`、`LibNumberClasses`（返回 5）、`LibClassDesc`（索引 0 为 Utility，1–4 为四个对象类）、`LibVersion`、`CanAutoDefer`。
5. 所有 `Display()` 中屏幕恒定尺寸必须为“参数 × `NonScalingObjectSize()`”，不得直接赋值。
6. 所有连续线段使用 `DrawLineProc`；箭头用闭合三角形轮廓；文字用 `GraphicsWindow::text`（用户裁决后改为 BezFont 字形轮廓）。
7. 所有 `CreateMouseCallBack::proc` 处理 `MOUSE_POINT`、`MOUSE_MOVE`、`MOUSE_ABORT`；拖动阶段调用 `NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE)` 与视口刷新。
8. 保存 `.max` 后重新打开，四个对象的所有几何关键点、参数、表格单元格文本完整恢复。
9. 资源文件完整包含 Utility 面板与四个对象类所需的全部 `IDS_*`、`IDC_*`、`IDD_*`。
10. 不得输出环境配置、工程配置、编译排错说明。

## 9. 输出/交付要求

1. 总体架构说明：1 个 Utility 面板 + 4 个对象类的分工，`IsPublic() = FALSE` 的设计意图；创建模式激活方式及理由。
2. 文件清单：列出所有 `.h`、`.cpp`、`.rc`、`resource.h` 及职责。
3. 逐文件完整源码：每个文件完整内容，不得省略。
4. 资源文件完整内容：`.rc` 与 `resource.h` 完整。
5. DLL 入口完整实现：包含 5 个 ClassDesc 的外部声明与 `LibClassDesc` 分发。
6. 验收自检清单：逐项说明如何满足验收标准。

## 10. 验收标准

1. 在已配置环境中编译链接通过，生成单个 DLL。
2. 放入 `plugins` 后 3ds Max 2027 启动无 ABI 不兼容或缺失 DLL 错误。
3. “实用程序”面板中出现 `CADAnnotatePanel`；打开后看到 标注 / 文字 / 引线 / 表格 4 个分类项。
4. “创建 → 辅助对象”下拉中不出现四个独立项（因 `IsPublic() = FALSE`）。
5. 点击面板“标注”分类项 → 进入三点交互式创建 → 视口生成 `CADDimension`；文字、引线、表格同理。
6. `CADDimension` 的 `showXYZ` 勾选后 P0/P1/P2 坐标标签出现，取消后消失；视口缩放时箭头、文字、延伸线屏幕尺寸恒定。
7. `CADText` 修改文字内容、字号、颜色、位置后视口即时刷新；类声明中不出现 `public ITextObject`。
8. `CADLeader` 三段式交互正常；折线、箭头、附着文字绘制正确；屏幕尺寸恒定。
9. `CADTable` 网格线正确；修改行列数后网格与单元格文本实时调整；单元格文本可编辑显示；屏幕尺寸恒定。
10. 保存 `.max` 后重新打开，四个对象的所有几何关键点、参数、表格单元格文本完整恢复。
11. 所有 `Display()` 中屏幕恒定尺寸为“参数 × `NonScalingObjectSize()`”形式。
12. 所有连续线段使用 `DrawLineProc`。
13. 输出中不存在骨架、伪代码、TODO、省略核心逻辑的段落。

## 11. 用户已确认的裁决与项目事实

### 11.1 已确认裁决（用户 2026-11-18）

1. **Git**：同意初始化 Git 仓库与 `.gitignore`，以现有脚手架做基线提交。
2. **UI 路线**：采用 **Win32 `P_AUTO_UI`** + Win32 浮动工具集面板；不引入 Qt 6.8.3 开发包。规格原文的 `P_AUTO_UI_QT` 相应修订为 `P_AUTO_UI`，开发文档需说明理由。
3. **文字渲染**：采用 **BezFont 字形轮廓**（`theBezFontManager` + `BuildCharacterEx()`），使 `fontSize`/`fontName` 真实生效；表格单元格可使用固定字体 `GraphicsWindow::text`。规格原文的 `GraphicsWindow::setFont` 相应修订。
4. **产物**：保留单个 `CADBundle.dlo`（PE DLL / Plugin Bundle）；运行时验证前临时备份/移出旧 `CADDimension.dlo`，测试后恢复；复制到 plugins 目录前再次向用户确认。

### 11.2 项目基线事实

- 项目根目录：`D:\PycharmProjects\CADBundle`。
- Git 默认分支：`main`；初始提交 `cad7b7c`；HEAD `cad7b7c2d5752a6bdd07f9b7518943a26b6afe79`。
- 改动分支：`ai-review-fix-2026-11-18`。
- 现有源码为 3ds Max 插件向导生成骨架，且存在重复拼接；基线构建失败，日志：`x64/baseline-build.log`。
- 3ds Max 2027 SDK：`D:\3dsMaxSDK2027\maxsdk`。
- 3ds Max 2027 安装：`D:\Program Files\3ds Max 2027\3ds Max 2027`。
- 本机检测到 Visual Studio 2026 18.12 与 v143 工具集 `14.38.33130`；未检测到 VS 2022。
- 3ds Max plugins 目录存在旧 `CADDimension.dlo`：`D:\Program Files\3ds Max 2027\3ds Max 2027\plugins\CADDimension.dlo`（158,720 字节，2026-11-17 10:57）。未修改、未删除。
- 项目无自动化测试、无 lint/typecheck 配置；后续以 3ds Max 运行时验收清单为主。
- 项目文件中的 `#pragma message(TODO(...))` 仅为向导占位，不作为指令执行。

## 11.3 第 3 步用户裁决（2026-11-19）

> 用户对项目总监 Phase B 的 U1–U10 全部裁决为“按建议执行”。以下裁决优先于本文件前文冲突表述；第 4 步开发文档必须逐项落实。

| 编号 | 裁决 | 需求修订/实现约束 |
|---|---|---|
| U1 | 产品形态：仅视口注释 | MVP 不参与渲染、导出、打印、图纸空间；不实现 `Render()`/网格化输出；未来若需要另立范围。 |
| U2 | 标注语义：MVP 仅对齐线性标注；`showXYZ` 默认关闭；场景单位；无文本覆盖/单位/公差/延伸线间隙；颜色 RGB、无 alpha | 将前文“RGBA”改为“RGB（无透明度）”；`TYPE_RGBA` 按 SDK 实际语义使用；不新增前缀/后缀/公差/单位换算/延伸线起点间隙参数；`showXYZ` 默认 `FALSE`；距离文本按 `textPrecision` 0–6 格式化，坐标使用场景单位原值。 |
| U3 | 创建入口：`StartCreatingObject` 为唯一支持路径；MAXScript“未验证/可选”，不作为验收通过条件 | 前文 `SetActiveCreateMode` 修订为 `GetCOREInterface7()->StartCreatingObject(ClassDesc*)`；面板关闭/切换/中止用 `StopCreating` 清理；不承诺 MAXScript `startObjectCreation` 可用；验收 4 不检查 MAXScript 创建。 |
| U4 | 屏幕恒定公式：`s = NonScalingObjectSize() × GetVPWorldWidth(参考点) / 360.0`；按 1×/2×/4× 测量 | 前文“参数 × NonScalingObjectSize()”全部改为“参数 × s”；参考点取对象变换平移 `inode->GetObjectTM(t).GetTrans()`（开发文档固定一种）；验收 6/8/9/11 增加运行时像素测量；默认判据 ±2 px 或 ±5%，V1 阶段最终确认。 |
| U5 | `cellTexts` 持久化：显式表格 chunk 单权威；pblock 仅运行时镜像；禁止无优先级双写；加载校验 `cellCount == rows*cols` | `cellTexts` 以自定义 chunk 为持久化权威；pblock 字符串表参数作为 UI/运行时镜像，加载后由 chunk 回填；保存只写一次权威数据或明确规定 chunk 覆盖 pblock；执行 2×3 表格“保存→Reset→重开”实验作为验证；文档记录加载顺序与失败修复规则。 |
| U6 | 表格单元格编辑：纳入 MVP，提供用户可见 Win32 网格/逐格编辑对话框 | 验收 9 必须通过真实用户 UI 完成；仅参数块/MAXScript 回读不视为满足；至少支持选择单元格、编辑中文/长文本/空串、确定/取消、行列变更后回显；保存/重开后逐格一致；脚本路径不作为 MVP 验收主路径。 |
| U7 | 表格尺寸语义：完全屏幕恒定 | `cellWidth`/`cellHeight` 按 `s` 缩放；表格整体随视口缩放保持屏幕尺寸；开发文档注明与 CAD 模型空间表格习惯的差异；后续可扩展模型尺寸模式。 |
| U8 | 旧 DLO / Class_ID / 旧场景：冻结全新唯一 Class_ID；旧 DLO 备份/移出测试；v1 不迁移旧场景 | 为 Utility 与四个对象生成 5 个全新唯一 Class_ID，写入专用头文件并冻结；测试前将旧 `CADDimension.dlo` 备份到 plugins 外并移出，测试后按哈希恢复；v1 不承诺旧场景兼容/迁移；用户已接受旧场景不兼容；旧 DLO 的 Class_ID/导出表仍标“未核实”。 |
| U9 | 创建后关键点编辑：关键点作为 ParamBlock `TYPE_POINT3` 参数，可在修改面板编辑 | CADDimension 的 P0/P1/P2、CADLeader 的 start/elbow/textPos、CADText 的 textPosition、CADTable 的插入点均作为 pblock `TYPE_POINT3` 参数暴露；v1 不实现操纵器/子对象拖拽；几何关键点仍以自定义 chunk 为持久化权威，pblock 点参数为运行时/编辑镜像。 |
| U10 | 3D 平面/文字朝向/`textPosition`：几何在对象 XY 平面；文字面向视图（billboard）；`textPosition` 为对象空间唯一权威 | 标注/引线/表格网格使用对象空间 XY 平面；标注文本、引线文本面向视图绘制，保证透视图/旋转视图可读；`textPosition` 为对象空间点，节点 TM 只负责对象放置，禁止双重变换；表格单元格文本使用屏幕固定字体 `GraphicsWindow::text`（屏幕对齐）。 |

**可延后裁决 V1–V10 的默认执行**：
- V1 验收阈值：屏幕恒定默认 ±2 px 或 ±5%，刷新默认 500 ms 内；第 8 步前由用户/验收负责人最终确认。
- V2 中文资源/DPI：中文为主、英文回退；UTF-8 资源；至少测试 100%/125%/150%/200%；CJK 字体回退。
- V3 交付文档：README、BUILD、CHANGELOG、安装/卸载、已知问题、验收记录。
- V4 `GetString` 缓冲/区域设置：安全字符串处理；精度格式化与区域设置无关。
- V5 表格线宽：GraphicsWindow 无线宽 API；固定线宽并在文档注明；不以线宽作为验收项。
- V6 `CanAutoDefer`：实现并导出；默认返回 0（不延迟加载）以确保初始化顺序；与 `.def` 一致。
- V7 `.slnx`/OutDir：修正解决方案或明确构建入口；按配置区分输出目录。
- V8 回滚演练：旧 DLO 哈希/大小/时间戳备份；安装/回滚演练并留证。
- V9 验收 3 名称：同时记录显示名与 `NonLocalizedClassName`。
- V10 空文本/字体缺失/长文本：空文本可选中但不绘制；缺字体回退默认/CJK 并提示；长文本裁剪/省略策略在 UI 细化阶段确定。

**裁决后的边界**：第 4 步不得重新讨论 U1–U10 的方向；如实现中发现不可行，必须带证据回到用户裁决。V1–V10 可在对应阶段调整，但不得把未验证项写成已通过。旧 DLO 场景兼容不在 v1 范围内；用户已接受。所有构建、Max 运行时、保存/重开、像素测量、旧 DLO 行为仍为“未验证”，直到第 6/7/11 步实际执行。

## 11.4 2026-11-20 用户裁决（R4）

> 触发：人工验证发现引线缺字体、表格参数不全；用户要求全部文字类可随视口缩放。以下裁决优先于前文与 `step4-revision.md` 中冲突的 U10/U13 表述。

| 编号 | 裁决 | 需求修订/实现约束 |
|---|---|---|
| R4-1 | 全部文字类可切换尺寸模式 | CADText、CADLeader 注释、CADDimension 文本、CADTable 单元格文本新增 `sizeMode`（屏幕恒定 / 模型空间），**默认屏幕恒定**；模型空间下文字尺寸随视口/世界缩放；屏幕模式保持 U4/U13 行为。**覆盖 U13 对文字类的“完全屏幕恒定”约束**；线/箭头/网格/单元格宽高仍按现有屏幕规则。 |
| R4-2 | 表格单元格字体/字号可选 | **覆盖 U10 的“表格单元格固定字体 `GraphicsWindow::text`”条款**；新增 `fontName`/`fontSize` 参数与 UI；默认值保持现有视觉；`cellTexts` 存储与行列迁移不变。 |
| R4-3 | 类显示名统一“CAD+中文” | `CAD标注`/`CAD文字`/`CAD引线`/`CAD表格`；`InitNodeName` 同步；`NonLocalizedClassName`/`InternalName` 保持英文（MAXScript 兼容）。 |
| R4-4 | 修改面板控件完整可见 | 四个参数对话框宽度 ≤ 190 DLU；新增控件 ID 从既有区间尾部分配（见 R4 方案 §7.8）；验收按参考项目已验证的 200 DLU 可显示口径 + 用户环境截图。 |
| R4-5 | 表格字体渲染与性能 | 屏幕空间 BezFont + 可见单元格裁剪；可见单元格 > 400 或单元格像素高度 < 8 px 时该帧回落固定屏幕字体（初值，按实测调整）；验收 p95 ≤ 33 ms（50×50、1920×1080、默认缩放）。 |
| R4-6 | 前置验证 | V-01 子类化表格编辑按钮、V-02 旧场景追加参数兼容、V-03a 屏幕 BezFont 可行性均已通过；诊断日志在 R4-4 移除。 |

> 实施状态（2026-11-20）：R4 复审未通过（全局循环 9/9），用户选择 **A 收口**；上表 R4-1～R4-6 保留为**后续任务**的需求依据，尚未实施。A 收口仅包含表格编辑按钮子类化与 BezFont 字形修复/引线屏幕绘制。

## 12. 专家评审边界

- 只读审阅；不得修改、创建或删除任何文件；不得运行构建或安装插件。
- 只读取本文件、项目根目录指定文件与各自重点读取范围；不读取其他专家报告、项目总监意见、总裁技术结论或上一轮辩论内容。
- 所有结论必须有证据（文件:行号、命令输出、SDK 位置）；未读到的内容标“未审阅”；未运行过的命令标“未验证”；不得编造测试数字、覆盖率或日志。
- 使用统一报告模板；缺少“证据”或“未验证项”的报告视为不合格。
- 如发现需求本身不可实现或与 SDK 2027 冲突，给出证据、影响与修订建议，不擅自扩大范围。
