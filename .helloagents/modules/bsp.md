# BSP 外设封装

## 职责

`bsp/` 提供面向上层模块的 STM32 外设封装，当前包含 CAN、UART、PWM、IIC、USB CDC 和 DWT 时间基准。BSP 层负责保存 HAL 句柄、维护对象表、处理 DMA/ISR/HAL 回调分发，并用统一错误码向上层暴露操作结果。

## 行为规范

- CAN: `bsp/bsp_can` 维护 CAN 对象表，支持对象式接收回调、标准帧发送、DJI 电机电流帧打包，并保留旧接口弱回调兼容路径。
- UART: `bsp/bsp_uart` 使用单缓冲循环 DMA 接收和双缓冲 DMA 发送，HAL 回调只做缓冲切片和发送状态推进。
- PWM: `bsp/bsp_pwm` 保存定时器句柄、通道、占空比和启动状态，公开初始化、启动、占空比设置、停止和错误读取接口。
- IIC: `bsp/bsp_iic` 通过 I2C Instance 反查对象，按 HAL I2C 事件类型分发用户注册回调。
- USB CDC: `bsp/bsp_usb` 封装 FS CDC 设备，发送前复制数据到 CubeMX 生成的 `UserTxBufferFS`，并分发 CDC OUT/IN 完成事件。
- DWT: `bsp/dwt` 使用 Cortex-M DWT CYCCNT 提供秒、毫秒、微秒时间轴和忙等待延时。

## 注释约定

- 分区标题使用 `// ==================== ... ====================`。
- 宏、类型、字段、函数和重要约束使用 `//!`。
- 注释只描述客观行为、调用约束、缓冲生命周期和中断上下文注意事项，不替代代码逻辑。

## 依赖关系

- HAL/CubeMX: `main.h`, `can.h`, `usart.h`, `usb_device.h`, `usbd_cdc_if.h`, `stm32f4xx_hal_i2c.h`
- 通用组件: `comp_cmd.h`, `comp_utils.h`
- RTOS/系统: `cmsis_os.h`, FreeRTOS 任务或中断上下文

## 最近变更

- 2026-06-07: 按 `modules/dr16` 风格统一 `bsp/**/*.c` 与 `bsp/**/*.h` 注释，未改变运行逻辑。
