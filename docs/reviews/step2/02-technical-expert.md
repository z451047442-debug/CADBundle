# 【3ds Max SDK 技术专家｜系统分解 + 风险驱动设计 + 接口契约思维】

## 一、审阅范围

### 1.1 已审阅内容
- 项目指定文件（全部读取）：`docs/requirements/CADAnnotatePanel.md`（202 行）、`CADBundle.cpp`（215 行）、`DllEntry.cpp`（205 行）、`CADBundle.h`（59 行）、`CADBundle.rc`（248 行）、`resource.h`（73 行）、`CADBundle.vcxproj`（66 行）、`CADBundle.def`（21 行）、`CADBundle.slnx`（1 行）、`3dsmaxsdk_preinclude.h`（43 行）、`x64/baseline-build.log`（94 行）。
- SDK（定向抽样）：`GetCOREInterface.h`、`maxapi.h`、`plugapi.h`、`utilapi.h`、`hold.h`、`plugin.h`、`iparamb2.h`、`paramtype.h`、`object.h`、`GraphicsWindow.h`、`bezfont.h`；样例 `tapehelp.cpp`、`prothelp.cpp`、`pthelp.cpp`、`boxobj.cpp`、`text.cpp`、`arc.cpp`、`Hub.cpp`、`CATHierarchyRoot.cpp`、`EggShapeHelpers.cpp`、`util.cpp`、`utiltest.cpp`、`avg_dlx.cpp`、`primitives.inl`。
- 覆盖率估计：项目指定文件 11/11（约 100%）；SDK 全库未审阅，仅按 5 个关键问题定向抽样约 24 个文件，不能代表 SDK 全面覆盖。

### 1.2 未审阅内容
- SDK 其余头文件、样例与工具链；3ds Max 安装目录二进制；旧 `CADDimension.dlo` 内部实现；其他专家报告、项目总监意见、总裁技术结论（按约束明确不读取）。未启动 3ds Max 做运行时验证。

### 1.3 过程与注入说明
- 只读审阅：未修改、删除任何项目文件；未运行构建、未安装插件、未复制文件到 3ds Max 目录、未联网、未执行 git 写操作。
- 需披露：统计搜索结果时曾在系统临时目录生成两个临时文本文件（`/tmp/sacm.txt`、`/tmp/slb.txt`），不属于项目目录，未改动任何项目内容。
- 提示注入：未发现试图改变审阅行为的恶意注入；规格中的 `TODO`、`#pragma message` 与“不得输出环境配置”等文字均按待审数据对待，未作为指令执行。

## 二、关键发现

