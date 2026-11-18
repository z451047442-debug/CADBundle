【持久化与验收风险专家｜FMEA + 可验证性设计 + 防御性编程】
（第 1/2 部分）

## 一、审阅范围

### 已读文件与证据

- `D:\PycharmProjects\CADBundle\docs\requirements\CADAnnotatePanel.md`：1–202 行，已全文读取。
- `D:\PycharmProjects\CADBundle\CADBundle.cpp`：1–214 行，已全文读取。
- `D:\PycharmProjects\CADBundle\DllEntry.cpp`：1–204 行，已全文读取。
- `D:\PycharmProjects\CADBundle\CADBundle.rc`：1–248 行，已全文读取。
- `D:\PycharmProjects\CADBundle\resource.h`：1–72 行，已全文读取。
- `D:\PycharmProjects\CADBundle\CADBundle.h`：1–58 行，已全文读取。
- `D:\PycharmProjects\CADBundle\CADBundle.vcxproj`：1–66 行，已全文读取。
- `D:\PycharmProjects\CADBundle\CADBundle.def`：1–20 行，已全文读取。
- `D:\PycharmProjects\CADBundle\CADBundle.slnx`：1 行，已全文读取。
- `D:\PycharmProjects\CADBundle\x64\baseline-build.log`：1–93 行，已全文读取。
- `D:\PycharmProjects\CADBundle\x64\Release\Plugins\`：只读目录列表显示为空，无 `.dlo`/DLL 产物。
- 旧插件 `D:\Program Files\3ds Max 2027\3ds Max 2027\plugins\CADDimension.dlo`：只读元数据与字符串检查。文件为 158,720 字节，时间 2026-11-17 10:57:06 +0800；SHA-256 为 `92b62a52fa288deafc0405df6d1dda4dee1756ac784382359c52969adffb963a`。可打印字符串含 `CADDimension`、`CADText`、`CADLeader`、`CADTable` 及 `LibClassDesc` 等导出名；可打印字符串显示其导入 `MSVCP140.dll`、`VCRUNTIME140.dll`、`VCRUNTIME140_1.dll`、`core.dll`，未发现 Qt 字符串。未反汇编或读取 PE 导出表，Class_ID、`LibNumberClasses` 与实际依赖表均未验证。
- SDK 重点读取：`include\paramtype.h`、`include\iparamb2.h`、`include\maxapi.h`、`include\object.h`、`include\plugapi.h`、`include\plugin.h`、`include\maxscript\maxscript.h`、`include\maxscript\protocols\primitives.inl`、`include\bezfont.h`；样例 `samples\objects\tapehelp.cpp`、`samples\objects\text.cpp`、`samples\objects\prothelp.cpp` 的持久化/引用相关段落。
- Git：只读 `git log`、`git status`；基线提交 `cad7b7c`，分支 `ai-review-fix-2026-11-18`，审阅时工作区仅见未跟踪的 `docs/`。

### 未审阅或未展开

- 未逐行审阅 `3dsmaxsdk_preinclude.h`、`.gitignore`、项目根目录之外的其余文件、SDK 的其他样例与完整实现。
- 未读取旧 `CADDimension.dlo` 的 PE 导出表、Class_ID、`LibNumberClasses`、真实依赖表；仅做了可打印字符串与文件哈希检查。
- 未读取其他专家报告、项目总监意见或总裁技术结论。
- 未发现明确的提示注入。需求文档中的约束按需求数据审阅；源码中的 `#pragma message(TODO(...))` 仅作为占位数据，不作为指令执行。

### 运行时覆盖

- 未运行 MSBuild，未安装插件，未复制文件到 3ds Max 目录，未启动 3ds Max，未运行 MAXScript，未创建/保存/重开 `.max`，未测量屏幕像素。
- 因此运行时行为覆盖为 0；除基线构建日志所述失败外，本报告不把任何运行时结论写成事实。

---

## 二、关键发现

### 2.1 总判断

当前基线不是可交付候选，也不具备进入第 7/8 步门禁的条件。按现有静态证据：

- 验收 1、3、5、10、13 已可判为失败或未实现。
- 验收 2、6、7、8、9、11、12 未运行或没有实现，不能判为通过。
- 验收 4 在四个对象类尚不存在时没有实际对象可暴露；`IsPublic()` 又与需求相反，不能作为通过证据。

### 2.2 P0 发现

#### [P0] P0-1 基线当前不可编译，无法生成任何可验收产物

