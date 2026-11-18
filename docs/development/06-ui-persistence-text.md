# Utility 面板、资源、持久化与文本公共设计

## 1. Utility 面板 CADAnnotatePanel

- 基类 `UtilityObj`；独立 `ClassDesc2`；`UTILITY_CLASS_ID`；`IsPublic()=TRUE`；Class_ID `0xb8266181, 0x1d05484b`；Category “CAD Tools”。
- `BeginEditParams(Interface*, IUtil*)` 创建单实例 Win32 浮动对话框（`CreateDialogParam`，父窗口为 3ds Max 主窗口），非 rollup；`EndEditParams` 销毁并置空。
- 面板只有 4 个分类按钮 + 说明 + 状态行 + 关闭，不承载对象参数。
- 按钮：标注/文字/引线/表格；点击调用 `GetCOREInterface7()->StartCreatingObject(desc)`；不承诺 MAXScript。
- 状态机：`Idle`、`Creating(class)`、`Closing`；用单一 `HWND` + `IsWindow()` 守卫。
- 同一按钮重复点击：忽略并提示；创建中点击另一分类：先 `StopCreating`，再启动新分类；失败显示本地化错误并回到 Idle。
- `WM_CLOSE`/`EndEditParams`：若 `IsCreatingObject()`，在回调外调用 `StopCreating()`；清理按钮状态；销毁窗口；重开为空闲。
- 用 `IsCreatingObject()` 在 `WM_ACTIVATE`/轻量定时器同步状态；不假设面板拥有焦点。
- 面板关闭后再次打开不得残留创建模式；测试“打开→创建→关闭→重开→Esc/右键中止”。
- 中文 UI；UTF-8 资源；CJK 字体回退；高 DPI 使用 `GetUIScaleFactor`/`WM_DPICHANGED`（V2）。

## 2. 资源 ID 规划（新 `res/resource.h` 单份）

### 2.1 字符串

| ID | 值 | 用途 |
|---|---|---|
| IDS_LIBDESCRIPTION | 100 | “CAD 注释工具集” |
| IDS_CATEGORY | 101 | “CAD Tools” |
| IDS_UTILITY_CLASS | 102 | “CAD 注释面板” |
| IDS_DIM_CLASS | 103 | “CAD 标注” |
| IDS_TEXT_CLASS | 104 | “CAD 文字” |
| IDS_LEADER_CLASS | 105 | “CAD 引线” |
| IDS_TABLE_CLASS | 106 | “CAD 表格” |
| IDS_PANEL_TITLE | 107 | “CAD 注释” |
| IDS_BTN_DIM/TEXT/LEADER/TABLE | 108–111 | 4 个按钮 |
| IDS_DESC_DIM/TEXT/LEADER/TABLE | 112–115 | 按钮说明 |
| IDS_STATUS_READY/CREATING/DONE/ABORTED/ERROR | 116–120 | 状态行 |
| IDS_PARAMS | 121 | “参数” |
| IDS_TABLE_EDIT | 122 | “编辑单元格…” |
| IDS_FONT_FALLBACK | 123 | “字体不可用，已回退” |
| IDS_DEFAULT_TEXT/NOTE | 124–125 | 默认文字/引线注释 |
| IDS_ERR_CREATE/FONT/TABLE | 126–128 | 错误提示 |
| IDS_ACCEPT/CANCEL/CLOSE | 129–131 | 按钮 |

### 2.2 对话框

| ID | 值 | 用途 |
|---|---|---|
| IDD_CAD_PANEL | 200 | 浮动工具集面板 |
| IDD_CAD_DIM | 201 | CADDimension 参数 |
| IDD_CAD_TEXT | 202 | CADText 参数 |
| IDD_CAD_LEADER | 203 | CADLeader 参数 |
| IDD_CAD_TABLE | 204 | CADTable 参数 |
| IDD_CAD_TABLE_EDITOR | 205 | 表格单元格编辑 |