| 编号 | 级别 | 结论与证据 | 影响 | 建议 | 置信度 |
|---|---|---|---|---|---|
| P0-1 | P0 | 源码整段重复拼接：`CADBundle.cpp` 第 108–215 行为 1–107 的重复；`DllEntry.cpp` 第 103–205 行为 1–102 的重复；`CADBundle.h`、`CADBundle.rc`、`resource.h`、`CADBundle.def` 同样重复。`x64/baseline-build.log:17-93` 给出 C2086、C2374、C2084、C2011、C2027、C2355、C4005 等错误 | 基线不可编译，验收标准 1 直接失败，任何后续验证都无法进行 | 先按“每文件一份实现”去重；类声明迁移到头文件并加 `#pragma once`；重新编译取干净日志 | 高 |
| P0-2 | P0 | 注册架构未实现 5 个 ClassDesc：`DllEntry.cpp:48-51` `LibNumberClasses()` 返回 1；`:54-61` `LibClassDesc` 仅 case 0 返回 `GetCADDimensionDesc()`；`CADBundle.cpp:60` `IsPublic() { return TRUE; }`；不存在 `CADAnnotatePanel` 的 `UtilityObj`/ClassDesc | 验收标准 3（面板出现）、4（辅助对象不出现四个独立项）、5（面板按钮创建）全部失败；“创建→辅助对象”会出现 `CADDimension`，与需求相反 | 实现 5 个 ClassDesc：索引 0 为 `CADAnnotatePanel`（`UTILITY_CLASS_ID`，`IsPublic=TRUE`），1–4 为四个对象类（`HelperObject`，`IsPublic=FALSE`）；`LibNumberClasses` 返回 5 | 高 |
| P0-3 | P0 | 对象类为骨架：无 `ParamBlockDesc2`；`GetCreateMouseCallBack` 声明于 `CADBundle.cpp:46/153`、定义于 `96-100/203-207` 返回 `nullptr`；无 `Display`、`HitTest`、`GetLocalBoundBox`、`GetWorldBoundBox`、`BeginEditParams`、`EndEditParams`；`Clone` 定义于 `83-87/190-194` 返回 `nullptr`；`Save`/`Load` 于 `26-27/133-134` 仅返回 `IO_OK` | 对象不可参数化、不可创建、不可绘制、不可选中、不可持久化；验收标准 5–10 全部失败 | 按本报告 Q2–Q4 的骨架逐类实现；共享逻辑可放公共基类，但四个类的声明、ClassDesc、资源必须齐全 | 高 |
| P0-4 | P0 | 同一类两个 Class_ID 宏：`CADBundle.cpp:15` `Class_ID(0x34883ae1, 0x5594bda4)` 与 `:122` `Class_ID(0xa517250d, 0x8cd4f7c3)`；基线与日志 `x64/baseline-build.log:45-47` 报 C4005 宏重定义 | 去重时必须二选一；若与旧 `CADDimension.dlo` 的 ID 不一致，旧场景类解析、卸载替换和反序列化均可能出现冲突或丢失 | 为 5 个类固定唯一 Class_ID（含四个对象类与 Utility），写成常量并在开发文档登记；不得在实现过程中再改 | 高 |
| P1-1 | P1 | 规格候选 API `Interface::SetActiveCreateMode(Class_ID)` 在 SDK 中不存在（`D:\3dsMaxSDK2027\maxsdk` 全目录检索 0 匹配）。SDK 可用路径为 `GetCOREInterface7()->StartCreatingObject(ClassDesc*)`（`GetCOREInterface.h:30`、`maxapi.h:8701`），结束用 `Interface::StopCreating()`（`maxapi.h:5414-5425`）；另有 `CreateInstance` + `CreateObjectNode`（`maxapi.h:5227-5241`、`:8706`） | 按规格原文实现会编译失败；创建模式若未正确收尾，可能残留 CommandMode，或触发 `BeginCreate/EndCreate` 相关崩溃警告（`plugapi.h:1080-1106`） | 采用 `StartCreatingObject`；在 `EndEditParams`、面板关闭、`MOUSE_ABORT` 三条路径统一清理；创建过程中不得绕过 CommandMode 栈 | 高 |
| P1-2 | P1 | 屏幕恒定尺寸公式不完整。规格 §2.7、§4、§5、§6、§7、§8.5、§10.11 均写作“参数值 × `NonScalingObjectSize()`”。SDK 文档明确其含义为像素换算需再乘 `GetVPWorldWidth(point)/360.0f`（`maxapi.h:1144`、`:1152`、`:1163-1187`，注释中的示例即 `NonScalingObjectSize() * GetVPWorldWidth(tm.GetTrans())/(float)360.0`） | 仅乘 `NonScalingObjectSize()` 时尺寸随缩放变化，验收“视口缩放时箭头、文字、延伸线屏幕尺寸恒定”失败 | 统一封装 `float S(ViewExp* vpt, const Matrix3& tm) { return NonScalingObjectSize() * vpt->GetVPWorldWidth(tm.GetTrans()) / 360.0f; }`，所有屏幕恒定元素乘 `S`；同步修订规格文字 | 高 |
| P1-3 | P1 | 参数块未接为引用：`CADBundle.cpp:37/144` `NumSubs() { return 1; }`，但无 `NumRefs`、`GetReference`、`SetReference`、`SubAnim`、`SubAnimName`；`P_AUTO_CONSTRUCT` 要求 pblock 位于指定引用索引（`iparamb2.h:3267-3271`）；构造函数也未调用 `MakeAutoParamBlocks(this)`（`iparamb2.h:2661`） | pblock 未加入引用链时，撤销/重做、引用消息、场景保存、参数 UI 与析构释放均不可靠；`NumSubs` 与无 SubAnim 不一致可能造成轨道视图/引用树异常 | 采用样例 `pthelp.cpp:102-123`、`:241-249` 的模式：构造函数 `MakeAutoParamBlocks(this)`；`NumRefs>=1`；ref 0 为 pblock；`GetReference/SetReference` 仅处理 ref 0；实现 `SubAnim/SubAnimName`；`NotifyRefChanged` 按 `iparamb2.h` 常规处理 `REFMSG_CHANGE` 并失效缓存/重绘 | 高 |
| P1-4 | P1 | `Clone` 返回 `nullptr`；`Save/Load` 无分块。若复制对象、保存后重开均会丢失几何关键点与表格文本。SDK 样例 `pthelp.cpp:622-630` 使用 `ReplaceReference(0, remap.CloneRef(pblock2))` + `BaseClone`；`pthelp.cpp:673-712`、`boxobj.cpp:442-487` 给出自定义块 + `ParamBlock2PLCB` 的加载模式 | 验收标准 10（保存重开完整恢复）失败；复制/粘贴对象可能为空或崩溃 | 实现 `Clone`：`BaseClone`、`remap.CloneRef(pblock)`、复制 P0/P1/P2、`cellTexts`、缓存与字体名等；`Save` 写版本化自定义块（几何点、表格文本），`Load` 读块并校验版本；pblock 本身由 `P_AUTO_CONSTRUCT` 自动持久化 | 高 |
| P1-5 | P1 | 资源不足：`CADBundle.rc` 仅有一个 `IDD_PANEL`（第 28 行，且在 152 行重复），无 Utility 面板对话框、无 4 个对象类的 auto UI 所需控件/字符串；`resource.h` 重复定义且 `IDC_CLOSEBUTTON/IDC_DOSTUFF=1000`、`IDC_EDIT=1490`、`IDC_SPIN=1496` 与实际使用不一致；缺少 `IDS_*`/`IDC_*`/`IDD_*` 新 ID | 无法构建完整 UI；`P_AUTO_UI` 的 p_ui 资源 ID 缺失会在加载时断言或显示空面板 | 统一分配 ID 区间（对话框 `IDD_*`、控件 `IDC_*`、字符串 `IDS_*`），在 `resource.h` 单份定义，在 `.rc` 单份实现；每个对象类一个对话框模板，Utility 面板一个 Win32 对话框模板 | 高 |
| P1-6 | P1 | BezFont 生命周期与缓存未实现。SDK `text.cpp:918-934` 在析构中 `CloseFont` 并清空字符缓存；`ChangeFont`/`FlushCharCache` 见 `text.cpp:1554-1574`、`:1587-1592`；BezFont API 见 `bezfont.h:244-246`（`OpenFont/CloseFont`）、`:258`（`BuildCharacterEx`）、`:265`（`theBezFontManager`） | 字体句柄泄漏或悬空；改动字体/字号后字形不刷新；卸载插件时释放管理器句柄可能导致崩溃 | 每对象持有自己的 `BezFont*`（或字体名+句柄缓存）；字体/字号/文本变更时 `FlushCharCache` 并重建 `BezierShape` 缓存；对象析构/`EndEditParams` 时 `CloseFont`；全局字体管理器句柄不得在 `LibShutdown` 中强制关闭仍被存活对象引用的字体 | 高 |
| P1-7 | P1 | 字符串表 UI 与行列重排无自动机制。`TYPE_STRING_TAB` 见 `paramtype.h:153`；`P_VARIABLE_SIZE` 见 `iparamb2.h:74`、`:3373-3374`；`TYPE_STRINGLISTBOX` 见 `paramtype.h:596-604`，文档称“4 个对话框项 ID”，实际列举 listbox+Add+Replace+Delete+CustEdit 共 5 个，存在文档不一致；SDK 样例中未找到 `TYPE_STRINGLISTBOX` 用法。规格要求修改 rows/cols 后 `cellTexts` 按行优先自动新增/截断并重绘，这不会由参数块自动完成 | 单元格文本编辑可能无法通过 auto UI 完整实现；行列变化后表格文本错位；持久化是自动的，但重排逻辑必须手写 | 用 `TYPE_STRING_TAB` + `P_VARIABLE_SIZE` 保存行优先数据；用 `ParamMap2UserDlgProc`（或自定义 Win32 子对话框）承载逐格编辑与重排；在 `PBAccessor`/`NotifyRefChanged` 中处理 rows/cols 变化：按新尺寸复制、空串补齐、截断多余；显式 `Save/Load` 兜底 | 中 |
| P1-8 | P1 | 闭合三角形与 `DrawLineProc` 冲突：`object.h:5877-5889` 的 `DrawLineProc::proc` 硬编码 `gw->polyline(n, p, nullptr, nullptr, 0, nullptr)`，`closed=0`；`GraphicsWindow::polyline` 本身支持 `closed` 参数（`GraphicsWindow.h:1199`） | 箭头无法用规格要求的“所有连续线段统一使用 `DrawLineProc`”画出闭合轮廓，必须破例 | 在箭头处直接调用 `vpt->getGW()->polyline(3, pts, nullptr, nullptr, 1, nullptr)`；或派生 `DrawLineProc` 覆盖 `proc` 传 `closed=1`。建议在规格中允许“闭合三角形”作为 `DrawLineProc` 的显式例外 | 高 |
| P1-9 | P1 | `CanAutoDefer` 缺失：`DllEntry.cpp` 无该导出，`CADBundle.def` 无该符号；SDK `plugin.h:58-63` 说明其可选、返回 FALSE/0 可阻止延迟加载，`util.cpp:109` 样例返回 0 | 规格 §8.4 明确要求该导出；缺失时延迟加载行为由 Max 决定，插件的 `LibInitialize` 顺序存在不确定性 | 增加 `CanAutoDefer()`（是否需要延迟加载取决于类注册是否依赖 `LibInitialize`；按规格明确实现并返回值），并在 `.def` 中导出（若采用显式导出表） | 中 |
| P1-10 | P1 | Utility 面板与创建模式生命周期未实现：无 `CADAnnotatePanel` 类；`EndEditParams` 不存在；关闭面板时无 `StopCreating`/CommandMode 清理；创建回调缺失 | 面板无法打开或打开后无法创建；残留创建模式会导致后续视图操作异常；`BeginCreate/EndCreate` 使用不当可崩溃（`plugapi.h:1080-1106` 明确警告） | 用 `UtilityObj::BeginEditParams(Interface*, IUtil*)`（`utilapi.h:70`）创建 Win32 浮动对话框，`EndEditParams`（`:77`）销毁并清理；点击分类按钮调用 `GetCOREInterface7()->StartCreatingObject(desc)`；面板关闭时若处于创建模式则 `StopCreating()`；不要在无 CommandMode 情况下直接 `BeginCreate/EndCreate` | 高 |
| P2-1 | P2 | `TYPE_RGBA` 仅 RGB，无 alpha：`paramtype.h:34`。规格 §4、§5、§6、§7 要求“RGBA”，但参数类型不支持透明度 | 规格中的颜色透明度无法按“RGBA”实现；若强行用 4 个分量需另一套控件与存储 | 需求修订为“RGB 颜色”；若业务确需透明度，新增独立 `TYPE_FLOAT` 参数（如 `lineAlpha`）并自定义绘制/混合，但这是范围扩展，需用户裁决 | 高 |
| P2-2 | P2 | `IsPublic=FALSE` 与 MAXScript 创建存在风险：`avg_dlx.cpp:3148` 的 `okToCreate_cf` 检查 `cd->IsPublic() && cd->OkToCreate(...)`；`startObjectCreation` 仅为 MAXScript 原语声明（`primitives.inl:352`） | 规格 §2.3 写“只能通过面板按钮或 MAXScript 创建”，但非公开类很可能被 MAXScript 的创建路径拒绝（运行时未验证，推测）；若验收包含 MAXScript 创建，会失败 | 若用户接受仅面板创建，修订规格文字；若必须 MAXScript 创建，需评估 `IsPublic=TRUE` + 其他隐藏手段，但这会破坏“不出现在创建→辅助对象”的验收。二者存在需求冲突，应由用户/总监裁决 | 中 |
| P2-3 | P2 | `TYPE_STRINGLISTBOX` 文档 4 vs 5 个 ID：`paramtype.h:596-604`；无样例可对照 | 按文档“4 个 ID”配置会少一个 CustEdit 或按钮，编译可通过但 UI 缺失/行为异常 | 以实际字段说明为准逐个核对 5 个 ID；在实现文档中记录该不一致；运行时验证按钮与编辑框行为 | 中 |
| P2-4 | P2 | 样例 `tapehelp.cpp:507-542` 的 `HitTest` 在命中早退路径（`:530`）未恢复 `savedLimits`（`:524` 保存、`:542` 恢复），属于 SDK 样例缺陷 | 直接照抄会导致图形窗口限制状态泄漏，影响后续绘制 | 实现时使用 RAII/`finally` 式恢复，确保所有 return 路径都恢复 `savedLimits`；不要照抄早退分支 | 高 |
| P2-5 | P2 | `NumSubs()=1` 但无 `SubAnim/SubAnimName`：`CADBundle.cpp:37/144`；对照 `pthelp.cpp:102-107` | 引用树/轨道视图可能显示异常或访问越界；规格 §8.1 未要求，但与 `NumSubs` 语义不一致 | 要么实现 `SubAnim/SubAnimName` 返回 pblock 名称，要么 `NumSubs` 返回 0；建议与 `pthelp` 一致实现 | 中 |
| P2-6 | P2 | 包围盒未实现，且屏幕恒定元素的“世界空间包络”无法固定：需在 `GetLocalBoundBox/GetWorldBoundBox` 中为箭头/文字/表格外扩留量；`GetWorldBoundBox` 可能收到 `vp==nullptr`（多处于视图无关调用） | 剔除/拾取/缩放到对象时裁剪错误 | 本地盒按几何点计算并加固定裕量；世界盒取对象矩阵变换后的 8 点；对 `vp` 为空做退化处理；屏幕恒定元素按近似最大屏幕尺寸换算到世界空间加裕量 | 中 |
| P2-7 | P2 | Utility 不能使用对象 `P_AUTO_UI` 路径：`ClassDesc2::BeginEditParams(IObjParam*, ReferenceMaker*, ULONG, Animatable*)`（`iparamb2.h:2612`）是对象编辑参数流程；`UtilityObj` 只有 `BeginEditParams(Interface*, IUtil*)`（`utilapi.h:70`） | 若把 Utility 面板按对象 auto UI 设计，无法接入或出现断言 | Utility 面板单独实现 Win32 对话框与控件逻辑；四个对象类的参数 UI 仍用 `P_AUTO_UI`；两者不要混用 | 高 |
| P2-8 | P2 | 表格“线宽”无公开 API：`GraphicsWindow.h` 检索未发现 `LineWidth`/`setLineWidth`/线宽成员；仅有 `setLineColor`（`object.h:5895-5896` 附近） | 规格 §7 要求“表格线宽”乘屏幕因子，但无法直接设置线宽；若用多条偏移线模拟会显著增加复杂度 | 需求修订为“线宽固定/不支持”；或用屏幕恒定偏移绘制双线模拟粗线。需用户确认视觉要求 | 中 |
| P2-9 | P2 | 卸载资源释放风险：`LibShutdown`/全局对象需避免释放仍被存活对象使用的字体/字符缓存；当前无 `controlsInit` 与 UI 资源清理逻辑；`GetString` 使用 `static TCHAR buffer[256]`（`DllEntry.cpp:91-101`） | 插件卸载/重载时可能崩溃或字符串截断；多线程/重入下静态缓冲不安全 | `LibShutdown` 只清理 DLL 级资源（控件库、字符串、全局注册），不关闭由对象持有的 BezFont；对象析构负责字体；`GetString` 可保留但类名很短，风险低；若需要更安全可改为线程局部/调用方缓冲 | 中 |
| P3-1 | P3 | 规格中已过时表述：`P_AUTO_UI_QT`（用户已裁决改 `P_AUTO_UI`）、`GraphicsWindow::setFont`（SDK 无此函数，用户已裁决 BezFont）、`ITextObject` 相关说明 | 不影响实现，但会造成文档与代码不一致，增加后续评审歧义 | 在开发文档中记录三处修订及理由；需求基线同步更新 | 低 |
| P3-2 | P3 | `GetString` 静态缓冲区 256 字符：`DllEntry.cpp:91-101` | 对短类名无实际影响；若未来本地化字符串过长可能截断 | 标注为已知限制；如需扩展改为 512 或调用方缓冲 | 低 |

