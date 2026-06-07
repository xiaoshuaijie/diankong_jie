#include "dm_motor_ctrl.h"

#include "FreeRTOS.h"
#include "comp_utils.h"
#include "dm_motor_drv.h"
#include "queue.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "task.h"
#include <math.h>

typedef struct {
  uint16_t rec_id;
  uint8_t data[CAN_DATA_SIZE];
} dm_motor_feedback_item_t;

motor_t motor[num];

static StaticQueue_t dm_motor_feedback_queue_cb[num];
static uint8_t dm_motor_feedback_queue_storage[num]
                                               [sizeof(dm_motor_feedback_item_t)];
static QueueHandle_t dm_motor_feedback_queue[num];
static bool dm_motor_feedback_queue_ready = false;

static int dm_motor_float_to_uint(float value, float min, float max,
                                  uint8_t bits) {
  if ((max <= min) || (bits == 0U) || (bits >= 31U)) {
    return 0;
  }

  if (value < min) {
    value = min;
  } else if (value > max) {
    value = max;
  }

  const float span = max - min;
  const float offset = value - min;
  const uint32_t range = (1UL << bits) - 1UL;

  return (int)((offset * (float)range) / span);
}

static float dm_motor_uint_to_float(int value, float min, float max,
                                    uint8_t bits) {
  if ((max <= min) || (bits == 0U) || (bits >= 31U)) {
    return min;
  }

  const float span = max - min;
  const uint32_t range = (1UL << bits) - 1UL;

  return (((float)value) * span / (float)range) + min;
}

static int8_t dm_motor_get_index_by_id(uint16_t rec_id) {
  if ((rec_id < 0x20U) || (rec_id > 0x25U)) {
    return -1;
  }

  return (int8_t)(rec_id - 0x20U);
}

static bool dm_motor_frame_is_feedback(const BSP_CAN_Frame_t *frame) {
  if (frame == NULL) {
    return false;
  }

  if ((frame->ide_ != CAN_ID_STD) || (frame->rtr_ != CAN_RTR_DATA)) {
    return false;
  }

  if (frame->size_ < CAN_DATA_SIZE) {
    return false;
  }

  return dm_motor_get_index_by_id((uint16_t)frame->id_) >= 0;
}

static void dm_motor_dispatch_feedback(motor_num motor_index, uint8_t *rx_data) {
  ASSERT(motor_index < num);
  ASSERT(rx_data != NULL);

  if (rx_data == NULL) {
    return;
  }

  dm_motor_fbdata(&motor[motor_index], rx_data);
  receive_motor_data(&motor[motor_index], rx_data);
}

static void dm_motor_can_rx_callback(STM32CAN_t *can,
                                     const BSP_CAN_Frame_t *frame) {
  RM_UNUSED(can);

  if (!dm_motor_frame_is_feedback(frame)) {
    return;
  }

  if (!dm_motor_feedback_queue_ready) {
    return;
  }

  const int8_t index = dm_motor_get_index_by_id((uint16_t)frame->id_);
  if ((index < 0) || (dm_motor_feedback_queue[index] == NULL)) {
    return;
  }

  dm_motor_feedback_item_t item = {
      .rec_id = (uint16_t)frame->id_,
  };
  for (uint8_t i = 0U; i < CAN_DATA_SIZE; ++i) {
    item.data[i] = frame->data_[i];
  }

  BaseType_t need_switch = pdFALSE;
  (void)xQueueOverwriteFromISR(dm_motor_feedback_queue[index], &item,
                               &need_switch);
  portYIELD_FROM_ISR(need_switch);
}

