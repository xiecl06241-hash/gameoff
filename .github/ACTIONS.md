# GitHub Actions

## 自动检查

`Repository checks` 在推送到任意分支或标签、向任意分支提交或更新 PR、手动运行时执行：

- Ubuntu / Windows 上用 Python 3.13 检查所有已跟踪 Python 脚本的语法。
- 检查 JSON、重复键、商品标签、场景和模型目录中的资产引用，以及二进制资产的 Git LFS 规则。
- 用固定版本 actionlint 校验工作流，下载时核对官方校验和。

这些检查不下载 LFS 大文件，不需要 UE，也不编译 C++ 或运行游戏。编辑器脚本只检查语法，不在普通 Python 中导入 `unreal`。

Dependabot 每周一 09:00（Asia/Tokyo）检查 Actions 依赖并提交更新 PR。Actions 固定到完整提交 SHA。

## 仓库活动记录

在 **Actions → Repository activity log** 查看每次事件的摘要、日志及 `activity.json` 下载文件。记录保留 90 天（受仓库保留策略限制），不取消较早的事件运行；它是活动记录，不是永久、不可篡改的审计存储。

- 所有分支和标签的推送（包括强制推送）、创建和删除。
- 所有目标分支的 PR 生命周期，包括关闭、合并、重开、审核请求、标签及自动合并状态；另记录审核、行内评论及普通评论。
- Issue、Discussion 及评论、Release、Fork、Star、Wiki、标签、里程碑、分支保护规则、部署、包、Pages、提交状态、检查和合并队列的 Actions 支持事件。
- 外部派发、手动运行，以及现有检查和 Windows 构建工作流的运行状态。新增其他工作流时，需把名称加入 `activity.yml` 的 `workflow_run.workflows`。

记录只包含事件类型、动作、分支／标签、提交 SHA、PR／Issue 编号等元数据，不复制原始事件、账号信息、正文、评论、提交作者或邮箱。记录工作流不检出代码，不执行 PR 或分支代码；`pull_request_target` 仅用于读取 PR 事件元数据。Windows 本机构建仍单独手动运行。

覆盖边界：GitHub Actions 不能记录 clone、fetch、pull 等读取操作，也不覆盖所有权限／设置变更、未推送的本地操作、Fork 内未提交 PR 的分支操作。历史事件不会补录。GitHub 未投递、被平台限制、等待批准、超出保留期或工作流被修改／禁用的事件也无法保证留存；例如一次创建／删除超过三个标签、一次推送超过 5,000 个分支、部分由 `GITHUB_TOKEN` 产生的事件不会触发。完整审计需要另外部署 Webhook 接收存储或使用适用的 GitHub 审计能力。

分支必须包含更新后的工作流文件，才能在该分支的后续 push/create 事件中使用新规则；删除和多数仓库级事件依赖默认分支中的工作流。从旧提交创建的分支需要同步工作流。外部 PR 的代码检查仍可能需要维护者批准。

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
