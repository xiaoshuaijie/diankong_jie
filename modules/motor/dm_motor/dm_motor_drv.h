#ifndef __DM_MOTOR_DRV_H__
#define __DM_MOTOR_DRV_H__
#include "bsp_can.h"
#include "can.h"
#include "dm_motor_def.h"
#include "main.h"

#define MIT_MODE 0x000
#define POS_MODE 0x100
#define SPD_MODE 0x200
#define PSI_MODE 0x300

#define KP_MIN 0.0f
#define KP_MAX 500.0f
#define KD_MIN 0.0f
#define KD_MAX 5.0f

typedef enum {
  Motor1,
  Motor2,
  Motor3,
  Motor4,
  Motor5,
  Motor6,

  num
} motor_num;

typedef enum { mit_mode = 1, pos_mode = 2, spd_mode = 3, psi_mode = 4 } mode_e;

void dm_motor_ctrl_send(hcan_t *hcan, motor_t *motor);
err_t dm_motor_ctrl_send_can(STM32CAN_t *can, motor_t *motor);
void dm_motor_enable(hcan_t *hcan, motor_t *motor);
err_t dm_motor_enable_can(STM32CAN_t *can, motor_t *motor);
void dm_motor_disable(hcan_t *hcan, motor_t *motor);
err_t dm_motor_disable_can(STM32CAN_t *can, motor_t *motor);
void dm_motor_clear_para(motor_t *motor);
void dm_motor_clear_err(hcan_t *hcan, motor_t *motor);
err_t dm_motor_clear_err_can(STM32CAN_t *can, motor_t *motor);
void dm_motor_fbdata(motor_t *motor, uint8_t *rx_data);

void enable_motor_mode(hcan_t *hcan, uint16_t motor_id, uint16_t mode_id);
void disable_motor_mode(hcan_t *hcan, uint16_t motor_id, uint16_t mode_id);

void mit_ctrl(hcan_t *hcan, motor_t *motor, uint16_t motor_id, float pos,
              float vel, float kp, float kd, float tor);
void pos_ctrl(hcan_t *hcan, uint16_t motor_id, float pos, float vel);
void spd_ctrl(hcan_t *hcan, uint16_t motor_id, float vel);
void psi_ctrl(hcan_t *hcan, uint16_t motor_id, float pos, float vel, float cur);

void save_pos_zero(hcan_t *hcan, uint16_t motor_id, uint16_t mode_id);
void clear_err(hcan_t *hcan, uint16_t motor_id, uint16_t mode_id);

void read_motor_data(hcan_t *hcan, uint16_t id, uint8_t rid);
void read_motor_ctrl_fbdata(hcan_t *hcan, uint16_t id);
void write_motor_data(hcan_t *hcan, uint16_t id, uint8_t rid, uint8_t d0,
                      uint8_t d1, uint8_t d2, uint8_t d3);
void save_motor_data(hcan_t *hcan, uint16_t id, uint8_t rid);

#endif /* __DM_MOTOR_DRV_H__ */
