#include "main.h"
#include "NRF24L01.h"
#include <stdint.h>
#include <string.h>

/* 公共全局数据缓冲区，32字节数据 + 1字节数据长度 */
uint8_t NRF24L01_txbuffer[33]; 
uint8_t NRF24L01_rxbuffer_pipe0[33];
uint8_t NRF24L01_rxbuffer_pipe1[33];
uint8_t NRF24L01_rxbuffer_pipe2[33];
uint8_t NRF24L01_rxbuffer_pipe3[33];
uint8_t NRF24L01_rxbuffer_pipe4[33];
uint8_t NRF24L01_rxbuffer_pipe5[33];

/* CE 和 CSN 引脚控制函数（static，仅供内部使用） */
static void NRF24L01_CE_High(void)
{
    HAL_GPIO_WritePin(GPIOB, NRF_CE_Pin, GPIO_PIN_SET);
}

static void NRF24L01_CE_Low(void)
{
    HAL_GPIO_WritePin(GPIOB, NRF_CE_Pin, GPIO_PIN_RESET);
}

static void NRF24L01_CSN_High(void)
{
  HAL_GPIO_WritePin(GPIOA, NRF_CSN_Pin, GPIO_PIN_SET);
} 

static void NRF24L01_CSN_Low(void)
{
  HAL_GPIO_WritePin(GPIOA, NRF_CSN_Pin, GPIO_PIN_RESET);
}

/* 寄存器读写函数 */
/**
  * @brief  读取 NRF24L01 寄存器
  * @param  hspi: SPI 句柄指针
  * @param  reg: 要读取的寄存器地址
  */
static uint8_t NRF24L01_ReadReg(SPI_HandleTypeDef *hspi, uint8_t reg)
{
  if ( hspi == NULL) {
    /* 用户可添加自己的实现来报告文件名和行号 */
    return 0xFF;
  }

  static uint8_t CommandByte;
  static uint8_t RegisterData;
  CommandByte = NRF_CMD_R_REGISTER | (reg & 0x1F);
  
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
    /* 用户可添加自己的实现来报告文件名和行号 */
    NRF24L01_CSN_High();
    return 0xFF;
  }
  if(HAL_SPI_Receive(hspi, &RegisterData, 1, 100) != HAL_OK) {
    /* 用户可添加自己的实现来报告文件名和行号 */
    NRF24L01_CSN_High();
    return 0xFF;  
  }
  NRF24L01_CSN_High();

  return RegisterData;
}

/**
  * @brief  写入 NRF24L01 寄存器
  * @param  hspi: SPI 句柄指针
  * @param  reg: 要写入的寄存器地址
  * @param  Vlaue: 要写入寄存器的值
  */
static void NRF24L01_WriteReg(SPI_HandleTypeDef *hspi, uint8_t reg, uint8_t Vlaue)
{
    if ( hspi == NULL) {
      /* 用户可添加自己的实现来报告文件名和行号 */
      return;
    }
    
    static uint8_t CommandByte;
    CommandByte = NRF_CMD_W_REGISTER | (reg & 0x1F);

    NRF24L01_CSN_Low();
    if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
        /* 用户可添加自己的实现来报告文件名和行号 */
        NRF24L01_CSN_High();
        return;
    }
    if (HAL_SPI_Transmit(hspi, &Vlaue, 1, 100) != HAL_OK) {
        /* 用户可添加自己的实现来报告文件名和行号 */
        NRF24L01_CSN_High();
        return;
    }
    NRF24L01_CSN_High();
}

/**
  * @brief  从 NRF24L01 寄存器读取多个字节
  * @param  hspi: SPI 句柄指针
  * @param  reg: 要读取的寄存器地址
  * @param  data: 数据缓冲区指针
  * @param  length: 要读取的字节数
  */
