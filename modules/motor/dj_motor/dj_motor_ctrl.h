#ifndef __DJ_MOTOR_CTRL_H__
#define __DJ_MOTOR_CTRL_H__

#include "bsp_can.h"
#include "dj_motor_drv.h"
#include "main.h"

// 大疆电机数组声明
extern dj_motor_t dj_motor[DJ_MOTOR_MAX];

// 函数声明

/**
 * @brief 大疆电机系统初始化
 * @details 初始化所有大疆电机，包括M3508、M6020、M2006
 */
void dj_motor_system_init(void);

/**
 * @brief 初始化 DJI 电机反馈静态队列
 * @return err_t OK=初始化成功
 */
err_t dj_motor_feedback_queue_init(void);

/**
 * @brief 在任务中刷新 DJI 电机反馈
 * @param timeout_ms 保留参数，当前实现非阻塞消费每个电机的最新反馈
 * @return err_t OK=刷新成功
 */
err_t dj_motor_update_feedback(uint32_t timeout_ms);

/**
 * @brief 绑定对象式 CAN 接收回调
 * @param can CAN BSP 控制块
 * @return err_t OK=绑定成功, 其他=参数错误
 */
err_t dj_motor_attach_can(STM32CAN_t *can);

/**
 * @brief 大疆电机控制发送函数
 * @param hcan CAN句柄
 * @details 发送控制电流到所有电机
 */
void dj_motor_control_send(hcan_t *hcan);

/**
 * @brief 使用对象式 CAN 发送 1~4 号电机控制电流
 * @param can CAN BSP 控制块
 * @return err_t OK=发送成功
 */
err_t dj_motor_control_send_can(STM32CAN_t *can);

// CAN回调函数声明已在 bsp_can.h 中

/**
 * @brief 读取电机反馈数据
 * @param motor_id 电机ID (0x201-0x208)
 * @return 电机结构体指针
 */
dj_motor_t *dj_motor_get_feedback(uint16_t motor_id);

/**
 * @brief 设置电机电流控制
 * @param motor_id 电机ID (1-8)
 * @param current 电流值 (-16384~16384)
 */
void dj_motor_set_current_by_id(uint8_t motor_id, int16_t current);


#endif /* __DJ_MOTOR_CTRL_H__ */
