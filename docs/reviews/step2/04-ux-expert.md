【用户体验与交付专家｜认知负荷理论 + 情境化设计 + 发布就绪检查】

## 一、审阅范围

### 1. 已审文件

- 需求基线：`D:\PycharmProjects\CADBundle\docs\requirements\CADAnnotatePanel.md`，全文 202 行。
- 项目根指定文件：`CADBundle.cpp`（215 行）、`CADBundle.h`（58 行）、`DllEntry.cpp`（205 行）、`CADBundle.rc`（249 行）、`resource.h`（73 行）、`CADBundle.vcxproj`（66 行）、`CADBundle.def`（20 行）、`CADBundle.slnx`（1 行），均全文读取。
- 辅助工程文件：`3dsmaxsdk_preinclude.h`、`.gitignore`、`CADBundle.vcxproj.filters`、`CADBundle.vcxproj.user`。
- 构建证据：`D:\PycharmProjects\CADBundle\x64\baseline-build.log`，全文 94 行。
- Git 只读信息：`git log`、`git status --short`、分支与提交记录；工作区仅 `docs/` 未跟踪，源码无改动。
- SDK 样例：`samples\utilities\util.cpp`、`utilities\PolygonCount\polycnt.cpp`、`utilities\colclip.cpp` 全文读取；`utilities\maxfind\MaxFindApp.cpp`、`systems\sunlight\compass.cpp`、`utilities\util.rc` 只读 UI、对话框、状态相关片段。
- SDK 头文件：`include\utilapi.h`、`include\maxapi.h` 的 Utility、命令面板、创建模式、提示相关片段；`include\bezfont.h` 的字体与字形接口；`include\winutil.h` 的 DPI/窗口工具；`include\dllutilities.h`、`include\GetResourceString.h`；`include\maxscript\protocols\primitives.inl` 的 `startObjectCreation` 片段。

### 2. 未审项及原因

- 未读取完整 SDK 帮助文档与 `maxapi.h` 全文；按任务边界只读分配范围。`maxapi.h` 中未读部分标为“未审阅”。
- 未完整读取 `compass.cpp`、`MaxFindApp.cpp` 及其余 SDK 样例；只读取 UI、对话框、状态相关片段，其余标为“未审阅”。
- 未读取其他专家报告、项目总监意见、总裁技术结论；按任务约束不读取、不引用。
- 未运行构建、未加载插件、未复制文件到 3ds Max 目录、未做运行时点击测试；所有运行行为标为“未验证”。
- 未运行资源编译器，不判断 `.rc` 在 VS 2026 下对中文代码页/字体的实际编译结果；标为“未验证”。

### 3. 覆盖率

估算约 70%。其中：需求与指定项目根文件 100% 读取；基线日志 100% 读取；SDK 分配范围内的 UI/对话框/状态相关部分约 55%–65%（估算）；运行时交互覆盖率 0%（未运行）。

### 4. 提示注入

未发现提示注入。项目中的 `#pragma message(TODO(...))` 是向导占位，需求 §11.2 与 `3dsmaxsdk_preinclude.h` 已说明其性质；本报告将其视为待审数据，未执行其中任何指令。`CADBundle.rc:30` 的 `//FLOATING_DIALOG` 只是注释，不构成行为实现。

## 二、关键发现

### P0-1 当前源码重复拼接，基线构建失败，无法生成可交付 `.dlo`

- **结论**：当前工程不是“待验收实现”，而是不可编译的重复拼接骨架。构建在 C++ 编译阶段即失败，尚未进入链接与资源阶段。
- **证据**：`x64\baseline-build.log:17-93` 连续出现 C2086、C2374、C2084、C2011、C2027、C2355 等错误；`DllEntry.cpp:17` 与 `:119` 重复定义 `hInstance`，`:26` 与 `:128` 重复 `DllMain`，`:48` 与 `:150` 重复 `LibNumberClasses`；`CADBundle.cpp:15` 与 `:122` 定义了两套不同的 `CADDimension_CLASS_ID`；`CADBundle.rc:20-31` 与 `:144-155` 重复对话框；`resource.h:7-25` 与 `:43-61` 重复 ID。
- **影响**：验收标准第 1 条直接失败；没有 `.dlo`，用户无法安装、无法启动验证，后续所有 UX 场景均不可执行。
- **建议**：将每个定义收敛为唯一来源，删除重复拼接；把“干净构建成功”作为发布门禁；重新生成并留存新的构建日志。修复时优先解决 Class_ID 两套值并存问题，避免复制粘贴掩盖真实标识。
- **置信度**：高。

### P0-2 唯一用户入口与四个对象类均未实现，只有 `CADDimension` 骨架且公开暴露

