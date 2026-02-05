#!/bin/bash

set -euo pipefail # 命令失败、使用未定义变量、管道失败时，直接退出

BASE_DIR="images"

# 在二级子目录中查找 PDF 文件并转换为 PNG
for dir in "$BASE_DIR"/*/; do
    for pdf in "$dir"/*.pdf; do
        [ -e "$pdf" ] || continue # 无PDF文件时跳过

        png="${pdf%.pdf}.png" # 后缀删除：${var%pattern}

        echo "Converting: $pdf -> $png"
        convert -density 300 "$pdf" -quality 100 "$png"
    done
done