/**
************************************************************************
* @brief:      	dm4310_motor_init: DM4310电机初始化函数
* @param:      	void
* @retval:     	void` `
* @details:    	初始化1个DM4310型号的电机，设置默认参数和控制模式。
*               设置ID、控制模式和命令模式等信息。
************************************************************************
**/
void dm_motor_init(void) {
  // 初始化Motor1和Motor2的电机结构
  memset(&motor[Motor1], 0, sizeof(motor[Motor1]));
  memset(&motor[Motor2], 0, sizeof(motor[Motor2]));
  memset(&motor[Motor3], 0, sizeof(motor[Motor3]));
  memset(&motor[Motor4], 0, sizeof(motor[Motor4]));
  memset(&motor[Motor5], 0, sizeof(motor[Motor5]));
  memset(&motor[Motor6], 0, sizeof(motor[Motor6]));

  /******************关节1 */
  // 设置Motor1的电机信息
  motor[Motor1].id = 0x10;
  motor[Motor1].mst_id = 0x20;
  motor[Motor1].tmp.read_flag = 1;
  motor[Motor1].ctrl.mode = pos_mode;
  motor[Motor1].ctrl.vel_set = 0.0f;
  motor[Motor1].ctrl.pos_set = 0.0f;
  motor[Motor1].ctrl.cur_set = 0.0f;
  motor[Motor1].ctrl.kp_set = 0.0f;
  motor[Motor1].ctrl.kd_set = 0.0f;
  motor[Motor1].tmp.PMAX = 3.141592f;
  motor[Motor1].tmp.VMAX = 45.0f;
  motor[Motor1].tmp.TMAX = 20.0f;
  // 设置Motor2的电机信息
  motor[Motor2].id = 0x11;
  motor[Motor2].mst_id = 0x21;
  motor[Motor2].tmp.read_flag = 1;
  motor[Motor2].ctrl.mode = pos_mode;
  motor[Motor2].ctrl.vel_set = 0.0f;
  motor[Motor2].ctrl.pos_set = 0.0f;
  motor[Motor2].ctrl.cur_set = 0.0f;
  motor[Motor2].ctrl.kp_set = 0.0f;
  motor[Motor2].ctrl.kd_set = 0.0f;
  motor[Motor2].tmp.PMAX = 3.141592f;
  motor[Motor2].tmp.VMAX = 45.0f;
  motor[Motor2].tmp.TMAX = 40.0f;
  // 设置Motor3的电机信息
  motor[Motor3].id = 0x12;
  motor[Motor3].mst_id = 0x22;
  motor[Motor3].tmp.read_flag = 1;
  motor[Motor3].ctrl.mode = pos_mode;
  motor[Motor3].ctrl.vel_set = 0.0f;
  motor[Motor3].ctrl.pos_set = 0.0f;
  motor[Motor3].ctrl.cur_set = 0.0f;
  motor[Motor3].ctrl.kp_set = 0.0f;
  motor[Motor3].ctrl.kd_set = 0.0f;
  motor[Motor3].tmp.PMAX = 3.141592f;
  motor[Motor3].tmp.VMAX = 45.0f;
  motor[Motor3].tmp.TMAX = 40.0f;
  // 设置Motor4的电机信息
  motor[Motor4].id = 0x13;
  motor[Motor4].mst_id = 0x23;
  motor[Motor4].tmp.read_flag = 1;
  motor[Motor4].ctrl.mode = pos_mode;
  motor[Motor4].ctrl.vel_set = 0.0f;
  motor[Motor4].ctrl.pos_set = 0.0f;
  motor[Motor4].ctrl.cur_set = 0.0f;
  motor[Motor4].ctrl.kp_set = 0.0f;
  motor[Motor4].ctrl.kd_set = 0.0f;
  motor[Motor4].tmp.PMAX = 3.141592f;
  motor[Motor4].tmp.VMAX = 30.0f;
  motor[Motor4].tmp.TMAX = 10.0f;
  // 设置Motor5的电机信息
  motor[Motor5].id = 0x14;
  motor[Motor5].mst_id = 0x24;
  motor[Motor5].tmp.read_flag = 1;
  motor[Motor5].ctrl.mode = pos_mode;
  motor[Motor5].ctrl.vel_set = 0.0f;
  motor[Motor5].ctrl.pos_set = 0.0f;
  motor[Motor5].ctrl.cur_set = 0.0f;
  motor[Motor5].ctrl.kp_set = 0.0f;
  motor[Motor5].ctrl.kd_set = 0.0f;
  motor[Motor5].tmp.PMAX = 3.141592f;
  motor[Motor5].tmp.VMAX = 30.0f;
  motor[Motor5].tmp.TMAX = 10.0f;
  // 设置Motor6的电机信息
  motor[Motor6].id = 0x15;
  motor[Motor6].mst_id = 0x25;
  motor[Motor6].tmp.read_flag = 1;
  motor[Motor6].ctrl.mode = pos_mode;
  motor[Motor6].ctrl.vel_set = 0.0f;
  motor[Motor6].ctrl.pos_set = 0.0f;
  motor[Motor6].ctrl.cur_set = 0.0f;
  motor[Motor6].ctrl.kp_set = 0.0f;
  motor[Motor6].ctrl.kd_set = 0.0f;
  motor[Motor6].tmp.PMAX = 3.141592f;
  motor[Motor6].tmp.VMAX = 30.0f;
  motor[Motor6].tmp.TMAX = 10.0f;
}