- **结论**：当前没有 `CADAnnotatePanel` Utility 面板，也没有 `CADText`、`CADLeader`、`CADTable`；唯一存在的 `CADDimension` 仍是骨架，且 `IsPublic()` 返回 `TRUE`，与需求相反。
- **证据**：`DllEntry.cpp:48-51` 的 `LibNumberClasses()` 返回 1；`:54-61` 的 `LibClassDesc(int i)` 只在 `case 0` 返回 `GetCADDimensionDesc()`；`CADBundle.cpp:19`、`:57` 只有 `CADDimension` 与其 ClassDesc；`:60`、`:167` 的 `IsPublic()` 返回 `TRUE`；`:83-107`、`:190-214` 为 TODO/TODO 占位实现。需求要求 5 个 ClassDesc，见 `CADAnnotatePanel.md:19`、`:140`。
- **影响**：“实用程序”面板不会出现工具集入口，验收 3–9 无法开始；即使补注册，唯一类会以公开项出现在“创建 → 辅助对象”，验收 4 也会失败。用户感知不到“一个工具集面板”，而是“一个未完成插件”。
- **建议**：实现 1 个 `UtilityObj` 面板与 4 个 `HelperObject`；`LibNumberClasses()` 返回 5，索引 0 为面板、1–4 为四个对象；四个对象的 `IsPublic()` 返回 `FALSE`；为每个对象补 `ParamBlockDesc2`、`P_AUTO_UI` 参数界面与 `BeginEditParams`/`EndEditParams`。
- **置信度**：高。

### P0-3 面板资源是 `WS_CHILD` 占位模板，不是已裁决的 Win32 浮动工具集面板

- **结论**：`IDD_PANEL` 是 108×156 DLU、`WS_CHILD | WS_VISIBLE` 的子窗口模板，只有占位文本，没有 4 个分类按钮、说明、状态区、关闭按钮；它不是可用的浮动面板。
- **证据**：`CADBundle.rc:28-31`：`IDD_PANEL DIALOG DISCARDABLE 0, 0, 108, 156`、`STYLE WS_CHILD | WS_VISIBLE`、注释 `//FLOATING_DIALOG`、`FONT 8, "Microsoft Sans Serif"`；`:39` 为 `"TODO: Place panel controls here."`；`resource.h:19-25` 只有 `IDD_PANEL` 与通用控件 ID。对照 SDK：`util.rc:63-65` 的 `IDD_COLORCLIP_PANEL` 为 `WS_CHILD` 的 rollup 面板；`util.rc:75-78` 的 `IDD_COLORCLIP_FLOATER` 为 `DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU` 的独立浮动窗口。
- **影响**：即使补上面板类，该模板也只能作为嵌入式子页，无法呈现用户要求的 4 个分类、关闭行为和浮动窗口；验收 3 无法通过。
- **建议**：新增独立 `IDD_PANEL_FLOATER`，使用 `DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX`（是否加 `WS_THICKFRAME` 由尺寸策略决定），定义 4 个分类按钮、每项一行说明、状态文本和一个关闭按钮；使用 `CreateDialogParam` 创建，`CenterWindow` 定位，单实例激活。参数 UI 另建独立 `IDD_*`，不要复用该模板。
- **置信度**：高。

### P1-1 面板生命周期与交互状态机缺失，打开/关闭/切换/中止/重复点击均未定义

- **结论**：当前没有任何 `UtilityObj` 的 `BeginEditParams`/`EndEditParams` 实现，也没有窗口句柄的单实例管理、关闭处理、创建状态同步或重开清理逻辑。
- **证据**：`CADBundle.cpp` 全文无 `UtilityObj`、`BeginEditParams`、`EndEditParams`、`CreateDialog`、`DestroyWindow`；`DllEntry.cpp:54-61` 只有对象类描述；`utilapi.h:61-107` 明确 `UtilityObj` 生命周期契约；`utilapi.h:52-60` 警告 modeless utility 对话框可能与其他 MAX 窗口冲突；SDK 范例 `polycnt.cpp:252-266` 用 `hDlg` 判空、已存在则 `SetActiveWindow`，`:229-237` 在取消时执行 `EndDialog`、释放 spinner、`iu->CloseUtility()`、置空句柄；`colclip.cpp:395-413` 展示 rollup 的 `AddRollupPage`/`DeleteRollupPage` 成对管理。
- **影响**：可能出现重复面板、已销毁窗口指针悬挂、关闭面板后仍处于创建模式、下一次视口点击意外生成对象；这些都会直接造成用户误操作与数据污染。
- **建议**：实现显式状态机：`Idle`、`Creating(class)`、`Closing`；用单一 `HWND` 与 `IsWindow()` 守卫；`BeginEditParams` 首次创建、再次激活已有窗口；`EndEditParams` 销毁并置空；`WM_CLOSE` 先按状态清理再销毁；`WM_ACTIVATE` 或轻量定时器同步 `IsCreatingObject()`；任何异常路径都要恢复按钮与状态。
- **置信度**：高（代码缺失事实）；具体运行表现未验证，置信度中。

