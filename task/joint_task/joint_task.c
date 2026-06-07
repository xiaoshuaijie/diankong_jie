#include "can.h"
#include "cmsis_os2.h"
#include "dm_motor_def.h"
#include "dm_motor_drv.h"
#include "stm32f405xx.h"
#define LOG_LOCAL_PORT BSP_LOG_PORT_CTRL

#include "joint_task.h"
#include "joint_control.h"
#include "dm_motor_ctrl.h"
#include "bsp_log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "bsp_can.h"
#include "cmsis_os.h"
extern motor_t motor[num]; 
extern float pos[num];
void Start_joint_task(void *argument) {
  RM_UNUSED(argument);
  
  

  LOGINFO("[joint_task] start");

  // 先初始化 DM 电机对象，补齐关节控制依赖的 ID、模式与力矩范围
  dm_motor_init();

  // save_pos_zero(&hcan1, motor[Motor1].id, PSI_MODE);
  // save_pos_zero(&hcan1, motor[Motor2].id, PSI_MODE);
  // save_pos_zero(&hcan1, motor[Motor3].id, PSI_MODE);
   // save_pos_zero(&hcan1, motor[Motor4].id, PSI_MODE);
  // save_pos_zero(&hcan1, motor[Motor5].id, PSI_MODE);
  // save_pos_zero(&hcan1, motor[Motor6].id, PSI_MODE); 
  // 初始化关节位置为0
   //save_pos_zero(&hcan1, motor[Motor4].id, PSI_MODE);

  LOGINFO("[joint_task] ready");
  

  while (1) {
    // 保持稳定的 1ms 调度节拍，避免额外调度抖动放大 CAN 发送卡顿
    Joint_Mode();
    osDelay(1);
  
    // joint_ctrl();
//    for(int i=Motor1;i<num;i++){
//     dm_motor_enable(&hcan1,&motor[i]);
//    }
// // for (uint8_t i = 0; i < num; i++) {

// //   pos_ctrl(&hcan1, i, pos[i], JOINT_FIXED_TARGET_SPEED);
// //   vTaskDelay(1);
// //    }
//
    
  }
}
