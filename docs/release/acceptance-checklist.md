# 验收清单与证据包（CADBundle 0.1.0 Unreleased）

> 状态：**待人工运行验证**。任何未执行项必须写“未验证”，不得写成通过。

## 1. 基线

| 项目 | 值 |
|---|---|
| 分支 | `ai-review-fix-2026-11-18` |
| 发布候选提交 | `889cc02`（B7）之后的工作树构建；最终合并前以提交 SHA 为准 |
| 构建 | Release x64，`MSBuild -t:Rebuild`，无 warning/error |
| 产物 | `build/x64/Release/plugins/CADBundle.dlo`，164,864 字节 |
| SHA-256 | `0191f6540cd4067f9eb604a05d9d25e6b792f61522488789784d518c9c740c1b` |
| 导出 | LibDescription / LibNumberClasses / LibClassDesc / LibVersion / LibInitialize / LibShutdown / CanAutoDefer，全部正确 |
| 架构/依赖 | x64；core/Geom/maxutil/ParamBlk2 + CRT；无 Qt |
| V1 阈值 | 屏幕恒定允许偏差 ≤ max(2 px, 5%)（已裁决） |

## 2. 验收判据映射（A1–A13）

| 编号 | 判据 | 方法/测试 | 结果 |
|---|---|---|---|
| A1 | 单 DLL、5 ClassDesc、7 导出 | 静态 + 第 8 步证据包 | 待归档 |
| A2 | 可加载、Utility 列表出现“CAD 注释” | 安装后启动 Max | 未验证 |
| A3 | 面板 4 按钮、状态行、关闭清理 | T3/T4 | 未验证 |
| A4 | 对象不在创建下拉暴露（IsPublic=FALSE） | 创建面板逐项检查 | 未验证 |
| A5 | 四类对象可由面板创建 | T4/T7/T9/T10（含 B-21 点序） | 未验证 |
| A6 | 屏幕恒定（箭头/文字/延伸线/表格） | T6，1×/2×/4×，3 次，V1 判据 | 未验证 |
| A7 | 文字：字体/字号/颜色生效，回退可见 | T7/T8 | 未验证 |
| A8 | 引线：折线/箭头/注释，坐标 1e-4 一致 | T9 | 未验证 |
| A9 | 表格：网格、编辑器、行列迁移 | T10–T13 | 未验证 |
| A10 | 保存→Reset→重开逐字段一致；2×3 实验 | T14/T15/T16 | 未验证 |
| A11 | 屏幕恒定公式为完整式 | 静态（与 A12 交叉） | 静态通过 |
| A12 | 连续线经 DrawLineProc/polyline；无 DrawLine/DrawRectangle | 静态扫描 | 静态通过（0 处） |
| A13 | 完整性：无 TODO/占位、资源 ID 一致、行数约束 | 静态 | 静态通过 |

完整 T1–T21 矩阵与回滚模板见 `docs/development/07-test-release.md` §9/§10。

## 3. 证据包模板

每次运行验证建立独立目录（不入库），至少包含：

```
evidence/<日期>/
  environment.txt          # Max 版本、系统版本、DPI、显示器分辨率、显卡
  build.txt                # 构建命令、日志尾部、产物 SHA-256
  screenshots/             # 按测试编号命名：T03-panel.png、T06-1x.png ...
  measurements.csv         # 像素测量原始数据
  fields.csv               # 保存/重开逐字段比较
  notes.md                 # 偏差、失败步骤、复现条件
  test.max                 # 保存的测试场景（非生产数据）
```

**measurements.csv** 表头：

```
test_id,view,zoom,tool,element,expected_px,measured_px,deviation_px,deviation_pct,repetition,pass
```

**fields.csv** 表头：

```
object_type,object_name,field,value_before,value_after,tolerance,pass
```

- 浮点比较容差 1e-4；`cellTexts` 逐格列出完整字符串；2×3 实验使用六个唯一字符串。
- 坐标/角度/箭头方向按 07 §9 B-17 判据（坐标 1e-4；箭头 dot ≥ 0.999；note 精确一致）。

## 4. 运行验证步骤（第 7/11 步）

1. **备份旧插件（条件执行）**：若 `<Max>\plugins\CADDimension.dlo` 存在，复制到 plugins 之外的备份目录并记录 SHA-256/大小；不存在则记录“不存在”。
2. **安装**：复制 `CADBundle.dlo` 到 `<Max>\plugins`。
3. **启动与加载**：验证 A2/A3，截图面板。
4. **创建与交互**：按 T4/T7/T9/T10 执行；**B-21 点序**：1 次点击应为 point 0 提交，3 次点击为 point 0/1/2，最后一点 STOP；若实际不同，**立即停止并回报**，不得自行改点序或记为通过。
5. **屏幕恒定**：T6，固定 1280×720、透视+正交、1×/2×/4×，各 3 次，记录原始像素。
6. **保存/重开**：T14（四类对象逐字段）、T15（2×3 表格）、T16（损坏 chunk 不崩溃）。
7. **中止/关闭/撤销/复制**：T17/T18。
8. **恢复旧插件（若备份过）**：删除新 DLO，恢复备份，核对 SHA-256/大小一致（B-06）。
9. **回滚（失败时）**：按 `docs/development/07-test-release.md` §10 三段模板执行；先报告冲突/失败原因，不擅自 `reset --hard`。

## 5. 已知未验证项（不得写成通过）

- DLL 运行时加载、ABI、Utility 列表显示。
- 四类对象创建、点序（B-21）、中止清理。
- 面板/参数对话框/ColorSwatch/单元格编辑器在 Max 中的实际行为。
- BezFont 字形、CJK 回退、billboard 朝向。
- 屏幕恒定 1×/2×/4× 像素测量（V1 判据已定，数据未采）。
- T14/T15/T16 保存重开与损坏 chunk。
- DPI 100/125/150/200%、字体缺失、空文本/长文本。
- 克隆/复制/撤销/重做、旧 DLO 共存/恢复。
- 旧 `.max` 场景兼容（v1 不承诺）。

## 6. 定版条件（流程 11.6）

人工运行验证通过后，才在独立发布分支完成：

1. 更新版本号与 VERSIONINFO；
2. 更新 `CHANGELOG.md`（去除 Unreleased，记录验证证据路径与产物 SHA-256）；
3. 经合并流程进入主干。
