# CAD 注释工具集（CADBundle）

3ds Max 2027 插件：编译为单个 Plugin Bundle（`CADBundle.dlo`），注册 1 个 Utility 面板与 4 个非公开 HelperObject。

## 功能

- **CAD 注释 Utility 面板**：4 个分类按钮，下拉为标注、文字、引线、表格；状态行显示创建模式。
- **CADDimension**：三点线性标注（起点、终点、尺寸线参考点）；延伸线、闭合三角箭头、精度 0–6、可选坐标标签、RGB 颜色。
- **CADText**：单击放置文字；BezFont 真实字形与字号；字体缺失时回退默认/CJK 字体；billboard 面向视图。
- **CADLeader**：三点引线（起点、拐点、文本位置）；折线、闭合三角箭头、注释文本。
- **CADTable**：单击插入表格；行列 1–50、单元格宽高、RGB 颜色；修改面板内"编辑单元格…"提供网格编辑器（行列迁移按交集保留）。

4 个对象类 `IsPublic() = FALSE`：不在"创建 → 辅助对象"下拉中暴露，只能经面板按钮（`StartCreatingObject`）创建。

## 环境要求

- 3ds Max 2027 x64
- Visual Studio 2022 或更新版本（v143 工具集）、C++20、Windows SDK 10.0.19041.0
- 3ds Max 2027 SDK（默认 `D:\3dsMaxSDK2027\maxsdk`）
- 不需要 Qt 开发包（参数 UI 使用 Win32 `P_AUTO_UI`）

## 构建

见 [BUILD.md](BUILD.md)。产物：`build\x64\Release\plugins\CADBundle.dlo`。

## 安装

1. 关闭 3ds Max。
2. 若 `<3ds Max 2027>\plugins` 中存在旧 `CADDimension.dlo`，先备份并记录 SHA-256（步骤见 [docs/release/acceptance-checklist.md](docs/release/acceptance-checklist.md)）。
3. 复制 `CADBundle.dlo` 到 `<3ds Max 2027>\plugins`。
4. 启动 3ds Max → 工具（Utilities）→ 选择 **CAD 注释**。

卸载：删除 `CADBundle.dlo`；如曾备份旧插件，按备份恢复。

## 使用

| 工具 | 交互 | 参数入口 |
|---|---|---|
| 标注 | 依次点击起点、终点、尺寸线位置 | 修改面板 |
| 文字 | 单击放置 | 修改面板（内容/高度/字体/颜色） |
| 引线 | 依次点击起点、拐点、文本位置 | 修改面板（注释/箭头/文字高度/颜色） |
| 表格 | 单击插入 | 修改面板（行列/单元格宽高/颜色/“编辑单元格…”） |

## 已知限制

- 仅视口注释，不参与渲染、打印或导出（用户裁决 U1）。
- 标注为对齐线性标注；`showXYZ` 默认关闭；颜色为 RGB，无透明度（U2）。
- 表格为屏幕恒定尺寸；单元格文本使用屏幕固定字体（U7/U10）。
- 创建入口仅支持面板按钮；MAXScript 创建未验证，不作为验收条件（U3）。
- 旧场景兼容不在 v1 范围（U8）。

## 版本状态

**0.1.0 — Unreleased（待人工运行验证）**。按流程要求，版本号与 VERSIONINFO 资源仅在人工验证通过后统一更新，本阶段不声明已发布。

## 文档索引

- [BUILD.md](BUILD.md)：构建、校验与常见问题
- [CHANGELOG.md](CHANGELOG.md)：变更记录（Unreleased）
- [docs/release/acceptance-checklist.md](docs/release/acceptance-checklist.md)：验收清单、证据包模板、运行验证指引
- [docs/development/00-plan.md](docs/development/00-plan.md)：可执行方案
- [docs/development/07-test-release.md](docs/development/07-test-release.md)：测试矩阵与回滚模板
- [docs/reviews/loop-ledger.md](docs/reviews/loop-ledger.md)：过程账本
