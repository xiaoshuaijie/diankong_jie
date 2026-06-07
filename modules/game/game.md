# game 模块使用说明（UART BSP 循环 DMA 接收版）

## 1. 功能概述

`game` 模块提供裁判系统和图传链路数据接收与解析能力，核心特点：

- 使用 `bsp_uart` 的 `STM32UART_t` 单缓冲循环 DMA RX。
- UART idle 分片回调只负责把字节写入软件 FIFO，并通知 FreeRTOS 任务。
- 业务解析放在 `Game_Update()` 中完成，支持粘包/半包处理。
- 解析入口统一走 `remote_process()`，并写入 `CommuniCateTypeDef`。

---

## 2. 相关接口

头文件：`modules/game/game.h`

- `void remote_control_data_init(void);`
  - 清空全局数据 `custom_robot_data`。

- `void remote_process(uint8_t *data, CommuniCateTypeDef *custom_robot_data);`
  - 对单帧数据做 `SOF + CRC8 + CRC16 + CMD_ID` 检测并写入对应结构体。

- `err_t Game_Init(Game_t *self, UART_HandleTypeDef *uart_handle, CommuniCateTypeDef *target);`
  - 初始化裁判系统接收对象，绑定 UART 句柄、RX DMA 缓冲和解析目标。

- `err_t Game_Start(Game_t *self);`
  - 启动 UART 循环 DMA 接收。

- `void Game_Update(Game_t *self, uint32_t timeout_ms);`
  - 任务周期入口：等待 RX 通知，处理 FIFO、半包缓存和裁判系统帧解析。

---

## 3. FreeRTOS 任务接入

当前工程通过 `task/game_task.c` 接入：

```c
static Game_t game_instance;
remote_control_data_init();
err_t status = Game_Init(&game_instance, &huart4, &custom_robot_data);
game = &game_instance;
game->thread_alert = xTaskGetCurrentTaskHandle();
status = Game_Start(game);
```

任务循环中持续调用：

```c
for (;;) {
  Game_Update(game, 10U);
}
```

`Core/Src/freertos.c` 在 `USER CODE` 区域创建 `game_task` 线程。

---

## 4. 参数与可维护点

可在 `game.h` 中配置：

- `GAME_RX_DMA_BUF_SIZE`：底层 UART DMA 原始接收缓冲长度，默认 `128`。
- `GAME_RX_FIFO_LEN`：ISR 到任务的环形 FIFO 长度，默认 `512`。
- `GAME_MAX_PAYLOAD_LEN`：允许解析的最大负载长度，默认 `128`。

建议：

- 提高波特率或帧密度时，先调大 `GAME_RX_FIFO_LEN`。
- 若协议扩展了更大 payload，同步增大 `GAME_MAX_PAYLOAD_LEN`。

---

## 5. 数据读取方式

解析后的数据在全局变量中维护：

- `custom_robot_data.judge_data`：裁判系统 0x0001~0x020E 等数据。
- `custom_robot_data.remote_control_data`：0x0304 键鼠遥控数据。
- `custom_robot_data.custom_robot_data`：0x0302 自定义控制器数据。
- `custom_robot_data.map_command`：0x0303 小地图数据。

上层业务直接读取对应字段即可。

---

## 6. 注意事项

- 不建议与其他模块复用同一 UART RX DMA（会互相覆盖 DMA 接收配置）。
- `remote_process()` 已做 CRC 校验，校验不通过帧会被丢弃。
- FIFO 溢出时会清空 FIFO 并丢弃当前半包缓存，避免错位数据继续污染解析状态。
- 接收链路依赖 `bsp_uart.c` 中的 `HAL_UARTEx_RxEventCallback()` 分发，不再依赖 `HAL_UART_RxCpltCallback()` 钩子。