err_t dm_motor_feedback_queue_init(void) {
  if (dm_motor_feedback_queue_ready) {
    return OK;
  }

  for (uint8_t i = 0U; i < (uint8_t)num; ++i) {
    dm_motor_feedback_queue[i] = xQueueCreateStatic(
        1U, sizeof(dm_motor_feedback_item_t),
        dm_motor_feedback_queue_storage[i], &dm_motor_feedback_queue_cb[i]);

    if (dm_motor_feedback_queue[i] == NULL) {
      dm_motor_feedback_queue_ready = false;
      return NO_MEM;
    }
  }

  dm_motor_feedback_queue_ready = true;
  return OK;
}

err_t dm_motor_update_feedback(uint32_t timeout_ms) {
  RM_UNUSED(timeout_ms);

  if (!dm_motor_feedback_queue_ready) {
    return INIT_ERR;
  }

  for (uint8_t i = 0U; i < (uint8_t)num; ++i) {
    dm_motor_feedback_item_t item;
    if (xQueueReceive(dm_motor_feedback_queue[i], &item, 0U) == pdPASS) {
      const int8_t index = dm_motor_get_index_by_id(item.rec_id);
      if (index >= 0) {
        dm_motor_dispatch_feedback((motor_num)index, item.data);
      }
    }
  }

  return OK;
}

