#include "vofa_usb_task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "comp_utils.h"
#include "task.h"
#include "usb_device.h"
#include "vofa_usb.h"

#define VOFA_USB_TASK_UPDATE_TIMEOUT_MS (10u)   //! VOFA USB 任务收发周期
#define VOFA_USB_WAIT_CONFIG_DELAY_MS (20u)     //! USB 未枚举时的等待周期

extern USBD_HandleTypeDef hUsbDeviceFS;

//! VOFA USB 上行调试数据：默认发送 speed/angle/mit 三个浮点通道。
static float vofa_usb_data[3] = {0.0f, 0.0f, 0.0f};

//! 模块内部回调桥接：让 VofaUsb 驱动保持通用，任务层决定命令如何影响业务数据。
static void OnVofaUsbCommand(const char *name, float value)
{
  VofaUsbTask_OnCommand(name, value);
}

//! 按名称读取 VOFA USB 最近收到的参数。
bool VofaUsbTask_GetParameter(const char *name, float *value)
{
  return VofaUsb_GetParameter(name, value);
}

//! 读取 speed 参数。
bool VofaUsbTask_GetSpeed(float *speed)
{
  return VofaUsb_GetSpeed(speed);
}

//! 读取 angle 参数。
bool VofaUsbTask_GetAngle(float *angle)
{
  return VofaUsb_GetAngle(angle);
}

//! 默认命令处理：把 speed/angle/mit 写入上行调试数组。
//! 业务代码可提供强定义覆盖该弱函数，扩展更多命令。
__attribute__((weak)) void VofaUsbTask_OnCommand(const char *name, float value)
{
  if (strcmp(name, "speed") == 0) {
    vofa_usb_data[0] = value;
  } else if (strcmp(name, "angle") == 0) {
    vofa_usb_data[1] = value;
  } else if (strcmp(name, "mit") == 0) {
    vofa_usb_data[2] = value;
  }
}

//! VOFA USB FreeRTOS 任务：等待 USB CDC 枚举后周期性发送 firewater 数据帧。
void vofa_usb_task(void *argument)
{
  RM_UNUSED(argument);

  static VofaUsb_t vofa_usb;  // 静态存储，保证任务整个生命周期内有效。
  err_t status = VofaUsb_Init(&vofa_usb, &hUsbDeviceFS);
  vofa_usb.thread_alert = xTaskGetCurrentTaskHandle();
  VofaUsb_SetCommandCallback(&vofa_usb, OnVofaUsbCommand);

  if (status == OK) {
    status = VofaUsb_Start(&vofa_usb);
  }
  ASSERT(status == OK);
  if (status != OK) {
    vTaskDelete(NULL);
    return;
  }

  for (;;) {
    if (!STM32USB_IsConfigured(&vofa_usb.usb_)) {
      VofaUsb_Update(&vofa_usb, 0U);
      vTaskDelay(pdMS_TO_TICKS(VOFA_USB_WAIT_CONFIG_DELAY_MS));
      continue;
    }

    const err_t send_err =
        VofaUsb_Send(&vofa_usb, vofa_usb_data, ARRAY_LEN(vofa_usb_data));
    if ((send_err != OK) && (send_err != BUSY) && (send_err != STATE_ERR)) {
      ASSERT(send_err == OK);
    }

    VofaUsb_Update(&vofa_usb, VOFA_USB_TASK_UPDATE_TIMEOUT_MS);
  }
}
