# Qt 掼蛋四人游戏

这是一个独立的 Qt 6 Widgets 桌面版掼蛋小游戏，不会改动上级目录里的 C++ 练习文件。

## 功能

- 两副牌 108 张，四人两队，首副打 2。
- 支持 1 人对 3 个电脑，也支持 4 人本地轮流。
- 鼠标点选手牌，点击“出牌 / 过牌 / 提示 / 整理”。
- 牌面由 `QPainter` 绘制并缓存为图片，不依赖外部素材。
- 支持逢人配、主要牌型、炸弹、同花顺、王炸、接风、升级、进贡/还贡/抗贡。

## 构建

需要 Qt 6 Widgets、CMake、Ninja 或 MinGW Makefiles。

```powershell
cmake -S guandan_qt -B guandan_qt/build -G Ninja -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\mingw_64
cmake --build guandan_qt/build
guandan_qt\build\GuandanQt.exe
```

如果 Qt 安装在其他目录，把 `CMAKE_PREFIX_PATH` 改成包含 `Qt6Config.cmake` 的 Qt 套件目录。

## 规则测试

```powershell
cmake --build guandan_qt/build --target guandan_rules_tests
ctest --test-dir guandan_qt/build --output-on-failure
```

