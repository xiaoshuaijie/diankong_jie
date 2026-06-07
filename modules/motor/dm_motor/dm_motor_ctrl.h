#ifndef __DM_MOTOR_CTRL_H__
#define __DM_MOTOR_CTRL_H__
#include "bsp_can.h"
#include "dm_motor_drv.h"
#include "main.h"
#include <stdint.h>
extern int8_t motor_id;

// extern uint32_t motor1_data_sent;
// extern uint32_t motor2_data_sent;
// extern uint32_t motor3_data_sent;
// extern uint32_t motor4_data_sent;

extern motor_t motor[num];

typedef union {
  float f_val;
  uint32_t u_val;
  uint8_t b_val[4];
} float_type_u;

void dm_motor_init(void);

err_t dm_motor_feedback_queue_init(void);
err_t dm_motor_update_feedback(uint32_t timeout_ms);
err_t dm_motor_attach_can(STM32CAN_t *can);

void read_all_motor_data(hcan_t *hcan, motor_t *motor);
void receive_motor_data(motor_t *motor, uint8_t *data);

// CAN接收回调函数声明已在 bsp_can.h 中

// 电机数据解析和打包
void dm_motor_parse_fb_data(motor_t *motor, uint8_t *rx_data);
void dm_motor_pack_ctrl_data(motor_t *motor, uint8_t *tx_data);

// 电机数据转换函数
int read_dm_motor_pos(float pos_float, float p_max, float p_min);
int read_dm_motor_vel(float vel_float, float v_max, float v_min);
int read_dm_motor_tor(float tor_float, float t_max, float t_min);
float set_dm_motor_pos(int pos_int, float p_max, float p_min);
float set_dm_motor_vel(int vel_int, float v_max, float v_min);
float set_dm_motor_tor(int tor_int, float t_max, float t_min);

#endif /* __DM_MOTOR_CTRL_H__ */
