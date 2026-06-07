#include "dm_motor_drv.h"
#include "can.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

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

static uint16_t dm_motor_get_mode_id(const motor_t *motor) {
  if (motor == NULL) {
    return 0U;
  }

  switch (motor->ctrl.mode) {
  case mit_mode:
    return MIT_MODE;
  case pos_mode:
    return POS_MODE;
  case spd_mode:
    return SPD_MODE;
  case psi_mode:
    return PSI_MODE;
  default:
    return 0U;
  }
}

static void dm_motor_pack_mode_cmd(uint8_t data[8], uint8_t tail) {
  memset(data, 0xFF, 8U);
  data[7] = tail;
}

static err_t dm_motor_send_can(STM32CAN_t *can, uint16_t id,
                               const uint8_t *data, size_t len) {
  if ((can == NULL) || (data == NULL)) {
    return PTR_NULL;
  }

  return STM32CAN_Send(can, id, data, len);
}

static void dm_motor_pack_float(uint8_t *data, float value) {
  memcpy(data, &value, sizeof(value));
}

static void dm_motor_pack_u16(uint8_t *data, uint16_t value) {
  memcpy(data, &value, sizeof(value));
}

static void dm_motor_pack_mit_ctrl(uint8_t data[8], const motor_t *motor,
                                   float pos, float vel, float kp, float kd,
                                   float tor) {
  const uint16_t pos_tmp =
      (uint16_t)dm_motor_float_to_uint(pos, -motor->tmp.PMAX, motor->tmp.PMAX,
                                       16U);
  const uint16_t vel_tmp =
      (uint16_t)dm_motor_float_to_uint(vel, -motor->tmp.VMAX, motor->tmp.VMAX,
                                       12U);
  const uint16_t tor_tmp =
      (uint16_t)dm_motor_float_to_uint(tor, -motor->tmp.TMAX, motor->tmp.TMAX,
                                       12U);
  const uint16_t kp_tmp =
      (uint16_t)dm_motor_float_to_uint(kp, KP_MIN, KP_MAX, 12U);
  const uint16_t kd_tmp =
      (uint16_t)dm_motor_float_to_uint(kd, KD_MIN, KD_MAX, 12U);

  data[0] = (uint8_t)(pos_tmp >> 8);
  data[1] = (uint8_t)pos_tmp;
  data[2] = (uint8_t)(vel_tmp >> 4);
  data[3] = (uint8_t)(((vel_tmp & 0xFU) << 4) | (kp_tmp >> 8));
  data[4] = (uint8_t)kp_tmp;
  data[5] = (uint8_t)(kd_tmp >> 4);
  data[6] = (uint8_t)(((kd_tmp & 0xFU) << 4) | (tor_tmp >> 8));
  data[7] = (uint8_t)tor_tmp;
}

static void dm_motor_pack_pos_ctrl(uint8_t data[8], float pos, float vel) {
  dm_motor_pack_float(&data[0], pos);
  dm_motor_pack_float(&data[4], vel);
}

static void dm_motor_pack_spd_ctrl(uint8_t data[4], float vel) {
  dm_motor_pack_float(&data[0], vel);
}

static void dm_motor_pack_psi_ctrl(uint8_t data[8], float pos, float vel,
                                   float cur) {
  const uint16_t u16_vel = (uint16_t)(vel * 100.0f);
  const uint16_t u16_cur = (uint16_t)(cur * 10000.0f);

  dm_motor_pack_float(&data[0], pos);
  dm_motor_pack_u16(&data[4], u16_vel);
  dm_motor_pack_u16(&data[6], u16_cur);
}

static err_t dm_motor_send_mode_cmd_can(STM32CAN_t *can, uint16_t motor_id,
                                        uint16_t mode_id, uint8_t tail) {
  uint8_t data[8];
  dm_motor_pack_mode_cmd(data, tail);
  return dm_motor_send_can(can, (uint16_t)(motor_id + mode_id), data, 8U);
}

/**
************************************************************************
* @brief:      	dm4310_enable: 启用DM4310电机控制模式函数
* @param[in]:   hcan:    指向CAN_HandleTypeDef结构的指针
* @param[in]:   motor:   指向motor_t结构的指针，包含电机相关信息和控制参数
* @retval:     	void
* @details:    	根据电机控制模式启用相应的模式，通过CAN总线发送启用命令
*               支持的控制模式包括位置模式、位置速度控制模式和速度控制模式
************************************************************************
**/
void dm_motor_enable(hcan_t *hcan, motor_t *motor) {
  switch (motor->ctrl.mode) {
  case mit_mode:
    enable_motor_mode(hcan, motor->id, MIT_MODE);
    break;
  case pos_mode:
    enable_motor_mode(hcan, motor->id, POS_MODE);
    break;
  case spd_mode:
    enable_motor_mode(hcan, motor->id, SPD_MODE);
    break;
  case psi_mode:
    enable_motor_mode(hcan, motor->id, PSI_MODE);
    break;
  default:
    break;
  }
}