static void NRF24L01_ReadMultiReg(SPI_HandleTypeDef *hspi, uint8_t reg, uint8_t *data, uint8_t length)
{
  if (hspi == NULL || data == NULL || length == 0) {
    /* 用户可添加自己的实现来报告文件名和行号，
  例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
    return;
  }
    
  uint8_t CommandByte = NRF_CMD_R_REGISTER | (reg & 0x1F);
    
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
      NRF24L01_CSN_High();
      /* 用户可添加自己的实现来报告文件名和行号，
    例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
      return;
  }
  if (HAL_SPI_Receive(hspi, data, length, 100) != HAL_OK) {
      NRF24L01_CSN_High();
      /* 用户可添加自己的实现来报告文件名和行号，
    例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
      return;
  }
  NRF24L01_CSN_High();
}

/**
  * @brief  向 NRF24L01 寄存器写入多个字节
  * @param  hspi: SPI 句柄指针
  * @param  reg: 要写入的寄存器地址
  * @param  data: 数据缓冲区指针
  * @param  length: 要写入的字节数
  */
static void NRF24L01_WriteMultiReg(SPI_HandleTypeDef *hspi, uint8_t reg, uint8_t *data, uint8_t length)
{
  if (hspi == NULL || data == NULL || length == 0) {
    /* 用户可添加自己的实现来报告文件名和行号，
  例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
    return;
  }
    
  uint8_t CommandByte = NRF_CMD_W_REGISTER | (reg & 0x1F);
    
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
      NRF24L01_CSN_High();
      /* 用户可添加自己的实现来报告文件名和行号，
    例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
      return;
  }
  if (HAL_SPI_Transmit(hspi, data, length, 100) != HAL_OK) {
      NRF24L01_CSN_High();
      /* 用户可添加自己的实现来报告文件名和行号，
    例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
      return;
  }
  NRF24L01_CSN_High();
}

void NRF24L01_FlushRxFIFO(SPI_HandleTypeDef *hspi)
{
  if ( hspi == NULL) {
    /* 用户可添加自己的实现来报告文件名和行号，
  例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
    return;
  }
    
  uint8_t CommandByte = NRF_CMD_FLUSH_RX;
    
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
  /* 用户可添加自己的实现来报告文件名和行号，
例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
    NRF24L01_CSN_High();
    return;
  }
  NRF24L01_CSN_High();
}

void NRF24L01_FlashTxFIFO(SPI_HandleTypeDef *hspi)
{
  if ( hspi == NULL) {
    /* 用户可添加自己的实现来报告文件名和行号，
  例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
    return;
  }
    
  uint8_t CommandByte = NRF_CMD_FLUSH_TX;
    
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
  /* 用户可添加自己的实现来报告文件名和行号，
例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
    NRF24L01_CSN_High();
    return;
  }
  NRF24L01_CSN_High();
}

/**
  * @brief  设置 NRF24L01 工作模式
  * @param  hspi: SPI 句柄指针
  * @param  mode: 要设置的工作模式
  */
void NRF24L01_SetMode(SPI_HandleTypeDef *hspi, NRF_ModeTypeDef mode)
{
  if (hspi == NULL) {
    /* 用户可添加自己的实现来报告文件名和行号，
    例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
      return;
  }
  NRF24L01_CE_Low();

  static uint8_t config_reg;
  config_reg = NRF24L01_ReadReg(hspi, NRF_REG_CONFIG);
  
  switch (mode) {
    case NRF_MODE_POWER_DOWN:
        config_reg &= ~NRF_POWER_UP;  /* 清除 PWR_UP 位 */
        NRF24L01_WriteReg(hspi, NRF_REG_CONFIG, config_reg);
        NRF24L01_CE_Low();
        break;
        
    case NRF_MODE_STANDBY_I:
        config_reg |= NRF_POWER_UP;   /* 置位 PWR_UP 位 */
        NRF24L01_WriteReg(hspi, NRF_REG_CONFIG, config_reg);
        NRF24L01_CE_Low();
        break;
        
    case NRF_MODE_RX:
        config_reg |= NRF_POWER_UP;   /* 置位 PWR_UP 位 */
        config_reg |= NRF_PRIMARY_RX; /* 置位 PRIM_RX 位 */
        NRF24L01_WriteReg(hspi, NRF_REG_CONFIG, config_reg);
        // 清除 RX_DR 中断标志
        NRF24L01_WriteReg(hspi, NRF_REG_STATUS, NRF_STATUS_RX_DR); 
        // 清空 RX FIFO
        NRF24L01_FlushRxFIFO(hspi);

        NRF24L01_CE_High();
        HAL_Delay(1);
        break;
        
    case NRF_MODE_TX:
        config_reg |= NRF_POWER_UP;    /* 置位 PWR_UP 位 */
        config_reg &= ~NRF_PRIMARY_RX; /* 清除 PRIM_RX 位 */
        NRF24L01_WriteReg(hspi, NRF_REG_CONFIG, config_reg);
        // 清除 TX_DS 和 NRF_STATUS_MAX_RT 中断标志
        NRF24L01_WriteReg(hspi, NRF_REG_STATUS, NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT);
        // 清空 TX FIFO
        NRF24L01_FlashTxFIFO(hspi);

        NRF24L01_CE_High();
        HAL_Delay(1);
        break;
    default:
        break;
  }
}

/**
  * @brief  获取 NRF24L01+ 默认配置
  * @param  init: 用于填充默认值的初始化结构体指针
  */
void NRF24L01_GetDefaultConfig(NRF_InitTypeDef *init)
{
  if (init == NULL) {
  /* 用户可添加自己的实现来报告文件名和行号，
例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
    return;
  }
    
  /* CONFIG 寄存器默认值 */
  init->InterruptConfig = NRF_INTERRUPT_TX_DS_DISABLE;  /* 禁用 TX 数据发送中断 */
  init->CRCConfig = NRF_CRC_ENABLE;                     /* 启用 CRC */
  init->CRCLength = NRF_CRC_LENGTH_2BYTE;               /* 2 字节 CRC */
  init->Mode =NRF_MODE_STANDBY_I;                       /* Standby-I 模式 */
  
  /* EN_AA 寄存器默认值 */
  init->AutoAckPipes = NRF_PIPE0_BIT;               /* 仅管道 0 启用自动应答 */
  
  /* EN_RXADDR 寄存器默认值 */
  init->RxPipes = NRF_PIPE0_BIT;                    /* 仅启用管道 0 */
  
  /* SETUP_AW 寄存器默认值 */
  init->AddrWidth = NRF_ADDR_WIDTH_5BYTES;          /* 5 字节地址宽度 */
  
  /* SETUP_RETR 寄存器默认值 */
  init->AutoRetransDelay = NRF_ARD_500US;           /* 500us 重传延迟 */
  init->AutoRetransCount = NRF_ARC_5_RETRANS;       /* 最多 5 次重传 */
  
  /* RF_CH 寄存器默认值 */
  init->RFChannel = 2;                              /* 通道 2 (2.402GHz) */
  
  /* RF_SETUP 寄存器默认值 */
  init->DataRate = NRF_DATA_RATE_1MBPS;             /* 1Mbps 数据速率 */
  init->RFPower = NRF_RF_POWER_0DBM;                /* 0dBm 输出功率 */
  
  /* DYNPD 寄存器默认值 */
  init->DynamicPayloadPipes = NRF_PIPE0_BIT;        /* 管道 0 启用动态载荷 */
  
  /* FEATURE 寄存器默认值 */
  init->Features = NRF_FEATURE_EN_DPL;              /* 启用动态载荷长度 */
  
  /* 载荷宽度默认值 */
  init->PayloadWidthPipe0 = 32;                     /* 管道 0 使用 32 字节 */
  init->PayloadWidthPipe1 = 0;                      /* 管道 1 未使用 */
  init->PayloadWidthPipe2 = 0;                      /* 管道 2 未使用 */
  init->PayloadWidthPipe3 = 0;                      /* 管道 3 未使用 */
  init->PayloadWidthPipe4 = 0;                      /* 管道 4 未使用 */
  init->PayloadWidthPipe5 = 0;                      /* 管道 5 未使用 */
  
  /* 地址默认值 */
  uint8_t defaultTxAddr[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
  uint8_t defaultRxAddr0[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
  uint8_t defaultRxAddr1[5] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
  
  memcpy(init->TxAddress, defaultTxAddr, 5);
  memcpy(init->RxAddressPipe0, defaultRxAddr0, 5);
  memcpy(init->RxAddressPipe1, defaultRxAddr1, 5);
  init->RxAddressPipe2 = 0xC3;
  init->RxAddressPipe3 = 0xC4;
  init->RxAddressPipe4 = 0xC5;
  init->RxAddressPipe5 = 0xC6;
}

/**
  * @brief  使用配置结构体初始化 NRF24L01+ 模块，并拉低 CE 引脚
  * @param  hspi: SPI 句柄指针
  * @param  init: 初始化结构体指针
  * @note   此函数替代旧版 NRF24L01_Init 函数
  */
void NRF24L01_Init(SPI_HandleTypeDef *hspi, NRF_InitTypeDef *init)
{
  if (hspi == NULL || init == NULL) {
    /* 用户可添加自己的实现来报告文件名和行号，
  例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
    return;
  }

  HAL_Delay(100);
  NRF24L01_CE_Low();

  /* 根据各配置字段构建 CONFIG 寄存器值 */
  uint8_t config_value = 0;
  config_value |= init->InterruptConfig;    /* MASK_RX_DR, MASK_TX_DS, MASK_MAX_RT */
  config_value |= init->CRCConfig;          /* EN_CRC */
  config_value |= init->CRCLength;          /* CRCO */
  
  /* 写入配置寄存器 */
  NRF24L01_WriteReg(hspi, NRF_REG_CONFIG, config_value);
  NRF24L01_WriteReg(hspi, NRF_REG_EN_AA, init->AutoAckPipes);
  NRF24L01_WriteReg(hspi, NRF_REG_EN_RXADDR, init->RxPipes);
  NRF24L01_WriteReg(hspi, NRF_REG_SETUP_AW, init->AddrWidth);
  
  /* 构建 SETUP_RETR 寄存器值 */
  uint8_t setup_retr_value = init->AutoRetransDelay | init->AutoRetransCount;
  NRF24L01_WriteReg(hspi, NRF_REG_SETUP_RETR, setup_retr_value);
  
  /* 写入 RF 通道 */
  NRF24L01_WriteReg(hspi, NRF_REG_RF_CH, init->RFChannel);
  
  /* 构建 RF_SETUP 寄存器值 */
  uint8_t rf_setup_value = init->DataRate | init->RFPower;
  NRF24L01_WriteReg(hspi, NRF_REG_RF_SETUP, rf_setup_value);
  
  /* 写入动态载荷和功能寄存器 */
  NRF24L01_WriteReg(hspi, NRF_REG_DYNPD, init->DynamicPayloadPipes);
  NRF24L01_WriteReg(hspi, NRF_REG_FEATURE, init->Features);
  
  /* 清除状态寄存器 */
  NRF24L01_WriteReg(hspi, NRF_REG_STATUS, 0x70);
  
  /* 清空 RX FIFO */
  NRF24L01_FlushRxFIFO(hspi);
  
  /* 写入载荷宽度 */
  if (init->PayloadWidthPipe0 > 0) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_PW_P0, init->PayloadWidthPipe0);
  }
  if (init->PayloadWidthPipe1 > 0) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_PW_P1, init->PayloadWidthPipe1);
  }
  if (init->PayloadWidthPipe2 > 0) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_PW_P2, init->PayloadWidthPipe2);
  }
  if (init->PayloadWidthPipe3 > 0) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_PW_P3, init->PayloadWidthPipe3);
  }
  if (init->PayloadWidthPipe4 > 0) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_PW_P4, init->PayloadWidthPipe4);
  }
  if (init->PayloadWidthPipe5 > 0) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_PW_P5, init->PayloadWidthPipe5);
  }
  
  /* 写入地址 */
  /* 根据 AddrWidth 确定实际地址长度 */
  uint8_t addr_length = 5;
  if (init->AddrWidth == NRF_ADDR_WIDTH_3BYTES) {
      addr_length = 3;
  } else if (init->AddrWidth == NRF_ADDR_WIDTH_4BYTES) {
      addr_length = 4;
  }
  
  /* 写入 TX 地址 */
  NRF24L01_WriteMultiReg(hspi, NRF_REG_TX_ADDR, init->TxAddress, addr_length);
  
  /* 写入已启用管道的 RX 地址 */
  if (init->RxPipes & NRF_PIPE0_BIT) {
      NRF24L01_WriteMultiReg(hspi, NRF_REG_RX_ADDR_P0, init->RxAddressPipe0, addr_length);
  }
  if (init->RxPipes & NRF_PIPE1_BIT) {
      NRF24L01_WriteMultiReg(hspi, NRF_REG_RX_ADDR_P1, init->RxAddressPipe1, addr_length);
  }
  if (init->RxPipes & NRF_PIPE2_BIT) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_ADDR_P2, init->RxAddressPipe2);
  }
  if (init->RxPipes & NRF_PIPE3_BIT) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_ADDR_P3, init->RxAddressPipe3);
  }
  if (init->RxPipes & NRF_PIPE4_BIT) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_ADDR_P4, init->RxAddressPipe4);
  }
  if (init->RxPipes & NRF_PIPE5_BIT) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_ADDR_P5, init->RxAddressPipe5);
  }
  /* 设置初始模式 */
  NRF24L01_SetMode(hspi, init->Mode);
}

