# 构建、测试、验收与发布

## 1. 构建与静态检查

### 1.1 构建命令

```bash
MSYS_NO_PATHCONV=1 "/d/Program Files/Microsoft Visual Studio/18/Enterprise/MSBuild/Current/Bin/MSBuild.exe" \
  CADBundle.vcxproj -p:Configuration=Release -p:Platform=x64 -m -v:minimal
```

- 退出码必须为 0；仅生成一个 `CADBundle.dlo`；记录产物路径、大小、时间、SHA-256。
- 保留完整构建日志；新增警告需逐条处理或书面说明。

### 1.2 产物检查

- `dumpbin /headers`：x64 PE DLL。
- `dumpbin /exports`：LibDescription、LibNumberClasses、LibClassDesc、LibVersion、LibInitialize、LibShutdown、CanAutoDefer。
- `dumpbin /dependents`：仅预期 CRT/Max 依赖；无 Qt 依赖。
- 静态检查：`LibNumberClasses=5`；5 个 ClassDesc 索引正确；四个对象 `IsPublic=FALSE`、Utility `TRUE`；两个旧 Class_ID 宏不再出现；无 TODO/`#pragma message`/`To be implemented` 占位；`DrawLineProc` 连续线；箭头闭合；`NonScalingObjectSize * GetVPWorldWidth / 360` 出现于每个 Display。
- 资源审计：`IDS/IDC/IDD` 无重复/未定义；所有 `p_ui` 引用存在；UTF-8 资源编译通过。

## 2. 运行前备份与安装

1. 关闭 3ds Max。
2. 计算旧插件哈希：`D:\Program Files\3ds Max 2027\3ds Max 2027\plugins\CADDimension.dlo`（158,720 字节，SHA-256 `92b62a52fa288deafc0405df6d1dda4dee1756ac784382359c52969adffb963a`，2026-11-17 10:57）。复制到 plugins 外的备份目录，不删除。
3. 将旧 DLO 从 plugins 移出；记录移出前后哈希。
4. 复制新 `CADBundle.dlo` 到 plugins；复制前必须再次取得用户确认。
5. 启动 3ds Max 2027；记录版本、插件列表、加载错误（若有）。
6. 测试结束后：移除新 DLO；按哈希恢复旧 DLO；核对大小/哈希；重启确认旧类恢复、新类消失。

## 3. 测试矩阵

| 编号 | 场景 | 步骤 | 预期 |
|---|---|---|---|
| T1 构建 | Release x64 | 构建命令 | 退出码 0；单 `.dlo`；导出/依赖正确 |
| T2 加载 | 新 DLO 单独在 plugins | 启动 Max | 无加载错误；Utility 出现；创建菜单无四类 |
| T3 面板 | 打开/关闭/重开 10 次 | 点击 Utility 条目 | 单实例；无残留创建模式、无崩溃 |
| T4 标注创建 | 点击“标注” | 三点：P0、P1、P2 | 对象生成；尺寸线/延伸线/箭头/距离文本正确 |
| T5 标注参数 | 修改 showXYZ/箭头/文本高度/延伸/颜色/精度 | 修改面板 | 即时刷新；showXYZ 开关正确；精度 0/6 |
| T6 屏幕恒定 | 固定相机/视口 | 1×/2×/4× | 箭头/文本/延伸线像素极差 ≤2 px 或 ≤5%（V1） |
| T7 文字 | 单击放置；改文本/字号/字体/颜色/位置 | 修改面板 | 500 ms 内刷新；中文字形；无 `ITextObject` |
| T8 字体缺失 | 输入不存在字体 | 修改面板 | 回退默认/CJK；提示；不崩溃 |
| T9 引线 | 点击“引线” | 三点 | 折线/箭头/note 正确；屏幕恒定 |
| T10 表格创建 | 点击“表格” | 单击插入 | rows+1 水平线、cols+1 垂直线 |
| T11 表格编辑 | 打开“编辑单元格…” | 改 A1/B2/中文/长文本/空串 | 网格即时显示；确定/取消正确 |
| T12 行列迁移 | 2×3→3×2、1→50、50→1 | 修改 rows/cols | 按 (row,col) 交集保留；新增空；超出丢弃；不崩溃 |
| T13 50×50 | 50 行 50 列 | 创建/编辑/保存/重开 | 无崩溃；性能可接受（阈值项目组定） |
| T14 持久化 | 四类对象 + 参数 + 表格文本 | 保存→Reset→重开 | 逐字段一致；cellTexts 逐格一致；浮点 1e-4 |
| T15 2×3 实验 | 六格唯一字符串 | 保存→Reset→重开 | 全部一致；否则 chunk 权威规则回退 |
| T16 损坏 chunk | 截断/非法 chunk | 打开场景 | 返回 IO_ERROR/默认状态；不崩溃 |
| T17 撤销/复制 | 创建/修改/复制/撤销/重做 | 标准操作 | 无重复节点/悬挂引用/崩溃 |
| T18 中止/关闭 | 创建中 Esc/右键/关闭面板 | 固定序列 | 不生成对象；模式清理；重开空闲 |
| T19 旧 DLO | 移出/恢复 | 两次启动 | 无类冲突；恢复后旧类可用 |
| T20 回滚 | 移除新 DLO，恢复旧 DLO | 按第 2 节 | 哈希一致；启动正常 |

