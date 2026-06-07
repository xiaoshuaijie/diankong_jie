
#include "stm32f405xx.h"
#include <stdbool.h>
#include <stdint.h>
#include "joint_control.h"
#include "dm_motor_drv.h"
#include "comp_utils.h"
#include "FreeRTOS.h"
#include "task.h"
#include "vt13.h"
#include "cmsis_os2.h"
#include "pid_incremental.h"
extern motor_t motor[num];
extern  vt13_cmd_rc_t vt13_cmd_rc;
uint8_t joint_enable_single = 0U;
 uint8_t joint_mode = 0U;
 uint8_t joint_send_phase = 0U;
 Joint_mode_t joint_mode_t;
bool joint_send_flag[2] = {false, false};
extern uint16_t TB6210_angle;
// 串联六轴关节编号示意（底座 -> 末端执行器）：
//   底座 -> [Motor1] -> [Motor2] -> [Motor3] -> [Motor4] -> [Motor5] -> [Motor6] -> 末端
//             下关节组（joint_down_ctrl）        上关节组（joint_up_ctrl）
//   pos[0..5] 与 Motor1..Motor6 一一对应，角度正负方向沿用各轴零点和限位定义。
// 关节电机目标角
float pos[num] = {1.2f, 0.0f, 0.0f, -2.7f, -1.15f, 0.5f};
static const float joint_angle_limit_min[num] = {
    JOINT_MOTOR1_ANGLE_LIMIT_MIN, JOINT_MOTOR2_ANGLE_LIMIT_MIN,
    JOINT_MOTOR3_ANGLE_LIMIT_MIN, JOINT_MOTOR4_ANGLE_LIMIT_MIN,
    JOINT_MOTOR5_ANGLE_LIMIT_MIN, JOINT_MOTOR6_ANGLE_LIMIT_MIN,
};

static const float joint_angle_limit_max[num] = {
    JOINT_MOTOR1_ANGLE_LIMIT_MAX, JOINT_MOTOR2_ANGLE_LIMIT_MAX,
    JOINT_MOTOR3_ANGLE_LIMIT_MAX, JOINT_MOTOR4_ANGLE_LIMIT_MAX,
    JOINT_MOTOR5_ANGLE_LIMIT_MAX, JOINT_MOTOR6_ANGLE_LIMIT_MAX,
};
 PIDInstance_jie pid_location_joint[num];
// 速度环PID实例
 PIDInstance_jie pid_speed_joint[num];
void joint_scan(uint16_t *mode,uint16_t *joint_mode)
{
  if(*mode==1)
  {
    if(*joint_mode==0U)
    {
   *joint_mode=3;  //按下时长按
     }
  else if(*mode==2)
  {
    *joint_mode=1;
  }
}
  else 
  {
if(*joint_mode==3)
  {
    *joint_mode=2;
  }
  else if(*joint_mode==1)
{
  *joint_mode=0U;
}
  }
  

}

//键鼠控制
void joint_mouse_ctrl(void)
{
  if(joint_mode_t.A==3U)
  {
TB6210_angle+=20;
CONSTRAIN_PTR(&TB6210_angle, 0, 180);
  }
  if(joint_mode_t.D==3U)
  {
TB6210_angle-=20;
CONSTRAIN_PTR(&TB6210_angle, 0, 180);
  }
  //   if(joint_mode_t.B==0)
  //   {
  // if(joint_mode_t.A==3)
  // {
  //   pos[0]+=0.0005;
  // }
  // if(joint_mode_t.D==3)
  // {
  //   pos[0]-=0.0005;
  // }
  // if(joint_mode_t.W==3)
  // {
  //   pos[1]-=0.0005;
  // }
  // if(joint_mode_t.S==3)
  // {
  //   pos[1]+=0.0005;
  // }
  // if(joint_mode_t.Q==3)
  // {
  //   pos[2]+=0.0005;
  // }
  // if(joint_mode_t.E==3)
  // {
  //   pos[2]-=0.0005;
  // }
  //   }


  // else if(joint_mode_t.B==2)
  //   {
  // if(joint_mode_t.A==3)
  // {
  //   pos[3]+=0.0005;
  // }
  // if(joint_mode_t.D==3)
  // {
  //   pos[3]-=0.0005;
  // }
  // if(joint_mode_t.W==3)
  // {
  //   pos[4]-=0.0005;
  // }
  // if(joint_mode_t.S==3)
  // {
  //   pos[4]+=0.0005;
  // }
  // if(joint_mode_t.Q==3)
  // {
  //   pos[5]+=0.0005;
  // }
  // if(joint_mode_t.E==3)
  // {
  //   pos[5]-=0.0005;
  // }
  //   }


}