err_t dm_motor_enable_can(STM32CAN_t *can, motor_t *motor) {
  if ((can == NULL) || (motor == NULL)) {
    return PTR_NULL;
  }

  const uint16_t mode_id = dm_motor_get_mode_id(motor);
  if (mode_id == 0U) {
    return ARG_ERR;
  }

  return dm_motor_send_mode_cmd_can(can, motor->id, mode_id, 0xFCU);
}
/**
************************************************************************
* @brief:      	dm4310_disable: 禁用DM4310电机控制模式函数
* @param[in]:   hcan:    指向CAN_HandleTypeDef结构的指针
* @param[in]:   motor:   指向motor_t结构的指针，包含电机相关信息和控制参数
* @retval:     	void
* @details:    	根据电机控制模式禁用相应的模式，通过CAN总线发送禁用命令
*               支持的控制模式包括位置模式、位置速度控制模式和速度控制模式
************************************************************************
**/
void dm_motor_disable(hcan_t *hcan, motor_t *motor) {
  switch (motor->ctrl.mode) {
  case mit_mode:
    disable_motor_mode(hcan, motor->id, MIT_MODE);
    break;
  case pos_mode:
    disable_motor_mode(hcan, motor->id, POS_MODE);
    break;
  case spd_mode:
    disable_motor_mode(hcan, motor->id, SPD_MODE);
    break;
  case psi_mode:
    disable_motor_mode(hcan, motor->id, PSI_MODE);
    break;
  default:
    break;
  }
  dm_motor_clear_para(motor);
}

err_t dm_motor_disable_can(STM32CAN_t *can, motor_t *motor) {
  if ((can == NULL) || (motor == NULL)) {
    return PTR_NULL;
  }

  const uint16_t mode_id = dm_motor_get_mode_id(motor);
  if (mode_id == 0U) {
    return ARG_ERR;
  }

  const err_t error = dm_motor_send_mode_cmd_can(can, motor->id, mode_id, 0xFDU);
  dm_motor_clear_para(motor);
  return error;
}
/**
************************************************************************
* @brief:      	dm4310_ctrl_send: 发送DM4310电机控制命令函数
* @param[in]:   hcan:    指向CAN_HandleTypeDef结构的指针
* @param[in]:   motor:   指向motor_t结构的指针，包含电机相关信息和控制参数
* @retval:     	void
* @details:    	根据电机控制模式发送相应的命令到DM4310电机
*               支持的控制模式包括位置模式、位置速度控制模式和速度控制模式
************************************************************************
**/
void dm_motor_ctrl_send(hcan_t *hcan, motor_t *motor) {
  switch (motor->ctrl.mode) {
  case mit_mode:

    mit_ctrl(hcan, motor, motor->id, motor->ctrl.pos_set, motor->ctrl.vel_set,
             motor->ctrl.kp_set, motor->ctrl.kd_set, motor->ctrl.tor_set);
    break;
  case pos_mode:
    pos_ctrl(hcan, motor->id, motor->ctrl.pos_set, motor->ctrl.vel_set);
    break;
  case spd_mode:
    spd_ctrl(hcan, motor->id, motor->ctrl.vel_set);
    break;
  case psi_mode:
    psi_ctrl(hcan, motor->id, motor->ctrl.pos_set, motor->ctrl.vel_set,
             motor->ctrl.cur_set);
    break;
  }
}