## 三、五个关键问题的直接回答

### Q1：5 个 ClassDesc 注册/分发、4 个对象 `IsPublic=FALSE`、单 Utility 入口、创建模式激活的可行性与风险

1. 单 DLL 注册 5 个 ClassDesc 完全可行：`LibNumberClasses()` 返回 5，`LibClassDesc(0)` 返回 Utility，1–4 返回四个对象类。注意 `GetString`、`HInstance` 等 DLL 级资源必须单份；每个 ClassDesc 必须是静态生命周期对象，不能返回栈/临时对象。
2. `IsPublic=FALSE` 符合 SDK 设计意图：`plugapi.h:1041-1046` 明确私有类用于同一 DLL 内其他插件创建、不出现在 UI 中；Utility 必须 `IsPublic=TRUE` 才能出现在实用程序面板。
3. 创建模式激活：`SetActiveCreateMode` 不存在，必须改用 `GetCOREInterface7()->StartCreatingObject(ClassDesc*)`（`maxapi.h:8701`）或非交互式 `CreateInstance` + `CreateObjectNode`。`StartCreatingObject` 会进入创建 CommandMode，交互结束由 Max 调用 `StopCreating`；实现中必须在 `EndEditParams`、面板关闭、`MOUSE_ABORT` 时兜底清理。
4. 主要风险：
   - 4 个对象类与 Utility 的 Class_ID 必须唯一且稳定（当前 P0-4 冲突）；
   - 旧 `CADDimension.dlo` 若占用相同 ID，会出现重复类/场景解析不确定性；按用户裁决在运行验证前临时备份/移出并恢复；
   - 非公开类的 MAXScript 创建存在被 `okToCreate` 拒绝的风险（P2-2）；
   - 创建模式下若直接 `BeginCreate/EndCreate` 而未正确管理 CommandMode 栈，SDK 明确警告可能崩溃（`plugapi.h:1080-1106`）。
