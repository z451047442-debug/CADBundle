# 构建指南（CADBundle）

## 前置条件

| 项目 | 要求 |
|---|---|
| 3ds Max | 2027 x64 |
| 3ds Max SDK | 2027，默认路径 `D:\3dsMaxSDK2027\maxsdk`（工程通过属性表引用） |
| Visual Studio | 2022 或更新，含 v143 工具集与 C++20 |
| Windows SDK | 10.0.19041.0 |
| Qt | **不需要**（参数 UI 使用 Win32 `P_AUTO_UI`） |

## 目录结构

```
CADBundle.slnx / CADBundle.vcxproj   工程文件
CADBundle.def                        DLL 导出表（7 个必需导出）
src/                                 源码（入口、面板、4 类对象、公共设施）
res/                                 资源（对话框、字符串、控件 ID）
docs/                                需求、方案、评审、账本、发布文档
build/<Platform>/<Config>/plugins/   构建产物输出目录
x64/*.log                            构建日志（已忽略，不入库）
```

## 构建

在仓库根目录执行（Git Bash）：

```bash
MSYS_NO_PATHCONV=1 \
"/d/Program Files/Microsoft Visual Studio/18/Enterprise/MSBuild/Current/Bin/MSBuild.exe" \
  CADBundle.vcxproj -p:Configuration=Release -p:Platform=x64 -m -v:minimal
```

全新重建（发布候选）：

```bash
MSYS_NO_PATHCONV=1 \
"/d/Program Files/Microsoft Visual Studio/18/Enterprise/MSBuild/Current/Bin/MSBuild.exe" \
  CADBundle.vcxproj -t:Rebuild -p:Configuration=Release -p:Platform=x64 -m -v:minimal
```

也可在 Visual Studio 中打开 `CADBundle.slnx` 构建 Release|x64。

## 产物

`build/x64/Release/plugins/CADBundle.dlo`（PE DLL，x64）。单文件包含代码与资源；不生成 `.mui`。

## 产物校验

```bash
DUMPBIN="/d/Program Files/Microsoft Visual Studio/18/Enterprise/VC/Tools/MSVC/14.38.33130/bin/Hostx64/x64/dumpbin.exe"

# 架构
MSYS_NO_PATHCONV=1 "$DUMPBIN" //nologo -headers build/x64/Release/plugins/CADBundle.dlo | grep -i "8664 machine"

# 7 个必需导出
MSYS_NO_PATHCONV=1 "$DUMPBIN" //nologo -exports build/x64/Release/plugins/CADBundle.dlo \
  | grep -E "LibDescription|LibNumberClasses|LibClassDesc|LibVersion|LibInitialize|LibShutdown|CanAutoDefer"

# 依赖（应包含 Max SDK 库与 CRT，不含 Qt）
MSYS_NO_PATHCONV=1 "$DUMPBIN" //nologo -dependents build/x64/Release/plugins/CADBundle.dlo | grep -i "\.dll"

# 哈希
sha256sum build/x64/Release/plugins/CADBundle.dlo
```

## 构建配置要点（勿随意改动）

- `TargetExt=.dlo`：3ds Max 插件扩展名。
- `PostBuildEventUseInBuild=false`：关闭 SDK 的 `RunMUIRCT`，避免资源被拆为 `.mui`，保证单 DLL 交付。
- 关闭 COMDAT 折叠（`/OPT:NOICF`）：避免小函数被折叠导致导出别名错误。
- `/utf-8` 与中文资源代码页 65001：保证中文界面与字符串正确编码。
- 导出函数使用 `extern "C"` 并由 `CADBundle.def` 按名称导出。

## 常见问题

| 现象 | 处理 |
|---|---|
| `RunMUIRCT` 报多语言资源失败 | 确认 `PostBuildEventUseInBuild=false` 未被覆盖 |
| 导出表出现 SDK 同名符号 | 确认未重新启用 `/OPT:ICF` |
| 中文乱码 | 确认 `/utf-8` 与 `.rc` 的 `code_page(65001)` |
| 找不到 SDK | 检查属性表中的 `MaxSdkDir`（默认指向 `D:\3dsMaxSDK2027\maxsdk`） |

## 版本与发布

构建产物当前为 **0.1.0（2026-11-22）**；VERSIONINFO 已加入。发布流程见 `docs/release/acceptance-checklist.md`。