err_t dm_motor_ctrl_send_can(STM32CAN_t *can, motor_t *motor) {
  if ((can == NULL) || (motor == NULL)) {
    return PTR_NULL;
  }

  uint8_t data[8] = {0};
  uint16_t id = motor->id;

  switch (motor->ctrl.mode) {
  case mit_mode:
    id = (uint16_t)(motor->id + MIT_MODE);
    dm_motor_pack_mit_ctrl(data, motor, motor->ctrl.pos_set,
                           motor->ctrl.vel_set, motor->ctrl.kp_set,
                           motor->ctrl.kd_set, motor->ctrl.tor_set);
    return dm_motor_send_can(can, id, data, 8U);
  case pos_mode:
    id = (uint16_t)(motor->id + POS_MODE);
    dm_motor_pack_pos_ctrl(data, motor->ctrl.pos_set, motor->ctrl.vel_set);
    return dm_motor_send_can(can, id, data, 8U);
  case spd_mode:
    id = (uint16_t)(motor->id + SPD_MODE);
    dm_motor_pack_spd_ctrl(data, motor->ctrl.vel_set);
    return dm_motor_send_can(can, id, data, 4U);
  case psi_mode:
    id = (uint16_t)(motor->id + PSI_MODE);
    dm_motor_pack_psi_ctrl(data, motor->ctrl.pos_set, motor->ctrl.vel_set,
                           motor->ctrl.cur_set);
    return dm_motor_send_can(can, id, data, 8U);
  default:
    return ARG_ERR;
  }
}

/**
************************************************************************
* @brief:      	dm4310_clear: 清除DM4310电机控制参数函数
* @param[in]:   motor:   指向motor_t结构的指针，包含电机相关信息和控制参数
* @retval:     	void
* @details:    	将DM4310电机的命令参数和控制参数清零，包括位置、速度、
*               比例增益(KP)、微分增益(KD)和扭矩
************************************************************************
**/
void dm_motor_clear_para(motor_t *motor) {
  motor->ctrl.kd_set = 0;
  motor->ctrl.kp_set = 0;
  motor->ctrl.pos_set = 0;
  motor->ctrl.vel_set = 0;
  motor->ctrl.tor_set = 0;
  motor->ctrl.cur_set = 0;
}
/**
************************************************************************
* @brief:      	dm4310_clear_err: 清除DM4310电机错误函数
* @param[in]:   hcan: 	 指向CAN控制结构体的指针
* @param[in]:  	motor:   指向电机结构体的指针
* @retval:     	void
* @details:    	根据电机的控制模式，调用对应模式的清除错误函数
************************************************************************
**/
void dm_motor_clear_err(hcan_t *hcan, motor_t *motor) {
  switch (motor->ctrl.mode) {
  case mit_mode:
    clear_err(hcan, motor->id, MIT_MODE);
    break;
  case pos_mode:
    clear_err(hcan, motor->id, POS_MODE);
    break;
  case spd_mode:
    clear_err(hcan, motor->id, SPD_MODE);
    break;
  case psi_mode:
    clear_err(hcan, motor->id, PSI_MODE);
    break;
  default:
    break;
  }
}

err_t dm_motor_clear_err_can(STM32CAN_t *can, motor_t *motor) {
  if ((can == NULL) || (motor == NULL)) {
    return PTR_NULL;
  }

  const uint16_t mode_id = dm_motor_get_mode_id(motor);
  if (mode_id == 0U) {
    return ARG_ERR;
  }

  return dm_motor_send_mode_cmd_can(can, motor->id, mode_id, 0xFBU);
}
/**
************************************************************************
* @brief:      	dm4310_fbdata: 获取DM4310电机反馈数据函数
* @param[in]:   motor:    指向motor_t结构的指针，包含电机相关信息和反馈数据
* @param[in]:   rx_data:  指向包含反馈数据的数组指针
* @retval:     	void
* @details:    	从接收到的数据中提取DM4310电机的反馈信息，包括电机ID、
*               状态、位置、速度、扭矩以及相关温度参数
************************************************************************
**/
void dm_motor_fbdata(motor_t *motor, uint8_t *rx_data) {
  motor->para.id = (rx_data[0]) & 0x0F;
  motor->para.state = (rx_data[0]) >> 4;
  motor->para.p_int = (rx_data[1] << 8) | rx_data[2];
  motor->para.v_int = (rx_data[3] << 4) | (rx_data[4] >> 4);
  motor->para.t_int = ((rx_data[4] & 0xF) << 8) | rx_data[5];
  motor->para.pos = dm_motor_uint_to_float(motor->para.p_int, -motor->tmp.PMAX,
                                           motor->tmp.PMAX, 16U);
  motor->para.vel = dm_motor_uint_to_float(motor->para.v_int, -motor->tmp.VMAX,
                                           motor->tmp.VMAX, 12U);
  motor->para.tor = dm_motor_uint_to_float(motor->para.t_int, -motor->tmp.TMAX,
                                           motor->tmp.TMAX, 12U);
  motor->para.Tmos = (float)(rx_data[6]);
  motor->para.Tcoil = (float)(rx_data[7]);
}