- **结论**：`CADBundle.cpp`、`DllEntry.cpp`、`.rc`、`resource.h`、`.def`、`.h` 均存在整段重复拼接；基线构建在编译阶段失败，没有进入链接，也没有产物。
- **证据**：
  - `CADBundle.cpp` 第 1–107 行与第 108–214 行是两份完整骨架；第 15 行 `Class_ID(0x34883ae1, 0x5594bda4)` 与第 122 行 `Class_ID(0xa517250d, 0x8cd4f7c3)` 冲突；第 19 行与第 126 行重复定义 `CADDimension`，第 57 行与第 164 行重复定义 `CADDimensionClassDesc`。
  - `DllEntry.cpp` 第 1–101 行与第 103–203 行重复；第 17 行与第 119 行重复 `hInstance`，第 26 行与第 128 行重复 `DllMain`。
  - `CADBundle.rc` 第 1–123 行与第 125–246 行重复；`resource.h` 第 1–36 行与第 37–72 行重复；`CADBundle.def` 第 1–10 行与第 11–20 行重复；`CADBundle.h` 第 1–29 行与第 30–57 行重复，但因 `#pragma once` 未造成同等的重复定义。
  - `x64\baseline-build.log` 第 17–93 行记录 `C2086`、`C2374`、`C2084`、`C4005`、`C2011`、`C2027`、`C2355`；日志在编译错误处结束，没有链接成功记录。
  - `x64\Release\Plugins\` 目录为空。
- **影响**：验收 1 直接失败；后续所有运行时、持久化、界面验收均无对象可测。
- **建议**：不要对重复拼接做逐行修补；先恢复单一源码基线，确保每个符号、资源、导出只定义一次，再增加“构建失败即阻断”的门禁。
- **置信度**：高（静态日志与源码结构证据充分）。

#### [P0] P0-2 类注册只有 1 个类，没有 Utility 面板和其余三个对象类

- **结论**：`DllEntry.cpp` 当前只注册 `CADDimension`，`LibNumberClasses()` 返回 1；没有 `CADAnnotatePanel`、`CADText`、`CADLeader`、`CADTable` 的 ClassDesc。
- **证据**：
  - `DllEntry.cpp:48-51` 返回 `1`；`DllEntry.cpp:54-61` 仅在索引 0 返回 `GetCADDimensionDesc()`，其余返回 0。
  - `CADBundle.cpp:19-54` 只定义 `CADDimension`，没有其他对象类。
  - 需求第 19 行要求 `LibNumberClasses()` 返回 5，`LibClassDesc(0)` 为 Utility，`LibClassDesc(1..4)` 为四个对象类；第 140 行再次要求入口完整注册 5 个 ClassDesc。
- **影响**：验收 3、4、5 无法成立；用户看不到 “CAD 注释” 工具集入口，更无法从面板创建对象。
- **建议**：按需求实现 5 个独立 ClassDesc，固定索引顺序，并增加导出/运行时注册检查。
- **置信度**：高。

#### [P0] P0-3 `Save`/`Load` 是空实现，参数块未构造、未引用，持久化路径当前不存在

- **结论**：所有对象的 `Save`/`Load` 直接返回 `IO_OK`，没有任何几何点、参数块版本或表格单元格数据的写入；`pblock` 成员未初始化，构造时也没有创建参数块或建立引用。
- **证据**：
  - `CADBundle.cpp:26-27` 与 `CADBundle.cpp:133-134`：`Load`/`Save` 均为 `return IO_OK;`。
  - `CADBundle.cpp:23` 与 `CADBundle.cpp:130`：`IParamBlock2* pblock;` 未初始化。
  - `CADBundle.cpp:52` 与 `CADBundle.cpp:159`：构造函数为空，没有 `MakeAutoParamBlocks` 或 `ReplaceReference`。
  - `CADBundle.cpp:41` 与 `CADBundle.cpp:148`：`GetParamBlockByID` 直接解引用 `pblock`，没有空指针保护。
  - `CADBundle.cpp:37` 与 `CADBundle.cpp:144` 只有 `NumSubs()` 返回 1，没有 `NumRefs`、`SetReference`、`GetReference`。
  - 需求第 38 行要求每个对象实现 `Save`/`Load`、`SetReference`/`GetReference`/`NumRefs`/`NotifyRefChanged`，并显式保存几何关键点与无法自动持久化的数据。
- **影响**：保存后重开对象只能得到默认/零值；保存时不会写入几何点；创建或调用参数块时可能空指针崩溃；参数块生命周期没有引用管理，复制、撤销、重做也会失效。验收 10 不可能通过。
- **建议**：先实现参数块构造与引用管理，再实现显式几何/表格 chunk；在 `GetParamBlockByID` 等处加空指针保护；用可回读的数据导出验证，而不是只看对象是否出现。
- **置信度**：高（静态）；运行时崩溃概率高，但未运行验证。

#### [P0] P0-4 两个冲突的 Class_ID 使旧场景兼容和新插件注册都不可确定

- **结论**：同一个 `CADDimension` 在同一编译单元中有两个不同的 Class_ID；宏重定义后实际使用哪一个取决于包含/编译顺序，旧场景与新对象的类解析没有稳定基线。
- **证据**：
  - `CADBundle.cpp:15`：`Class_ID(0x34883ae1, 0x5594bda4)`。
  - `CADBundle.cpp:122`：`Class_ID(0xa517250d, 0x8cd4f7c3)`。
  - `x64\baseline-build.log:45-47` 记录 `C4005 CADDimension_CLASS_ID` 宏重定义；`:53-55` 记录类重定义。
  - 旧 `CADDimension.dlo` 字符串含四个 CAD 类名，但 Class_ID 未验证。
- **影响**：如果新插件使用新 ID，旧 `.max` 中由旧 DLO 保存的对象无法解析；如果复用旧 ID，又可能和新类参数块布局不一致，造成反序列化错位。旧 DLO 与新 DLL 共存时的类归属也不确定。验收 10 和用户旧场景数据都受影响。
- **建议**：在首次发布/首次保存用户场景前冻结每个类的 Class_ID；明确旧 DLO 与新插件的 ID/共存策略；不要在没有旧场景迁移结论前替换旧插件。该项需要用户或项目负责人裁决，不能默认。
- **置信度**：冲突本身高；旧 DLO 精确 ID 与兼容性为“未验证/推测”。

#### [P0] P0-5 需求中的屏幕恒定公式不完整，按原文实现将无法保证缩放恒定

- **结论**：需求把屏幕恒定尺寸定义为“参数值 × `NonScalingObjectSize()`”，但 SDK 的典型非缩放对象实现还需要乘视口世界宽度因子；单独乘 `NonScalingObjectSize()` 不能抵消视口缩放。验收 11 把这个不完整公式固化为静态判据，可能产生“代码符合标准但行为不恒定”的假通过。
- **证据**：
  - 需求第 35–36 行、第 76 行、第 141 行均要求“参数值 × `NonScalingObjectSize()`”。
  - `D:\3dsMaxSDK2027\maxsdk\include\maxapi.h:1178-1187` 的 SDK 示例：`scaleFactor = vpt->NonScalingObjectSize() * vpt->GetVPWorldWidth(tm.GetTrans())/(float)360.0;`，`NonScalingObjectSize()` 的默认值为 1.0。
  - `maxapi.h:1152` 声明 `GetVPWorldWidth(const Point3 wPoint)`。
- **影响**：箭头、文字、延伸线、表格线在缩放时仍可能改变屏幕尺寸，验收 6、8、9 失败；验收 11 的静态形式反而可能掩盖问题。
- **建议**：将公式修订为 `s = NonScalingObjectSize() * GetVPWorldWidth(参考点) / 360.0`（或等价的屏幕到世界换算），并在验收中同时要求静态公式和运行时像素测量；否则不能把验收 11 当作充分条件。
- **置信度**：高（SDK 示例直接支持）。

### 2.3 P1 发现

#### [P1] P1-1 四个对象类当前 `IsPublic()` 返回 `TRUE`，与需求相反

- **结论**：`CADDimensionClassDesc::IsPublic()` 在两个重复副本中都返回 `TRUE`，而需求要求四个对象类返回 `FALSE`。
- **证据**：`CADBundle.cpp:60`、`CADBundle.cpp:167`；需求第 13、21、138 行。
- **影响**：对象类会暴露在“创建 → 辅助对象”中，验收 4 失败；如果临时改为 `FALSE`，还要验证非公开类的创建路径，见 P1-2。
- **建议**：对象类 `IsPublic()` 改为 `FALSE`；Utility 保持 `TRUE`；同时保留可脚本化创建能力。
- **置信度**：高。

#### [P1] P1-2 SDK 中没有找到 `SetActiveCreateMode`，创建路径必须按需求回退并实测

- **结论**：在 SDK `include` 中未检索到 `SetActiveCreateMode`；需求的备用路径 `ExecuteMAXScriptScript` 存在，但 `startObjectCreation` 对 `IsPublic()=FALSE` 的类是否有效尚未验证。
- **证据**：
  - `grep -rn "SetActiveCreateMode" D:\3dsMaxSDK2027\maxsdk` 无匹配。
  - `include\maxscript\maxscript.h:354` 存在 `ExecuteMAXScriptScript(...)`。
  - `include\maxscript\protocols\primitives.inl:352` 注册 `startObjectCreation`。
  - `include\plugapi.h:1096`、`:1106` 有 `ClassDesc::BeginCreate`/`EndCreate`；`maxapi.h:5241` 有 `CreateInstance`，`:5439` 有 `NonMouseCreate`。
- **影响**：面板按钮可能无法进入交互式创建，验收 5 失败；也可能创建后残留创建模式。
- **建议**：按需求第 22–25 行选择实际可用 API 并记录理由；先用最小实验验证 `startObjectCreation` 对非公开类是否可用。若不可用，研究 `BeginCreate/EndCreate` 或 `CreateInstance` + 鼠标回调的回退路径；不要把类临时改成公开来掩盖问题。该实验未运行，结论“未验证”。
- **置信度**：API 缺失为高；运行时行为为中/未验证。

#### [P1] P1-3 对象类缺少需求 8.1 的大量必需方法

- **结论**：当前 `CADDimension` 只有骨架方法，没有 `Display`、`GetLocalBoundBox`、`GetWorldBoundBox`、`HitTest`、`BeginEditParams`、`EndEditParams`、`NumRefs`/`SetReference`/`GetReference` 等完整实现；其他三个对象类不存在。
- **证据**：`CADBundle.cpp:19-54` 只包含 `Load`/`Save`、`ClassID`、`GetClassName`、`NumSubs`、`NumParamBlocks`、`GetParamBlock`、`GetParamBlockByID`、`DeleteThis`、`GetCreateMouseCallBack`、`Eval`；`GetCreateMouseCallBack` 返回 `nullptr`（`CADBundle.cpp:96-100`），`Clone` 返回 `nullptr`（`:83-87`）。需求第 137 行列出完整方法清单。
- **影响**：对象无法可靠显示、选中、编辑参数或参与交互；验收 5–10 无法通过。
- **建议**：按对象逐类实现完整生命周期；先做最小可显示/可选中/可保存闭环，再扩展绘制细节。
- **置信度**：高。

#### [P1] P1-4 参数块引用、`Clone` 和重绘通知缺失，复制/粘贴/撤销会失效

- **结论**：`CADDimension` 没有 `NumRefs`/`SetReference`/`GetReference`，`Clone` 返回 `nullptr`，`NotifyRefChanged` 仅返回 `REF_SUCCEED`，没有重绘逻辑。
- **证据**：`CADBundle.cpp:34`、`:37`、`:83-93`、`:141`、`:144`、`:190-200`；需求第 137 行。
- **影响**：参数块没有所有权和生命周期；复制/粘贴/撤销/重做可能崩溃或丢参数；修改参数不会触发正确重绘。
- **建议**：构造时建立 `pblock` 引用，`NumRefs` 至少覆盖参数块，`Clone` 使用 `remap` 复制；`NotifyRefChanged` 对参数变化调用 `NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE)` 并刷新视口。
- **置信度**：高。

#### [P1] P1-5 表格行列调整和 `cellTexts` 的行优先映射存在数据串格风险，且编辑 UI 未定义

- **结论**：需求要求修改行列数后按新行列调整 `cellTexts`，但没有定义列数变化时旧单元格如何按 `(row, col)` 映射；`TYPE_STRING_TAB` 的自动 UI 是否能提供二维单元格编辑也未验证。当前没有 `SetCount`/`ZeroCount` 或编辑界面代码。
- **证据**：
  - 需求第 119–133 行定义 `cellTexts` 行优先、修改行列数后新增为空/减少截断；第 167 行要求“单元格文本可编辑显示”。
  - `include\paramtype.h:153` 定义 `TYPE_STRING_TAB`；`:601` 的 `TYPE_STRINGLISTBOX` 仅适用于字符串表。
  - `include\iparamb2.h:1286`、`:1291` 有 `ZeroCount`/`SetCount`。
  - `CADBundle.cpp` 中没有 `TYPE_STRING_TAB`、`SetCount`、`ZeroCount` 或表格 UI 代码。
- **影响**：从 2×3 调整为 3×2 时，按 `idx = row*cols + col` 的线性复制会把旧单元格放到错误位置或丢数据；`SetCount` 后继续使用旧字符串指针可能崩溃；验收 9、10 失败。
- **建议**：明确并固化映射规则：按旧/新行列的 `(row, col)` 交集保留，新增为空，超出新边界的单元格丢弃并记录警告；`SetCount` 后重新取指针再填充；行列两个参数变化时用“正在调整”标志合并一次重算。编辑 UI 需明确为自定义 Win32 表格编辑对话框或可验证的 MAXScript 路径；不能假定自动 UI 已提供二维编辑。
- **置信度**：数据串格风险高；自动 UI 具体行为为中/未验证。

#### [P1] P1-6 资源文件仍是向导占位，面板按钮、类字符串和资源 ID 不完整

- **结论**：`IDD_PANEL` 仍是 108×156 的 `WS_CHILD` 占位模板，文本为 “Generated by ... wizard” 和 “TODO: Place panel controls here.”；没有标注/文字/引线/表格四个按钮的资源 ID，也没有 Utility 面板和四个类所需的完整字符串。
- **证据**：
  - `CADBundle.rc:28-41` 的 `IDD_PANEL` 和占位文本；`:98-106` 只有 `IDS_LIBDESCRIPTION`、`IDS_CATEGORY`、`IDS_CLASS_NAME`、`IDS_PARAMS`、`IDS_SPIN`。
  - `resource.h:7-11` 只有 5 个字符串 ID；`:19-25` 只有 `IDD_PANEL` 和少量控件 ID；`:21-22` 的 `IDC_CLOSEBUTTON` 与 `IDC_DOSTUFF` 都是 1000，存在资源 ID 冲突。
  - 需求第 54、145 行要求完整定义按钮 ID、字符串、对话框模板。
- **影响**：验收 3 直接失败；面板无法显示四个分类项；资源 ID 冲突会造成控件行为错误。
- **建议**：重做单一 `.rc`/`resource.h`，为面板和四个类分配唯一 `IDS_*`、`IDC_*`、`IDD_*`；面板增加 4 个按钮及说明区；用资源审计脚本检查重复 ID。
- **置信度**：高。

#### [P1] P1-7 旧 `CADDimension.dlo` 的共存/场景兼容风险不能只按“改文件名”处理

- **结论**：旧插件仍包含四个 CAD 类名，且与新插件同处 `plugins` 目录时存在类名/Class_ID/加载顺序冲突风险；旧场景是否能在新插件下解析，取决于 Class_ID 与参数块版本，而这两者尚未验证。
- **证据**：
  - 旧 DLO 的只读字符串含 `CADDimension`、`CADText`、`CADLeader`、`CADTable`；文件哈希 `92b62a52...963a`，大小 158,720 字节。
  - 需求第 180 行要求运行时验证前“临时备份/移出旧 `CADDimension.dlo`，测试后恢复”。
  - 需求第 187 行确认现有源码为重复拼接骨架、基线构建失败。
- **影响**：启动时可能出现重复类、类解析到旧 DLO、或新插件类不生效；如果新插件 Class_ID 与旧 DLO 不同，旧场景中的对象无法由新插件恢复；如果相同，又可能因参数块布局不同而错位。
- **建议**：测试前记录旧 DLO 哈希并复制/移出到 `plugins` 目录之外，重启 Max 验证旧类消失；再安装新插件测试；测试后按哈希恢复旧 DLO。最终是“保留旧 DLO 共存”还是“替换并迁移旧场景”，需要用户/项目负责人明确裁决；本报告不替做决定。
- **置信度**：文件事实高；冲突与场景兼容性为中（Class_ID/导出未验证）。

#### [P1] P1-8 ABI/CRT/依赖兼容没有形成可验证证据链

- **结论**：当前没有可检查的 `.dlo` 产物，也没有依赖表；仅凭“使用 SDK 工程”不能证明 3ds Max 2027 启动时不会出现 ABI 或缺失 DLL 错误。
- **证据**：
  - `CADBundle.vcxproj:22` 设置 `MaxSDK=D:\3dsMaxSDK2027\maxsdk`；`:46` 链接多个 SDK 库；`:47` 使用 `CADBundle.def`；`:51-52` 只编译两个 `.cpp`。
  - 需求第 190 行记录本机 VS 2026 与 v143 工具集 `14.38.33130`；第 191 行记录旧 DLO 的运行时依赖特征。
  - 旧 DLO 字符串显示使用 `MSVCP140`/`VCRUNTIME140`/`VCRUNTIME140_1`/`core.dll`；未发现 Qt 字符串，但这不等于不存在其他依赖。
  - `dumpbin` 当前不在 PATH 中，未完成依赖/导出检查。
- **影响**：验收 2 无法判定；可能出现启动加载失败、CRT 不匹配、x64/平台不匹配或缺失运行库。
- **建议**：用 SDK 提供的工程属性构建，确认 `/MD` 动态 CRT 与目标 Max 一致；产物生成后执行 `dumpbin /headers`（x64）、`dumpbin /exports`、`dumpbin /dependents`，并记录 Max 版本、SDK 版本、工具集版本和产物 SHA-256。此项未运行，结论“未验证”。
- **置信度**：中（静态配置明确，运行时未验证）。

#### [P1] P1-9 面板关闭后的创建模式残留没有实现保护

- **结论**：需求明确要求面板关闭后再次打开状态正常、不残留创建模式；当前没有面板代码，也没有 `EndEditParams` 中停止创建、清理按钮状态或销毁窗口的实现。
- **证据**：需求第 51–52 行；`maxapi.h:5425` 声明 `Interface::StopCreating()`；`CADBundle.cpp`/`DllEntry.cpp` 没有面板类或 `BeginEditParams`/`EndEditParams`。
- **影响**：关闭面板后鼠标仍处于创建模式，下一次点击可能意外生成对象；验收 3/5 失败或用户体验严重退化。
- **建议**：`EndEditParams` 调用 `ip->StopCreating()`，清除当前创建类别和按钮状态，销毁 Win32 窗口并把句柄置空；测试“打开→点击创建→关闭面板→重新打开→Esc/右键中止”的完整序列。
- **置信度**：高（需求与缺失实现）；运行时未验证。

#### [P1] P1-10 `.def` 重复且缺少 `CanAutoDefer`，导出契约不完整

- **结论**：`CADBundle.def` 有两个完整的 `EXPORTS` 段，且没有 `CanAutoDefer`；需求第 140 行要求实现 `CanAutoDefer`。
- **证据**：`CADBundle.def:1-20`；`DllEntry.cpp:41-89` 只导出 `LibDescription`、`LibNumberClasses`、`LibClassDesc`、`LibVersion`、`LibInitialize`、`LibShutdown`；`include\plugin.h:58-63` 说明 `CanAutoDefer` 为可选导出，`plugin.h:74-80` 列出必需导出。
- **影响**：重复导出段可能导致链接错误或导出重复；缺少 `CanAutoDefer` 不满足需求；验收 13 的“完整实现”无法成立。
- **建议**：只保留一个 `.def`，加入 `CanAutoDefer`（若需求坚持），并在构建后用 `dumpbin /exports` 核对每个导出名和序号。
- **置信度**：高。

#### [P1] P1-11 几何退化、文字变换与字体回退没有防御性设计

- **结论**：需求要求三点式标注/引线、单击文字/表格，但没有规定 `P0==P1`、`start==elbow`、零长度方向、`textPosition` 与对象变换的优先级、字体缺失时的回退；当前没有对应实现或校验。
- **证据**：
  - 需求第 56–77 行（标注三点、法线/垂直距离）、第 97–113 行（引线三点）、第 79–95 行（文字位置、字号、字体）没有退化/回退规则。
  - 需求第 179 行要求 BezFont 使 `fontSize`/`fontName` 真实生效。
  - `CADBundle.cpp` 没有 `Display`、BezFont、字体选择或点校验代码。
- **影响**：零长度方向可能导致除零/NaN；`textPosition` 与对象 TM 同时使用可能双重变换；字体缺失可能空白或崩溃；验收 6–9 不稳定。
- **建议**：对点距离设最小阈值并跳过退化绘制；明确 `textPosition` 是对象空间点还是对象变换位置，只选一种作为权威；字体缺失时回退默认字体并记录；用极端坐标、零长度、缺失字体测试。
- **置信度**：高（需求缺口）；运行时影响为中。

### 2.4 P2/P3 发现

#### [P2] P2-1 `.slnx` 为空，`OutDir` 被所有配置共用，构建入口和产物来源容易混淆

- **结论**：`CADBundle.slnx` 只有 `<Solution />`，不能作为项目容器；`CADBundle.vcxproj:32` 的 `OutDir` 固定为同一个 `x64\Release\Plugins\`，Debug/Hybrid/Release 会覆盖同一路径；工程只包含两个 `.cpp`。
- **证据**：`CADBundle.slnx:1`；`CADBundle.vcxproj:32`、`:51-52`。
- **影响**：门禁可能构建不到实际项目，或拿到错误配置的产物；新增类文件未加入工程会导致“源码有、产物无”。
- **建议**：门禁显式构建 `CADBundle.vcxproj`，按配置区分输出目录，并把新增的源文件/资源加入工程；记录产物路径与配置。
- **置信度**：高。

#### [P2] P2-2 `GetString` 固定 256 缓冲区、精度/区域设置没有边界测试

- **结论**：`GetString` 使用静态 `TCHAR buf[256]`，长字符串会被截断；`textPrecision` 的 0–6 范围、格式化小数点与区域设置没有测试。
- **证据**：`DllEntry.cpp:91-101` 与 `DllEntry.cpp:193-203`；需求第 67 行定义 `textPrecision` 范围 0–6。
- **影响**：长文字/字体名被截断；某些区域设置下距离文本用逗号小数点，验收文本比较失败。
- **建议**：对长字符串使用安全长度检查或动态缓冲；格式化使用区域无关方式；测试 0/6 位精度、中文、长字符串和不同区域设置。
- **置信度**：中。

#### [P3] P3-1 缺少可复现的数据导出/自动化测试，验收 10 的证据强度不足

- **结论**：需求第 192 行明确项目无自动化测试、无 lint/typecheck；如果没有测试专用的参数/几何导出，验收 10 只能依赖人工读 UI 或截图，无法证明“完整恢复”。
- **证据**：需求第 192 行；项目根目录没有测试工程或测试脚本；`Save`/`Load` 当前为空。
- **影响**：重开场景后“看起来一样”不能证明 P0/P1/P2、`cellTexts`、颜色、精度、字体等字段逐项一致；门禁证据不可审计。
- **建议**：增加只读的测试数据导出（插件调试日志或 MAXScript 可读接口），在保存前、重开后导出同一字段清单并做自动比较；正式版可关闭导出。该建议需要项目负责人确认是否纳入范围。
- **置信度**：高。

---

## 三、问题 1：验收标准 1–13 的可验证性与量化改写

### 结论

[P1] 按原文，验收 3、5、6、7、8、9 含有“出现/进入/正确/即时/恒定/可编辑”等主观词，不能直接作为门禁判据；验收 10 需要额外定义字段清单；验收 11 是静态客观标准但公式不完整；验收 13 只能部分客观化。验收 1、4、12 可以直接静态判定；验收 2 需要依赖检查。

### 逐项改写表

| 编号 | 原判定性 | 建议的可执行判定与阈值 |
|---|---|---|
| 1 编译链接单个 DLL | 客观 | 执行 `msbuild D:\PycharmProjects\CADBundle\CADBundle.vcxproj /p:Configuration=Release /p:Platform=x64`，退出码必须为 0；`D:\PycharmProjects\CADBundle\x64\Release\Plugins\CADBundle.dlo` 存在且唯一；产物 SHA-256 记录。当前基线日志失败，当前为 FAIL。 |
| 2 启动无 ABI/缺失 DLL | 客观（需依赖检查） | 在目标 3ds Max 2027 上启动；无加载错误对话框；插件列表可见；`dumpbin /dependents` 只出现预期 CRT/Max DLL；记录 Max 版本、SDK 版本、工具集和产物哈希。不能只凭“Max 没弹窗”判过。 |
| 3 面板与 4 分类项 | 半客观（视觉） | Utility 列表精确出现 `CADAnnotatePanel`；打开后 4 个按钮标签精确为“标注/文字/引线/表格”；保存全窗口截图；必要时用 OCR 或双人复核。 |
| 4 四类不在创建菜单 | 客观 | 静态检查四个 ClassDesc 的 `IsPublic()` 为 `FALSE`；运行时打开“创建 → 辅助对象”并截图，四个名称均不出现；同时记录 MAXScript 创建仍可用。 |
| 5 点击后交互创建 | 半客观（需固定步骤） | 每类记录对象数 `N`；点击分类按钮；按固定点击序列操作（标注/引线 3 点，文字/表格 1 点）；对象数变为 `N+1`；`classof`/`GetClassName` 匹配；最后一点后创建模式退出。 |
| 6 `showXYZ` 与屏幕恒定 | 原文主观 | 拆为 6a/6b。6a：固定相机和视口 1280×720，切换 `showXYZ`，裁剪 P0/P1/P2 标签区域，要求标签出现/消失且像素差超过约定阈值。6b：在 1×/2×/4× 缩放下截图，测量箭头、文字高度、延伸线像素长度，要求极差 ≤2 px 或相对偏差 ≤5%（阈值需验收人确认）。 |
| 7 文字刷新与无 `ITextObject` | 半客观 | 7a：分别修改 `textString`、`fontSize`、`textColor`、`textPosition`，每次提交后 ≤1 次重绘或 500 ms 内截图发生变化，无需手动刷新。7b：静态检索 `public\s+ITextObject` 必须无命中。 |
| 8 引线交互、绘制、恒定 | 原文主观 | 三次点击使用已知坐标；断言 `start`/`elbow`/`textPos`；检查两段线端点、`start` 处箭头方向、`textPos` 处 `note`；屏幕恒定按 6b 协议测量。 |
| 9 表格网格、调整、编辑、恒定 | 原文主观 | 断言 `rows+1` 条水平线、`cols+1` 条垂直线；`cellTexts` 数量 = `rows×cols`；修改行列后按约定的 `(row,col)` 映射检查；通过指定 UI 编辑一个单元格并回读；屏幕恒定按 6b 协议。 |
| 10 保存重开完整恢复 | 客观（前提是数据可导出） | 固定四个对象及全部字段；保存 `A.max`；Reset/新建场景；重新打开；导出同一字段清单；逐项比较（浮点容差 1e-4，字符串精确），并再次保存/重开。没有数据导出则只能算人工观察，不能算强证据。 |
| 11 参数×`NonScalingObjectSize()` | 客观（静态）但公式错误 | 静态扫描所有 `Display()`；同时必须通过 6b 的运行时像素测量。仅满足字面公式不能判过，需先修订公式加入视口宽度因子。 |
| 12 连续线使用 `DrawLineProc` | 客观（静态） | 扫描所有线段绘制调用；主要连续线必须经 `DrawLineProc`；不得以 `GraphicsWindow::DrawLine`/`DrawRectangle` 为主；代码评审确认。 |
| 13 无骨架/TODO/省略 | 部分客观 | 检索 `TODO`、`#pragma message`、`To be implemented`、`return nullptr` 等占位；建立“类/方法/行为”覆盖矩阵；不能仅凭 grep 无命中证明“核心逻辑没有省略”。 |

