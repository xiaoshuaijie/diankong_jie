## [0.1.0] - 2026-06-01

### 快速修改
- **[构建配置]**: 将 component 目录加入 CMake 构建目标 — by xiaoshuaijie
  - 类型: 快速修改（无方案包）
  - 文件: CMakeLists.txt:46-60
- **[串口 BSP]**: 将 STM32UART_Init 错误返回值对齐到 comp_cmd.h 的 err_t 枚举 — by xiaoshuaijie
  - 类型: 快速修改（无方案包）
  - 文件: bsp/bsp_uart/bsp_uart.c:1-126
- **[串口 BSP]**: 迁移单缓冲 DMA RX 与双缓冲 DMA TX 能力 — by xiaoshuaijie
  - 方案: [202606011629_uart_dma_sections](archive/2026-06/202606011629_uart_dma_sections/)
  - 文件: bsp/bsp_uart/bsp_uart.h:1, bsp/bsp_uart/bsp_uart.c:1
- **[串口 BSP]**: 按参考 UART BSP 风格补充 DMA RX/TX 注释 — by xiaoshuaijie
  - 类型: 快速修改（无方案包）
  - 文件: bsp/bsp_uart/bsp_uart.c:1-600, bsp/bsp_uart/bsp_uart.h:1-120