/**
************************************************************************
* @brief:      	enable_motor_mode: 启用电机模式函数
* @param[in]:   hcan:     指向CAN_HandleTypeDef结构的指针
* @param[in]:   motor_id: 电机ID，指定目标电机
* @param[in]:   mode_id:  模式ID，指定要开启的模式
* @retval:     	void
* @details:    	通过CAN总线向特定电机发送启用特定模式的命令
************************************************************************
**/
void enable_motor_mode(hcan_t *hcan, uint16_t motor_id, uint16_t mode_id) {
  uint8_t data[8];
  uint16_t id = motor_id + mode_id;

  dm_motor_pack_mode_cmd(data, 0xFCU);

  (void)dm_can_send_data(hcan, id, data, 8);
}
/**
************************************************************************
* @brief:      	disable_motor_mode: 禁用电机模式函数
* @param[in]:   hcan:     指向CAN_HandleTypeDef结构的指针
* @param[in]:   motor_id: 电机ID，指定目标电机
* @param[in]:   mode_id:  模式ID，指定要禁用的模式
* @retval:     	void
* @details:    	通过CAN总线向特定电机发送禁用特定模式的命令
************************************************************************
**/
void disable_motor_mode(hcan_t *hcan, uint16_t motor_id, uint16_t mode_id) {
  uint8_t data[8];
  uint16_t id = motor_id + mode_id;

  dm_motor_pack_mode_cmd(data, 0xFDU);

  dm_can_send_data(hcan, id, data, 8);
}
/**
************************************************************************
* @brief:      	save_pos_zero: 保存位置零点函数
* @param[in]:   hcan:     指向CAN_HandleTypeDef结构的指针
* @param[in]:   motor_id: 电机ID，指定目标电机
* @param[in]:   mode_id:  模式ID，指定要保存位置零点的模式
* @retval:     	void
* @details:    	通过CAN总线向特定电机发送保存位置零点的命令
************************************************************************
**/
void save_pos_zero(hcan_t *hcan, uint16_t motor_id, uint16_t mode_id) {
  uint8_t data[8];
  uint16_t id = motor_id + mode_id;

  dm_motor_pack_mode_cmd(data, 0xFEU);

  dm_can_send_data(hcan, id, data, 8);
}
/**
************************************************************************
* @brief:      	clear_err: 清除电机错误函数
* @param[in]:   hcan:     指向CAN_HandleTypeDef结构的指针
* @param[in]:   motor_id: 电机ID，指定目标电机
* @param[in]:   mode_id:  模式ID，指定要清除错误的模式
* @retval:     	void
* @details:    	通过CAN总线向特定电机发送清除错误的命令。
************************************************************************
**/
void clear_err(hcan_t *hcan, uint16_t motor_id, uint16_t mode_id) {
  uint8_t data[8];
  uint16_t id = motor_id + mode_id;

  dm_motor_pack_mode_cmd(data, 0xFBU);

  dm_can_send_data(hcan, id, data, 8);
}
/**
************************************************************************
* @brief:      	mit_ctrl: MIT模式下的电机控制函数
* @param[in]:   hcan:
*指向CAN_HandleTypeDef结构的指针，用于指定CAN总线
* @param[in]:   motor_id:	电机ID，指定目标电机
* @param[in]:   pos:			位置给定值
* @param[in]:   vel:			速度给定值
* @param[in]:   kp:				位置比例系数
* @param[in]:   kd:				位置微分系数
* @param[in]:   torq:			转矩给定值
* @retval:     	void
* @details:    	通过CAN总线向电机发送MIT模式下的控制帧。
************************************************************************
**/
void mit_ctrl(hcan_t *hcan, motor_t *motor, uint16_t motor_id, float pos,
              float vel, float kp, float kd, float tor) {
  uint8_t data[8];
  uint16_t id = motor_id + MIT_MODE;

  dm_motor_pack_mit_ctrl(data, motor, pos, vel, kp, kd, tor);

  dm_can_send_data(hcan, id, data, 8);
}
/**
************************************************************************
* @brief:      	pos_speed_ctrl: 位置速度控制函数
* @param[in]:   hcan:
*指向CAN_HandleTypeDef结构的指针，用于指定CAN总线
* @param[in]:   motor_id:	电机ID，指定目标电机
* @param[in]:   vel:			速度给定值
* @retval:     	void
* @details:    	通过CAN总线向电机发送位置速度控制命令
************************************************************************
**/
void pos_ctrl(hcan_t *hcan, uint16_t motor_id, float pos, float vel) {
  uint16_t id;
  uint8_t data[8];

  id = motor_id + POS_MODE;
  dm_motor_pack_pos_ctrl(data, pos, vel);

  dm_can_send_data(hcan, id, data, 8);
}
/**
************************************************************************
* @brief:      	speed_ctrl: 速度控制函数
* @param[in]:   hcan: 		指向CAN_HandleTypeDef结构的指针，用于指定CAN总线
* @param[in]:   motor_id: 电机ID，指定目标电机
* @param[in]:   vel: 			速度给定值
* @retval:     	void
* @details:    	通过CAN总线向电机发送速度控制命令
************************************************************************
**/
void spd_ctrl(hcan_t *hcan, uint16_t motor_id, float vel) {
  uint16_t id;
  uint8_t data[4];

  id = motor_id + SPD_MODE;
  dm_motor_pack_spd_ctrl(data, vel);

  dm_can_send_data(hcan, id, data, 4);
}

