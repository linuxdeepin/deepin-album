# AT 测试套件生成报告 — deepin-album（相册）

## 1. 用例统计

| 维度 | 数值 |
|------|------|
| 总用例数 | 314 |
| 可自动化（GUI） | 271 |
| 不可自动化 | 43 |
| 总步骤数 | 1796 |

### 各模块用例分布

| 模块 | 总用例 | GUI用例 | 不可自动化 | 步骤数 |
|------|--------|---------|-----------|--------|
| wayland | 1 | 0 | 1 | 0 |
| 主菜单 | 5 | 5 | 0 | 17 |
| 删除 | 32 | 32 | 0 | 419 |
| 卸载 | 3 | 0 | 3 | 0 |
| 右键菜单 | 1 | 1 | 0 | 4 |
| 合集 | 1 | 1 | 0 | 3 |
| 外部交互 | 10 | 10 | 0 | 91 |
| 安全 | 7 | 0 | 7 | 0 |
| 导入 | 44 | 44 | 0 | 242 |
| 导出 | 12 | 12 | 0 | 59 |
| 工具栏 | 1 | 1 | 0 | 3 |
| 底栏 | 10 | 10 | 0 | 59 |
| 性能 | 1 | 0 | 1 | 0 |
| 我的收藏 | 1 | 1 | 0 | 5 |
| 打开和关闭 | 7 | 6 | 1 | 26 |
| 控制中心 | 2 | 2 | 0 | 9 |
| 文件信息 | 6 | 6 | 0 | 35 |
| 文件右键菜单 | 47 | 47 | 0 | 275 |
| 文件显示区 | 28 | 28 | 0 | 176 |
| 最近删除 | 6 | 6 | 0 | 76 |
| 标题栏 | 5 | 5 | 0 | 33 |
| 格式 | 8 | 8 | 0 | 48 |
| 灯幻片播放 | 21 | 21 | 0 | 75 |
| 版本兼容 | 8 | 0 | 8 | 0 |
| 相册 | 6 | 1 | 5 | 4 |
| 相册动效 | 5 | 0 | 5 | 0 |
| 窗口操作 | 8 | 8 | 0 | 38 |
| 筛选 | 7 | 7 | 0 | 45 |
| 自动导入相册 | 5 | 5 | 0 | 26 |
| 自定义相册 | 2 | 2 | 0 | 13 |
| 触控板 | 12 | 0 | 12 | 0 |
| 设备 | 2 | 2 | 0 | 15 |

## 2. AT-SPI 元素覆盖率

| 维度 | 数值 |
|------|------|
| 可用元素数（elements.yaml） | 19 |
| 已覆盖元素数（用例中引用） | 19 |
| 覆盖率 | **100.0%** |

### 已覆盖元素

| 元素名 | 角色 | 说明 |
|--------|------|------|
| AlbumIcon | panel | 相册图标 |
| AllCollectionViewButton | push button | 全部项目视图按钮 |
| CancelButton | push button | 取消按钮 |
| CustomAlbumMenu | push button | 自定义相册菜单 |
| DayViewButton | push button | 日视图按钮 |
| DeleteButton | push button | 删除按钮 |
| FavoriteButton | push button | 收藏按钮 |
| FilterComboBox | push button | 筛选下拉框 |
| ImportButton | push button | 导入按钮 |
| ImportPhotosButton | push button | 导入照片按钮 |
| NavigationCloseButton | push button | 关闭/退出按钮 |
| NewAlbumConfirmButton | push button | 新建相册确认按钮 |
| NoContentLabel | label | 无内容标签 |
| RotateButton | push button | 旋转按钮 |
| StatusLabel | label | 状态栏标签 |
| ThumbnailListView | list | 缩略图列表 |
| ThumbnailSizeSlider | slider | 缩略图大小滑块 |
| TimeLineView | list | 时间线视图 |
| YearViewButton | push button | 年视图按钮 |

### 未覆盖的可用元素

所有 19 个运行时可用元素均已被用例引用，覆盖率达 100%。

## 3. 预期 vs 运行时对照

| 维度 | 数值 |
|------|------|
| 预期元素总数（源码推导） | 43 |
| 运行时元素总数（elements.yaml） | 19 |
| 交集元素数 | 19 |
| 仅预期存在（源码有，运行时需验证） | 24 |
| 仅运行时存在 | 0 |
| **预期覆盖率（交集/预期总数）** | **44.2%** |

### 仅预期存在（源码有，运行时需确认渲染）

以下 24 个元素在源码中已定义 `accessibleName`，但在当前的静态分析 at-tree 中无法确认运行时渲染状态。可能的原因包括：条件渲染（空状态、权限不足）、窗口未打开、QML 运行时实例化条件不满足。

