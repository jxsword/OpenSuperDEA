
# 命名与注释规范

> 本文档定义本项目中所有 C++ 源码、CMake 脚本和 Markdown 文档的命名与注释规范。

## 一、命名规范

### 1.1 通用规则

| 元素 | 规范 | 示例 |
|---|---|---|
| 变量 | `snake_case` | `int user_count;` |
| 函数 | `snake_case` | `void calculate_sum();` |
| 类、结构体 | `PascalCase` | `class DataProcessor;` |
| 枚举类型 | `PascalCase` | `enum class ColorMode;` |
| 命名空间 | `snake_case` | `namespace math_utils;` |
| 常量 | `ALL_CAPS` | `constexpr int MAX_RETRY = 3;` |
| 宏 | `ALL_CAPS` | `#define DEBUG_MODE 1` |
| 枚举值 | `ALL_CAPS` | `enum Color { RED, GREEN, BLUE };` |
| 模板参数 | `PascalCase` | `template <typename T, typename Allocator>` |
| 成员变量 | `snake_case` + 可选后缀 `_` | `int buffer_size_;` |
| 全局变量 | 尽量避免；必要时 `g_snake_case` | `int g_global_counter;` |
| 文件名 | `snake_case` | `structured_bindings.cpp` |
| 目录名 | `snake_case` | `move_semantics/` |

### 1.2 布尔变量

布尔变量应以 `is_`、`has_`、`can_`、`should_` 等前缀开头：

```cpp
bool is_valid = true;
bool has_permission = false;
bool can_retry = true;
```

### 1.3 函数命名

- 普通函数：动词 + 名词，`snake_case`
  - `calculate_total()`
  - `validate_input()`
  - `get_user_name()`
- 返回布尔值的函数：`is_` / `has_` / `can_` 前缀
  - `is_empty()`
  - `has_value()`
- 回调/事件函数：`on_` 前缀
  - `on_click()`
  - `on_completion()`

### 1.4 类命名

- 类名使用 `PascalCase`，名词或名词短语
- 抽象基类可加 `I` 前缀（接口）或 `Base` 后缀
- 异常类以 `Exception` 或 `Error` 结尾

```cpp
class FileReader;
class IDataSource;
class BaseParser;
class NetworkException;
```

### 1.5 常量与宏

- 编译期常量使用 `constexpr` + `ALL_CAPS`
- 宏使用 `ALL_CAPS`，并加项目前缀避免冲突
- 避免使用宏定义常量，优先 `constexpr`

```cpp
constexpr int MAX_BUFFER_SIZE = 4096;
constexpr double PI = 3.14159265358979323846;
#define CPP_STANDARD_VERSION_MAJOR 1
```

### 1.6 命名空间

- 全小写，单词间用下划线分隔
- 嵌套命名空间使用 C++17 简洁语法

```cpp
namespace cpp_standard::math_utils {
// ...
} // namespace cpp_standard::math_utils
```

## 二、注释规范（Doxygen）

### 2.1 文件级注释

**所有 `.cpp` 和 `.h` 文件必须在文件开头包含 Doxygen 文件级注释**：

```cpp
/**
 * @file structured_bindings.cpp
 * @brief 展示 C++17 结构化绑定的基本用法。
 * @details 包含多个用例：pair 解包、tuple 解包、结构体绑定、数组绑定。
 * @section compiler_support 编译器支持
 * GCC:   从 7.1 版本开始完整支持
 * Clang: 从 4.0 版本开始支持
 * MSVC:  从 19.12 (VS 2017 15.5) 开始支持
 * 编译选项: -std=c++17 (GCC/Clang), /std:c++17 /utf-8 (MSVC)
 * @note 本特性在 C++17 中引入，C++20 中增加了对位域的支持。
 * @warning MSVC 19.11 及以下版本不支持，编译会报错。
 */
```

### 2.2 函数注释

```cpp
/**
 * @brief 计算两个整数的和。
 * @param a 第一个加数
 * @param b 第二个加数
 * @return 两数之和
 * @note 此函数为 constexpr，可在编译期求值。
 * @sa subtract(), multiply()
 */
int add(int a, int b);
```

### 2.3 类注释

```cpp
/**
 * @brief 一个简单的 RAII 资源管理类。
 * @tparam T 资源类型
 * @details 构造时获取资源，析构时自动释放，禁止拷贝，允许移动。
 */
template <typename T>
class ScopedResource {
    // ...
};
```

### 2.4 枚举注释

```cpp
/**
 * @brief 表示操作结果的状态码。
 */
enum class StatusCode {
    SUCCESS,        /**< 操作成功 */
    NOT_FOUND,      /**< 资源未找到 */
    PERMISSION_DENIED, /**< 权限不足 */
    TIMEOUT         /**< 操作超时 */
};
```

### 2.5 注释语言

- **除专业术语外，全部使用中文**
- 保留英文的专业术语示例：
  - RAII、SFINAE、CRTP、ODR、UB
  - lambda、coroutine、concept、ranges、modules
  - move semantics、copy elision、value category
- 行内注释使用 `//`，Doxygen 注释使用 `/** ... */`
- 复杂逻辑处必须有行内中文注释说明意图

### 2.6 注释最佳实践

- 注释"为什么"，而不是"是什么"——代码本身已经说明"是什么"
- 避免注释与代码重复
- 及时更新注释，过期的注释比没有注释更糟糕
- TODO/FIXME 注释格式：`// TODO(作者): 说明` / `// FIXME: 说明`

## 三、代码格式

### 3.1 缩进与空格

- 使用 4 空格缩进，不使用 Tab
- 运算符两侧加空格：`a + b`，不是 `a+b`
- 逗号后加空格：`func(a, b, c)`，不是 `func(a,b,c)`
- 括号内侧不加空格：`func(a)`，不是 `func( a )`

### 3.2 大括号

- 采用 Allman 风格（大括号独占一行）

```cpp
if (condition)
{
    do_something();
}
else
{
    do_other();
}
```

### 3.3 行宽

- 建议每行不超过 100 字符
- 超过时合理换行，保持可读性

## 四、CMake 命名规范

- 目标名：`snake_case`，与特性目录名对应
- 变量名：`snake_case`，项目级变量加 `CPP_STANDARD_` 前缀
- 选项名：`ALL_CAPS`，加 `CPP_STANDARD_` 前缀
- 函数名：`snake_case`

```cmake
add_executable(structured_bindings_demo structured_bindings.cpp)
option(CPP_STANDARD_ENABLE_GTEST "启用 GoogleTest" ON)
set(CPP_STANDARD_BUILD_TESTS TRUE)
```

## 五、Markdown 文档规范

- 标题层级：`#` 一级，`##` 二级，`###` 三级
- 代码块标注语言：`cpp`、`cmake`、`bash`
- 表格使用标准 Markdown 表格语法
- 链接使用有意义的链接文字，不使用裸 URL
- 中文与英文/数字之间加空格
