# UART DMA 区块迁移任务

@status: in_progress
@type: implementation

## LIVE_STATUS
{"status":"completed","completed":3,"failed":0,"pending":0,"total":3,"done":3,"percent":100,"current":"UART DMA 区块迁移完成","updated_at":"2026-06-01 16:31:00"}

## 任务
- [√] 1.1 补充 `bsp_uart.h` 的单缓冲 RX 与双缓冲 TX 类型和 API 声明 | depends_on: []
- [√] 1.2 在 `bsp_uart.c` 中迁移单缓冲 DMA RX 实现 | depends_on: [1.1]
- [√] 1.3 在 `bsp_uart.c` 中迁移双缓冲 DMA TX 实现和 HAL 回调分发 | depends_on: [1.1, 1.2]