5. 结论：架构可行，但必须先完成 P0 去重与注册实现；创建路径采用 `Interface7`，不要按规格原文调用不存在的 API。

### Q2：`ParamBlockDesc2` + Win32 `P_AUTO_UI` 的参数/控件配置

| 参数类型 | 声明要点 | `p_ui` 控件配置 | 注意事项 |
|---|---|---|---|
| bool | `TYPE_BOOL` | `TYPE_SINGLECHECKBOX`, 控件 ID | 默认值与勾选状态由 pblock 管 |
| float | `TYPE_FLOAT` | `TYPE_SPINNER`, `EDITTYPE_FLOAT` 或 `EDITTYPE_UNIVERSE`, 编辑框 ID + 微调 ID + 缩放 | 屏幕尺寸参数建议用 `EDITTYPE_FLOAT`；世界尺寸用 `EDITTYPE_UNIVERSE`；样例见 `paramtype.h:477/480/485` |
| int | `TYPE_INT` | `TYPE_SPINNER`, `EDITTYPE_INT`, 编辑框/微调 ID | 行/列范围用 `p_range` 约束（§7 建议 1–50） |
| color | `TYPE_RGBA` | `TYPE_COLORSWATCH`, 色块 ID | 仅 RGB，无 alpha（P2-1） |
| string | `TYPE_STRING` | `TYPE_EDITBOX`, 编辑框 ID | 字符串长度由 pblock 管 |
| string table | `TYPE_STRING_TAB` + `P_VARIABLE_SIZE` | `TYPE_STRINGLISTBOX`, 最少 listbox ID、Add/Replace/Delete/CustEdit 共 5 个 ID | 文档称 4 个 ID，与实际枚举不一致（P2-3）；行列重排不会自动发生，需自定义 `ParamMap2UserDlgProc`/`PBAccessor`（P1-7） |
| Point3 | `TYPE_POINT3` | 6 个控件 ID（3 编辑框 + 3 微调）+ `SPIN_AUTOSCALE` | 样例 `boxobj.cpp:187-190`、`EggShapeHelpers.cpp:43-47` |