### P1-2 创建模式 API 选择与“关闭后不残留创建模式”策略未落地

- **结论**：需求候选 `Interface::SetActiveCreateMode(Class_ID)` 在 SDK 2027 头文件中不存在；当前代码既未采用实际可用 API，也未记录修订。关闭面板时的创建模式清理没有实现方案。
- **证据**：对 `D:\3dsMaxSDK2027\maxsdk\include` 检索 `SetActiveCreateMode` 无命中；实际 API 为 `maxapi.h:8701-8703` 的 `Interface7::StartCreatingObject(ClassDesc*)`、`IsCreatingObject(const Class_ID&)`、`IsCreatingObject()`；`maxapi.h:5414-5425` 提供 `Interface::StopCreating()`，文档说明其结束当前创建模式并调用 `ClassDesc::EndCreate()`，但不应从对象创建回调内部调用；`startObjectCreation` 仅作为 MAXScript 原语出现在 `primitives.inl:352-353`；需求 §2.4 允许以实际 SDK API 为准并记录修订（`CADAnnotatePanel.md:22-25`）。
- **影响**：按需求字面实现会编译失败；若退化为拼接 MAXScript 字符串，则类名、本地化、错误处理与状态同步都会变差。面板关闭时若不调用 `StopCreating()`，用户下次点击视口可能继续创建，形成“隐藏的创建模式”。
- **建议**：明确采用 `GetCOREInterface7()->StartCreatingObject(desc)` 与 `IsCreatingObject()`；在面板关闭、切换分类、Max 退出、`EndEditParams` 时按需调用 `GetCOREInterface()->StopCreating()`；将修订写入开发文档。`ExecuteMAXScriptScript("startObjectCreation ...")` 仅作为兜底，不作为主路径。
- **置信度**：高（API 是否存在）；`StopCreating()` 在面板关闭路径上的运行语义未验证，置信度中。

### P1-3 创建过程缺少“正在创建什么、下一步点什么、如何取消”的反馈；错误与空状态无设计

- **结论**：当前没有状态栏、视口提示、完成/中止提示，也没有空文本、缺字体、空表格、行列超限、创建失败的处理证据。用户只能靠猜测完成三点/单击交互。
- **证据**：`CADBundle.rc:28-41` 无状态文本、无按钮、无提示控件；源码中未见 `DisplayTempPrompt`、`PushPrompt`、`PopPrompt` 调用；可用反馈 API 见 `maxapi.h:3521-3555`；字体可用 `bezfont.h:243` 的 `FontExists` 与 `:257` 的 `BuildCharacterEx`（字符不可用时返回 FALSE、宽度为零）；表格行列与文本规则见 `CADAnnotatePanel.md:115-133`。
- **影响**：三点式标注与三段式引线最容易让用户在中途迷失；中止与失败静默会制造“点了没反应”的感受，增加重复点击与误创建。
- **建议**：面板内设置固定状态行，并在视口使用提示栈：
  - 进入创建：按钮呈按下态，状态“正在创建：标注”；视口提示“指定第一点”。
  - 第 1 点后：“指定第二点”；第 2 点后：“指定尺寸线位置”；引线同理。
  - 全程提示“Esc 或右键取消”。
  - 完成：“已创建，可在修改面板调整参数”；中止：“已取消创建”；失败：本地化错误提示并恢复空闲。
  - 空文本：不绘制文字但保留有效包围盒；空表格：只绘制网格；缺字体：回退到默认/CJK 字体并一次性提示；行列超限：参数范围 1–50，加载旧数据时钳制并提示；长单元格文本裁剪或省略。
- **置信度**：高（缺失事实）；具体提示文案与运行表现未验证，置信度中。

### P1-4 资源计划不完整且存在 ID 冲突，无法支撑中文面板与四类参数 UI

