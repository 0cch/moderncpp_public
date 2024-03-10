# C++静态分析工具之Cppcheck相关代码

视频地址：[C++静态分析工具之Cppcheck](https://www.bilibili.com/video/BV1dC411a73g/)

- 安装

``` bash
sudo apt install cppcheck
```
- 编译和安装

``` bash
sudo apt install libpcre3 libpcre3-dev
git clone https://github.com/danmar/cppcheck.git --branch 2.13.0 --depth=1
cd cppcheck
make MATCHCOMPILER=yes FILESDIR=/usr/share/cppcheck HAVE_RULES=yes CXXFLAGS="-O2 -DNDEBUG -Wall -Wno-sign-compare -Wno-unused-function"
sudo make install MATCHCOMPILER=yes FILESDIR=/usr/share/cppcheck
```

- 检查命令行

``` bash
cppcheck test1.cpp

cppcheck --enable=all test1.cpp

cppcheck --enable=all --suppress=missingIncludeSystem test1.cpp

cppcheck --enable=all --suppress=missingIncludeSystem --inline-suppr test2.cpp
```

- 代码

``` c++
// cppcheck-suppress-file unusedFunction
#include <iostream>
#include <memory>
#include <vector>

void fill(std::vector<int>& v) {
  for (int& x : v) {
    x = 1;
  }
}

bool has_one(const std::vector<int>& v) {
  for (int x : v) {
    if (x == 1) {
      return true;
    }
  }
  return false;
}

class StrWrap {
  std::string str_;
  int unused_;

 public:
  StrWrap(std::string str) : str_(str) {}
  std::string Get() { return str_; }
};

void out_bounds(std::string& str) {
  if (str == "hello") str[10] = 'x';
}

std::vector<int> first;
std::vector<int> second;
void iter_cmp() {
  if (first.begin() == second.begin()) {
  }
}

bool resource_leak() {
  FILE* f = fopen("file", "r");
  if (!f) {
    return false;
  }
  return true;
}

bool memory_leak() {
  int* a = new int;
  if (!a) {
    return false;
  }
  return true;
}

void local_var(int** a) {
  int b = 1;
  *a = &b;
}

void bad_erase() {
  std::vector<int> items = {1, 2, 3};
  std::vector<int>::iterator iter;
  for (iter = items.begin(); iter != items.end(); ++iter) {
    if (*iter == 2) {
      items.erase(iter);
    }
  }
}
```

- CMakeLists.txt

``` cmake
cmake_minimum_required(VERSION 3.0)
project(cpptest)

set(CMAKE_CXX_STANDARD 20)

# 将编译命令导出到 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 添加编译选项以显示警告信息
add_compile_options(-Wall -Wextra)

# 添加可执行文件
add_executable(cpptest test.cpp)

# 设置cppcheck路径
set(CPPCHECK_EXECUTABLE "/usr/bin/cppcheck")

# 将cppcheck添加到编译目标之后
add_custom_command(TARGET cpptest
                   POST_BUILD
                   COMMAND ${CPPCHECK_EXECUTABLE} --enable=all --suppress=missingIncludeSystem
                           --std=c++20 --force --quiet --project=compile_commands.json
                   COMMENT "Running cppcheck after build"
)
```