### 2.3 控件（唯一，不复用旧 1000/1456/1490/1496）

| 范围 | 用途 |
|---|---|
| IDC_PANEL_BTN_DIM..TABLE 1001–1004 | 4 个分类按钮 |
| IDC_PANEL_STATUS 1005 / CLOSE 1006 / DESC_* 1007–1010 | 面板状态/关闭/说明 |
| IDC_DIM_P0X..P2Z + SPIN 1100–1117 | 三点 Point3 |
| IDC_DIM_SHOWXYZ 1118；ARROW 1119–1120；TEXTH 1121–1122；EXTOFF 1123–1124；LINECOL 1125；TEXTCOL 1126；PREC 1127–1128 | 标注参数 |
| IDC_TEXT_POSX..Z + SPIN 1200–1205；STRING 1206；SIZE 1207–1208；FONT 1209；COLOR 1210 | 文字参数 |
| IDC_LEAD_STARTX..TEXTPOSZ + SPIN 1300–1317；NOTE 1318；ARROW 1319–1320；TEXTH 1321–1322；LINECOL 1323；TEXTCOL 1324 | 引线参数 |
| IDC_TABLE_POSX..Z + SPIN 1400–1405；ROWS 1406–1407；COLS 1408–1409；CW 1410–1411；CH 1412–1413；EDIT 1414；LINECOL 1415；TEXTCOL 1416 | 表格参数 |
| IDC_TABLE_EDIT_LIST 1500；CELL 1501；ROWS 1502–1503；COLS 1504–1505；APPLY 1506；CANCEL 1507；STATUS 1508 | 表格编辑器 |

- `.rc` 使用 `#pragma code_page(65001)`，`LANGUAGE LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED`；保留英文回退资源或英文 `NonLocalizedClassName`。
- 资源审计：编译前用脚本检查重复 ID/未定义 ID；所有 `p_ui` 引用的控件必须存在。
- 旧 `.rc`/`resource.h` 的重复块和冲突 ID 全部重建，不复用旧值。

## 3. 持久化公共规则

- 参数：pblock 自动持久化（颜色、布尔、浮点、整数、字符串、Point3 镜像、字符串表镜像）。
- 几何点与 `cellTexts`：显式自定义 chunk 为唯一权威；pblock 镜像只用于编辑/运行时。
- 加载顺序：系统先恢复 pblock 作为镜像，再读取 chunk；chunk 值覆盖镜像；表格 rows/cols/cellTexts 在 chunk 读取后统一回填；无 chunk 时用 pblock 默认/镜像；pblock 不得在 chunk 之后反向覆盖；实现文档统一此顺序（B-12）。
- 禁止无优先级双写：运行时修改只写 pblock（镜像），保存时写 chunk；加载后以 chunk 回填镜像。不得在加载后用 pblock 覆盖 chunk。
- 版本：每类 chunk 有 DWORD 版本；pblock 用 `P_VERSION` + `ParamBlock2PLCB`（如布局变化）。
- 字符串表：长度前缀 + 字符数据，Unicode/UTF-8 安全；校验 `cellCount==rows*cols`、rows/cols 1–50、长度上限。
- 点：拒绝 NaN/Inf；退化点允许但显示跳过。
- 损坏：未知 chunk 跳过；结构损坏返回 `IO_ERROR`，保持默认状态，不崩溃。
- 2×3 表格“保存→Reset→重开”实验作为验收 10 前置；若失败，回到设计，不改权威规则。

### 3.1 字段级权威矩阵（B-12）

