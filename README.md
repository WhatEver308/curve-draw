# Curve Draw

一个基于 C++ 和 Windows GDI 接口实现的轻量级曲线绘制工具。本项目演示了如何在原生 Windows 环境下进行高性能的几何图形渲染。

## 功能特性

- **原生 GDI 渲染**：不依赖外部图形库（如 OpenGL/DirectX），直接调用 Windows 核心 API。
- **动态曲线生成**：代码层级支持数学模型驱动的曲线绘制。

## 文件说明

- main.cpp: 项目核心代码，包含窗口过程函数及绘图逻辑。
- .gitignore: 已配置过滤 .exe、.txt、.vscode/ 及其它构建产物。
- output/: (已忽略) 默认用于存放程序运行生成的导出内容。

## 构建指南

### 前置条件
- **操作系统**：Windows (GDI 专有)
- **编译器**：MinGW-w64 (建议 g++ 10.0 及以上)

### 命令行构建
打开 PowerShell 或 CMD，执行以下命令：

`powershell
# 编译并立即运行
g++ main.cpp -o curve_draw.exe -lgdi32 && ./curve_draw.exe

# 纯窗口模式构建 (无控制台)
g++ main.cpp -o curve_draw.exe -lgdi32 -mwindows
`