- **结论**：`.rc`/`resource.h` 只有 5 个字符串、1 个占位对话框和少量通用控件 ID；没有面板标题、4 个分类按钮、4 个对象类名、状态/错误/帮助文案，也没有四个对象参数块的 `IDD_*`/`IDC_*`。`IDC_CLOSEBUTTON` 与 `IDC_DOSTUFF` 同为 1000，是确定冲突。
- **证据**：`resource.h:7-11` 仅有 `IDS_LIBDESCRIPTION` 至 `IDS_SPIN`；`:19` 仅 `IDD_PANEL`；`:21-22` 两个不同名称共用 1000；`:23-25` 仅 `IDC_COLOR`、`IDC_EDIT`、`IDC_SPIN`；`CADBundle.rc:98-104` 仅 `CADDimension` 的 5 条字符串；需求 §3 要求按钮 ID、字符串、对话框模板完整定义（`CADAnnotatePanel.md:54`），§8.9 要求全部 `IDS_*`、`IDC_*`、`IDD_*`（`:145`）。
- **影响**：无法显示中文按钮和类名；无法按 Win32 `P_AUTO_UI` 路线为四个对象提供参数界面，验收 6–9 的“修改参数即时刷新”无法实现；`WM_COMMAND` 无法区分冲突 ID，关闭按钮或其他命令可能触发错误动作。
- **建议**：在实现前冻结资源 ID 分配表：
  - `IDS_PANEL_TITLE`、`IDS_BTN_DIMENSION/TEXT/LEADER/TABLE`、`IDS_DESC_*`；
  - `IDS_STATUS_READY/CREATING/ABORTED/DONE/ERROR_*`；
  - 四个类各自的 `IDS_CLASS_*`，不要共用 `IDS_CLASS_NAME`；
  - `IDD_PANEL_FLOATER` 与四个参数 `IDD_*`；
  - `IDC_PANEL_BTN_*`、`IDC_PANEL_STATUS`、`IDC_PANEL_CLOSE` 及参数控件 ID。
  同时修正 1000 冲突，更新 `_APS_NEXT_*`，补齐版本信息与图标资源。
- **置信度**：高。

### P1-5 中文资源、字体回退、高 DPI 与窗口尺寸均未达到发布要求

- **结论**：当前 `.rc` 仅英文、代码页 1252、8pt `Microsoft Sans Serif`、固定 108×156 DLU；没有中文字符串、CJK 字体回退、DPI 缩放或最小尺寸/换行策略。
- **证据**：`CADBundle.rc:20-21` 为 `LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US` 与 `#pragma code_page(1252)`；`:28-31` 固定尺寸与 8pt 字体；`:98-104` 英文文案；SDK 提供 `winutil.h:46` 的 `MaxSDK::GetUIScaleFactor`、`:310` 的 `CenterWindow`，当前源码未使用；需求要求面板有中文分类“标注 / 文字 / 引线 / 表格”（`CADAnnotatePanel.md:161`）。
- **影响**：中文标签会缺失或依赖系统回退，8pt 字体在高 DPI 下过小；108×156 放不下 4 个按钮加说明，中文更易截断；150%/200% 缩放下窗口可能超出屏幕或控件错位。
- **建议**：确定中文资源策略（中文资源块或统一 UTF-8 `#pragma code_page(65001)`，并保留英文回退）；为面板选择 CJK 可用字体或让 3ds Max 提供 UI 字体；按 DLU 重新设计尺寸并设最小尺寸与可调整布局；使用 `GetUIScaleFactor`/`UIScaled`，处理 `WM_DPICHANGED`；在 100%、125%、150%、200% 与多显示器间实测。`.rc` 中文编译行为需单独验证，不能只靠静态推断。
- **置信度**：高（当前状态）；生产可用性需运行验证，置信度中。

### P1-6 交付形态与文档不足，当前不能作为可发布包交付