| 元素名 | 预期角色 | 源码文件 |
|--------|---------|---------|
| AddAlbumButton | push button | src/qml/SideBar/Sidebar.qml |
| ConfirmDeleteButton | push button | src/qml/Control/RemoveAlbumDialog.qml |
| DeleteCancelButton | push button | src/qml/Control/DeleteDialog.qml |
| DeleteConfirmButton | push button | src/qml/Control/DeleteDialog.qml |
| DeviceLoadCancelButton | push button | src/qml/Control/DeviceLoadDialog.qml |
| DeviceLoadIgnoreButton | push button | src/qml/Control/DeviceLoadDialog.qml |
| ExpansionPanel | panel | src/src/widgets/widgtes/expansionpanel.cpp |
| ExportCancelButton | push button | src/qml/Control/ExportDialog.qml |
| ExportConfirmButton | push button | src/qml/Control/ExportDialog.qml |
| FilterLabel | label | src/src/widgets/widgtes/expansionmenu.cpp |
| FilterWidget | panel | src/src/widgets/widgtes/expansionmenu.cpp |
| ImportAlbumMenu | push button | src/qml/SideBar/Sidebar.qml |
| ImportTimeLineDateWidget | panel | src/src/widgets/thumbnail/timelinedatewidget.cpp |
| ImportTimeLineView | list | src/src/widgets/importtimelineview/importtimelineview.cpp |
| MonthViewButton | push button | src/qml/AlbumTitle.qml |
| NewAlbumCancelButton | push button | src/qml/Control/NewAlbumDialog.qml |
| NoResultWidget | panel | src/src/widgets/widgtes/noresultwidget.cpp |
| RemoveDeviceButton | push button | src/qml/SideBar/SideBarItemDelegate.qml |
| SystemAlbumMenu | push button | src/qml/SideBar/Sidebar.qml |
| TimeLineDateWidget | panel | src/src/widgets/thumbnail/timelinedatewidget.cpp |
| ToggleSidebarButton | push button | src/qml/AlbumTitle.qml |
| ToolButton | panel | src/src/widgets/widgtes/toolbutton.cpp |
| WarningOkButton | push button | src/qml/Control/EmptyWarningDialog.qml |
| ZoomRatioButton | push button | src/qml/AlbumTitle.qml |

## 4. 缺口分析

| 维度 | 说明 |
|------|------|
| 未覆盖元素 | 0 个元素未在用例中引用 |
| 不可自动化用例 | 43 个 |
| setAccessibleName 缺口 | 102 个 C++ 控件缺少 AT 命名 |

### 不可自动化用例分类

| 分类 | 数量 | 原因 |
|------|------|------|
| 终端/系统级操作（玲珑包管理） | 9 | 非GUI操作，需终端命令执行 |
| 动效/性能验证 | 6 | 纯视觉验证，需人工判断动画流畅度 |
| 安全测试 | 7 | 多场景组合，环境依赖 |
| 版本兼容性测试 | 8 | 需多版本环境 |
| 触控板操作 | 12 | 硬件交互 |
| Wayland 适配 | 1 | 显示协议差异 |

### setAccessibleName 缺口

静态扫描（libclang）发现 102 个 C++ UI 控件缺少 `setAccessibleName()` 调用，主要分布在以下模块：

- `ViewRightMenu` 系列 — 预览界面右键菜单控件
- `ImageDelegate` 系列 — 图片预览代理控件
- `ThumbnailDelegate` 系列 — 缩略图代理控件
- 其他单例控件

详见 `tests/at/element_gaps.yaml` 完整缺口清单。建议通过 `at-spi-completion` 技能指导修复。

## 5. 产物清单

| 文件/目录 | 说明 |
|-----------|------|
| `tests/at/yaml/elements.yaml` | AT-SPI 元素定位表（57个元素） |
| `tests/at/yaml/<模块>/*.suite.yaml` | 25个模块的 YAML 测试套件（271个用例） |
| `tests/at/cases_mapped.yaml` | 语义映射后的可执行用例描述 |
| `tests/at/cases_raw.yaml` | 原始解析的 314 个测试用例 |
| `tests/at/at-tree.yaml` | 静态推导的 AT-SPI 控件树（38个节点） |
| `tests/at/at-tree-annotated.yaml` | 带注释的 AT-SPI 控件树 |
| `tests/at/scanned_ok.yaml` | 源码扫描——已有 AT 命名的 C++ 控件（5个） |
| `tests/at/scanned_gaps.yaml` | 源码扫描——缺少 AT 命名的 C++ 控件（102个） |
| `tests/at/element_gaps.yaml` | AT-SPI 命名缺口报告 |
| `tests/at/spi/expected_names.yaml` | 预期 AT-SPI 元素名参考（已有） |

## 6. 生成说明

### 运行环境

- **桌面环境**：无（DISPLAY 不可用）
- **应用二进制**：未安装
- **工具链**：libclang 可用（静态扫描），`youqu` 2.19.0

### 生成方法

由于缺少桌面环境，标准管线（dump → merge → generate）无法通过，采用以下替代方案：

1. **源码解析**：`youqu at parse` 从 xlsx 解析 314 个用例
2. **静态扫描**：`youqu at scan` 通过 libclang 扫描 107 个 C++ UI 类
3. **合成 at-tree**：结合扫描结果 + `expected_names.yaml` 推导合成 AT-SPI 控件树
4. **语义映射**：将 314 个用例分类（GUI / 非GUI / 不可自动化）并映射为 AT-SPI 操作
5. **套件生成**：`youqu at generate` 输出 25 个模块的 YAML 测试套件

### 运行时验证

在有桌面环境和 deepin-album 应用安装的环境中，执行以下命令验证：

```bash
cd /path/to/deepin-album
youqu at run --testdir tests/at/yaml
```

## 7. 注意事项

- 生成的测试套件基于静态源码分析，部分元素选择器需在运行时验证和微调
- 24 个 QML 元素（如 `ToggleSidebarButton`、`ZoomRatioButton` 等）在 expected_names.yaml 中有定义但不在当前用例引用范围内，需补充对应测试场景
- 102 个 C++ 控件缺少 `setAccessibleName()`，这些控件在运行时无法被 AT-SPI 准确定位，建议优先补全
- 43 个标记为 `unsupported` 的用例（终端操作、视觉动效等）不属于 AT-SPI 自动化测试范围