- `P_AUTO_CONSTRUCT` 的 pblock 引用索引必须与 `NumRefs/SetReference/GetReference` 一致（`iparamb2.h:3267-3271`），否则加载失败或引用错位。
- `P_AUTO_UI_QT` 与 `P_AUTO_UI` 不可同时使用；用户已裁决 Win32 `P_AUTO_UI`，因此必须移除/不生成 QT 路径。
- pblock 由系统持久化；`cellTexts`、几何关键点等非 pblock 数据需要自定义 `Save/Load`（Q4）。
- Utility 面板不能复用对象 `P_AUTO_UI` 流程，必须自建 Win32 对话框（P2-7）。
- 字符串表读改写建议：`GetValue` 取 `MSTR*` 数组，修改后按行优先重建并 `SetValue`；行列变化时先校验 rows/cols 再重排，所有修改通过 pblock 触发引用消息，从而自动重绘并进入撤销记录。

### Q3：Display/HitTest/GetLocalBoundBox/GetWorldBoundBox/CreateMouseCallBack/Save/Load/NotifyRefChanged 的正确骨架与陷阱

**签名（以 SDK 2027 为准）：**
- `void Display(TimeValue t, INode* inode, ViewExp* vpt, int flags)`（`object.h:599`）
- `int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2* p, ViewExp* vpt)`（`object.h:575`）
- `void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vp, Box3& box)`（`object.h:632`）
- `void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vp, Box3& box)`（`object.h:623`）
- `CreateMouseCallBack* GetCreateMouseCallBack()`（`object.h:644`）
- `IOResult Save(ISave* isave)`, `IOResult Load(ILoad* iload)`
- `RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate)`

**Display 骨架要点：**
- 先取对象变换 `inode->GetObjectTM(t)`；取 `float s = NonScalingObjectSize() * vpt->GetVPWorldWidth(tm.GetTrans()) / 360.0f`（P1-2）。
- `GraphicsWindow* gw = vpt->getGW()`；保存并设置 `gw->setTransform(...)` 和必要的 `setRndLimits`/`setLineColor`；结束恢复。
- 所有屏幕恒定长度（箭头、文字高度、延伸线超长、表格单元宽高）都乘 `s`。
- 连续线段用 `DrawLineProc`；闭合三角形需要直接 `gw->polyline(3, pts, nullptr, nullptr, 1, nullptr)`（stock `DrawLineProc::proc` 硬编码 `closed=0`，P1-8）。
- BezFont 文字：`theBezFontManager->OpenFont(...)`、`BuildCharacterEx(...)` 得到 `BezierShape` 轮廓，再用当前变换/缩放绘制多段线；字形缓存按文本/字体/字号失效；不要用 `GraphicsWindow::setFont`（不存在）。
- 单元格文字可用固定字体 `GraphicsWindow::text`（用户裁决），但尺寸换算仍需乘 `s`。

**HitTest 骨架要点：**
- `if (vpt == nullptr) return 0;` 防御空视图。
- 保存 `int savedLimits = gw->getRndLimits()`；设置 `GW_PICK`、清除 `GW_ILLUM`；使用 `MakeHitRegion`（`object.h:5834`）构建矩形/点命中区域。
- 所有 return 路径必须恢复 `savedLimits`（样例 `tapehelp.cpp:530` 的早退未恢复，是反例，P2-4）。
- 命中判断可用 `gw->checkHitCode()`，或对线段/箭头/文字包围盒做数学判断后 `vpt->setHitCode`；不要只依赖绘制结果。
- 命中区域应包含屏幕恒定元素的换算尺寸，否则缩放时命中范围与实际绘制不一致。

**包围盒要点：**
- 本地盒：由 P0/P1/P2/表格四角等几何点计算并加固定裕量；若仅有屏幕恒定元素，仍需给最小盒。
- 世界盒：对本地盒 8 点乘 `inode->GetObjectTM(t)`；`vp` 为空的路径使用保守裕量。屏幕恒定元素在 3D 空间没有固定范围，应按最大可视缩放估算或使用对象位置附近的保守盒，避免缩放到对象时被裁剪。

**CreateMouseCallBack 要点：**
- `proc(HWND hwnd, int msg, int point, int flags, IPoint2 m, Matrix3& mat)` 处理 `MOUSE_POINT`、`MOUSE_MOVE`、`MOUSE_ABORT`（`maxapi.h:1908-1910`、`:1929`）。
- 每点都用 `vpt->SnapPoint` 或 `Interface::SnapPoint` 取对象空间点；拖动阶段更新对象状态并 `NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE)` + `ip->RedrawViews(...)`；最后返回 `CREATE_STOP`，中止返回 `CREATE_ABORT`，继续返回 `CREATE_CONTINUE`。
- 需要 `theHold`/创建管理器在创建结束时自动做撤销；不要在 `MOUSE_POINT` 中重复 `Hold`。
- 创建模式下所有异常路径必须清理（P1-1/P1-10）。