- **结论**：只有一个失败的构建工程；没有 README、安装/卸载说明、版本号、CHANGELOG、已知问题、验收清单；`.slnx` 为空；所有配置输出到同一 Release 目录；`.def` 缺少需求要求的 `CanAutoDefer`；旧 `CADDimension.dlo` 冲突未形成操作文档。
- **证据**：`CADBundle.slnx:1` 为 `<Solution />`；`CADBundle.vcxproj:32` 的 `OutDir` 对 Debug/Hybrid/Release 均指向 `x64\Release\Plugins\`；`CADBundle.def:3-8` 只有 6 个导出，无 `CanAutoDefer`；需求 §8.4 要求 `CanAutoDefer`（`CADAnnotatePanel.md:140`）；§11.1.4 要求处理旧 `CADDimension.dlo`（`:180`）；§11.2 给出旧文件大小与路径（`:191`）；项目根目录清单无 README/CHANGELOG/安装说明；`x64\` 仅构建日志，无 `.dlo`。
- **影响**：用户无法安全安装、验证和回滚；开发者可能用 Debug 产物覆盖 Release；旧插件可能导致类冲突或加载异常；出现问题时无版本信息可追溯。
- **建议**：交付包至少包含：
  - `CADBundle.dlo`（Release x64，文件名与版本固定）；
  - `README.md`：支持 3ds Max 2027、安装/卸载步骤、首次验证、旧插件冲突处理、回滚方法；
  - `CHANGELOG.md`：版本、日期、Class_ID、UI/交互变更；
  - `BUILD.md`：SDK 路径、工具集、构建命令、输出目录；
  - 验收清单与测试记录；
  - 文件 SHA-256 与构建时间。
  工程侧按配置区分 `OutDir`，补 `CanAutoDefer`，加入 VERSIONINFO；旧 `.dlo` 按“备份/移出—验证—恢复”流程执行，不得静默覆盖。
- **置信度**：高。

### P2-1 `GetString` 使用静态 256 缓冲区，缺失资源返回 `NULL`，无回退

- **结论**：字符串加载器不是线程安全的，缓冲区固定 256 字符，资源缺失时返回 `NULL`；本地化类名/标签缺失时可能显示空白或被截断。
- **证据**：`DllEntry.cpp:91-101`（单例 `static TCHAR buf[256]`、`LoadString` 失败返回 `NULL`）；SDK 提供线程安全的 `MaxSDK::GetResourceStringAsMSTR`（`dllutilities.h:38-49`）。
- **影响**：中文或英文资源缺项时，类名、分类名、状态文本可能为空；多个窗口同时取字符串时内容可能被覆盖。是否会导致更严重问题需运行验证，不在此断言。
- **建议**：改用 `MaxSDK::GetResourceStringAsMSTR` 并保存返回的 `MSTR` 生命周期；资源缺失时回退到 `NonLocalizedClassName` 或明确默认文案；不要返回裸 `NULL` 给 UI 层。
- **置信度**：高。

### P2-2 工程结构可维护性差，新增四个类会放大复制粘贴风险

- **结论**：重复代码贯穿 `.cpp`、`.rc`、`resource.h`、`.def`；解决方案为空；项目过滤器只登记现有两个 `.cpp`；全部配置共用输出目录。当前两套 Class_ID 已证明重复拼接会造成实质分歧。
- **证据**：`x64\baseline-build.log:17-93`；`CADBundle.cpp:15` 与 `:122`；`CADBundle.def:3-8` 与 `:13-18`；`resource.h:7-25` 与 `:43-61`；`CADBundle.slnx:1`；`CADBundle.vcxproj:32`；`CADBundle.vcxproj.filters` 仅含现有两个 `.cpp`。
- **影响**：修一处漏一处；四个对象和面板加入后文件迅速膨胀，违背单文件 500 行以内的维护要求；Debug 构建可能覆盖 Release 交付物。
- **建议**：按职责拆分 `CADAnnotatePanel`、`CADDimension`、`CADText`、`CADLeader`、`CADTable` 及资源 ID 头文件；每个定义唯一化；按配置设置 `OutDir`；补齐 `.filters`；提供可用的 `.sln`/`.slnx` 或明确的构建入口。
- **置信度**：高。

### P2-3 验收第 3 条的字面名称与本地化显示名可能冲突

- **结论**：验收要求“实用程序面板中出现 `CADAnnotatePanel`”，但 `ClassName()` 返回本地化字符串；若实施中文显示名，UI 不会出现该英文字面量，容易误判失败。
- **证据**：`CADAnnotatePanel.md:161`；`CADBundle.cpp:62`、`:169` 的 `ClassName()` 使用 `GetString(IDS_CLASS_NAME)`；`:63`、`:170` 的 `NonLocalizedClassName()` 为 `CADDimension`。
- **影响**：验收人员按英文名查找可能判定失败，或为了通过验收而放弃中文名，损害本地化目标。
- **建议**：验收同时记录“界面显示名”和“脚本名 `NonLocalizedClassName`”；在需求中明确面板与四个对象的显示名、脚本名对应表。
- **置信度**：高。

### P3-1 分类按钮与插件描述的可理解性、可访问性仍可提升

- **结论**：四个分类目前只有名词，没有动作提示或步骤提示；插件描述仍是 `"CAD style dimension"`，与“CAD 注释工具集”不一致；未见工具提示、Tab 顺序、Esc 键行为的设计。
- **证据**：`CADBundle.rc:100` 的 `IDS_LIBDESCRIPTION "CAD style dimension"`；需求允许每个分类下方显示简短说明（`CADAnnotatePanel.md:49-50`）；当前 `.rc` 无工具提示或帮助文本资源。
- **影响**：首次使用者不知道“标注”点击后进入三点创建，认知负荷高；键盘用户操作路径不明确；插件管理器中的描述误导用户。
- **建议**：按钮文案改为“标注（三点）”“文字（单击放置）”“引线（三点）”“表格（单击插入）”；每项下一行说明用途；补工具提示、合理 Tab 顺序、Esc/右键取消提示；插件描述改为“CAD 注释工具集”。
- **置信度**：中。

## 三、关键问题直接回答

### 1. 4 按钮浮动面板是否清晰？打开、关闭、切换、创建、中止、重复点击应如何表现？是否可能残留创建模式？

4 个分类在认知负荷上是合理的（4 个选项处于工作记忆舒适区），但“只有名词、没有动作与步骤说明”会增加首次使用成本。当前代码没有任何实现，因此以下均为建议目标行为，不是已验证行为：

| 事件 | 建议行为 | 关键点 |
|---|---|---|
| 首次打开 | 创建单实例浮动面板，居中或靠近主窗口；4 按钮可用；状态“就绪” | 不自动进入创建模式 |
| 再次打开 | 激活已有窗口，不创建第二个 | `IsWindow(hDlg)` + `SetActiveWindow` |
| 空闲关闭 | 销毁窗口、置空句柄、恢复空闲状态 | 不在后台残留隐藏窗口 |
| 创建中关闭 | 先 `StopCreating()`，确认退出创建模式后再销毁；下次打开为空闲 | 若运行验证发现部分节点残留，则改为禁用关闭并提示先取消 |
| 点击同一按钮 | 已在该类创建中则忽略并提示，不重启、不重复建对象 | 防重复点击 |
| 点击另一分类 | 先结束当前创建，再启动新分类；若无法可靠结束，则禁用其他按钮并提示 | 保证同时只有一个创建模式 |
| 完成 | 结束创建，按钮恢复，状态“已创建，可在修改面板调整” | 不擅自打开修改面板也可，但必须告知入口 |
| 中止 | `CREATE_ABORT`/`StopCreating` 后恢复空闲，状态“已取消” | 不残留半成品节点 |
| 创建失败/类未注册 | 恢复空闲并显示本地化错误，不挂起 | 不能静默失败 |
| 切换命令面板/选择集 | 面板保持可用；用 `IsCreatingObject()` 重新同步状态 | modeless 面板不得假设自己拥有焦点 |
| Max 退出 | `EndEditParams`/关闭路径释放窗口，不崩溃、不卡死 | 必须纳入验收 |

### 2. 用户如何知道正在创建什么、下一步点什么、如何取消？错误、空文本、缺字体、空表格、行列溢出如何反馈？

- 创建前：按钮下方有用途说明，例如“标注：三点确定起点、终点、尺寸线位置”。
- 创建中：按钮呈按下态；面板状态行显示“正在创建：标注”；视口提示逐点变化：“指定第一点” → “指定第二点” → “指定尺寸线位置”；全程显示“Esc 或右键取消”。
- 完成/中止：状态行分别显示“已创建，可在修改面板调整参数”“已取消创建”，按钮恢复。
- 空文本：不绘制文字，但对象仍有有效包围盒，可选中、可编辑；不弹错误。
- 缺字体：`FontExists()` 失败时回退到默认或 CJK 字体，状态行一次性提示“字体不可用，已回退”，不阻塞创建。
- 字形缺失：`BuildCharacterEx()` 失败时跳过该字形或替换为占位符，不能崩溃或产生无效宽度。
- 空表格：只绘制网格；单元格文本为空时跳过绘制。
- 行列溢出：参数范围 1–50；UI 层限制输入；加载旧数据超出范围时钳制并提示；长文本裁剪或省略，避免撑破单元格。
- 创建失败：使用本地化错误文本，恢复空闲；不要用连续模态弹窗打断用户。
- 反馈通道优先级：面板状态行 + 视口临时提示为主；只有“类未注册/无法进入创建模式”等硬失败才使用消息框。

### 3. 中文资源、字体回退、高 DPI、窗口尺寸与 `.rc`/`resource.h` 资源计划是否达到生产可用？

未达到。当前是英文 1252、8pt `Microsoft Sans Serif`、108×156 固定尺寸，只有 5 条字符串和 1 个占位对话框；没有中文按钮/类名/状态/错误资源，没有四类参数 UI 的 `IDD`/`IDC`，并且存在 1000 号 ID 冲突。资源计划必须先冻结，再写代码。

资源计划至少应覆盖：

- 面板标题、4 个分类按钮、每项说明、状态行、关闭按钮、工具提示；
- 四个对象类的本地化类名（分别独立 ID，不共用 `IDS_CLASS_NAME`）；
- 四套参数 `IDD_*` 与对应 `IDC_*`，以及参数标签、单位、错误文案；
- 创建步骤、完成、中止、字体回退、表格超限等状态文案；
- 版本信息、图标（如交付要求包含）；
- 中文与英文的回退关系，以及 `_APS_NEXT_*` 的更新。

窗口尺寸建议以 DLU 重新设计，设置最小尺寸，并验证 100%、125%、150%、200% DPI 与多显示器移动；`GetUIScaleFactor` 已在 `winutil.h:46` 提供，但当前未使用。中文 `.rc` 的代码页/编译表现未验证，必须在 VS 2026 环境实测。

### 4. 交付形态是否足以发布？还缺哪些文档与验收场景？

不足以发布。单个 `.dlo` 的产品形态本身符合需求，但当前没有可用的 `.dlo`，也没有版本、变更记录、安装/卸载说明、已知问题、回滚方案与验收清单；`.slnx` 为空，所有配置共用 Release 输出目录，`.def` 缺少 `CanAutoDefer`，旧 `CADDimension.dlo` 冲突没有操作化流程。

发布前至少需要：

- `README.md`：支持版本、安装/卸载、首次启动验证、旧插件冲突处理、回滚；
- `CHANGELOG.md`：版本、日期、Class_ID、UI/交互变化；
- `BUILD.md`：SDK 路径、工具集、构建命令、输出与校验；
- 验收清单与测试记录；
- Release 包校验值；
- 明确的旧 `.dlo` 备份/移出/恢复步骤；
- 版本信息与插件描述。

验收场景见下一节第 6 项，当前全部未运行，均为“未验证”。

### 5. 从用户角度还应补充哪些验收场景？可执行的检查步骤是什么？

需要补充“首次加载、重复开关、创建中关闭、切换分类、中止、重复点击、字体切换/缺失、DPI、保存重开、空文本、表格边界、退出 Max”等场景。可直接执行的步骤已列在“五、下一步建议”第 6 项。

## 四、未验证项

以下均未运行，不能据此宣称通过：

- 未验证：当前源码能否在清理重复拼接后编译、链接并生成单个 `.dlo`。
- 未验证：插件放入 `plugins` 后 3ds Max 2027 能否无 ABI/缺失 DLL 错误启动。
- 未验证：“实用程序”面板是否出现工具集条目，以及 4 个分类按钮是否可见、可点击。
- 未验证：面板首次打开、重复打开、关闭、创建中关闭、重开后的窗口数量与状态。
- 未验证：`StartCreatingObject`/`IsCreatingObject` 在面板按钮场景下的实际行为，包括是否切换命令面板、焦点与鼠标状态。
- 未验证：`StopCreating()` 从面板关闭路径调用后，是否会清理未完成的对象/节点；是否存在部分创建残留。
- 未验证：三点标注、单点文字、三点引线、单点表格的实际创建与拖动预览。
- 未验证：右键/Esc 中止后，按钮状态、视口提示、创建模式与选择状态是否全部恢复。
- 未验证：重复点击同一按钮、创建中点击另一分类的实际结果与是否存在两个创建模式。
- 未验证：空文本、纯空白文本、超长中文文本的绘制、包围盒、选择与参数保留。
- 未验证：缺字体、字体名非法、中文字体、字形缺失时的回退与提示。
- 未验证：空表格、1×1、50×50、超限行列、长单元格文本的绘制、性能与保存恢复。
- 未验证：中文 `.rc` 代码页/字体在 VS 2026 资源编译器中的实际表现。
- 未验证：100%、125%、150%、200% DPI、多显示器、跨屏移动、屏幕外恢复、最小尺寸与布局换行。
- 未验证：保存 `.max` 后重开，四个对象的关键点、参数、`cellTexts` 是否完整恢复。
- 未验证：撤销/重做、克隆、删除后重建是否产生重复节点或悬挂引用。
- 未验证：旧 `CADDimension.dlo` 与新 `CADBundle.dlo` 共存时是否出现类冲突；当前只有需求中的旧文件事实，没有运行结果。
- 未验证：3ds Max 在面板打开或创建中退出时是否崩溃、卡死或留下残留窗口。
- 未验证：modeless 面板在切换 Utility/Modify/Create、选择对象、打开其他对话框时是否保持响应且不干扰主窗口。
- 未审阅：`maxapi.h` 全文、`compass.cpp` 与 `MaxFindApp.cpp` 的未读部分、其余 SDK 样例与完整 SDK 帮助文档。
- 未审阅：其他专家报告、项目总监意见、总裁技术结论；按任务约束未读取。
- 未验证：本报告未生成任何截图、日志、覆盖率数字或用户测试结果；文中所有行为判断均来自静态阅读或 SDK 文档，不是运行结论。

## 五、可能与其他专家冲突的判断

以下只陈述本报告结论，未引用任何他人报告。

- **关于创建 API**：需求候选 `Interface::SetActiveCreateMode` 在 SDK 2027 中不存在。若其他实现路线坚持按字面调用，会直接编译失败。本报告结论是改用 `Interface7::StartCreatingObject` + `IsCreatingObject`，并记录需求修订；MAXScript `startObjectCreation` 只作为兜底。
- **关于面板形态**：`AddRollupPage` 的停靠 rollup 实现更省事，但不满足用户已裁决的 Win32 浮动面板，也不便于表达“关闭面板后重开”的独立窗口状态。本报告结论是坚持 `WS_POPUP` 浮动对话框，而不是把 `IDD_PANEL` 简单改成 rollup。
- **关于“关闭时绝不残留创建模式”**：`StopCreating()` 提供了可行的清理入口，但静态文档不能证明未完成对象/节点一定会被完全清理。本报告结论是把“创建中关闭”列为必测场景，并准备“无法可靠取消时禁用关闭并提示”的降级方案；不把“绝不残留”当作已证明事实。
- **关于验收名称**：验收第 3 条要求看到 `CADAnnotatePanel`，若实现中文显示名，界面不会出现该英文字面量。本报告结论是验收同时检查显示名与 `NonLocalizedClassName`/脚本名，避免为了字面验收牺牲本地化。
- **关于严重级别**：构建失败与面板缺失属于核心不可用，定为 P0；如果只按“源码结构”评估而忽略验收路径，可能低估为普通缺陷。本报告维持 P0。

## 六、下一步建议

按发布阻断优先排序，建议项目组评估后执行：

1. **先恢复可构建、可加载的最小闭环**：清理重复拼接，统一 Class_ID，确保生成单个 `.dlo`；先只注册 Utility 面板，验证“启动—Utility 列表—打开/关闭面板”这一条路径，再并行实现四个对象。
2. **实现面板状态机与单实例管理**：按第“三、1”节的状态表实现；先做“标注”一条完整垂直切片：打开面板 → 点击标注 → 状态/视口提示 → 三点创建 → 完成/中止 → 关闭/重开。该切片通过后，再复制到文字、引线、表格。
3. **先冻结资源 ID 与文案表**：按 P1-4 的建议建立 ID 分配表，确定中文/英文文案、四套参数 `IDD`/`IDC`、状态与错误文本；修正 1000 冲突后再动 UI 代码，避免边写边改 ID。
4. **补创建反馈与边界状态**：状态行、视口提示栈、Esc/右键提示、完成/中止/失败反馈；空文本、缺字体、空表格、行列超限、长文本裁剪；字体回退与一次性提示。
5. **本地化与 DPI 硬化**：确定中文资源策略与 CJK 字体；按 DLU 重设面板尺寸与最小尺寸；处理 `GetUIScaleFactor` 与 `WM_DPICHANGED`；在 100%/125%/150%/200% 与多显示器下实测并记录截图。
6. **建立可执行验收场景（建议纳入发布门禁）**：
   1. **首次加载**：关闭 Max，备份旧 `CADDimension.dlo`，仅放新 `CADBundle.dlo` 到 plugins，启动。期望：无加载错误；Utility 中出现条目；打开后看到 4 个分类及说明；关闭后无残留窗口。
   2. **重复开关**：同一会话打开/关闭面板 10 次。期望：始终只有一个面板；无崩溃、无卡死、无重复窗口。
   3. **四类创建**：分别点击标注、文字、引线、表格，完成交互。期望：生成对应对象；状态行与视口提示逐步正确；完成后按钮恢复。
   4. **创建中关闭**：点击标注，点下第一点后关闭面板。期望：不再处于创建模式；下一次视口点击不生成对象；重开面板为空闲；若无法保证，则关闭按钮应禁用并提示先取消。
   5. **切换分类与重复点击**：创建中点击另一分类；连续双击同一分类。期望：始终只有一个创建模式；不重复建对象；行为与文档一致。
   6. **中止**：创建中按 Esc 或右键。期望：不生成对象；状态行显示“已取消”；下一次点击恢复正常选择/创建。
   7. **空文本与长文本**：`CADText.textString` 设为空、纯空格、超长中文。期望：不崩溃；空文本对象仍可选中；长文本裁剪或省略，不撑破视口与面板。
   8. **字体切换与缺失**：切换字体、输入不存在的字体名、使用中文字体。期望：回退可见且有提示；不崩溃；字号真实生效。
   9. **表格边界**：空表、1×1、50×50、输入 51 行/列、长单元格文本。期望：范围被限制或钳制；网格正确；文本可恢复；50×50 不出现明显卡顿（性能阈值由项目组定）。
   10. **DPI 与多显示器**：在 100%/125%/150%/200% 下打开、创建、关闭；拖到不同 DPI 显示器。期望：按钮、状态、关闭按钮完整可见；文本不截断；窗口可恢复。
   11. **保存与重开**：创建四类对象并设置参数、表格文本，保存 `.max` 后重开。期望：关键点、参数、`cellTexts`、显示状态完整恢复；面板重开为空闲。
   12. **撤销/重做与删除重建**：创建后撤销、重做、删除、重建。期望：无重复节点、无悬挂引用、无崩溃。
   13. **旧插件隔离与退出**：旧 `.dlo` 存在与移出两种状态各启动一次；面板打开时退出 Max；创建中退出。期望：无类冲突、无崩溃、无卡死；退出后可正常重启。
   14. **面板协同**：面板打开时切换 Utility/Modify/Create、选择对象、打开其他对话框。期望：面板保持响应；不阻塞主窗口；创建状态不丢失或错乱。
7. **整理发布交付物**：按 P1-6 的清单补齐 README、CHANGELOG、BUILD、安装/卸载、已知问题、回滚与验收记录；按配置区分输出目录；补 `CanAutoDefer` 与 VERSIONINFO；生成 Release 包与 SHA-256；按需求执行旧 `.dlo` 备份/移出/恢复流程。
8. **保持可维护性**：拆分面板、四个对象、创建回调、显示/命中测试与资源头；单文件控制在 500 行以内；删除重复拼接；补齐 `.filters`；提供明确构建入口，避免下一次修复再次引入两套 Class_ID 或两套资源。
