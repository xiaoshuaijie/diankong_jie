#include "bsp_can.h"
#include "can.h"
#include "stm32f4xx_hal_def.h"
#include <stdint.h>

// ==================== 发送辅助 ====================

//! 尝试最多 3 次寻找空邮箱并发送 CAN 帧，避免邮箱瞬时忙导致直接丢包。
static uint8_t can_send_with_retry(CAN_HandleTypeDef *hcan,
                                   CAN_TxHeaderTypeDef *tx_header,
                                   uint8_t *data, uint32_t *mailbox) {
  for (uint8_t attempt = 0; attempt < 3; ++attempt) {
    if (HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0) {
      continue;
    }

    if (HAL_CAN_AddTxMessage(hcan, tx_header, data, mailbox) == HAL_OK) {
      return 1;
    }
  }

  return 0;
}

uint32_t can2_rx_callback_cnt = 0;

// ==================== 初始化与滤波器配置 ==================== 
/**
 * @brief 初始化CAN总线
 *
 * @param hcan CAN编号
 * @param Callback_Function 处理回调函数
 */
void CAN_Init(CAN_HandleTypeDef *hcan) {
  HAL_CAN_Start(hcan);
  __HAL_CAN_ENABLE_IT(hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
  __HAL_CAN_ENABLE_IT(hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
}
/**
 * @brief 配置CAN的16位滤波器
 *
 * @param hcan CAN编号
 * @param Object_Para 编号 | FIFOx | ID类型 | 帧类型
 * @param ID1 第一个16位过滤规则
 * @param Mask1 第一个16位掩码
 * @param ID2 第二个16位过滤规则
 * @param Mask2 第二个16位掩码
 */
void CAN_Filter_Mask_Config_16bit(CAN_HandleTypeDef *hcan, uint8_t Object_Para,
                                  uint16_t ID1,
                                  uint16_t Mask1, // 第一个16位过滤规则
                                  uint16_t ID2,
                                  uint16_t Mask2) // 第二个16位过滤规则
{
  CAN_FilterTypeDef can_filter_init_structure;

  // 检测关键传参
  assert_param(hcan != NULL);

  // 16位模式下，标准帧/扩展帧的ID需按16位格式对齐（无需32位移位，直接使用16位值）
  // 第一个过滤ID和掩码（存储在FilterIdHigh和FilterMaskIdHigh）
  can_filter_init_structure.FilterIdHigh = ID1;       // 第一个16位ID
  can_filter_init_structure.FilterMaskIdHigh = Mask1; // 第一个16位掩码

  // 第二个过滤ID和掩码（存储在FilterIdLow和FilterMaskIdLow）
  can_filter_init_structure.FilterIdLow = ID2;       // 第二个16位ID
  can_filter_init_structure.FilterMaskIdLow = Mask2; // 第二个16位掩码

  // 滤波器序号（0~27，CAN1用0~13，CAN2用14~27）
  can_filter_init_structure.FilterBank = Object_Para >> 3;

  // 绑定的FIFO（0或1）
  can_filter_init_structure.FilterFIFOAssignment = (Object_Para >> 2) & 0x01;

  // 使能滤波器
  can_filter_init_structure.FilterActivation = ENABLE;

  // 掩码模式（16位模式仍支持ID掩码）
  can_filter_init_structure.FilterMode = CAN_FILTERMODE_IDMASK;

  // 关键：设置为16位位宽
  can_filter_init_structure.FilterScale = CAN_FILTERSCALE_16BIT;

  // 从机起始滤波器组（固定14）
  can_filter_init_structure.SlaveStartFilterBank = 14;

  // 应用配置
  HAL_CAN_ConfigFilter(hcan, &can_filter_init_structure);
}
/**
 * @brief 配置CAN的滤波器
 *
 * @param hcan CAN编号
 * @param Object_Para 编号 | FIFOx | ID类型 | 帧类型
 * @param ID ID
 * @param Mask_ID 屏蔽位(0x3ff, 0x1fffffff)
 */
void CAN_Filter_Mask_Config_32bit(CAN_HandleTypeDef *hcan, uint8_t Object_Para,
                                  uint32_t ID, uint32_t Mask_ID) {
  CAN_FilterTypeDef can_filter_init_structure;

  // 检测关键传参
  assert_param(hcan != NULL);

  if ((Object_Para & 0x02)) {
    // 标准帧
    // 掩码后ID的高16bit
    can_filter_init_structure.FilterIdHigh = ID << 3 >> 16;
    // 掩码后ID的低16bit
    can_filter_init_structure.FilterIdLow =
        ID << 3 | ((Object_Para & 0x03) << 1);
    // ID掩码值高16bit
    can_filter_init_structure.FilterMaskIdHigh = Mask_ID << 3 >> 16;
    // ID掩码值低16bit
    can_filter_init_structure.FilterMaskIdLow =
        Mask_ID << 3 | ((Object_Para & 0x03) << 1);
  } else {
    // 扩展帧
    // 掩码后ID的高16bit
    can_filter_init_structure.FilterIdHigh = ID << 5;
    // 掩码后ID的低16bit
    can_filter_init_structure.FilterIdLow = ((Object_Para & 0x03) << 1);
    // ID掩码值高16bit
    can_filter_init_structure.FilterMaskIdHigh = Mask_ID << 5;
    // ID掩码值低16bit
    can_filter_init_structure.FilterMaskIdLow = ((Object_Para & 0x03) << 1);
  }

  // 滤波器序号, 0-27, 共28个滤波器, can1是0~13, can2是14~27
  can_filter_init_structure.FilterBank = Object_Para >> 3;
  // 滤波器绑定FIFOx, 只能绑定一个
  can_filter_init_structure.FilterFIFOAssignment = (Object_Para >> 2) & 0x01;
  // 使能滤波器
  can_filter_init_structure.FilterActivation = ENABLE;
  // 滤波器模式, 设置ID掩码模式
  can_filter_init_structure.FilterMode = CAN_FILTERMODE_IDMASK;
  // 32位滤波
  can_filter_init_structure.FilterScale = CAN_FILTERSCALE_32BIT;
  // 从机模式选择开始单元
  can_filter_init_structure.SlaveStartFilterBank = 14;

  HAL_CAN_ConfigFilter(hcan, &can_filter_init_structure);
}

/**
 * @brief 发送数据帧
 *
 * @param hcan CAN编号
 * @param ID ID
 * @param Data 被发送的数据指针
 * @param Length 长度
 * @return 1=发送成功, 0=发送失败
 */
uint8_t dj_CAN_Send_Data(CAN_HandleTypeDef *hcan, uint16_t ID, int16_t cm1_iq,
                         int16_t cm2_iq, int16_t cm3_iq, int16_t cm4_iq,
                         uint16_t Length) {
  CAN_TxHeaderTypeDef tx_header = {0};
  uint32_t used_mailbox;

  uint8_t data[8];
  // 检测关键传参
  assert_param(hcan != NULL);

  tx_header.StdId = ID;
  tx_header.ExtId = 0;
  tx_header.IDE = 0;
  tx_header.RTR = 0;
  tx_header.DLC = Length;
  tx_header.TransmitGlobalTime = DISABLE;
  // DJI 电调协议使用高字节在前的 int16 电流值。
  data[0] = (uint8_t)(cm1_iq >> 8);
  data[1] = (uint8_t)cm1_iq;
  data[2] = (uint8_t)(cm2_iq >> 8);
  data[3] = (uint8_t)cm2_iq;
  data[4] = (uint8_t)(cm3_iq >> 8);
  data[5] = (uint8_t)cm3_iq;
  data[6] = (uint8_t)(cm4_iq >> 8);
  data[7] = (uint8_t)cm4_iq;
  /*找到空的发送邮箱，把数据发送出去*/
  return can_send_with_retry(hcan, &tx_header, data, &used_mailbox);
}
/**
************************************************************************
* @brief:      	canx_bsp_receive(CAN_HandleTypeDef *hcan, uint8_t *buf)
* @param:       hcan: CAN句柄
* @param[out]:  rec_id: 	接收到数据的CAN设备ID
* @param:       buf：接收数据缓存
* @retval:     	接收的数据长度
* @details:    	接收数据
************************************************************************
**/
// ==================== 接收分发与弱回调 ====================

uint8_t canx_receive(hcan_t *hcan, uint16_t *rec_id, uint8_t *buf,
                     uint32_t fifo) {
  CAN_RxHeaderTypeDef rx_header;
  if (HAL_CAN_GetRxMessage(hcan, fifo, &rx_header, buf) == HAL_OK) {
    *rec_id = rx_header.StdId;
    return rx_header.DLC; // 接收数据长度
  } else {
    return 0;
  }
}

/**
************************************************************************
* @brief:      	dm_can1_rx_callback(void)
* @param:       void
* @retval:     	void
* @details:    	供用户调用的接收弱函数
************************************************************************
**/
__weak void dm_can1_rx_callback(hcan_t *hcan, uint16_t *rec_id,
                                uint8_t *rx_data) {}
__weak void dj_motor_can1_rx_callback(hcan_t *hcan, uint16_t *rec_id,
                                      uint8_t *rx_data) {}
/**
************************************************************************
* @brief:      	dm_can2_rx_callback(void)
* @param:       hcan: CAN句柄
* @param[out]:  rec_id: 	接收到数据的CAN设备ID
* @param:       buf：接收数据缓存
* @retval:     	void
* @details:    	供用户调用的接收弱函数
************************************************************************
**/
__weak void dm_can2_rx_callback(hcan_t *hcan, uint16_t *rec_id,
                                uint8_t *rx_data) {}
__weak void dj_motor_can2_rx_callback(hcan_t *hcan, uint16_t *rec_id,
                                      uint8_t *rx_data) {}
/**
************************************************************************
* @brief:      	HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
* @param:       hfdcan；FDCAN句柄
* @param:       RxFifo0ITs：中断标志位
* @retval:     	void
* @details:    	HAL库的CAN中断回调函数
************************************************************************
**/
//! FIFO0 消息挂起回调：当前用于 CAN1 的达妙电机接收分发。
void HAL_CAN_RxFifo0MsgPendingCallback(hcan_t *hcan) {

  uint16_t rec_id;
  uint8_t rx_data[8] = {0};
  canx_receive(hcan, &rec_id, rx_data, CAN_RX_FIFO0);

  if (hcan == &hcan1) {
    dm_can1_rx_callback(hcan, &rec_id, rx_data);
  }
}
//! FIFO1 消息挂起回调：当前用于 CAN2 的 DJI 电机接收分发。
void HAL_CAN_RxFifo1MsgPendingCallback(hcan_t *hcan) {
  uint16_t rec_id;
  uint8_t rx_data[8] = {0};
  canx_receive(hcan, &rec_id, rx_data, CAN_RX_FIFO1);

  if (hcan == &hcan2) {
    dj_motor_can2_rx_callback(hcan, &rec_id, rx_data);
  }
}
/**
************************************************************************
* @brief:      	canx_bsp_send_data(FDCAN_HandleTypeDef *hfdcan, uint16_t id,
*uint8_t *data, uint32_t len)
* @param:       hcan: CAN句柄
* @param:       id: 	CAN设备ID
* @param:       data: 发送的数据
* @param:       len:  发送的数据长度
* @retval:     	void
* @details:    	发送数据
************************************************************************
**/
// ==================== 通用发送接口 ====================

uint8_t dm_can_send_data(CAN_HandleTypeDef *hcan, uint16_t id, uint8_t *data,
                         uint32_t len) {
  CAN_TxHeaderTypeDef tx_header = {0};
  uint32_t used_mailbox;
  // 检测关键传参
  assert_param(hcan != NULL);
  assert_param(data != NULL);
  assert_param(len <= 8);

  tx_header.StdId = id;
  tx_header.ExtId = 0;
  tx_header.IDE = 0;
  tx_header.RTR = 0;
  tx_header.DLC = len;
  tx_header.TransmitGlobalTime = DISABLE;
  /*找到空的发送邮箱，把数据发送出去*/
  return can_send_with_retry(hcan, &tx_header, data, &used_mailbox);
}
