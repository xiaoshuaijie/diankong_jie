#include "dj_motor_ctrl.h"

#include "FreeRTOS.h"
#include "comp_utils.h"
#include "can.h"
#include "dj_motor_drv.h"
#include "queue.h"
#include "task.h"

typedef struct {
  uint16_t rec_id;
  uint8_t data[CAN_DATA_SIZE];
} dj_motor_feedback_item_t;

// 大疆电机数组定义
dj_motor_t dj_motor[DJ_MOTOR_MAX];

static StaticQueue_t dj_motor_feedback_queue_cb[DJ_MOTOR_MAX];
static uint8_t dj_motor_feedback_queue_storage[DJ_MOTOR_MAX]
                                              [sizeof(dj_motor_feedback_item_t)];
static QueueHandle_t dj_motor_feedback_queue[DJ_MOTOR_MAX];
static bool dj_motor_feedback_queue_ready = false;

/**
 * @brief 获取发送阶段实际下发的电流值。
 *
 * @param motor 电机对象指针
 * @return int16_t 当前设定电流
 */
static int16_t dj_motor_get_send_current(const dj_motor_t *motor) {
  ASSERT(motor);

  return motor->control.current_set;
}

/**
 * @brief 根据电机型号返回对应的电流限幅值。
 *
 * @param type 大疆电机型号
 * @return int16_t 该型号允许的最大电流绝对值
 */
static int16_t dj_motor_get_current_limit(dj_motor_type_e type) {
  switch (type) {
  case DJ_MOTOR_M3508:
    return (int16_t)M3508_MAX_CURRENT;
  case DJ_MOTOR_M6020:
    return (int16_t)M6020_MAX_CURRENT;
  case DJ_MOTOR_M2006:
    return (int16_t)M2006_MAX_CURRENT;
  default:
    return (int16_t)M3508_MAX_CURRENT;
  }
}

/**
 * @brief 按指定 CAN ID 范围分发电机反馈解析。
 *
 * @param rec_id 接收到的 CAN 标准帧 ID
 * @param rx_data 接收到的 8 字节数据
 * @param max_motor_id 允许处理的最大电机 ID
 */
static void dj_motor_dispatch_feedback_by_range(uint16_t rec_id,
                                                const uint8_t *rx_data,
                                                uint16_t max_motor_id) {
  dj_motor_t *motor = NULL;

  if (rx_data == NULL)
    return;

  if ((rec_id < DJ_MOTOR_1) || (rec_id > max_motor_id))
    return;

  motor = dj_motor_get_feedback(rec_id);
  if (motor != NULL) {
    dj_motor_parse_feedback(motor, rx_data);
  }
}

static int8_t dj_motor_get_index_by_id(uint16_t rec_id) {
  if ((rec_id < DJ_MOTOR_1) || (rec_id > DJ_MOTOR_8)) {
    return -1;
  }

  return (int8_t)(rec_id - DJ_MOTOR_1);
}

static bool dj_motor_frame_is_feedback(const BSP_CAN_Frame_t *frame) {
  if (frame == NULL) {
    return false;
  }

  if ((frame->ide_ != CAN_ID_STD) || (frame->rtr_ != CAN_RTR_DATA)) {
    return false;
  }

  if (frame->size_ < 7U) {
    return false;
  }

  return dj_motor_get_index_by_id((uint16_t)frame->id_) >= 0;
}

static void dj_motor_can_rx_callback(STM32CAN_t *can,
                                     const BSP_CAN_Frame_t *frame) {
  RM_UNUSED(can);

  if (!dj_motor_frame_is_feedback(frame)) {
    return;
  }

  if (!dj_motor_feedback_queue_ready) {
    return;
  }

  const int8_t index = dj_motor_get_index_by_id((uint16_t)frame->id_);
  if ((index < 0) || (dj_motor_feedback_queue[index] == NULL)) {
    return;
  }

  dj_motor_feedback_item_t item = {
      .rec_id = (uint16_t)frame->id_,
  };
  for (uint8_t i = 0U; i < CAN_DATA_SIZE; ++i) {
    item.data[i] = frame->data_[i];
  }

  BaseType_t need_switch = pdFALSE;
  (void)xQueueOverwriteFromISR(dj_motor_feedback_queue[index], &item,
                               &need_switch);
  portYIELD_FROM_ISR(need_switch);
}

