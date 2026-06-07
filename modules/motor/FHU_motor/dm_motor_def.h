#ifndef __DM_MOTOR_DEF_H__
#define __DM_MOTOR_DEF_H__
#include "main.h"
// 电机参数
typedef struct {
  uint8_t read_flag;
  uint8_t write_flag;
  uint8_t save_flag;

  float UV_Value;   // 低压保护值
  float KT_Value;   // 扭矩系数
  float OT_Value;   // 过温保护值
  float OC_Value;   // 过流保护值
  float ACC;        // 加速度
  float DEC;        // 减速度
  float MAX_SPD;    // 最大速度
  uint32_t MST_ID;  // 反馈ID
  uint32_t ESC_ID;  // 接收ID
  uint32_t TIMEOUT; // 超时警报时间
  uint32_t cmode;   // 控制模式
  float Damp;       // 电机粘滞系数
  float Inertia;    // 电机转动惯量
  uint32_t hw_ver;  // 保留
  uint32_t sw_ver;  // 软件版本号
  uint32_t SN;      // 保留
  uint32_t NPP;     // 电机极对数
  float Rs;         // 电阻
  float Ls;         // 电感
  float Flux;       // 磁链
  float Gr;         // 齿轮减速比
  float PMAX;       // 位置映射范围
  float VMAX;       // 速度映射范围
  float TMAX;       // 扭矩映射范围
  float I_BW;       // 电流环控制带宽
  float KP_ASR;     // 速度环Kp
  float KI_ASR;     // 速度环Ki
  float KP_APR;     // 位置环Kp
  float KI_APR;     // 位置环Ki
  float OV_Value;   // 过压保护值
  float GREF;       // 齿轮力矩效率
  float Deta;       // 速度环阻尼系数
  float V_BW;       // 速度环滤波带宽
  float IQ_cl;      // 电流环增强系数
  float VL_cl;      // 速度环增强系数
  uint32_t can_br;  // CAN波特率代码
  uint32_t sub_ver; // 子版本号
  float u_off;      // u相偏置
  float v_off;      // v相偏置
  float k1;         // 补偿因子1
  float k2;         // 补偿因子2
  float m_off;      // 角度偏移
  float dir;        // 方向
  float p_m;        // 电机位置
  float x_out;      // 输出轴位置
} esc_inf_t;

// 电机回传信息结构体
typedef struct {
  int id;      // 电机内部设置的 can id
  int state;   // 电机状态
  int p_int;   // 整型位置信息
  int v_int;   // 整型速度信息
  int t_int;   // 整型扭矩信息
  int kp_int;  // 整型Kp信息
  int kd_int;  // 整型Kd信息
  float pos;   // 最终解析出来的位置信息
  float vel;   // 最终解析出来的速度信息
  float tor;   // 最终解析出来的扭矩信息
  float Kp;    // 最终解析出来的Kp数据
  float Kd;    // 最终解析出来的Kd数据
  float Tmos;  // 板子MOS温度
  float Tcoil; // 电机线圈温度
} motor_fbpara_t;
// 电机参数设置结构体
typedef struct {
  uint8_t mode;  // 电机控制模式
  float pos_set; // 电机位置设定
  float vel_set; // 电机速度设定
  float tor_set; // 电机扭矩设定
  float cur_set; // 电机电流设定
  float kp_set;  // Kp设定
  float kd_set;  // Kd设定
} motor_ctrl_t;
typedef struct {
  uint16_t id;         // can id
  uint16_t mst_id;     // master id 标识作用
  motor_fbpara_t para; // 电机的反馈信息结构体
  motor_ctrl_t ctrl;   // 电机的控制参数结构体
  esc_inf_t tmp;       // 电机的寄存器参数结构体
} motor_t;

#endif
/* __DM_MOTOR_DEF_H__ */