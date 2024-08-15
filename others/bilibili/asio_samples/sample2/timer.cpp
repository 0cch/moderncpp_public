/**
 * 检测Windows环境宏_WIN32，设置_WIN32_WINNT为0x0601
 * 0x0601表示Windows 7
 */
#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif

#include <asio.hpp>
#include <chrono>
#include <iostream>

#if 0

void TimerHandler(const asio::error_code& error) {
  if (!error) {
    std::cout << "Timer expired!" << std::endl;
  } else {
    std::cerr << "Error: " << error.message() << std::endl;
  }
}

int main() {
  try {
    // 创建 io_context
    asio::io_context io_context;

    // 创建一个定时器，设置为2秒超时
    asio::steady_timer timer(io_context, std::chrono::seconds(2));

    // 设置定时器回调函数
    timer.async_wait(TimerHandler);

    // 运行 io_context 以处理异步事件
    io_context.run();
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}

#else

void TimerHandler(const asio::error_code& error, asio::steady_timer& timer) {
  if (!error) {
    std::cout << "Timer expired!" << std::endl;

    // 重新启动定时器，形成循环定时
    timer.expires_after(std::chrono::seconds(2));
    timer.async_wait(
        [&timer](const asio::error_code& ec) { TimerHandler(ec, timer); });
  } else {
    std::cerr << "Error: " << error.message() << std::endl;
  }
}

int main() {
  try {
    // 创建 io_context
    asio::io_context io_context;

    // 创建一个定时器，设置为2秒超时
    asio::steady_timer timer(io_context, std::chrono::seconds(2));

    // 设置初始定时器回调函数
    timer.async_wait(
        [&timer](const asio::error_code& ec) { TimerHandler(ec, timer); });

    // 运行 io_context 以处理异步事件
    io_context.run();
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}

#endif