/**
************************************************************************
* @brief:      	dj_motor_system_init: 大疆电机系统初始化函数
* @param:      	void
* @retval:     	void
* @details:    	初始化8个大疆电机，可配置M3508、M6020、M2006三种电机类型
*               默认配置: 前4个为M3508，后4个为M6020
*               可根据实际需求修改电机类型配置
************************************************************************
**/
void dj_motor_system_init(void) {
  // 初始化Motor1 - Motor4 (配置为M3508电机)
  dj_motor_init(&dj_motor[DJ_MOTOR1], DJ_MOTOR1, DJ_MOTOR_M3508, 0x201);
  dj_motor_init(&dj_motor[DJ_MOTOR2], DJ_MOTOR2, DJ_MOTOR_M3508, 0x202);
  dj_motor_init(&dj_motor[DJ_MOTOR3], DJ_MOTOR3, DJ_MOTOR_M3508, 0x203);
  dj_motor_init(&dj_motor[DJ_MOTOR4], DJ_MOTOR4, DJ_MOTOR_M3508, 0x204);

  // // 初始化Motor5 - Motor8 (配置为M6020电机 - 云台电机)
  dj_motor_init(&dj_motor[DJ_MOTOR5], DJ_MOTOR5, DJ_MOTOR_M6020, 0x205);
  dj_motor_init(&dj_motor[DJ_MOTOR6], DJ_MOTOR6, DJ_MOTOR_M6020, 0x206);
  dj_motor_init(&dj_motor[DJ_MOTOR7], DJ_MOTOR7, DJ_MOTOR_M6020, 0x207);
  dj_motor_init(&dj_motor[DJ_MOTOR8], DJ_MOTOR8, DJ_MOTOR_M6020, 0x208);

  // printf_t("DJ Motor System Initialized:\r\n");
  // printf_t("  Motor1-4: M3508 (0x201-0x204)\r\n");
  // printf_t("  Motor5-8: M6020 (0x205-0x208)\r\n");
}

err_t dj_motor_feedback_queue_init(void) {
  if (dj_motor_feedback_queue_ready) {
    return OK;
  }

  for (uint8_t i = 0U; i < DJ_MOTOR_MAX; ++i) {
    dj_motor_feedback_queue[i] = xQueueCreateStatic(
        1U, sizeof(dj_motor_feedback_item_t),
        dj_motor_feedback_queue_storage[i], &dj_motor_feedback_queue_cb[i]);

    if (dj_motor_feedback_queue[i] == NULL) {
      dj_motor_feedback_queue_ready = false;
      return NO_MEM;
    }
  }

  dj_motor_feedback_queue_ready = true;
  return OK;
}

err_t dj_motor_update_feedback(uint32_t timeout_ms) {
  RM_UNUSED(timeout_ms);

  if (!dj_motor_feedback_queue_ready) {
    return INIT_ERR;
  }

  for (uint8_t i = 0U; i < DJ_MOTOR_MAX; ++i) {
    dj_motor_feedback_item_t item;
    if (xQueueReceive(dj_motor_feedback_queue[i], &item, 0U) == pdPASS) {
      dj_motor_dispatch_feedback_by_range(item.rec_id, item.data, DJ_MOTOR_8);
    }
  }

  return OK;
}

err_t dj_motor_attach_can(STM32CAN_t *can) {
  if (can == NULL) {
    return PTR_NULL;
  }

  STM32CAN_SetRxCallback(can, dj_motor_can_rx_callback);
  return OK;
}

/**
************************************************************************
* @brief:      	dj_motor_control_send: 大疆电机控制发送函数
* @param:      	hcan: CAN句柄
* @retval:     	void
* @details:    	发送当前底盘使用的前4个大疆电机控制电流
*               当前实现通过 CAN ID 0x200 控制 1~4 号电机
*               若后续需要扩展 5~8 号电机，应在统一发送策略下继续补组发送
************************************************************************
**/
void dj_motor_control_send(hcan_t *hcan) {
  // 控制前4个电机 (Motor1-4)
  (void)dj_motor_set_current(hcan, DJ_MOTOR_GROUP_1,
                             dj_motor_get_send_current(&dj_motor[DJ_MOTOR1]),
                             dj_motor_get_send_current(&dj_motor[DJ_MOTOR2]),
                             dj_motor_get_send_current(&dj_motor[DJ_MOTOR3]),
                             dj_motor_get_send_current(&dj_motor[DJ_MOTOR4]));
}