### 关键建议

- 将 3/5/6/7/8/9 的“出现/进入/正确/即时/恒定/可编辑”替换为上述步骤、截图/数据证据和阈值。
- 验收 11 必须与运行时测量绑定，否则会与 P0-5 的公式缺陷冲突。
- 验收 13 必须增加功能矩阵和代码评审记录，不能只依赖文本搜索。

---

## 四、问题 2：Save/Load 中几何、参数块、`TYPE_STRING_TAB`、Class_ID 与版本兼容的失配风险

### 结论

[P0] 当前没有任何持久化实现；`Save`/`Load` 空实现、`pblock` 未构造、引用未建立，属于确定性数据丢失路径。`TYPE_STRING_TAB` 的自动持久化行为未经运行验证，不能假定；需要在设计上提供显式 chunk 或加载后校验。

### 失配点清单

| 数据/机制 | 当前证据 | 可能后果 |
|---|---|---|
| 几何关键点 P0/P1/P2、`start/elbow/textPos`、插入点 | `CADBundle.cpp:26-27`、`:133-134` 空 `Save`/`Load` | 重开得到零值/默认值；位置丢失 |
| `ParamBlock2` 参数 | `pblock` 未初始化（`:23`、`:130`），构造函数为空（`:52`、`:159`），无参数块描述/版本 | 创建/编辑空指针；参数未持久化；无法迁移 |
| `TYPE_STRING_TAB` `cellTexts` | 无 `TYPE_STRING_TAB`、`SetCount`、`ZeroCount` 代码；`paramtype.h:153`、`iparamb2.h:1286/1291` 仅说明 API 存在 | 单元格数量/顺序不一致；串格、丢格；字符串表重分配后旧指针失效 |
| Class_ID | `CADBundle.cpp:15` 与 `:122` 冲突；旧 DLO 四个类名 | 旧场景无法解析；新旧插件类归属不确定 |
| 引用与 `Clone` | 无 `NumRefs`/`SetReference`/`GetReference`；`Clone` 返回 `nullptr`（`:83-87`、`:190-194`） | 参数块生命周期错误；复制/粘贴/撤销崩溃或丢参数 |
| 版本迁移 | 无 `P_VERSION`、无 chunk 版本、无 `ParamBlock2PLCB` | 后续字段增删/重排造成旧场景错位 |
| 文字位置 | 需求同时出现 `textPosition` 参数和“对象变换位置”（第 87 行） | 双重变换或保存/加载位置不一致 |
| 损坏/截断文件 | 无长度校验、无 chunk 解析、无错误路径 | 越界读取、崩溃或静默使用错误数据 |