**Save/Load/NotifyRefChanged 要点：**
- 自定义块：`isave->BeginChunk(kPointsChunk)` + 保存 P0/P1/P2/表格文本 + `EndChunk`；`Load` 按 chunk ID 分发，未知块跳过；加版本号，旧版本用 `ParamBlock2PLCB` 迁移（`pthelp.cpp:673-712`、`boxobj.cpp:463-487`）。
- pblock 由 `P_AUTO_CONSTRUCT` 自动存取，不要手写重复。
- `NotifyRefChanged`：ref 0（pblock）的 `REFMSG_CHANGE` 失效字形/几何缓存并重绘；对几何点引用（若有 INode）同样处理；返回 `REF_SUCCEED`/`REF_DO_NOT_REF` 需按实际情况，不要一律 `REF_SUCCEED`+TODO。

### Q4：引用管理、Clone、Undo/Redo、字体句柄生命周期、缓存、卸载释放

1. 引用：
   - pblock 为 ref 0；`NumRefs` 至少 1；构造函数 `MakeAutoParamBlocks(this)`；`SetReference(0, pblock)` 仅接受 ref 0；`GetReference(0)` 返回 pblock。
   - 若对象需要引用 INode/其他对象（例如表格插入点绑定节点），扩展 ref 1..n，并在 `Save/Load` 中记录引用关系。
2. Clone：
   - `BaseClone(ip, remap, cloneFlags)`；`ReplaceReference(0, remap.CloneRef(pblock))`（`pthelp.cpp:622-630`）。
   - 复制 P0/P1/P2、布尔/颜色/字符串（pblock 已克隆）、`cellTexts` 副本、字体名与缓存状态；不要复制字体原始句柄（句柄按对象新建）。
3. Undo/Redo：
   - 参数块修改由 ParamBlock2 的撤销机制处理；创建撤销由创建管理器处理。
   - 多步程序化修改（如改变行列并重排 cellTexts）需要 `theHold.Begin()` … `theHold.Accept()`/`Cancel()`（`hold.h:111、:153、:221-240`、`:329`）。每个 `Begin` 必须有且只有一个 `Accept` 或 `Cancel`。
   - 不要在 `Display/HitTest` 中触发 Hold。
4. 字体生命周期：
   - 每个对象对字体名的实际 `BezFont*` 由对象自己 `OpenFont`，在字体名/字号变化时关闭旧句柄、清空缓存、重建；析构/`EndEditParams` 关闭。
   - 文本变化只清字形缓存（`text.cpp:1587-1592` 模式），不必重开字体。
   - 缓存键应包含文本、字体名、字号；避免只按文本缓存导致换字号不刷新。
5. 卸载释放：
   - `LibShutdown` 不要强制关闭仍被存活对象持有的字体；由对象析构负责。
   - 释放控件库/UI 资源、字符串表、全局 ClassDesc 相关资源；`GetString` 静态缓冲区无需释放。
   - 延迟加载（`CanAutoDefer`）决定 `LibInitialize` 时机；若类注册不依赖初始化，可返回非零允许延迟；若依赖，返回 0。需与实现一致并记录。

### Q5：规格中不存在或语义无效的条目及修订建议

| 规格条目 | SDK 2027 事实/证据 | 影响 | 建议修订 |
|---|---|---|---|
| §2.4 `Interface::SetActiveCreateMode(Class_ID)` | 全 SDK 检索 0 匹配；存在 `Interface7::StartCreatingObject(ClassDesc*)`（`maxapi.h:8701`） | 编译失败 | 改为 `GetCOREInterface7()->StartCreatingObject(desc)`；非交互创建用 `CreateInstance`+`CreateObjectNode` |
| §2.7/§4/§5/§6/§7/§8.5/§10.11 “参数 × NonScalingObjectSize()” | `maxapi.h:1144/1152/1163-1187`：需再乘 `GetVPWorldWidth(point)/360.0` | 屏幕恒定尺寸失效 | 公式改为 `参数 × NonScalingObjectSize() × GetVPWorldWidth(tm.GetTrans())/360.0`；封装为统一函数 |
| §2.5 原文 `P_AUTO_UI_QT` | 用户已裁决 Win32 `P_AUTO_UI`；`P_AUTO_UI` 与 `P_AUTO_UI_QT` 不可混用 | 若两者同时存在会冲突 | 文档、代码统一为 `P_AUTO_UI`，删除 QT 表述 |
| §2.7/§5 原文 `GraphicsWindow::setFont` | `GraphicsWindow.h` 无 `setFont`；BezFont 路线为 `theBezFontManager`/`BuildCharacterEx`（`bezfont.h:244-246/258/265`） | 编译失败/文字不渲染 | 按用户裁决改为 BezFont；表格单元格可用 `GraphicsWindow::text` |
| §4/§5/§6/§7 “RGBA” | `TYPE_RGBA` 无 alpha（`paramtype.h:34`） | 透明度需求无法满足 | 改为 RGB；若需透明度，新增独立 float 参数并自定义混合（范围扩展，需用户裁决） |
| §2.3 “只能通过面板按钮或 MAXScript 创建” | `IsPublic=FALSE` + `okToCreate` 检查（`avg_dlx.cpp:3148`）很可能禁止 MAXScript 创建；运行时未验证（推测） | 若验收依赖 MAXScript 创建则失败 | 建议改为“仅通过面板按钮创建”；若要保留 MAXScript，需重新评估 `IsPublic` 策略（与验收 4 冲突） |
| §2.7 “连续线段统一 DrawLineProc；箭头闭合三角形” | `object.h:5877-5889` 的 `DrawLineProc::proc` 硬编码 `closed=0` | 闭合三角形无法用该 proc 绘制 | 允许箭头使用 `gw->polyline(..., closed=1, ...)`，或派生专用 proc；其余线段仍用 `DrawLineProc` |
| §7 “表格线宽” | `GraphicsWindow.h` 无线宽 API；仅有颜色 | 线宽参数无法实现 | 改为固定线宽或双线模拟；需用户确认 |
| §3 Utility 面板假定可用 `P_AUTO_UI` | UtilityObj 的 `BeginEditParams(Interface*, IUtil*)` 不接入对象 auto UI（`utilapi.h:70`；`iparamb2.h:2612` 是对象路径） | 面板无法按 auto UI 生成 | Win32 浮动对话框自绘控件；对象类才用 `P_AUTO_UI` |
| §7 “cellTexts 行列变化自动调整” | `TYPE_STRING_TAB` 只负责值存储；无自动重排/编辑网格（`paramtype.h:153`、`:596-604`） | 行列变化后文本错位 | 在 `PBAccessor`/`NotifyRefChanged` 或自定义 UI 中按行优先重排；显式持久化 |
| §8.4 `CanAutoDefer` | `plugin.h:58-63`；当前未导出 | 延迟加载行为不确定 | 实现 `CanAutoDefer()` 并在 `.def` 导出；返回值按初始化依赖决定 |
| §8.1 `NumSubs` 与 `SubAnim` | 当前 `NumSubs=1` 但无 `SubAnim`；样例实现见 `pthelp.cpp:102-107` | 引用树/轨道视图异常 | 实现 `SubAnim/SubAnimName` 或改 `NumSubs=0` |
| §1.3/§8.1 `ITextObject` 禁止继承 | 判断正确：`ITextObject` 为文本对象访问接口，不是插件基类 | 无冲突 | 保留该要求 |