err_t dm_motor_attach_can(STM32CAN_t *can) {
  if (can == NULL) {
    return PTR_NULL;
  }

  STM32CAN_SetRxCallback(can, dm_motor_can_rx_callback);
  return OK;
}
/**
************************************************************************
* @brief:      	read_all_motor_data: 读取电机的所有寄存器的数据信息
* @param[in]:   hcan: CAN句柄
* @param[in]:  	motor_t：电机参数结构体
* @retval:     	void
* @details:    	逐次发送读取命令
************************************************************************
**/
void read_all_motor_data(hcan_t *hcan, motor_t *motor) {
  switch (motor->tmp.read_flag) {
  case 1:
    read_motor_data(hcan, motor->id, 0);
    break; // UV_Value
  case 2:
    read_motor_data(hcan, motor->id, 1);
    break; // KT_Value
  case 3:
    read_motor_data(hcan, motor->id, 2);
    break; // OT_Value
  case 4:
    read_motor_data(hcan, motor->id, 3);
    break; // OC_Value
  case 5:
    read_motor_data(hcan, motor->id, 4);
    break; // ACC
  case 6:
    read_motor_data(hcan, motor->id, 5);
    break; // DEC
  case 7:
    read_motor_data(hcan, motor->id, 6);
    break; // MAX_SPD
  case 8:
    read_motor_data(hcan, motor->id, 7);
    break; // MSC_ID
  case 9:
    read_motor_data(hcan, motor->id, 8);
    break; // ESC_ID
  case 10:
    read_motor_data(hcan, motor->id, 9);
    break; // TIMEOUT
  case 11:
    read_motor_data(hcan, motor->id, 10);
    break; // CTRL_MODE
  case 12:
    read_motor_data(hcan, motor->id, 11);
    break; // Damp
  case 13:
    read_motor_data(hcan, motor->id, 12);
    break; // Inertia
  case 14:
    read_motor_data(hcan, motor->id, 13);
    break; // Rsv1
  case 15:
    read_motor_data(hcan, motor->id, 14);
    break; // sw_ver
  case 16:
    read_motor_data(hcan, motor->id, 15);
    break; // Rsv2
  case 17:
    read_motor_data(hcan, motor->id, 16);
    break; // NPP
  case 18:
    read_motor_data(hcan, motor->id, 17);
    break; // Rs
  case 19:
    read_motor_data(hcan, motor->id, 18);
    break; // Ls
  case 20:
    read_motor_data(hcan, motor->id, 19);
    break; // Flux
  case 21:
    read_motor_data(hcan, motor->id, 20);
    break; // Gr
  case 22:
    read_motor_data(hcan, motor->id, 21);
    break; // PMAX
  case 23:
    read_motor_data(hcan, motor->id, 22);
    break; // VMAX
  case 24:
    read_motor_data(hcan, motor->id, 23);
    break; // TMAX
  case 25:
    read_motor_data(hcan, motor->id, 24);
    break; // I_BW
  case 26:
    read_motor_data(hcan, motor->id, 25);
    break; // KP_ASR
  case 27:
    read_motor_data(hcan, motor->id, 26);
    break; // KI_ASR
  case 28:
    read_motor_data(hcan, motor->id, 27);
    break; // KP_APR
  case 29:
    read_motor_data(hcan, motor->id, 28);
    break; // KI_APR
  case 30:
    read_motor_data(hcan, motor->id, 29);
    break; // OV_Value
  case 31:
    read_motor_data(hcan, motor->id, 30);
    break; // GREF
  case 32:
    read_motor_data(hcan, motor->id, 31);
    break; // Deta
  case 33:
    read_motor_data(hcan, motor->id, 32);
    break; // V_BW
  case 34:
    read_motor_data(hcan, motor->id, 33);
    break; // IQ_c1
  case 35:
    read_motor_data(hcan, motor->id, 34);
    break; // VL_c1
  case 36:
    read_motor_data(hcan, motor->id, 35);
    break; // can_br
  case 37:
    read_motor_data(hcan, motor->id, 36);
    break; // sub_ver
  case 38:
    read_motor_data(hcan, motor->id, 50);
    break; // u_off
  case 39:
    read_motor_data(hcan, motor->id, 51);
    break; // v_off
  case 40:
    read_motor_data(hcan, motor->id, 52);
    break; // k1
  case 41:
    read_motor_data(hcan, motor->id, 53);
    break; // k2
  case 42:
    read_motor_data(hcan, motor->id, 54);
    break; // m_off
  case 43:
    read_motor_data(hcan, motor->id, 55);
    break; // dir
  case 44:
    read_motor_data(hcan, motor->id, 80);
    break; // pm
  case 45:
    read_motor_data(hcan, motor->id, 81);
    break; // xout
  }
}
/**
************************************************************************
* @brief:      	receive_motor_data: 接收电机返回的数据信息
* @param:      	motor_t：电机参数结构体
* @param:      	data：接收的数据
* @retval:     	void
* @details:    	逐次接收电机回传的参数信息
************************************************************************
**/
void receive_motor_data(motor_t *motor, uint8_t *data) {
  if (motor->tmp.read_flag == 0)
    return;

  float_type_u y;

  if (data[2] == 0x33) {
    y.b_val[0] = data[4];
    y.b_val[1] = data[5];
    y.b_val[2] = data[6];
    y.b_val[3] = data[7];

    switch (data[3]) {
    case 0:
      motor->tmp.UV_Value = y.f_val;
      motor->tmp.read_flag = 2;
      break;
    case 1:
      motor->tmp.KT_Value = y.f_val;
      motor->tmp.read_flag = 3;
      break;
    case 2:
      motor->tmp.OT_Value = y.f_val;
      motor->tmp.read_flag = 4;
      break;
    case 3:
      motor->tmp.OC_Value = y.f_val;
      motor->tmp.read_flag = 5;
      break;
    case 4:
      motor->tmp.ACC = y.f_val;
      motor->tmp.read_flag = 6;
      break;
    case 5:
      motor->tmp.DEC = y.f_val;
      motor->tmp.read_flag = 7;
      break;
    case 6:
      motor->tmp.MAX_SPD = y.f_val;
      motor->tmp.read_flag = 8;
      break;
    case 7:
      motor->tmp.MST_ID = y.u_val;
      motor->tmp.read_flag = 9;
      break;
    case 8:
      motor->tmp.ESC_ID = y.u_val;
      motor->tmp.read_flag = 10;
      break;
    case 9:
      motor->tmp.TIMEOUT = y.u_val;
      motor->tmp.read_flag = 11;
      break;
    case 10:
      motor->tmp.cmode = y.u_val;
      motor->tmp.read_flag = 12;
      break;
    case 11:
      motor->tmp.Damp = y.f_val;
      motor->tmp.read_flag = 13;
      break;
    case 12:
      motor->tmp.Inertia = y.f_val;
      motor->tmp.read_flag = 14;
      break;
    case 13:
      motor->tmp.hw_ver = y.u_val;
      motor->tmp.read_flag = 15;
      break;
    case 14:
      motor->tmp.sw_ver = y.u_val;
      motor->tmp.read_flag = 16;
      break;
    case 15:
      motor->tmp.SN = y.u_val;
      motor->tmp.read_flag = 17;
      break;
    case 16:
      motor->tmp.NPP = y.u_val;
      motor->tmp.read_flag = 18;
      break;
    case 17:
      motor->tmp.Rs = y.f_val;
      motor->tmp.read_flag = 19;
      break;
    case 18:
      motor->tmp.Ls = y.f_val;
      motor->tmp.read_flag = 20;
      break;
    case 19:
      motor->tmp.Flux = y.f_val;
      motor->tmp.read_flag = 21;
      break;
    case 20:
      motor->tmp.Gr = y.f_val;
      motor->tmp.read_flag = 22;
      break;
    case 21:
      motor->tmp.PMAX = y.f_val;
      motor->tmp.read_flag = 23;
      break;
    case 22:
      motor->tmp.VMAX = y.f_val;
      motor->tmp.read_flag = 24;
      break;
    case 23:
      motor->tmp.TMAX = y.f_val;
      motor->tmp.read_flag = 25;
      break;
    case 24:
      motor->tmp.I_BW = y.f_val;
      motor->tmp.read_flag = 26;
      break;
    case 25:
      motor->tmp.KP_ASR = y.f_val;
      motor->tmp.read_flag = 27;
      break;
    case 26:
      motor->tmp.KI_ASR = y.f_val;
      motor->tmp.read_flag = 28;
      break;
    case 27:
      motor->tmp.KP_APR = y.f_val;
      motor->tmp.read_flag = 29;
      break;
    case 28:
      motor->tmp.KI_APR = y.f_val;
      motor->tmp.read_flag = 30;
      break;
    case 29:
      motor->tmp.OV_Value = y.f_val;
      motor->tmp.read_flag = 31;
      break;
    case 30:
      motor->tmp.GREF = y.f_val;
      motor->tmp.read_flag = 32;
      break;
    case 31:
      motor->tmp.Deta = y.f_val;
      motor->tmp.read_flag = 33;
      break;
    case 32:
      motor->tmp.V_BW = y.f_val;
      motor->tmp.read_flag = 34;
      break;
    case 33:
      motor->tmp.IQ_cl = y.f_val;
      motor->tmp.read_flag = 35;
      break;
    case 34:
      motor->tmp.VL_cl = y.f_val;
      motor->tmp.read_flag = 36;
      break;
    case 35:
      motor->tmp.can_br = y.u_val;
      motor->tmp.read_flag = 37;
      break;
    case 36:
      motor->tmp.sub_ver = y.u_val;
      motor->tmp.read_flag = 38;
      break;
    case 50:
      motor->tmp.u_off = y.f_val;
      motor->tmp.read_flag = 39;
      break;
    case 51:
      motor->tmp.v_off = y.f_val;
      motor->tmp.read_flag = 40;
      break;
    case 52:
      motor->tmp.k1 = y.f_val;
      motor->tmp.read_flag = 41;
      break;
    case 53:
      motor->tmp.k2 = y.f_val;
      motor->tmp.read_flag = 42;
      break;
    case 54:
      motor->tmp.m_off = y.f_val;
      motor->tmp.read_flag = 43;
      break;
    case 55:
      motor->tmp.dir = y.f_val;
      motor->tmp.read_flag = 44;
      break;
    case 80:
      motor->tmp.p_m = y.f_val;
      motor->tmp.read_flag = 45;
      break;
    case 81:
      motor->tmp.x_out = y.f_val;
      motor->tmp.read_flag = 0;
      break;
    }
  }
}