/**
************************************************************************
* @brief:      	pos_speed_ctrl: 混控模式
* @param[in]:   hcan:
*指向CAN_HandleTypeDef结构的指针，用于指定CAN总线
* @param[in]:   motor_id:	电机ID，指定目标电机
* @param[in]:   pos:			位置给定值
* @param[in]:   vel:			速度给定值
* @param[in]:   i:				电流给定值
* @retval:     	void
* @details:    	通过CAN总线向电机发送位置速度控制命令
************************************************************************
**/

void psi_ctrl(hcan_t *hcan, uint16_t motor_id, float pos, float vel,
              float cur) {
  uint16_t id;
  uint8_t data[8];

  id = motor_id + PSI_MODE;
  dm_motor_pack_psi_ctrl(data, pos, vel, cur);

  dm_can_send_data(hcan, id, data, 8);
}
/**
************************************************************************
* @brief:      	read_motor_data: 发送读取寄存器命令
* @param[in]:   id:    电机can id
* @param[in]:   rid:   寄存器地址
* @retval:     	void
* @details:    	读取电机参数
************************************************************************
**/
void read_motor_data(hcan_t *hcan, uint16_t id, uint8_t rid) {
  uint8_t can_id_l = id & 0xFF;        // 低 8 位
  uint8_t can_id_h = (id >> 8) & 0x07; // 高 3 位

  uint8_t data[4] = {can_id_l, can_id_h, 0x33, rid};
  dm_can_send_data(hcan, 0x7FF, data, 4);
}

/**
************************************************************************
* @brief:      	read_motor_ctrl_fbdata: 发送读取电机反馈数据的命令
* @param[in]:   id:    电机can id
* @retval:     	void
* @details:    	读取电机控制反馈的数据
************************************************************************
**/
void read_motor_ctrl_fbdata(hcan_t *hcan, uint16_t id) {
  uint8_t can_id_l = id & 0xFF;        // 低 8 位
  uint8_t can_id_h = (id >> 8) & 0x07; // 高 3 位

  uint8_t data[4] = {can_id_l, can_id_h, 0xCC, 0x00};
  dm_can_send_data(hcan, 0x7FF, data, 4);
}

/**
************************************************************************
* @brief:      	write_motor_data: 发送写寄存器命令
* @param[in]:   id:    电机can id
* @param[in]:   rid:   寄存器地址
* @param[in]:   d0-d3: 写入的数据
* @retval:     	void
* @details:    	向寄存器写入数据
************************************************************************
**/
void write_motor_data(hcan_t *hcan, uint16_t id, uint8_t rid, uint8_t d0,
                      uint8_t d1, uint8_t d2, uint8_t d3) {
  uint8_t can_id_l = id & 0x0F;
  uint8_t can_id_h = (id >> 4) & 0x0F;

  uint8_t data[8] = {can_id_l, can_id_h, 0x55, rid, d0, d1, d2, d3};
  dm_can_send_data(hcan, 0x7FF, data, 8);
}
/**
************************************************************************
* @brief:      	save_motor_data: 发送保存命令
* @param[in]:   id:    电机can id
* @param[in]:   rid:   寄存器地址
* @retval:     	void
* @details:    	保存写入的电机参数
************************************************************************
**/
void save_motor_data(hcan_t *hcan, uint16_t id, uint8_t rid) {
  uint8_t can_id_l = id & 0xFF;        // 低 8 位
  uint8_t can_id_h = (id >> 8) & 0x07; // 高 3 位

  uint8_t data[4] = {can_id_l, can_id_h, 0xAA, 0x01};
  dm_can_send_data(hcan, 0x7FF, data, 4);
}