| 数据 | pblock | 显式 chunk | 权威 | 加载顺序 |
|---|---|---|---|---|
| rows/cols | 自动保存 | 表格 chunk 记录 | chunk 覆盖 pblock | 先读 chunk，再设置 rows/cols 与 cellTexts |
| insertPos | TYPE_POINT3 镜像 | 表格 chunk | chunk | chunk 回填镜像 |
| cellTexts | TYPE_STRING_TAB 镜像 | 表格 chunk | chunk | 先清空/SetCount，再逐格回填 |
| cellWidth/cellHeight/颜色 | 自动保存 | 无 | pblock | 系统恢复 |
| 标注/引线关键点 | TYPE_POINT3 镜像 | 点 chunk | chunk | chunk 回填镜像 |
| textPosition | TYPE_POINT3 镜像 | 文本 chunk | chunk | chunk 回填镜像 |
| textString/fontSize/fontName/textColor | 自动保存 | 无 | pblock | 系统恢复 |
| showXYZ/precision/arrowSize/textHeight/extensionOffset/note | 自动保存 | 无 | pblock | 系统恢复 |

规则：保存时 chunk + pblock 自动写入；加载时 chunk 覆盖镜像；不得用 pblock 反向覆盖 chunk；运行时只改 pblock 镜像。

| Chunk | 值 | 内容 |
|---|---|---|
| kChunkDimVersion / Points | 0x0F01 / 0x0F02 | 版本 + P0/P1/P2 |
| kChunkLeaderVersion / Points | 0x0F03 / 0x0F04 | 版本 + start/elbow/textPos |
| kChunkTextVersion / Position | 0x0F05 / 0x0F06 | 版本 + textPosition |
| kChunkTableVersion / Data | 0x0F07 / 0x0F08 | 版本 + rows/cols/insertPos + cellTexts |

## 4. BezFont 文本公共设计

- `theBezFontManager.OpenFont(name,0)`；失败回退默认/CJK；`FontExists`/`GetFontInfo` 可用。
- `BuildCharacterEx(handle, charCode, 1000.0f, shape, width, 1, false)` 生成字形；按 `fontSize*s/1000` 缩放。
- 缓存键：字体名 + 字符码 + 字形版本；文本/字号变化只清布局缓存，字体变化关闭旧句柄并重开。
- Billboarding：用 `ViewExp::GetAffineTM(viewTM)` 取得视图轴，把字形轮廓放到面向视线的平面；保证透视图可读。
- 绘制：字形样条采样为折线，经 `DrawLineProc`/`gw->polyline` 绘制；颜色 `TEXT_COLOR`/`textColor`；这是对“连续线用 DrawLineProc”的文本特例。
- 缺字形：跳过或绘制占位框；不崩溃；一次性提示。
- 对象析构/字体切换关闭句柄；`LibShutdown` 不强制关闭存活对象字体。
- 表格单元格文本不经过 BezFont，使用 `GraphicsWindow::text`，屏幕固定字体；单元格最大 256 字符，显示超宽省略、存储完整。
- 表格完全屏幕恒定（U7）与 CAD 模型空间表格习惯不同；README/交付说明注明。
- DPI 用例见 T21（100/125/150/200%）；验收 3 同时记录显示名与 `NonLocalizedClassName`。

## 5. 屏幕缩放与 billboard

- `ScreenScale(vpt, tm) = NonScalingObjectSize() * GetVPWorldWidth(tm.GetTrans()) / 360.0`。
- 标注/引线/表格/文字高度、箭头、延伸线、单元格宽高全部乘 `s`。
- 文字用视图轴 billboard；表格网格在对象 XY 平面；表格文本屏幕对齐。
- 1×/2×/4× 像素测量；阈值 V1 最终确认；仅静态字面公式不判通过。

## 6. 面板与参数 UI 的交互

- 面板按钮只负责进入创建模式；参数在对象修改面板调整。
- 表格“编辑单元格…”按钮通过 `ParamMap2UserDlgProc` 捕获；打开 `IDD_CAD_TABLE_EDITOR`；确定后写 pblock 镜像并触发重绘；取消不修改。
- 文本/字体/颜色/精度修改由 pblock 自动 UI 触发 `NotifyRefChanged`，对象失效缓存并重绘。
- 面板关闭、切换分类、Max 退出、创建中 Esc/右键：统一清理；不得在创建回调内调用 `StopCreating`。
