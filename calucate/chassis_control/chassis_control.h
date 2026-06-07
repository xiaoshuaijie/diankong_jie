/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CALUCATE_CHASSIS_CONTROL_H
#define CALUCATE_CHASSIS_CONTROL_H



/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef struct
{
    float vx;
    float vy;
    float wz;
} chassis_control_command_t;

typedef struct
{
    chassis_control_command_t command;
    float command_limit;
    uint8_t enabled;
} chassis_control_state_t;


/**
  * @brief  Initialize the chassis motor speed PID controllers.
  * @retval None
  */
void dj_motor_speed_pid_init(void);

/**
  * @brief  Run one chassis control cycle.
  * @retval None
  */
void Chassis_Mode(void);



#endif /* CALUCATE_CHASSIS_CONTROL_H */
