#include "chassis_control.h"
#include "comp_cmd.h"
#include "comp_utils.h"
#include "pid_location.h"
#include "dj_motor_ctrl.h"
#include "vt13.h"

// 麦克纳姆轮底盘运动学参数
// 电机布局: Motor1-左前, Motor2-右前, Motor3-左后, Motor4-右后
// 前方
//  1   2
//  3   4
__weak vt13_cmd_rc_t vt13_cmd_rc;
__weak uint8_t joint_enable_single = 1U;

PIDInstance pid_dj_speed[DJ_MOTOR_MAX];
chassis_control_state_t chassis_control_state_;
float motor_speed[DJ_MOTOR_MAX];

static void dj_motor_speed_pid_init_single(PIDInstance *pid, float kp, float ki,
                                           float kd, float max_out) {
  PID_Init_Config_s config = {
      .Kp = kp,
      .Ki = ki,
      .Kd = kd,
      .MaxOut = max_out,
      .DeadBand = 0.0f,
      .Improve = PID_Integral_Limit | PID_Derivative_On_Measurement |
                 PID_OutputFilter | PID_DerivativeFilter,
      .IntegralLimit = 3000.0f,
      .CoefA = 0.0f,
      .CoefB = 0.0f,
      .Output_LPF_RC = 0.0002f,
      .Derivative_LPF_RC = 0.0002f,
  };

  PIDInit(pid, &config);
}
/**
 * @brief 初始化底盘电机速度环PID控制器
 * @details 初始化4个底盘电机的速度环PID参数（单速度环控制）
 */
void dj_motor_speed_pid_init(void) {
  dj_motor_speed_pid_init_single(&pid_dj_speed[DJ_MOTOR1], 12.0f, 0.0f, 0.0f,
                                 12000.0f);
  dj_motor_speed_pid_init_single(&pid_dj_speed[DJ_MOTOR2], 8.0f, 0.0f, 0.0f,
                                 12000.0f);
  dj_motor_speed_pid_init_single(&pid_dj_speed[DJ_MOTOR3], 8.0f, 0.0f, 0.0f,
                                 12000.0f);
  dj_motor_speed_pid_init_single(&pid_dj_speed[DJ_MOTOR4], 14.0f, 2.0f, 0.0f,
                                 12000.0f);
}
/**
 * @brief 单速度环PID控制函数
 * @details 直接根据目标速度控制电机电流，不涉及位置环
 * @param motor_id 电机ID (0-3, 对应DJ_MOTOR1-DJ_MOTOR4)
 * @param target_speed 目标速度 (rpm)
 * @note 适用于需要直接控制转速的场景
 */
void dj_motor_pid_control_speed(uint8_t motor_id, float target_speed) {
  // 参数检查
  if (motor_id >= DJ_MOTOR_MAX)
    return;

  // 获取当前电机转速
  float current_speed = dj_motor[motor_id].feedback.rpm_speed;




  // 速度环PID计算，输出目标扭矩/电流
  float motor_current =
      PIDCalculate(&pid_dj_speed[motor_id], current_speed, target_speed);

  // 限制电流输出范围
  motor_current = abs_clampf(motor_current, (float)M3508_MAX_CURRENT);

  // 设置电机电流
  dj_motor_set_current_by_id(motor_id + 1, (int16_t)motor_current);
}

/**
 * @brief 底盘停止控制函数
 * @details 停止麦克纳姆轮底盘的所有运动，包括：
 *          1. 清零底盘控制向量的所有分量（vx, vy, wz）
 *          2. 将所有四个电机的电流值设置为0
 *          3. 通过电机控制接口发送停止命令
 * @note 此函数用于紧急停止或安全停止场景
 * @note 电机布局: Motor1-左前, Motor2-右前, Motor3-左后, Motor4-右后
 */
static void chassic_stop(void) {
  // 清零底盘控制向量的速度分量
  chassis_control_state_.command.vx = 0.0f; // 前后方向速度
  chassis_control_state_.command.vy = 0.0f; // 左右方向速度
  chassis_control_state_.command.wz = 0.0f; // 旋转角速度

  // 设置各电机的电流值
  dj_motor_set_current_by_id(1, 0); // 设置电机1电流
  dj_motor_set_current_by_id(2, 0); // 设置电机2电流
  dj_motor_set_current_by_id(3, 0); // 设置电机3电流
  dj_motor_set_current_by_id(4, 0); // 设置电机4电流

}