/**
  * @brief  通过 NRF24L01 发送数据
  * @param  hspi: SPI 句柄指针
  * @param  Timeout: 等待 TX FIFO 可用的超时时间
  * @note   NRF24L01 在变长载荷模式下最大支持 32 字节载荷
  */
void NRF24L01_WriteTxFIFO(SPI_HandleTypeDef *hspi,uint32_t Timeout)
{
  static uint32_t tickstart;
  static uint8_t CommandByte;

  /* 参数检查 */
  if (hspi == NULL || NRF24L01_txbuffer[32] == 0 || NRF24L01_txbuffer[32] > 32) {
    /* 用户可添加自己的实现来报告文件名和行号，
    例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
    return;
  }

  /* 等待 TxFIFO 可用 */
  tickstart = HAL_GetTick();
  while(NRF24L01_IsTxFIFOFull(hspi)){
    if((HAL_GetTick() - tickstart ) > Timeout){
      /* 用户可添加自己的实现来报告文件名和行号，
      例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
      return;
    }
  };


  /* 写入 TxFIFO */
  CommandByte = NRF_CMD_W_TX_PAYLOAD;
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
    /* 用户可添加自己的实现来报告文件名和行号，
    例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
    NRF24L01_CSN_High();
    return;
  }
  if (HAL_SPI_Transmit(hspi, NRF24L01_txbuffer, NRF24L01_txbuffer[32], 100) != HAL_OK) {
    /* 用户可添加自己的实现来报告文件名和行号，
    例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
    NRF24L01_CSN_High();
    return;
  }
  NRF24L01_CSN_High();
}

/**
  * @brief  通过 NRF24L01 接收数据
  * @param  hspi: SPI 句柄指针
  * @note   缓冲区仅包含从 rxFIFO 实际接收到的数据
  *         长度参数保存接收到的字节数
  */
void NRF24L01_ReadRxFIFO(SPI_HandleTypeDef *hspi)
{
  static uint8_t cmd;
  static uint8_t Rx_payload_width;
  static uint8_t *NRF24L01_rxbuffer;

  /* 参数检查 */
  if (hspi == NULL) {
      /* 用户可添加自己的实现来报告文件名和行号，
    例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
    return ;
  }


  /* 读取 RX FIFO 中所有可用载荷 */
  while(!NRF24L01_IsRxFIFOEmpty(hspi)){
    /* 读取管道编号 */
    switch (NRF24L01_GetPipeNum(hspi)) {
      case 0:
          NRF24L01_rxbuffer = NRF24L01_rxbuffer_pipe0;
          break;
      case 1:
          NRF24L01_rxbuffer = NRF24L01_rxbuffer_pipe1;
          break;
      case 2:
          NRF24L01_rxbuffer = NRF24L01_rxbuffer_pipe2;
          break;
      case 3:
          NRF24L01_rxbuffer = NRF24L01_rxbuffer_pipe3;
          break;
      case 4:
          NRF24L01_rxbuffer = NRF24L01_rxbuffer_pipe4;
          break;
      case 5:
          NRF24L01_rxbuffer = NRF24L01_rxbuffer_pipe5;
          break;
      default:
          /* 无效管道编号，正常情况下不应发生 */
          return ;
    }

    /* 读取载荷宽度 */
    NRF24L01_CSN_Low();
    cmd = NRF_CMD_R_RX_PL_WID;
    if (HAL_SPI_Transmit(hspi, &cmd, 1, 100) != HAL_OK) {
        NRF24L01_CSN_High();
        /* 用户可添加自己的实现来报告文件名和行号，
      例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
        return ;
    }
    if (HAL_SPI_Receive(hspi, &Rx_payload_width, 1, 100) != HAL_OK) {
        NRF24L01_CSN_High();
        /* 用户可添加自己的实现来报告文件名和行号，
      例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
        return ;
    }
    NRF24L01_CSN_High();
    
    /* 读取实际载荷数据 */
    NRF24L01_CSN_Low();
    cmd = NRF_CMD_R_RX_PAYLOAD;
    if (HAL_SPI_Transmit(hspi, &cmd, 1, 100) != HAL_OK) {
        NRF24L01_CSN_High();
        /* 用户可添加自己的实现来报告文件名和行号，
      例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
        return ;
    }
    if (HAL_SPI_Receive(hspi, NRF24L01_rxbuffer, Rx_payload_width, 100) != HAL_OK) {
        NRF24L01_CSN_High();
        /* 用户可添加自己的实现来报告文件名和行号，
      例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
        return ;
    }
    NRF24L01_CSN_High();
    NRF24L01_rxbuffer[32] = Rx_payload_width;  /* 将载荷长度存储在最后一个字节 */
  }
}

/**
  * @brief  获取 NRF24L01 状态寄存器值
  * @param  hspi: SPI 句柄指针
  * @retval 状态寄存器值
  * @note   此函数读取 STATUS 寄存器 (0x07)
  *         该寄存器包含以下标志：
  *         - Bit 6: RX_DR - RX 数据就绪标志
  *         - Bit 5: TX_DS - TX 数据发送完成标志
  *         - Bit 4: MAX_RT - 达到最大重传次数标志
  *         - Bits 3:1: RX_P_NO - RX FIFO 中可用载荷对应的 RX 管道编号
  */
uint8_t NRF24L01_GetStatus(SPI_HandleTypeDef *hspi)
{
  return NRF24L01_ReadReg(hspi, NRF_REG_STATUS);
}

/**
  * @brief  获取 NRF24L01 FIFO 状态寄存器值
  * @param  hspi: SPI 句柄指针
  * @retval FIFO 状态寄存器值
  * @note   此函数读取 FIFO_STATUS 寄存器 (0x17)
  *         该寄存器包含以下标志：
  *         - Bit 6: TX_REUSE - TX 复用标志
  *         - Bit 5: TX_FULL - TX FIFO 满标志（FIFO_STATUS 寄存器）
  *         - Bit 4: TX_EMPTY - TX FIFO 空标志
  *         - Bit 1: RX_FULL - RX FIFO 满标志
  *         - Bit 0: RX_EMPTY - RX FIFO 空标志
  */
uint8_t NRF24L01_GetFIFOStatus(SPI_HandleTypeDef *hspi)
{
  return NRF24L01_ReadReg(hspi, NRF_REG_FIFO_STATUS);
}

/**
  * @brief  检查 RX 数据是否就绪（RX_DR 标志）
  * @param  hspi: SPI 句柄指针
  * @retval RX 数据就绪返回 1，否则返回 0
  */
uint8_t NRF24L01_IsRxDataReady(SPI_HandleTypeDef *hspi)
{
  uint8_t status = NRF24L01_GetStatus(hspi);
  return (status & NRF_STATUS_RX_DR) ? 1 : 0;
}

/**
  * @brief  检查 TX 数据是否已发送（TX_DS 标志）
  * @param  hspi: SPI 句柄指针
  * @retval TX 数据已发送返回 1，否则返回 0
  */
uint8_t NRF24L01_IsTxDataSent(SPI_HandleTypeDef *hspi)
{
  uint8_t status = NRF24L01_GetStatus(hspi);
  return (status & NRF_STATUS_TX_DS) ? 1 : 0;
}

/**
  * @brief  检查是否达到最大重传次数（MAX_RT 标志）
  * @param  hspi: SPI 句柄指针
  * @retval 达到最大重传次数返回 1，否则返回 0
  */
uint8_t NRF24L01_IsMaxRetransmit(SPI_HandleTypeDef *hspi)
{
  uint8_t status = NRF24L01_GetStatus(hspi);
  return (status & NRF_STATUS_MAX_RT) ? 1 : 0;
}

/**
  * @brief  获取 RX FIFO 中可用载荷对应的 RX 管道编号
  * @param  hspi: SPI 句柄指针
  * @retval 管道编号 (0-5)，若 RX FIFO 为空则返回 7
  * @note   返回下一个可用载荷的接收管道编号
  *         状态寄存器 Bits 3:1 保存管道编号
  *         值 7 表示 RX FIFO 为空
  */
uint8_t NRF24L01_GetRxPipeNumber(SPI_HandleTypeDef *hspi)
{
  uint8_t status = NRF24L01_GetStatus(hspi);
  return (status & NRF_STATUS_RX_P_NO) >> 1;
}

/**
  * @brief  检查 TX FIFO 是否已满（STATUS 寄存器中的 TX_FULL 标志）
  * @param  hspi: SPI 句柄指针
  * @retval TX FIFO 已满返回 1，否则返回 0
  * @note   此函数检查 STATUS 寄存器中的 TX_FULL 标志（bit 0）
  *         如需检查 FIFO_STATUS 寄存器中的 TX_FULL 标志（bit 5），请使用 NRF24L01_GetFIFOStatus 函数
  */
uint8_t NRF24L01_IsTxFIFOFull(SPI_HandleTypeDef *hspi)
{
  uint8_t status = NRF24L01_GetStatus(hspi);
  return (status & NRF_STATUS_TX_FULL) ? 1 : 0;
}

/**
  * @brief  检查 RX FIFO 是否为空（FIFO_STATUS 寄存器中的 RX_EMPTY 标志）
  * @param  hspi: SPI 句柄指针
  * @retval RX FIFO 为空返回 1，否则返回 0
  */
uint8_t NRF24L01_IsRxFIFOEmpty(SPI_HandleTypeDef *hspi)
{
  uint8_t fifo_status = NRF24L01_GetFIFOStatus(hspi);
  return (fifo_status & NRF_FIFO_STATUS_RX_EMPTY) ? 1 : 0;
}

/**
  * @brief  获取 RX FIFO 中可用载荷对应的 RX 管道编号
  * @retval 管道编号 (0-5)，若 RX FIFO 为空则返回 7
  * @note   返回下一个可用载荷的接收管道编号
  *         状态寄存器 Bits 3:1 保存管道编号
  *         值 7 表示 RX FIFO 为空
  */
uint8_t NRF24L01_GetPipeNum(SPI_HandleTypeDef *hspi)
{
  uint8_t status = NRF24L01_GetStatus(hspi);
  return (status & NRF_STATUS_RX_P_NO) >> 1;
}


/**
  * @brief  清除状态寄存器中的指定中断标志
  * @param  hspi: SPI 句柄指针
  * @param  flags: 要清除的中断标志（NRF_STATUS_RX_DR、NRF_STATUS_TX_DS、NRF_STATUS_MAX_RT 的按位或）
  *         此参数可使用 @ref NRF24L01_Status_Register_Bits 中的值或其按位或
  *         仅 NRF_STATUS_RX_DR、NRF_STATUS_TX_DS 和 NRF_STATUS_MAX_RT 可被清除
  * @note   向状态寄存器的中断位写入 1 可清除对应标志
  *         示例：NRF24L01_ClearInterruptFlags(hspi, NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT)
  */
void NRF24L01_ClearInterruptFlags(SPI_HandleTypeDef *hspi, uint8_t flags)
{
  /* 仅清除 RX_DR、TX_DS 和 MAX_RT 位（bits 6、5、4） */
  uint8_t flags_to_clear = flags & (NRF_STATUS_RX_DR | NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT);
  if (flags_to_clear != 0) {
    NRF24L01_WriteReg(hspi, NRF_REG_STATUS, flags_to_clear);
  }
}
