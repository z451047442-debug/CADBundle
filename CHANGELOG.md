# Changelog

本项目遵循“人工验证通过后才定版”的流程：当前为 Unreleased，版本号与 VERSIONINFO 资源在最终验证后统一更新。

## 0.1.0 — Unreleased（待人工运行验证）

### Added

- **Utility 面板** `CADAnnotatePanel`：Win32 非模态面板，4 个分类按钮（标注/文字/引线/表格）、状态行、关闭清理；类显示名 `CADAnnotatePanel`。
- **CADDimension**：三点线性标注；ParamBlock2（P0/P1/P2、showXYZ、箭头、文字高度、延伸线、RGB 颜色、精度）；延伸线、闭合三角箭头、BezFont 距离文本、可选坐标标签；退化几何点标记。
- **CADText**：单击放置；ParamBlock2（位置、内容、字号、字体、RGB 颜色）；BezFont 字形轮廓与 billboard；空心标记可选中。
- **CADLeader**：三点引线；ParamBlock2（起点/拐点/文本位置、注释、箭头、文字高度、RGB 颜色）；折线 + 闭合三角箭头 + 注释文本。
- **CADTable**：单击插入；ParamBlock2（插入点、行列 1–50、单元格宽高、RGB 颜色）；网格绘制；Win32 单元格编辑器（ListView、逐格编辑、行列迁移按交集保留）；单元格文本长度上限 256。
- **公共设施**：完整屏幕缩放公式（`NonScalingObjectSize × GetVPWorldWidth / 360`）、`DrawLineProc` 折线/闭合绘制、闭合箭头、BezFont 缓存与 CJK 回退、ParamBlock2 生命周期助手。
- **持久化**：四类对象各含版本 chunk 与数据 chunk（`0x0F01`–`0x0F08`）；表格显式 chunk 单权威；加载校验 `cellCount == rows*cols`、行列与长度上限、有限值。

### Changed

- **B7 持久化硬化**：四类对象改为在 `Load()` 中仅解析校验 chunk，注册 `ILoad::RegisterPostLoadCallback`（优先级 5），在全部引用就绪后回填 pblock 镜像，使 chunk 权威与系统加载顺序无关。
- 表格单元格文本截断阈值改用屏幕缩放后的宽度。

### Verification status

- 构建：Release x64 全新 Rebuild 通过，无 warning/error。
- 发布候选：`build/x64/Release/plugins/CADBundle.dlo`，164,864 字节，SHA-256 `0191f6540cd4067f9eb604a05d9d25e6b792f61522488789784d518c9c740c1b`。
- 导出：7 个必需导出全部正确；x64；无 Qt 依赖。
- **运行时未验证**：DLL 加载、面板、四类创建、屏幕恒定、保存/重开（T14/T15）、损坏 chunk、DPI、旧插件共存等，详见 `docs/release/acceptance-checklist.md`。

### 过程提交（摘要）

- `5573c51` B0 五类注册骨架
- `79f0780` B1 Utility 面板与创建入口
- `b9b17e5` B2 公共设施（屏幕缩放/绘制/BezFont）
- `d2ca006` B3 CADDimension
- `76a79af` B4 CADText
- `ccb8332` B5 CADLeader
- `890ef3c` B6 CADTable 与单元格编辑器
- `889cc02` B7 持久化硬化