### 建议的防御性设计

1. **冻结 Class_ID**：每个类一个唯一值，首次发布后不再改变；先确认旧 DLO 的 Class_ID 与旧场景兼容策略。宏和重复类必须只保留一份。
2. **参数块先于持久化**：构造函数中调用 `MakeAutoParamBlocks(this)` 或等价方式创建 `ParamBlockDesc2`；用 `P_VERSION` 标记参数版本；用 `ParamBlock2PLCB` 处理旧版本迁移。SDK 参考：`iparamb2.h:42-54`、`:283`、`:4386`、`:4411-4432`；`tapehelp.cpp:302` 的 `ReplaceReference(0, CreateParameterBlock(...))`；`text.cpp:2099-2241` 的 chunk + `ParamBlockPLCB` 示例。
3. **显式几何 chunk**：采用 `ISave::BeginChunk/Write/EndChunk` 和 `ILoad::OpenChunk/CurChunkID/Read/CloseChunk`。建议定义独立 chunk：
   - 类/几何版本：`DWORD`；
   - 几何点：P0/P1/P2 或 `start`/`elbow`/`textPos`，以及文字/表格插入点；
   - 表格维度：`rows`、`cols`、`cellCount`；
   - 单元格字符串：每个字符串写长度前缀再写字节，避免以空字符分隔造成截断/越界；
   - 未知 chunk：跳过；结构损坏：返回 `IO_ERROR` 并保持对象默认状态，不能崩溃。
   SDK 参考：`tapehelp.cpp:1094-1124`；`text.cpp:2099-2241`。