err_t dj_motor_control_send_can(STM32CAN_t *can) {
  if (can == NULL) {
    return PTR_NULL;
  }

  const int16_t current[4] = {
      dj_motor_get_send_current(&dj_motor[DJ_MOTOR1]),
      dj_motor_get_send_current(&dj_motor[DJ_MOTOR2]),
      dj_motor_get_send_current(&dj_motor[DJ_MOTOR3]),
      dj_motor_get_send_current(&dj_motor[DJ_MOTOR4]),
  };

  return STM32CAN_SendDjiCurrent(can, (uint32_t)DJ_MOTOR_GROUP_1, current);
}

/**
************************************************************************
* @brief:      	dj_motor_get_feedback: 读取电机反馈数据
* @param[in]:   motor_id: 电机ID (0x201-0x208)
* @retval:     	电机结构体指针
* @details:    	根据电机ID返回对应的电机结构体指针
*               可用于访问电机的反馈数据
************************************************************************
**/
dj_motor_t *dj_motor_get_feedback(uint16_t motor_id) {
  switch (motor_id) {
  case 0x201:
    return &dj_motor[DJ_MOTOR1];
  case 0x202:
    return &dj_motor[DJ_MOTOR2];
  case 0x203:
    return &dj_motor[DJ_MOTOR3];
  case 0x204:
    return &dj_motor[DJ_MOTOR4];
  case 0x205:
    return &dj_motor[DJ_MOTOR5];
  case 0x206:
    return &dj_motor[DJ_MOTOR6];
  case 0x207:
    return &dj_motor[DJ_MOTOR7];
  case 0x208:
    return &dj_motor[DJ_MOTOR8];
  default:
    return NULL;
  }
}

/**
************************************************************************
* @brief:      	dj_motor_set_current_by_id: 设置电机电流控制
* @param[in]:   motor_id: 电机ID (1-8)
* @param[in]:   current: 电流值 (-16384~16384)
* @retval:     	void
* @details:    	根据电机编号(1-8)设置对应电机的控制电流
*               电流范围: -16384 ~ 16384
*               对应M3508: -20A ~ 20A
*               对应M6020: 需查阅电调规格
*               对应M2006: -5A ~ 5A
************************************************************************

**/
void dj_motor_set_current_by_id(uint8_t motor_id, int16_t current) {
  dj_motor_t *motor = NULL;
  int16_t current_limit = 0;

  // 参数检查
  if (motor_id < 1 || motor_id > DJ_MOTOR_MAX) {
    return;
  }

  motor = &dj_motor[motor_id - 1];
  current_limit = dj_motor_get_current_limit(motor->type);

  // 按电机型号做电流限幅
  if (current > current_limit)
    current = current_limit;
  if (current < -current_limit)
    current = (int16_t)(-current_limit);

  // 设置对应电机的控制电流
  motor->control.current_set = current;
}


/**
************************************************************************
* @brief:      	dj_motor_can1_rx_callback: CAN1接收回调函数
* @param:       hcan: CAN句柄
* @param:       rec_id: 接收到数据的CAN设备ID
* @param:       rx_data: 接收数据缓存
* @retval:     	void
* @details:    	处理CAN1接收中断回调，根据接收到的ID和数据，
*               解析对应电机的反馈数据
*               支持的电机ID: 0x201-0x208
************************************************************************
**/
void dj_motor_can1_rx_callback(hcan_t *hcan, uint16_t *rec_id, uint8_t *rx_data) {
  RM_UNUSED(hcan);
  ASSERT(rec_id);

  dj_motor_dispatch_feedback_by_range(*rec_id, rx_data, DJ_MOTOR_4);
}

/**
************************************************************************
* @brief:      	dj_motor_can2_rx_callback: CAN2接收回调函数
* @param:       hcan: CAN句柄
* @param:       rec_id: 接收到数据的CAN设备ID
* @param:       rx_data: 接收数据缓存
* @retval:     	void
* @details:    	处理CAN2接收中断回调，功能与CAN1相同
*               用于支持双CAN总线的情况
************************************************************************
**/

void dj_motor_can2_rx_callback(hcan_t *hcan, uint16_t *rec_id, uint8_t *rx_data) {
  RM_UNUSED(hcan);
  ASSERT(rec_id);

  dj_motor_dispatch_feedback_by_range(*rec_id, rx_data, DJ_MOTOR_4);
}
