@echo off
echo ========================================
echo MN 按键模拟器 v1.0.0 - Git 提交准备
echo ========================================
echo.

echo 1. 初始化Git仓库...
git init

echo.
echo 2. 添加远程仓库...
git remote add origin https://github.com/kookoo2024/MyC.git

echo.
echo 3. 添加所有文件到暂存区...
git add .

echo.
echo 4. 提交文件...
git commit -m "Release v1.0.0 - MN按键模拟器正式版

✨ 新功能:
- 自定义按键模拟功能
- 三种模式：1次/多次/无限次
- 窗口置顶功能
- 全局热键支持（~ 键）

🔧 技术特性:
- 固定延迟：400-900ms随机间隔
- 人类化按键模拟
- 多线程处理
- 紧凑界面设计（320x260像素）

📦 文件结构:
- final_simple_ui.cpp - 源代码
- final_simple_ui.exe - 可执行程序
- compile_final_simple.bat - 编译脚本
- 完整的文档和说明文件"

echo.
echo 5. 创建版本标签...
git tag -a v1.0.0 -m "MN按键模拟器 v1.0.0 正式版

🎯 核心特性:
- 自定义按键模拟
- 三种模拟模式
- 固定延迟400-900ms
- 窗口置顶功能
- 全局热键控制

🎨 界面特色:
- 紧凑布局320x260像素
- 简洁操作无需配置
- 实时日志反馈
- 人类化按键模拟

📋 版本信息:
- 版本号: v1.0.0
- 发布日期: 2025-01-20
- 开发语言: C++
- 平台: Windows"

echo.
echo 6. 推送到GitHub...
echo 请手动执行以下命令完成推送:
echo.
echo git push -u origin main
echo git push origin v1.0.0
echo.

echo ========================================
echo Git 准备完成！
echo ========================================
echo.
echo 下一步操作:
echo 1. 检查文件状态: git status
echo 2. 推送主分支: git push -u origin main  
echo 3. 推送标签: git push origin v1.0.0
echo 4. 在GitHub上创建Release
echo.

pause
