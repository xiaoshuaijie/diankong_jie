#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FreeRTOS 中 VOFA USB 任务入口。
 *
 * 本函数由 Core/Src/freertos.c 的 USER CODE 区创建，用于启动 USB CDC 上的
 * VOFA 收发逻辑；USB Device 底层初始化仍由 defaultTask 调用 MX_USB_DEVICE_Init()。
 */
void vofa_usb_task(void *argument);

/**
 * @brief 读取 VOFA USB 接收到的 speed 参数。
 *
 * 上位机通过 USB CDC 发送 `speed=100!` 且解析成功后，本函数会返回 true，
 * 并把 100.0f 写入 speed 指向的位置。
 *
 * @param speed 输出参数地址，不能为 NULL。
 * @return true 表示 speed 已经至少收到过一次；false 表示参数为空或尚未收到。
 */
bool VofaUsbTask_GetSpeed(float *speed);

/**
 * @brief 读取 VOFA USB 接收到的 angle 参数。
 *
 * 上位机通过 USB CDC 发送 `angle=30!` 且解析成功后，本函数会返回 true，
 * 并把 30.0f 写入 angle 指向的位置。
 *
 * @param angle 输出参数地址，不能为 NULL。
 * @return true 表示 angle 已经至少收到过一次；false 表示参数为空或尚未收到。
 */
bool VofaUsbTask_GetAngle(float *angle);

/**
 * @brief 按名称读取 VOFA USB 参数表中的参数。
 *
 * 当前模块参数表包含 `speed`、`angle` 和 `mit`。该函数是
 * VofaUsb_GetParameter() 的 C 包装接口，适合任务层代码按字符串读取参数。
 *
 * @param name 参数名，例如 "speed"、"angle" 或 "mit"。
 * @param value 输出参数地址，不能为 NULL。
 * @return true 表示参数存在且已经收到过；false 表示参数不存在、参数为空或尚未收到。
 */
bool VofaUsbTask_GetParameter(const char *name, float *value);

/**
 * @brief VOFA USB ASCII 命令接收扩展点。
 *
 * VofaUsb 解析到合法的 `name=value!` 命令后，会先更新模块内部参数表，再调用
 * 此扩展点。默认实现为空业务映射，业务代码可在其他编译单元中提供同名强定义。
 *
 * @param name 命令名，生命周期仅在回调执行期间有效。
 * @param value 解析后的有限浮点值。
 */
void VofaUsbTask_OnCommand(const char *name, float value);

#ifdef __cplusplus
}
#endif
