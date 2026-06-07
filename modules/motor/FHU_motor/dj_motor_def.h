#ifndef __DJ_MOTOR_DEF_H__
#define __DJ_MOTOR_DEF_H__

#include "stdint.h"

// 大疆电机类型枚举
typedef enum {
  DJ_MOTOR_M3508, // M3508电机
  DJ_MOTOR_M6020, // M6020电机
  DJ_MOTOR_M2006  // M2006电机
} dj_motor_type_e;

// 大疆电机ID枚举
typedef enum {
  DJ_MOTOR_1 = 0x201,
  DJ_MOTOR_2 = 0x202,
  DJ_MOTOR_3 = 0x203,
  DJ_MOTOR_4 = 0x204,
  DJ_MOTOR_5 = 0x205,
  DJ_MOTOR_6 = 0x206,
  DJ_MOTOR_7 = 0x207,
  DJ_MOTOR_8 = 0x208
} dj_motor_id_e;

// 大疆电机控制组枚举
typedef enum {
  DJ_MOTOR_GROUP_1 = 0x200, // 控制电机1-4
  DJ_MOTOR_GROUP_2 = 0x1FF, // M3508控制电机5-8, M6020控制电机1~4
  DJ_MOTOR_GROUP_3 = 0x2FF, // M6020的电流控制
  DJ_MOTOR_GROUP_4 = 0x1FE, // M6020控制电机1~4
  DJ_MOTOR_GROUP_5 = 0x2FE, // M6020控制电机5-8
} dj_motor_group_e;

// 大疆电机反馈数据结构体
typedef struct {
  uint16_t encoder;   // 机械角度编码器值 (0-8191)
  int16_t rpm_speed;  // 转速 (RPM)
  int16_t current;    // 扭矩电流 (-16384~16384 对应实际电流)
  uint8_t temp;       // 温度 (摄氏度)
  uint8_t last_temp;  // 上次温度值
  float angle;        // 角度 (弧度, 0-2PI)
  float actual_angle_deg; // 实际角度 (度数, 0-360)
  float speed_rads;   // 角速度 (弧度/秒)
} dj_motor_feedback_t;

// 大疆电机控制参数结构体
typedef struct {
  int16_t current_set;  // 设定电流值 (-16384~16384)
  float speed_set;      // 设定转速 (RPM)
  float angle_set;      // 设定角度 (弧度)
  uint16_t motor_id;    // 电机ID (0x201-0x208)
  dj_motor_type_e type; // 电机类型
} dj_motor_control_t;

// 大疆电机完整结构体
typedef struct {
  dj_motor_id_e motor_id;       // 电机ID
  dj_motor_type_e type;         // 电机类型
  dj_motor_feedback_t feedback; // 反馈数据
  dj_motor_control_t control;   // 控制参数
} dj_motor_t;

// 大疆电机参数宏定义
// M3508参数
#define M3508_MAX_CURRENT 16384  // 最大电流值对应20A
#define M3508_MAX_RPM 8000       // 最大转速
#define M3508_ENCODER_RES 8192   // 编码器分辨率
#define M3508_REDUCTION_RATIO 19 // 减速比

// M6020参数
#define M6020_MAX_CURRENT 30000 // 最大电流值对应10A(实际控制范围)
#define M6020_MAX_RPM 6000      // 最大转速
#define M6020_ENCODER_RES 8192  // 编码器分辨率
#define M6020_REDUCTION_RATIO 1 // 减速比(直驱)

// M2006参数
#define M2006_MAX_CURRENT 10000  // 最大电流值对应5A
#define M2006_MAX_RPM 8000       // 最大转速
#define M2006_ENCODER_RES 8192   // 编码器分辨率
#define M2006_REDUCTION_RATIO 36 // 减速比

#endif /* __DJ_MOTOR_DEF_H__ */