//位置发送
static void joint_constrain_target(motor_num motor_id, float *target_angle) {
  ASSERT(motor_id < num);
  ASSERT(target_angle != NULL);

  JOINT_CONSTRAIN_TARGET_BY_ID(target_angle, motor_id);
}
void one_return(void)
{
  if(vt13_cmd_rc.B==1)
pos[0]=1.2f;
}


 void joint_enable(void)
{
  dm_motor_enable(&hcan1, &motor[0]);
  dm_motor_enable(&hcan1, &motor[1]);
  osDelay(1);
  dm_motor_enable(&hcan1, &motor[2]);
 
  dm_motor_enable(&hcan1, &motor[3]);
   osDelay(1);
  dm_motor_enable(&hcan1, &motor[4]);
  dm_motor_enable(&hcan1, &motor[5]);
  osDelay(1);

  joint_send_phase = 0U;
  
}
static void joint_disable(void)
{
  joint_enable_single = 1U;
  joint_send_phase = 0U;

  for(uint8_t i = 0; i < num; i++)
  {
    dm_motor_disable(&hcan1, &motor[i]);
    osDelay(1);
  }
}
void joint_up_ctrl(void)
{
  // if(joint_send_flag[0]==false)
  // {  
  // save_pos_zero(&hcan1, motor[Motor4].id, PSI_MODE);
  // save_pos_zero(&hcan1, motor[Motor5].id, PSI_MODE);
  // save_pos_zero(&hcan1, motor[Motor6].id, PSI_MODE); 
    
  // }
  // joint_send_flag[0] = true;

pos[3]+=vt13_cmd_rc.ch.l.x*0.0005;
pos[4]-=vt13_cmd_rc.ch.l.y*0.0005;
pos[5]-=vt13_cmd_rc.ch.r.x*0.0005;
//joint_constrain_target(3, &pos[3]);
//joint_constrain_target(4, &pos[4]);
//oint_constrain_target(5, &pos[5]);
// for (uint8_t i = 3; i < num; i++) {

//   pos_ctrl(&hcan1, motor[i].id, pos[i], JOINT_FIXED_TARGET_SPEED);
  
//    }
  

}
void joint_send_pos(void)
{


  /* Split position commands across 3 cycles to avoid short burst writes. */
  switch (joint_send_phase) {
  case 0:
    pos_ctrl(&hcan1, motor[Motor1].id, pos[0], 0.5f);
    pos_ctrl(&hcan1, motor[Motor2].id, pos[1], 0.5f);
    break;
  case 1:
    pos_ctrl(&hcan1, motor[Motor3].id, pos[2], 0.5f);
    
    pos_ctrl(&hcan1, motor[Motor4].id, pos[3], 0.5f);
    break;
  default:
    pos_ctrl(&hcan1, motor[Motor5].id, pos[4], 0.5f);
    pos_ctrl(&hcan1, motor[Motor6].id, pos[5], 0.5f);
    break;
  }

  joint_send_phase = (uint8_t)((joint_send_phase + 1U) % 3U);
}
 void joint_down_ctrl(void)
{
  // if(joint_send_flag[1]==false)
  // {
  // save_pos_zero(&hcan1, motor[Motor1].id, PSI_MODE);
  // save_pos_zero(&hcan1, motor[Motor2].id, PSI_MODE);
  // save_pos_zero(&hcan1, motor[Motor3].id, PSI_MODE);
  
  // }
  
  // joint_send_flag[1] = true;
pos[0]-=vt13_cmd_rc.ch.l.x*0.0005;
pos[1]-=vt13_cmd_rc.ch.l.y*0.0005;
pos[2]+=vt13_cmd_rc.ch.r.y*0.0005;
joint_constrain_target(0, &pos[0]);  //关节1由于零点问题，不需要限位
joint_constrain_target(1, &pos[1]);
joint_constrain_target(2, &pos[2]); 

// for (uint8_t i = 0; i < 3; i++) {

//   pos_ctrl(&hcan1, motor[i].id, pos[i], JOINT_FIXED_TARGET_SPEED);
  
//    }
 
}
void joint_mode_change(void)
{
if(vt13_cmd_rc.func.fn_1==1)
{
  joint_mode = 1U;
}
if(vt13_cmd_rc.func.fn_2==1)
{
  joint_mode = 2U;
}

}

void Joint_Mode(void)
{
  
  one_return();
  if (vt13_cmd_rc.mode_sw == vt13_CMD_SW_UP) {
 
      joint_mode = 0U;
      joint_disable();
  
   
  }

  if (vt13_cmd_rc.mode_sw == vt13_CMD_SW_MID) {
   
      joint_enable();
      joint_mode = 0U;
    
   
  }
joint_mouse_ctrl();
  if (vt13_cmd_rc.mode_sw == vt13_CMD_SW_DOWN) {
    joint_mode_change();
    if (joint_mode == 1U) {
      joint_down_ctrl();
    }
    if (joint_mode == 2U) {
      joint_up_ctrl();
    }
    joint_send_pos();
  }
}










