#!/bin/bash
# 构建脚本：清空build文件夹并构建项目

set -e  # 任何命令失败都停止执行

echo "=========================================="
echo "开始清空 build 文件夹..."
echo "=========================================="

# 删除build文件夹
if [ -d "build" ]; then
    rm -rf build
    echo "✓ build 文件夹已删除"
else
    echo "✓ build 文件夹不存在（跳过）"
fi

echo ""
echo "=========================================="
echo "开始构建项目..."
echo "=========================================="

# 创建build文件夹
mkdir -p build
cd build

# 运行CMake配置（使用Release模式以获得最佳性能）
cmake -DCMAKE_BUILD_TYPE=Release ..

# 编译
cmake --build . --config Release

echo ""
echo "=========================================="
echo "✓ 构建完成！"
echo "=========================================="
echo "可执行文件位置（Linux/Unix）："
echo "  - ./build/inOneWeekend"
echo "  - ./build/nextWeek"
echo "  - ./build/restOfYourLife"
echo "  - ./build/cuda_restOfYourLife"
echo ""
echo "运行脚本示例："
echo "  ./run.sh inOneWeekend output.ppm"
echo "  ./run.sh nextWeek output.ppm"
echo "  ./run.sh restOfYourLife output.ppm"
echo "  ./run.sh cuda_restOfYourLife output.ppm"