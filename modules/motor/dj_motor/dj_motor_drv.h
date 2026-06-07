#ifndef __DJ_MOTOR_DRV_H__
#define __DJ_MOTOR_DRV_H__

#include "bsp_can.h"
#include "can.h"
#include "dj_motor_def.h"
#include "main.h"
// 大疆电机数量定义
#define DJ_MOTOR_NUM 8

// 大疆电机控制枚举
typedef enum {
  DJ_MOTOR1 = 0,
  DJ_MOTOR2,
  DJ_MOTOR3,
  DJ_MOTOR4,
  DJ_MOTOR5,
  DJ_MOTOR6,
  DJ_MOTOR7,
  DJ_MOTOR8,
  DJ_MOTOR_MAX
} dj_motor_num_e;

// 函数声明

/**
 * @brief 初始化大疆电机
 * @param motor 电机结构体指针
 * @param motor_num 电机编号
 * @param type 电机类型 (M3508/M6020/M2006)
 * @param motor_id 电机CAN ID (0x201-0x208)
 */
void dj_motor_init(dj_motor_t *motor, dj_motor_num_e motor_num,
                   dj_motor_type_e type, uint16_t motor_id);

/**
 * @brief 设置大疆电机控制电流(带队列)
 * @param hcan CAN句柄
 * @param group 电机组 (GROUP_1控制电机1-4, GROUP_2控制电机5-8)
 * @param current1 电机1电流值 (-16384~16384)
 * @param current2 电机2电流值 (-16384~16384)
 * @param current3 电机3电流值 (-16384~16384)
 * @param current4 电机4电流值 (-16384~16384)
 * @return 1=直接发送成功, 0=放入队列或队列满
 */
uint8_t dj_motor_set_current(hcan_t *hcan, dj_motor_group_e group,
                             int16_t current1, int16_t current2, int16_t current3,
                             int16_t current4);

/**
 * @brief 解析大疆电机反馈数据
 * @param motor 电机结构体指针
 * @param rx_data 接收到的CAN数据数组
 */
void dj_motor_parse_feedback(dj_motor_t *motor, const uint8_t *rx_data);

/**
 * @brief 根据电机类型获取编码器分辨率
 * @param type 电机类型
 * @return 编码器分辨率
 */
uint16_t dj_motor_get_encoder_res(dj_motor_type_e type);

/**
 * @brief 根据电机类型获取减速比
 * @param type 电机类型
 * @return 减速比
 */
float dj_motor_get_reduction_ratio(dj_motor_type_e type);

/**
 * @brief 将编码器值转换为角度(弧度)
 * @param encoder 编码器值
 * @param encoder_res 编码器分辨率
 * @return 角度(弧度, 0-2PI)
 */
float dj_motor_encoder_to_angle(uint16_t encoder, uint16_t encoder_res);

/**
 * @brief 将转速(RPM)转换为角速度(rad/s)
 * @param rpm 转速(RPM)
 * @param reduction_ratio 减速比
 * @return 角速度(rad/s)
 */
float dj_motor_rpm_to_rads(int16_t rpm, float reduction_ratio);

void dj_motor_clear_control(dj_motor_t *motor);

#endif /* __DJ_MOTOR_DRV_H__ */
