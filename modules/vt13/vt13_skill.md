# joint `vt13` 模块说明

## 1. 当前定位

`engineer_joint/modules/vt13` 只在 joint 板使用，负责把 VT13 原始帧转换成两层结果：

- `vt13_cmd_rc_t`：保留 VT13 原生语义；
- `cmd_rc_t`：转换后的统一控制语义，供关节控制和双板发送使用。

## 2. 转换规则

- `mode_sw` -> `cmd_rc.sw_l`
- `fn_1` -> `cmd_rc.sw_r = MID`
- `fn_2` 或 `trigger` -> `cmd_rc.sw_r = DOWN`
- 默认 `cmd_rc.sw_r = UP`
- `pause` 为最高优先级安全态，直接把 `sw_l/sw_r` 压成停止挡
- `wheel` -> `cmd_rc.ch_res`
- `W/S/A/D/...` -> `cmd_rc.key`
- `pause/fn_1/fn_2/trigger/mouse_middle` -> `cmd_rc.res`

## 3. 安全行为

- CRC 错误、量程越界、帧头异常、模式位非法都判为坏帧。
- 初始化失败不会再返回假成功。
- DMA 完成回调先检查任务句柄，再发通知。
- 解析失败或离线时，任务会写回零快照而不是保留旧值。

## 4. 相关文件

- 模块实现：[vt13.c](/mnt/e/stm32cubemx%20exe/engineer_joint/modules/vt13/vt13.c)
- 任务实现：[vt13_task.c](/mnt/e/stm32cubemx%20exe/engineer_joint/task/vt13/vt13_task.c)
- 总体设计：[skill.md](/mnt/e/stm32cubemx%20exe/skill.md)
