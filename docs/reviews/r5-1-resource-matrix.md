# R5-1 资源差异表（V-04 证据基线）

- 来源：`res/CADBundle.rc`、`res/resource.h`
- 日期：2026-11-20
- 坐标单位：DLU；对话框宽度均 ≤190 DLU。
- DLU→像素（名义值，9 pt MS Shell Dlg）：约 1.5 px/DLU @100%。DPI 缩放按 1.0 / 1.25 / 1.5 / 2.0 计；最终以 V-04 截图实测为准。

## 对话框宽度

| 对话框 | 宽(DLU) | 最高控件右边界(DLU) | 100% | 125% | 150% | 200% |
|---|---:|---:|---:|---:|---:|---:|
| IDD_CAD_DIM | 190 | 182（字体下拉） | 285 | 356 | 428 | 570 |
| IDD_CAD_TEXT | 190 | 186（字符串/字体） | 285 | 356 | 428 | 570 |
| IDD_CAD_LEADER | 190 | 186（注释/字体） | 285 | 356 | 428 | 570 |
| IDD_CAD_TABLE | 190 | 186（字体下拉） | 285 | 356 | 428 | 570 |

边距：左 4 DLU；最右控件距对话框右边界 ≥4 DLU（DIM 为 8）。

## 新增/变更控件 ID

| 名称 | ID | 对话框 | 位置/尺寸(DLU) | 备注 |
|---|---:|---|---|---|
| IDC_DIM_FONT | 1130 | DIM | 70,156,112,120 | 方案遗漏，本次补齐 |
| IDC_LEAD_FONT | 1325 | LEADER | 32,98,154,120 | |
| IDC_TABLE_FONT | 1417 | TABLE | 32,74,154,120 | |
| IDC_TABLE_FONTSIZE | 1418 | TABLE | 32,92,26,10 | |
| IDC_TABLE_FONTSIZESPIN | 1419 | TABLE | 61,92,7,10 | |
| IDC_DIM_SIZEMODE | 1129 | DIM | 预留（R5-2） | 本批未放置 |
| IDC_TEXT_SIZEMODE | 1211 | TEXT | 预留（R5-2） | 本批未放置 |
| IDC_LEAD_SIZEMODE | 1326 | LEADER | 预留（R5-2） | 本批未放置 |
| IDC_TABLE_SIZEMODE | 1420 | TABLE | 预留（R5-2） | 本批未放置 |

## 新增字符串 ID

| 名称 | ID | 文本 |
|---|---:|---|
| IDS_SIZEMODE | 163 | 文字大小 |
| IDS_SIZEMODE_SCREEN | 164 | 屏幕恒定 |
| IDS_SIZEMODE_MODEL | 165 | 模型空间 |
| IDS_LEAD_FONT | 166 | 字体 |
| IDS_DIM_FONT | 167 | 字体 |
| IDS_TABLE_FONT | 168 | 字体 |
| IDS_TABLE_FONTSIZE | 169 | 字号 |

## 定位行压缩（四类共用样式）

| 元素 | 原 x/宽 | 新 x/宽 |
|---|---|---|
| 标签（如“P0 X”/“起点 X”） | 6 / 42 | 4 / 36 |
| X 数值框 | 50 / 34 | 42 / 28 |
| X 微调 | 85 / 7 | 71 / 7 |
| Y 标签 | 96 / 8 | 82 / 8 |
| Y 数值框 | 104 / 34 | 92 / 28 |
| Y 微调 | 139 / 7 | 121 / 7 |
| Z 标签 | 150 / 8 | 132 / 8 |
| Z 数值框 | 158 / 34 | 142 / 28 |
| Z 微调 | 193 / 7 | 171 / 7 |

## 自动断言

`build/verify/r5_1_static.ps1` 检查：

1. 四个对话框宽度字段 ≤190 DLU；
2. 新控件/字符串 ID 已定义，100–199 字符串编号无重复；
3. `src` 中无 `SetUserDlgProc`；
4. 四类 Render 均使用 `CADMeasureText`；
5. `FitCellText` 使用 `CADMeasureText` 且不再调用 `getTextExtents`；
6. `CADScreenDraw.cpp` 存在并已加入 `CADBundle.vcxproj`。

V-04 仍需在 4 档 DPI 下截图核对：控件在客户区、边距 ≥2 px、无重叠。