/**
************************************************************************
* @brief:      	read_dm_motor_pos:
*将电机位置浮点数转换为无符号整数（编码器格式）
* @param:      	pos_float: 位置浮点数值（单位：rad）
* @param:      	p_max: 位置最大值（单位：rad）
* @param:      	p_min: 位置最小值（单位：rad）
* @retval:     	16位无符号整数编码的位置值
* @details:    	用于将控制命令中的位置设定值转换为CAN通信所需的整数格式。
*               采用16位编码，可表示65536个位置离散值。
************************************************************************
**/
int read_dm_motor_pos(float pos_float, float p_max, float p_min) {
  return dm_motor_float_to_uint(pos_float, p_min, p_max, 16U);
}

/**
************************************************************************
* @brief:      	read_dm_motor_vel: 将电机速度浮点数转换为无符号整数
* @param:      	vel_float: 速度浮点数值（单位：rad/s）
* @param:      	v_max: 速度最大值（单位：rad/s）
* @param:      	v_min: 速度最小值（单位：rad/s）
* @retval:     	12位无符号整数编码的速度值
* @details:    	用于将控制命令中的速度设定值转换为CAN通信所需的整数格式。
*               采用12位编码，可表示4096个速度离散值。
************************************************************************
**/
int read_dm_motor_vel(float vel_float, float v_max, float v_min) {
  return dm_motor_float_to_uint(vel_float, v_min, v_max, 12U);
}

