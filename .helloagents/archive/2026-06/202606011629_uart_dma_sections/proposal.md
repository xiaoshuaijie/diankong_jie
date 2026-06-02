# UART DMA 区块迁移方案

## 类型
implementation

## 目标
参考 `C:/Users/24137/Downloads/bsp_usart_c.c` 和 `bsp_usart_c.h`，仅迁移以下能力到当前工程 UART BSP：

- 单缓冲 DMA RX
- 双缓冲 DMA TX

## 实施范围
- `bsp/bsp_uart/bsp_uart.h`: 补充单缓冲 RX 和双缓冲 TX 的结构体、回调类型、函数声明。
- `bsp/bsp_uart/bsp_uart.c`: 补充对应实现和 HAL 回调分发。
- `component/comp_utils.h`: 仅作为通用工具头使用，不放 UART 专用结构。

## 关键约束
- 错误返回类型统一使用 `component/comp_cmd.h` 中的 `err_t`。
- 不迁移参考文件中的双缓冲 DMA RX。
- 保持当前 STM32 HAL/CMake 工程结构。

## 验收标准
- 工程可通过 `cmake --build build/Debug --target diankong_jie`。
- `bsp_uart.h` 暴露单缓冲 RX 和双缓冲 TX API。
- `bsp_uart.c` 不再引用未定义的 `BSP_UART_ERROR_*` 或双缓冲 RX 类型。
