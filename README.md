# 更新内容
- 针对 UI 进行重构，采用 C++ 基类 + WBP 实现可供设计师进行自由设计的 UI 模块
- 加入logo动画，优化Title界面
- 加入开场剧情
- 重设计 UI 版面，使其更符合直觉
- 使用 Lut 贴图对游戏画面后处理
  
## 具体改动
- ../source 文件夹中新增 UI 文件夹，存放 UI 实现相关代码
- 修改 ShiftPlayerCotroller.h/.cpp 等一系列代码

## 后续优化方向
- 优化Title和Logo的美术表现
- 优化文本