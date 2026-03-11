#!/bin/bash
# 运行脚本：执行可执行程序并将输出重定向到指定的图像文件

if [ $# -lt 2 ]; then
    echo "使用方法: $0 <executable_name> <output_filename>"
    echo ""
    echo "参数说明:"
    echo "  executable_name: 可执行程序名称（不需要路径，自动查找）"
    echo "    可选值: inOneWeekend, nextWeek, restOfYourLife, cuda_restOfYourLife"
    echo "  output_filename: 输出图像文件名（支持自定义路径）"
    echo ""
    echo "使用示例:"
    echo "  $0 inOneWeekend output.ppm"
    echo "  $0 nextWeek images/render.ppm"
    echo "  $0 restOfYourLife /tmp/cornell_box.ppm"
    echo "  $0 cuda_restOfYourLife /tmp/cuda_render.ppm"
    exit 1
fi

EXECUTABLE=$1
OUTPUT_FILE=$2

# 构建完整的可执行文件路径
# 搜索顺序：build/, build/Release/, build/<config>/（支持多种CMake配置）
if [ -f "build/$EXECUTABLE" ]; then
    EXEC_PATH="build/$EXECUTABLE"
elif [ -f "build/Release/$EXECUTABLE" ]; then
    EXEC_PATH="build/Release/$EXECUTABLE"
elif [ -f "build/Debug/$EXECUTABLE" ]; then
    EXEC_PATH="build/Debug/$EXECUTABLE"
elif [ -f "$EXECUTABLE" ]; then
    EXEC_PATH="$EXECUTABLE"
else
    echo "❌ 错误：找不到可执行文件 $EXECUTABLE"
    echo "请确保已运行 build.sh 脚本进行编译"
    echo ""
    echo "搜索位置："
    echo "  - build/$EXECUTABLE"
    echo "  - build/Release/$EXECUTABLE"
    echo "  - build/Debug/$EXECUTABLE"
    exit 1
fi

# 创建输出目录（如果不存在）
OUTPUT_DIR=$(dirname "$OUTPUT_FILE")
if [ "$OUTPUT_DIR" != "." ] && [ "$OUTPUT_DIR" != "" ]; then
    mkdir -p "$OUTPUT_DIR"
fi

echo "=========================================="
echo "运行程序: $EXECUTABLE"
echo "输出图像: $OUTPUT_FILE"
echo "=========================================="
echo ""

# 执行程序并重定向输出到文件
time "$EXEC_PATH" > "$OUTPUT_FILE" 2>&1

if [ $? -eq 0 ]; then
    FILE_SIZE=$(ls -lh "$OUTPUT_FILE" | awk '{print $5}')
    echo ""
    echo "✓ 渲染完成！"
    echo "图像文件大小: $FILE_SIZE"
    echo "输出路径: $(cd "$(dirname "$OUTPUT_FILE")" && pwd)/$(basename "$OUTPUT_FILE")"
else
    echo "❌ 执行失败！"
    exit 1
fi