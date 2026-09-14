#!/bin/bash
#
# run_tests.sh — 一键编译并运行 deepin-album 单元测试
#
# 用法:
#   ./tests/run_tests.sh              # 构建并运行全部测试
#   ./tests/run_tests.sh --clean      # 清理构建目录后重新构建
#   ./tests/run_tests.sh --filter X   # 仅运行匹配的测试 (GTest --gtest_filter)
#   ./tests/run_tests.sh --coverage   # 生成覆盖率报告 (需安装 lcov)
#
# 环境变量:
#   BUILD_DIR   构建目录 (默认: build)
#   BUILD_TYPE  CMake 构建类型 (默认: Debug)

set -euo pipefail

# ---- 配置 ----
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${SOURCE_DIR}/build}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
COVERAGE=false
GTEST_FILTER=""
CLEAN=false

# ---- 解析参数 ----
while [[ $# -gt 0 ]]; do
    case "$1" in
        --clean)    CLEAN=true; shift ;;
        --coverage) COVERAGE=true; shift ;;
        --filter)   GTEST_FILTER="$2"; shift 2 ;;
        *) echo "未知参数: $1"; exit 1 ;;
    esac
done

echo "=========================================="
echo " deepin-album 单元测试一键运行"
echo "=========================================="
echo "构建目录: ${BUILD_DIR}"
echo "构建类型: ${BUILD_TYPE}"
echo "=========================================="

# ---- 清理 ----
if [[ "${CLEAN}" == true ]]; then
    echo "[1/4] 清理构建目录..."
    rm -rf "${BUILD_DIR}"
fi

# ---- CMake 配置 ----
echo "[2/4] CMake 配置..."
cmake_args=(
    -DBUILD_TESTS=ON
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
)
if [[ "${COVERAGE}" == true ]]; then
    cmake_args+=(-DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage -fno-inline")
fi
cmake -S "${SOURCE_DIR}" -B "${BUILD_DIR}" "${cmake_args[@]}"

# ---- 编译 ----
echo "[3/4] 编译测试目标..."
cmake --build "${BUILD_DIR}" --parallel "$(nproc)" --target test_thumbnaillistview test_albumcontrol

# ---- 运行测试 ----
echo "[4/4] 运行测试..."
export QT_QPA_PLATFORM=offscreen

test_targets=("test_thumbnaillistview" "test_albumcontrol")
overall_failed=0

for target in "${test_targets[@]}"; do
    binary="${BUILD_DIR}/tests/$(echo "${target}" | sed 's/^test_//')/${target}"

    # 如果默认路径不存在，尝试在构建目录中查找
    if [[ ! -f "${binary}" ]]; then
        binary=$(find "${BUILD_DIR}" -name "${target}" -type f -executable 2>/dev/null | head -1)
    fi

    if [[ -z "${binary}" || ! -f "${binary}" ]]; then
        echo "  [错误] 找不到测试可执行文件: ${target}"
        overall_failed=1
        continue
    fi

    echo ""
    echo "------------------------------------------"
    echo "运行: ${target}"
    echo "------------------------------------------"

    if [[ -n "${GTEST_FILTER}" ]]; then
        "${binary}" --gtest_filter="${GTEST_FILTER}" || overall_failed=1
    else
        "${binary}" || overall_failed=1
    fi
done

# ---- 覆盖率报告 (可选) ----
if [[ "${COVERAGE}" == true ]]; then
    echo ""
    echo "=========================================="
    echo "生成覆盖率报告..."
    echo "=========================================="
    if command -v lcov &>/dev/null; then
        lcov --capture --directory "${BUILD_DIR}" --output-file "${BUILD_DIR}/coverage.info" \
             --rc lcov_branch_coverage=1
        lcov --remove "${BUILD_DIR}/coverage.info" '/usr/*' '*/3rdparty/*' '*/tests/*' \
             --output-file "${BUILD_DIR}/coverage_filtered.info" --rc lcov_branch_coverage=1
        genhtml "${BUILD_DIR}/coverage_filtered.info" --output-directory "${BUILD_DIR}/coverage_html" \
                --rc lcov_branch_coverage=1
        echo "覆盖率 HTML 报告: ${BUILD_DIR}/coverage_html/index.html"
    else
        echo "  [警告] 未安装 lcov，跳过覆盖率报告生成。请安装: sudo apt install lcov"
    fi
fi

# ---- 汇总 ----
echo ""
echo "=========================================="
if [[ "${overall_failed}" -eq 0 ]]; then
    echo " ✅ 全部测试通过"
else
    echo " ❌ 存在失败的测试"
fi
echo "=========================================="

exit ${overall_failed}