/**
************************************************************************
* @brief:      	read_dm_motor_tor: 将电机扭矩浮点数转换为无符号整数
* @param:      	tor_float: 扭矩浮点数值（单位：N·m）
* @param:      	t_max: 扭矩最大值（单位：N·m）
* @param:      	t_min: 扭矩最小值（单位：N·m）
* @retval:     	12位无符号整数编码的扭矩值
* @details:    	用于将控制命令中的扭矩设定值转换为CAN通信所需的整数格式。
*               采用12位编码，可表示4096个扭矩离散值。
************************************************************************
**/
int read_dm_motor_tor(float tor_float, float t_max, float t_min) {
  return dm_motor_float_to_uint(tor_float, t_min, t_max, 12U);
}

/**
************************************************************************
* @brief:      	set_dm_motor_pos: 将无符号整数位置值转换为浮点数
* @param:      	pos_int: 16位无符号整数编码的位置值
* @param:      	p_max: 位置最大值（单位：rad）
* @param:      	p_min: 位置最小值（单位：rad）
* @retval:     	位置浮点数值（单位：rad）
* @details:    	用于解析电机反馈的位置数据，将CAN通信接收的整数格式
*               转换为实际物理量。采用16位解码。
************************************************************************
**/
float set_dm_motor_pos(int pos_int, float p_max, float p_min) {
  return dm_motor_uint_to_float(pos_int, p_min, p_max, 16U);
}

/**
************************************************************************
* @brief:      	set_dm_motor_vel: 将无符号整数速度值转换为浮点数
* @param:      	vel_int: 12位无符号整数编码的速度值
* @param:      	v_max: 速度最大值（单位：rad/s）
* @param:      	v_min: 速度最小值（单位：rad/s）
* @retval:     	速度浮点数值（单位：rad/s）
* @details:    	用于解析电机反馈的速度数据，将CAN通信接收的整数格式
*               转换为实际物理量。采用12位解码。
************************************************************************
**/
float set_dm_motor_vel(int vel_int, float v_max, float v_min) {
  return dm_motor_uint_to_float(vel_int, v_min, v_max, 12U);
}

/**
************************************************************************
* @brief:      	set_dm_motor_tor: 将无符号整数扭矩值转换为浮点数
* @param:      	tor_int: 12位无符号整数编码的扭矩值
* @param:      	t_max: 扭矩最大值（单位：N·m）
* @param:      	t_min: 扭矩最小值（单位：N·m）
* @retval:     	扭矩浮点数值（单位：N·m）
* @details:    	用于解析电机反馈的扭矩数据，将CAN通信接收的整数格式
*               转换为实际物理量。采用12位解码。
************************************************************************
**/
float set_dm_motor_tor(int tor_int, float t_max, float t_min) {
  return dm_motor_uint_to_float(tor_int, t_min, t_max, 12U);
}
/**
************************************************************************
* @brief:      	fdcan1_rx_callback: CAN1接收回调函数
* @param:      	void
* @retval:     	void
* @details:    	处理CAN1接收中断回调，根据接收到的ID和数据，执行相应的处理。
*               当接收到ID为0时，调用dm4310_fbdata函数更新Motor的反馈数据。
************************************************************************
**/
void dm_can1_rx_callback(hcan_t *hcan, uint16_t *rec_id, uint8_t *rx_data) {
  (void)hcan;

  ASSERT(rec_id != NULL);
  ASSERT(rx_data != NULL);
  if ((rec_id == NULL) || (rx_data == NULL)) {
    return;
  }

  switch (*rec_id) {
  case 0x20:
    dm_motor_dispatch_feedback(Motor1, rx_data);
    break;
  case 0x21:
    dm_motor_dispatch_feedback(Motor2, rx_data);
    break;
  case 0x22:
    dm_motor_dispatch_feedback(Motor3, rx_data);
    break;
  case 0x23:
    dm_motor_dispatch_feedback(Motor4, rx_data);
    break;
  case 0x24:
    dm_motor_dispatch_feedback(Motor5, rx_data);
    break;
  case 0x25:
    dm_motor_dispatch_feedback(Motor6, rx_data);
    break;
  }
}
void dm_can2_rx_callback(hcan_t *hcan, uint16_t *rec_id, uint8_t *rx_data) {
  (void)hcan;
  (void)rec_id;
  (void)rx_data;
}
