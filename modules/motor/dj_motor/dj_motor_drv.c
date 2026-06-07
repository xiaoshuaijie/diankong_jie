#include "dj_motor_drv.h"

#include "comp_utils.h"
#include "string.h"

// PI定义
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static float dj_motor_normalize_angle_deg(float angle) {
  while (angle >= 360.0f) {
    angle -= 360.0f;
  }

  while (angle < 0.0f) {
    angle += 360.0f;
  }

  return angle;
}

/**
************************************************************************
* @brief:      	dj_motor_init: 大疆电机初始化函数
* @param[in]:   motor:    指向dj_motor_t结构的指针
* @param[in]:   motor_num: 电机编号枚举
* @param[in]:   type:     电机类型 (M3508/M6020/M2006)
* @param[in]:   motor_id: 电机CAN ID (0x201-0x208)
* @retval:     	void
* @details:    	初始化大疆电机结构体，设置电机类型、ID和默认参数
************************************************************************
**/
void dj_motor_init(dj_motor_t *motor, dj_motor_num_e motor_num,
                   dj_motor_type_e type, uint16_t motor_id) {
  ASSERT(motor);
  RM_UNUSED(motor_num);

  memset(motor, 0, sizeof(dj_motor_t));

  motor->motor_id = motor_id;
  motor->type = type;

  // 初始化反馈数据
  motor->feedback.encoder = 0;
  motor->feedback.rpm_speed = 0;
  motor->feedback.current = 0;
  motor->feedback.temp = 0;
  motor->feedback.last_temp = 0;
  motor->feedback.angle = 0.0f;
  motor->feedback.actual_angle_deg = 0.0f;
  motor->feedback.speed_rads = 0.0f;

  // 初始化控制参数
  motor->control.current_set = 0;
  motor->control.speed_set = 0.0f;
  motor->control.angle_set = 0.0f;
  motor->control.motor_id = motor_id;
  motor->control.type = type;
}

/**
************************************************************************
* @brief:      	dj_motor_set_current: 设置大疆电机控制电流(带队列)
* @param[in]:   group:    电机组 (GROUP_1控制电机1-4, GROUP_2控制电机5-8)
* @param[in]:   current1: 电机1电流值 (-16384~16384)
* @param[in]:   current2: 电机2电流值 (-16384~16384)
* @param[in]:   current3: 电机3电流值 (-16384~16384)
* @param[in]:   current4: 电机4电流值 (-16384~16384)
* @retval:     	uint8_t: 1=直接发送成功, 0=放入队列或队列满
* @details:    	通过CAN总线发送控制电流到指定电机组的4个电机
*               使用带队列的发送函数，避免发送失败时数据丢失
*               标准帧ID 0x200控制电机1-4, 0x1FF控制电机5-8
************************************************************************
**/
uint8_t dj_motor_set_current(hcan_t *hcan, dj_motor_group_e group,
                             int16_t current1, int16_t current2, int16_t current3,
                             int16_t current4) {
  return dj_CAN_Send_Data(hcan, (uint16_t)group, current1, current2, current3, current4, 8);
}