## 4. 验收标准映射

- A1→T1；A2→T2；A3→T3；A4→T2/静态；A5→T4/T7/T9/T10；A6→T6；A7→T7/T8；A8→T9；A9→T10–T13；A10→T14/T15/T16；A11→T6 + 静态；A12→静态；A13→静态 + 失败用例。
- 静态检查可作证据，但不得替代运行时验收；共享引用不等于独立运行验证。
- 视觉“正确/美观”只能人工确认并标“人工观察”，不写成自动化通过。

## 5. 证据包

- 构建命令、完整日志、退出码、工具集/SDK/Max 版本。
- 产物 SHA-256、`dumpbin /headers|exports|dependents` 原始输出。
- 静态扫描原始输出（ClassDesc、IsPublic、TODO、DrawLineProc、屏幕公式、Save/Load）。
- 运行时截图：Utility 列表、面板 4 按钮、四类对象、表格编辑器、showXYZ、字体回退、DPI。
- 保存的 `.max` 路径/哈希；保存前/重开后字段清单与比较结果；`cellTexts` 完整列表。
- 1×/2×/4× 像素测量原始数据。
- 旧 DLO 哈希/大小/时间、备份路径、恢复后哈希。
- 未验证项清单与已知风险；不得把未运行项写成通过。

## 6. 发布包与版本

- 发布包：`CADBundle.dlo`、README、BUILD、CHANGELOG、安装/卸载说明、验收清单、SHA-256。
- 版本号与 CHANGELOG 在用户人工验证通过后，于独立发布分支 `chore/release-<版本号>` 更新，按第 10 步合并；不直接改主干，不与其他改动混合。
- 不执行 `git push`；推送由用户手动执行。
- 中间循环不重复更新版本号。

## 7. 失败条件与循环上限

- B0 构建连续 3 轮失败；`StartCreatingObject` 无法创建非公开对象；BezFont 不可用；表格编辑器无法完成；屏幕恒定超阈值；保存/重开丢字段；旧 DLO 冲突；无 Max 运行环境；U1–U10 被证明不可行。
- 同一问题最多 3 轮；全任务累计最多 9 轮；当前累计 6 轮（第 2/3 步），第 4 步未消耗返工轮次。
- 连续 2 轮无可验证进展立即停止，汇报阻塞点；不得用新子问题绕过计数器。

## 8. 人工验证清单（用户执行）

1. 备份/移出旧 DLO；安装新 DLO；启动 Max。
2. 打开面板，重复开关；检查 4 按钮与状态。
3. 四类对象各创建 1 个；检查显示与参数。
4. 标注 showXYZ/精度/颜色；文字字号/字体/颜色/位置；引线三点；表格 2×3 与 50×50。
5. 1×/2×/4× 缩放；记录像素。
6. 保存/重开；逐字段核对；表格逐格核对。
7. 中止/关闭/重开/撤销/复制。
8. 移除新 DLO，恢复旧 DLO；确认回滚。
9. 将异常现象、截图、命令输出反馈给项目经理；按严重程度决定回退到第 6/4/2 步。

> 本文件与 `00-plan.md` 共同构成第 4 步可执行方案；第 5 步项目总监独立评审通过后才进入第 6 步。