## 四、未验证项

1. 修复重复拼接后能否在本机 VS 2026 18.12/v143 下成功编译链接（未运行构建）。
2. 5 个 ClassDesc 在 3ds Max 2027 启动时能否无 ABI/缺失 DLL 错误加载（未启动 Max）。
3. `IsPublic=FALSE` 的四个类在 `StartCreatingObject`/`CreateInstance` 下能否正常创建（未运行，仅据文档推测可行）。
4. 非公开类能否被 MAXScript `startObjectCreation` 创建（未运行，`okToCreate` 代码提示很可能不能）。
5. `TYPE_STRINGLISTBOX` 的实际 ID 数量、按钮行为、`CustEdit` 回调（无样例，未运行）。
6. `TYPE_STRING_TAB` 在 `P_VARIABLE_SIZE` 下的存储/读取行为与自动 UI 显示（未运行）。
7. BezFont `BuildCharacterEx` 对中文/字体缺字、字号、字形方向的渲染表现（未运行）。
8. 屏幕恒定公式在透视/正交、不同 DPI、不同视口缩放下的实际像素一致性（未运行）。
9. 旧 `CADDimension.dlo` 与新 `CADBundle.dlo` 的 Class_ID 冲突情况及备份/恢复后的运行时行为（未运行）。
10. `CanAutoDefer` 返回 0/非 0 对类注册、字体管理器初始化、卸载顺序的实际影响（未运行）。
11. `StartCreatingObject` 对 HelperObject 的创建流程、`StopCreating` 时机、`BeginCreate/EndCreate` 是否由 Max 内部管理（未运行）。
12. `theHold` 与创建管理器在创建后对象的撤销/重做边界（未运行）。
13. Utility 面板 Win32 浮动对话框在 3ds Max 2027 中的停靠/浮动/关闭行为（未实现，未运行）。
14. 表格行/列改变时参数块消息顺序与 UI 重入行为（未实现，未运行）。
15. `DrawLineProc` 与 `gw->polyline(closed=1)` 混用时拾取/重绘兼容性（未运行）。
16. `GetString` 静态缓冲在中文资源/长字符串下的截断（未运行，代码审查仅见 256 上限）。
17. 插件卸载/重载时字体句柄与字符缓存释放的实际稳定性（未运行）。

## 五、可能与其他专家冲突的判断

以下仅为本人的独立技术判断，未读取其他报告、项目总监意见或总裁技术结论。若与其他专家结论不一致，属正常的技术分歧，应以证据和运行时验证为准。