/**
************************************************************************
* @brief:      	dj_motor_parse_feedback: 解析大疆电机反馈数据
* @param[in]:   motor:    电机结构体指针
* @param[in]:   rx_data:  接收到的CAN数据数组
* @retval:     	void
* @details:    	解析大疆电机反馈的8字节数据，提取机械角度、转速、电流和温度
*               数据格式:
*[角度高8位|角度低8位|转速高8位|转速低8位|电流高8位|电流低8位|温度|保留]
************************************************************************
**/
void dj_motor_parse_feedback(dj_motor_t *motor, const uint8_t *rx_data) {
  // 参数检查
  if (motor == NULL || rx_data == NULL) {
    return;
  }

  // 保存上次温度值
  motor->feedback.last_temp = motor->feedback.temp;

  // 解析机械角度编码器值 (0-8191)
  motor->feedback.encoder = ((uint16_t)rx_data[0] << 8) | rx_data[1];

  // 解析转速 (RPM, 有符号16位)
  motor->feedback.rpm_speed = ((int16_t)rx_data[2] << 8) | rx_data[3];

  // 解析扭矩电流 (有符号16位)
  motor->feedback.current = ((int16_t)rx_data[4] << 8) | rx_data[5];

  // 解析温度
  motor->feedback.temp = rx_data[6];

  // 获取编码器分辨率
  uint16_t encoder_res = dj_motor_get_encoder_res(motor->type);

  // 计算角度(弧度, 0-2PI)
  motor->feedback.angle =
      dj_motor_encoder_to_angle(motor->feedback.encoder, encoder_res);

  // 计算实际角度(度数, 0-360)
  float angle_deg = ((float)motor->feedback.encoder / encoder_res) * 360.0f;
  motor->feedback.actual_angle_deg = dj_motor_normalize_angle_deg(angle_deg);

  // 计算角速度(弧度/秒)
  float reduction_ratio = dj_motor_get_reduction_ratio(motor->type);
  motor->feedback.speed_rads =
      dj_motor_rpm_to_rads(motor->feedback.rpm_speed, reduction_ratio);
}

/**
************************************************************************
* @brief:      	dj_motor_get_encoder_res: 根据电机类型获取编码器分辨率
* @param[in]:   type: 电机类型
* @retval:     	编码器分辨率
* @details:    	返回对应电机型号的编码器分辨率
************************************************************************
**/
uint16_t dj_motor_get_encoder_res(dj_motor_type_e type) {
  switch (type) {
  case DJ_MOTOR_M3508:
    return M3508_ENCODER_RES;
  case DJ_MOTOR_M6020:
    return M6020_ENCODER_RES;
  case DJ_MOTOR_M2006:
    return M2006_ENCODER_RES;
  default:
    return 8192;
  }
}

/**
************************************************************************
* @brief:      	dj_motor_get_reduction_ratio: 根据电机类型获取减速比
* @param[in]:   type: 电机类型
* @retval:     	减速比
* @details:    	返回对应电机型号的减速比
************************************************************************
**/
float dj_motor_get_reduction_ratio(dj_motor_type_e type) {
  switch (type) {
  case DJ_MOTOR_M3508:
    return (float)M3508_REDUCTION_RATIO;
  case DJ_MOTOR_M6020:
    return (float)M6020_REDUCTION_RATIO;
  case DJ_MOTOR_M2006:
    return (float)M2006_REDUCTION_RATIO;
  default:
    return 1.0f;
  }
}

/**
************************************************************************
* @brief:      	dj_motor_encoder_to_angle: 将编码器值转换为角度(弧度)
* @param[in]:   encoder: 编码器值
* @param[in]:   encoder_res: 编码器分辨率
* @retval:     	角度(弧度, 0-2PI)
* @details:    	将编码器原始值转换为0-2PI范围内的弧度角度
************************************************************************
**/
float dj_motor_encoder_to_angle(uint16_t encoder, uint16_t encoder_res) {
  return ((float)encoder / (float)encoder_res) * 2.0f * M_PI;
}

/**
************************************************************************
* @brief:      	dj_motor_rpm_to_rads: 将转速(RPM)转换为角速度(rad/s)
* @param[in]:   rpm: 转速(RPM)
* @param[in]:   reduction_ratio: 减速比
* @retval:     	角速度(rad/s)
* @details:    	将RPM转速转换为弧度/秒单位的角速度，考虑减速比
************************************************************************
**/
float dj_motor_rpm_to_rads(int16_t rpm, float reduction_ratio) {
  return ((float)rpm / 60.0f * 2.0f * M_PI) / reduction_ratio;
}

/**
************************************************************************
* @brief:      	dj_motor_clear_control: 清除电机控制参数
* @param[in]:   motor: 电机结构体指针
* @retval:     	void
* @details:    	将电机的控制参数清零，包括电流、速度和角度设定值
************************************************************************
**/
void dj_motor_clear_control(dj_motor_t *motor) {
  ASSERT(motor);

  motor->control.current_set = 0;
  motor->control.speed_set = 0.0f;
  motor->control.angle_set = 0.0f;
}