4. **`TYPE_STRING_TAB` 的单一权威**：先做一次保存/重开实验，确认参数块是否完整恢复字符串表。若不能确认，明确以显式表格 chunk 为准，加载后同步到 `pblock`；不要让“自动持久化”和“显式 chunk”各自覆盖对方。`paramtype.h:153` 只证明类型存在，不证明当前实现的行为。
5. **行列变化按坐标迁移**：不要用线性 `idx` 直接复制。定义旧/新 `(row, col)` 交集保留、新增为空、超出新边界的单元格丢弃并记录；`SetCount(newRows*newCols)` 后重新取指针再填充；`rows`/`cols` 连续变化时合并一次重算并只刷新一次。
6. **加载后校验**：`cellCount` 必须等于 `rows*cols`；`rows`/`cols` 限制在 1–50；字符串长度设上限；点坐标拒绝 NaN/Inf；精度限制 0–6；不合法数据修复并记录，结构性损坏返回 `IO_ERROR`。
7. **保存/重开测试矩阵**：
   - 四个对象各一份，含边界坐标、空字符串、中文/长字符串、0/6 位精度、RGBA 边界；
   - 表格 1×1、2×3、3×2、50×50，修改行列后检查 `cellTexts`；
   - 保存 → Reset → 打开 → 字段逐项比较 → 再次保存/重开；
   - 截断/未知 chunk 测试；
   - 旧 DLO 生成的场景兼容性测试（若保留旧场景要求）；
   - 复制/粘贴/撤销/重做测试 `Clone` 与引用。
8. **版本兼容结论**：`P_VERSION` 只能处理参数块字段迁移，不能解决 Class_ID 变化。Class_ID 变化会使旧场景在类解析阶段就失败；必须在发布前决定“复用旧 ID + 迁移”还是“新 ID + 保留旧 DLO”，并据此测试。
