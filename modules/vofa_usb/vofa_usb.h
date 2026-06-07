#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "bsp_usb.h"
#include "comp_cmd.h"
#include "task.h"

// ==================== VOFA USB 协议与缓冲常量 ====================

#define VOFA_USB_TAIL_LEN (4u)       //! firewater 帧尾长度：00 00 80 7F
#define VOFA_USB_MAX_FLOATS (6u)     //! 单帧最多发送的 float 数量
#define VOFA_USB_TX_LEN (VOFA_USB_MAX_FLOATS * sizeof(float) + VOFA_USB_TAIL_LEN)
#define VOFA_USB_RX_LEN (128u)       //! CDC RX 到任务的环形 FIFO 长度，必须为 2 的幂
#define VOFA_USB_RX_CMD_BUF_LEN (64u)       //! ASCII 命令累积缓冲长度
#define VOFA_USB_SIGNAL_RAW_READY (1u << 6) //! USB 收到字节后通知任务处理的信号位

#ifdef __cplusplus
extern "C" {
#endif

//! 命令回调：解析到合法 name=value! 命令后调用。
typedef void (*VofaUsb_CommandCallback_t)(const char *name, float value);

//! VOFA USB 对象。
//! TX 发送 firewater 浮点帧，RX 接收 ASCII 命令并在任务上下文解析。
typedef struct {
  TaskHandle_t thread_alert;                    //! 接收事件通知的 FreeRTOS 任务句柄
  STM32USB_t usb_;                              //! USB CDC 收发通道
  err_t init_error_;                            //! USB BSP 初始化结果，Start 前保留
  VofaUsb_CommandCallback_t command_callback_;  //! 命令解析完成后的业务回调
} VofaUsb_t;

//! 初始化 VOFA USB 对象，绑定 USB Device 句柄。
err_t VofaUsb_Init(VofaUsb_t *self, USBD_HandleTypeDef *usb_handle);
//! 启动 VOFA USB；USB 设备初始化仍由 MX_USB_DEVICE_Init() 完成。
err_t VofaUsb_Start(VofaUsb_t *self);
//! 任务周期入口：处理 RX FIFO、解析 ASCII 命令和溢出恢复。
void VofaUsb_Update(VofaUsb_t *self, uint32_t timeout_ms);
//! 设置命令回调，不影响 USB CDC 收发状态。
void VofaUsb_SetCommandCallback(VofaUsb_t *self,
                                VofaUsb_CommandCallback_t callback);
//! 按名称读取参数表；参数存在且至少收到过一次时返回 true。
bool VofaUsb_GetParameter(const char *name, float *value);
//! 读取 speed 参数。
bool VofaUsb_GetSpeed(float *speed);
//! 读取 angle 参数。
bool VofaUsb_GetAngle(float *angle);
//! 按 VOFA firewater 格式发送 float 数组：payload 后追加 00 00 80 7F 帧尾。
err_t VofaUsb_Send(VofaUsb_t *self, const float *data, size_t size);

#ifdef __cplusplus
}
#endif