/**
************************************************************************
* @brief:      	chassic_control: 底盘运动控制函数
* @param:      	void
* @retval:     	void
* @details:    	根据遥控器输入控制麦克纳姆轮底盘运动
*               使用双闭环PID控制: 位置环(角度) -> 速度环 -> 电流输出
*               vx: 前后方向速度 (负=前进, 正=后退)
*               vy: 左右方向速度 (负=左移, 正=右移)
*               vw: 旋转角速度 (负=左转, 正=右转)
************************************************************************
**/
static void chassic_control(void) {
  /************双环************* */

  // 获取遥控器输入
  // ch.l.x: 左摇杆X轴 (左右) - 用于左右运动
  // ch.l.y: 左摇杆Y轴 (上下) - 用于前后运动
  // ch.r.x: 右摇杆X轴 (左右) - 用于旋转
  chassis_control_state_.command.vx = -vt13_cmd_rc.ch.l.x*3000; // 左右速度
  chassis_control_state_.command.vy = -vt13_cmd_rc.ch.l.y*3000; // 前后速度  
  chassis_control_state_.command.wz = -vt13_cmd_rc.ch.r.x*3000; // 旋转速度

  // 麦克纳姆轮逆运动学：将底盘速度 (vx, vy, wz) 分解为 4 个电机目标转速。
  //                      +vy(前)
  //                         ^
  //          II 象限       |       I 象限
  //       Motor1(左前)     |    Motor2(右前)
  //  -vx(左) <--------------+--------------> +vx(右)
  //       Motor3(左后)     |    Motor4(右后)
  //         III 象限       |      IV 象限
  //                         v
  //                      -vy(后)
  //
  // 平移合力: F_trans = vx * ex + vy * ey
  // - vx 决定左右平移分量，vy 决定前后平移分量，两者矢量相加后得到底盘平移方向。
  // - 例如 vx>0 && vy>0 时，平移合力指向右前(I象限)；vx<0 && vy>0 时指向左前(II象限)。
  //
  // 转向力矩: wz 只负责绕底盘几何中心叠加旋转趋势，不改变平移合力方向。
  // - +wz: 右转(顺时针)，在本式中等效为 Motor1/Motor3 增速，Motor2/Motor4 减速。
  // - -wz: 左转(逆时针)，在本式中等效为 Motor1/Motor3 减速，Motor2/Motor4 增速。
  //
  // 因此每个轮子的目标转速 = 平移分量贡献 + 转向分量贡献：
  // - 左前/右后轮同时带有 -vx 分量，右前/左后轮同时带有 +vx 分量。
  // - 左前/右前轮同时带有 +vy 分量，左后/右后轮同时带有 -vy 分量。
  // - 典型工况示例（仅看符号，正负表示当前电机定义下的正反转方向）：
  //   纯前进(vx=0, vy>0, wz=0): M1+, M2+, M3-, M4-
  //   纯右移(vx>0, vy=0, wz=0): M1-, M2+, M3+, M4-
  //   纯右转(vx=0, vy=0, wz>0): M1+, M2-, M3+, M4-
  //   斜向示例（取 |vx|=|vy|, wz=0，此时有两轮停转，便于检查方向定义）：
  //   右前(vx>0, vy>0): M1=0, M2+, M3=0, M4-
  //   左前(vx<0, vy>0): M1+, M2=0, M3-, M4=0
  //   右后(vx>0, vy<0): M1-, M2=0, M3+, M4=0
  //   左后(vx<0, vy<0): M1=0, M2-, M3=0, M4+
  motor_speed[0] = -chassis_control_state_.command.vx +
                   chassis_control_state_.command.vy +
                   -chassis_control_state_.command.wz; // Motor1: 左前
  motor_speed[1] = chassis_control_state_.command.vx +
                   chassis_control_state_.command.vy -
                   chassis_control_state_.command.wz; // Motor2: 右前
  motor_speed[2] = chassis_control_state_.command.vx -
                   chassis_control_state_.command.vy +
                   -chassis_control_state_.command.wz; // Motor3: 左后
  motor_speed[3] = -chassis_control_state_.command.vx -
                   chassis_control_state_.command.vy -
                   chassis_control_state_.command.wz; // Motor4: 右后
  /***********单环************* */
  // 使用单速度环PID控制各电机
  for (uint8_t i = 0; i <= DJ_MOTOR4; i++) {
    dj_motor_pid_control_speed(i, motor_speed[i]);
  }
}
void Chassis_Mode(void)
{
  if(vt13_cmd_rc.mode_sw==vt13_CMD_SW_MID )
  {
  { if(joint_enable_single==1)
    {
        chassic_control();
    }
 
  }
  }
else if (vt13_cmd_rc.mode_sw==vt13_CMD_SW_UP || vt13_cmd_rc.mode_sw==vt13_CMD_SW_DOWN)
{
   chassic_stop();
}
//chassic_stop();
}
