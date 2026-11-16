# GitHub Actions

## 自动检查

`Repository checks` 在推送到 `main`、向 `main` 提交 PR 或手动运行时执行：

- Ubuntu / Windows 上用 Python 3.13 检查所有已跟踪 Python 脚本的语法。
- 检查 JSON、重复键、商品标签、场景和模型目录中的资产引用，以及二进制资产的 Git LFS 规则。
- 用固定版本 actionlint 校验工作流，下载时核对官方校验和。

这些检查不下载 LFS 大文件，不需要 UE，也不编译 C++ 或运行游戏。编辑器脚本只检查语法，不在普通 Python 中导入 `unreal`。

Dependabot 每周一 09:00（Asia/Tokyo）检查 Actions 依赖并提交更新 PR。Actions 固定到完整提交 SHA。

## Windows 打包与回归

在仓库 **Actions → Windows build and gameplay tests → Run workflow** 中选择 `main`。
工作流会编译编辑器、打包 Development Windows 版本、执行 `Tools/check_gameplay.py` 的成功与故意失败用例，然后上传可下载的 Windows 构建和验证日志（保留 14 天）。它不会创建 Release 或推送生成文件。

首次运行前需要准备一台专用的 Windows x64 自托管 runner：

1. 安装 UE **5.8.2**、Visual Studio 2022 的 C++ 游戏开发工具、Windows SDK、Git / Git LFS 和 Git Bash。
2. 在仓库 **Settings → Actions → Runners** 按 GitHub 的步骤注册 runner，增加自定义标签 **`ue-5.8`**，保持 runner 为当前版本（Actions 使用 Node 24）。
3. 在 **Settings → Secrets and variables → Actions → Variables** 中设置 **`UE_ROOT`**，例如 `D:\UE\UE_5.8`。不设置时使用这个默认路径。
4. 让 runner 账户具备引擎、VS、Git Bash 的访问权限及足够的磁盘空间；`setup-python` 会提供 Python 3.13。

UE 构建只允许手动触发 `main`，不会因外部 PR 自动运行本机代码。工作流会创建独立的 `ci-运行ID-重试次数` 检出目录；不要把 runner 指向日常开发工作区。未注册并启动匹配的 runner 时，该工作流会等待，GitHub 托管 runner 不会代为安装 UE。

构建后的玩法自检使用 NullRHI，不要求显示器；它不代替画面、声音、手动通关或帧率验收。普通窗口化启动测试保留为本地检查。

## 本地检查（Git Bash）

```bash
python .github/scripts/check_repository.py
# 下载完 LFS 文件后，增加占位文件检查：
python .github/scripts/check_repository.py --require-lfs
# 仅检查本机引擎路径，避免启动编译：
python .github/scripts/build_windows.py --check-only
```

检查以 Git 已跟踪文件为范围；新文件需先 `git add`。打包脚本也可以在独立检出目录从 Git Bash 直接运行。
