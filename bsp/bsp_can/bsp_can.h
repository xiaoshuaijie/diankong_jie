#ifndef _CAN_FILTER_H
#define _CAN_FILTER_H

#include "can.h"
#include "main.h"

//* CAN 滤波器参数打包宏。
//? Object_Para = FilterBank | FIFO | ID类型 | 帧类型，便于一个字节描述滤波器配置。
#define CAN_FILTER(x) ((x) << 3)
typedef CAN_HandleTypeDef hcan_t;
// 接收队列
#define CAN_FIFO_0 (0 << 2)
#define CAN_FIFO_1 (1 << 2)

// 标准帧或扩展帧
#define CAN_STDID (0 << 1)
#define CAN_EXTID (1 << 1)

// 数据帧或遥控帧
#define CAN_DATA_TYPE (0 << 0)
#define CAN_REMOTE_TYPE (1 << 0)
//* DJI 电机反馈数据缓存。
//? CAN 反馈帧 8 字节通常包含编码器、转速、电流和温度。
typedef struct {
  uint16_t encoder;  // 编码器
  int16_t rpm_speed; // 转速
  int16_t current;   // 电流
  uint8_t temp;      // 温度
} M2006_TypeDef;

//}CAN_ID;
// 1 CAN接受报文时候:
// 格式:标识符--电调ID(0x201~8)+数据帧--8个字节,表示1个电机4种反馈信息
// 如0x201即1号电机标识符,后接数据帧8个字节(表示1号电机4种反馈信息),同理0x202~0x208
// 2 CAN发送报文时候:
// 格式:标识符---组号ID(0x200/0x1FF)+数据帧--8个字节,表示4个电机电流值
// 标识符0x200开头,接8个字节数据帧(表示电流值),控制前4个电机电流输出(每2个字节表示1个电机的高和低8位)
// 标识符0x1FF开头,接8个字节数据帧(表示电流值),控制后4个电机电流输出(同理)
// PS: 3508电机电流范围[-16384,16384],对应[-20A,20A]

//* 启动 CAN 并开启 FIFO0/FIFO1 消息挂起中断。
void CAN_Init(CAN_HandleTypeDef *hcan);
//* 配置 16 位掩码模式滤波器，一次配置两个 16 位规则。
void CAN_Filter_Mask_Config_16bit(CAN_HandleTypeDef *hcan, uint8_t Object_Para,
                                  uint16_t ID1,
                                  uint16_t Mask1, // 第一个16位过滤规则
                                  uint16_t ID2,
                                  uint16_t Mask2); // 第二个16位过滤规则
//* 配置 32 位掩码模式滤波器，适用于标准帧或扩展帧。
void CAN_Filter_Mask_Config_32bit(CAN_HandleTypeDef *hcan, uint8_t Object_Para,
                                  uint32_t ID, uint32_t Mask_ID);
//* 发送 DJI 电机四路电流控制帧。
uint8_t dj_CAN_Send_Data(CAN_HandleTypeDef *hcan, uint16_t ID, int16_t cm1_iq,
                         int16_t cm2_iq, int16_t cm3_iq, int16_t cm4_iq,
                         uint16_t Length);
void CAN_Send_Data_X8(CAN_HandleTypeDef *hcan, uint16_t ID);
//* 从指定 FIFO 读取一帧 CAN 数据，并返回 DLC 长度。
uint8_t canx_receive(hcan_t *hcan, uint16_t *recid, uint8_t *buf,
                     uint32_t fifo);
void dm_can1_rx_callback(hcan_t *hcan, uint16_t *rec_id, uint8_t *rx_data);
void dm_can2_rx_callback(hcan_t *hcan, uint16_t *rec_id, uint8_t *rx_data);
void dj_motor_can1_rx_callback(hcan_t *hcan, uint16_t *rec_id, uint8_t *rx_data);
void dj_motor_can2_rx_callback(hcan_t *hcan, uint16_t *rec_id, uint8_t *rx_data);
//* 通用 CAN 标准帧发送接口，最多发送 8 字节数据。
uint8_t dm_can_send_data(hcan_t *hcan, uint16_t id, uint8_t *data,
                         uint32_t len);

#endif