1. **关于 `IsPublic=FALSE`**：我认为它只是 UI 可见性/创建入口的过滤标志，不是安全边界，也不是“只能通过面板创建”的强制机制。文档（`plugapi.h:1041-1046`）的设计意图是“私有类供同一 DLL 内其他插件创建”，程序化 API 仍可按 `ClassDesc*` 创建。若有人认为 `IsPublic=FALSE` 能阻止一切非面板创建，我不认同。
2. **关于 MAXScript 创建**：规格假设“面板按钮或 MAXScript 均可创建”。我的判断是：`IsPublic=FALSE` 很可能使 MAXScript `startObjectCreation` 失败（`avg_dlx.cpp:3148` 的 `okToCreate_cf` 先检查 `IsPublic`），因此规格的“或 MAXScript”与验收 4 存在潜在冲突。这一点我标注为“推测”，需运行时验证；在没有证据前不应承诺 MAXScript 可创建。
3. **关于屏幕恒定尺寸公式**：规格反复写“参数 × `NonScalingObjectSize()`”，我认为这不完整，必须再乘 `GetVPWorldWidth(...)/360.0`（`maxapi.h:1163-1187`）。若其他专家按规格原文认可该公式，我保留不同意见；这是会导致验收 6/11 失败的关键点。
4. **关于 `TYPE_RGBA`**：规格写“RGBA”，但 SDK 类型只有 RGB。我认为应在需求层修订为 RGB，或增加独立 alpha 参数；不认为可以通过现有 `TYPE_RGBA` 直接得到 alpha。
5. **关于“所有连续线段统一用 `DrawLineProc`”**：规格同时要求“箭头用闭合三角形轮廓”，但 stock `DrawLineProc::proc` 硬编码 `closed=0`（`object.h:5888`）。我认为箭头必须作为例外直接 `polyline(..., 1, ...)`；若其他专家坚持所有线段一律用 `DrawLineProc`，则该验收项与箭头要求不可同时满足。
6. **关于 Utility 面板 `P_AUTO_UI`**：我认为 `P_AUTO_UI` 只适用于对象编辑参数流程，Utility 必须自建 Win32 对话框；若其他专家认为 Utility 也能直接复用对象 auto UI，根据 `utilapi.h:70` 与 `iparamb2.h:2612` 的签名差异，我持保留态度。
7. **关于字符串表 UI**：我认为 `TYPE_STRING_TAB` 能自动持久化，但不会自动提供行列编辑网格，也不会在 rows/cols 变化时自动重排 `cellTexts`；需要自定义 UI 与访问器。若其他专家认为 auto UI 即可完整满足 §7，我不认同。
8. **关于 `CanAutoDefer`**：规格要求该导出。我认为其返回值应依据“类注册是否依赖 `LibInitialize`”决定；若实现不依赖初始化，返回非 0 允许延迟加载是可接受的，但必须与 `.def` 和实际初始化时序一致。若其他专家主张无条件返回 0，我认为需要说明理由并与实现一致。
9. **关于表格线宽**：我在 `GraphicsWindow.h` 未找到线宽设置 API，因此认为“表格线宽”在当前路线下无法按参数直接实现。若其他专家给出可用 API，应以具体头文件/行号为准；在此之前我标记为受限项。
10. **关于样例缺陷**：`tapehelp.cpp:530` 的 `HitTest` 早退未恢复 `savedLimits` 是样例自身缺陷，不应被当作“SDK 推荐写法”照抄。若其他专家引用该样例作为正确骨架，我提醒此风险。

## 六、下一步建议

以下建议按优先级排列，均需项目经理/总监决定是否执行；本人未修改任何文件或运行构建。

1. **先修基线（P0）**：删除六个项目文件中的重复拼接内容；把类声明放进头文件并确保 `#pragma once`；统一 `CADBundle.h` 的 include；重新编译并保留干净日志。没有这一步，任何实现都无法验证。
2. **冻结 Class_ID 与类清单**：为 `CADAnnotatePanel` 与四个对象类各确定一个唯一 `Class_ID`，写死在实现与开发文档中；核对旧 `CADDimension.dlo` 的 ID（需用户提供或运行时检查），按用户裁决做好备份/恢复方案。
3. **搭注册骨架（P0）**：实现 `LibNumberClasses=5` 与 `LibClassDesc` 的 0–4 分发；Utility 用 `UTILITY_CLASS_ID`、`IsPublic=TRUE`；四个对象类 `HelperObject`、`IsPublic=FALSE`；增加 `CanAutoDefer` 并按 `.def` 导出规则处理。
4. **实现共享对象基类与参数块（P1）**：pblock 为 ref 0，`MakeAutoParamBlocks`，`NumRefs/GetReference/SetReference/SubAnim/SubAnimName` 完整；四个类各自的 `ParamBlockDesc2` 与 `P_AUTO_UI` 资源；先让对象能在修改面板显示参数并能持久化，再进入绘制。
5. **实现 Utility 面板与创建链路（P1）**：Win32 浮动对话框；4 个分类按钮；`GetCOREInterface7()->StartCreatingObject(desc)`；`EndEditParams`/面板关闭/`MOUSE_ABORT` 统一 `StopCreating` 与 CommandMode 清理；验证点选创建、取消、连续创建。
6. **实现绘制与命中（P1）**：先统一屏幕恒定函数 `S = NonScalingObjectSize() * GetVPWorldWidth()/360`；`DrawLineProc` 用于折线，箭头单独 `polyline(closed=1)`；`HitTest` 保存/恢复所有 GW 状态；包围盒加裕量。BezFont 文字与缓存独立成小模块。
7. **实现持久化与复制（P1）**：`Clone` 采用 `BaseClone` + `remap.CloneRef(pblock)`；`Save/Load` 用版本化自定义块保存 P0/P1/P2、表格文本；核对 pblock 自动持久化与自定义块不重复；实现 `NotifyRefChanged` 缓存失效与重绘。
8. **实现表格编辑与行列重排（P1/P2）**：`TYPE_STRING_TAB` + `P_VARIABLE_SIZE`；自定义 `ParamMap2UserDlgProc` 或等价 Win32 编辑方式；rows/cols 变化按行优先重排，空串补齐、多余截断；UI 与数据变更双向同步；必要时用 `theHold` 包裹多步修改。
9. **资源与 ID 整理（P1）**：单份 `resource.h`/`.rc`；为 Utility 对话框与 4 个对象 auto UI 分配完整 `IDD/IDC/IDS`；消除重复定义与冲突 ID；确认所有 p_ui 引用的资源存在。
10. **逐项做运行时验收（需用户授权）**：按 §10 的 13 条验收标准在 3ds Max 2027 中执行；验证前临时备份/移出旧 `CADDimension.dlo`，测试后恢复；重点观察创建模式残留、保存重开、视口缩放尺寸恒定、BezFont 中文、表格行列变化、卸载重载。
11. **同步修订需求基线**：将本报告 Q5 表格中的 11 项修订（创建 API、屏幕恒定公式、RGBA、setFont/BezFont、P_AUTO_UI_QT、MAXScript 表述、DrawLineProc 例外、表格线宽、Utility auto UI、字符串表重排、CanAutoDefer）提交用户/总监确认后写入开发文档。
12. **保持审阅纪律**：后续如要求我复核实现，请提供新的只读范围；我仍不会读取其他专家报告或替项目总监做决定。
