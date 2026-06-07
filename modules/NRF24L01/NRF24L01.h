/**
  ******************************************************************************
  * 作者：哔哩哔哩Up主"阿枫的手工铺"
  * 用途：开源免费，用户可以任意查看、使用和修改，并应用到自己的项目之中
  * 许可证：MIT许可证
  * 许可证详情：请查看项目根目录下的LICENSE文件
  * 声明：若二次转载需要注明出处及作者信息，
  *       程序版权归“阿枫的手工铺”所有，任何人或组织不得将其据为己有
  * 
  * 程序名称：			   	  NRF24L01+ 模块驱动程序
  * 程序创建时间：			  2026.02.06
  * 当前程序版本：			  V1.1
  * 当前版本发布时间：		2026.02.11
  * 
  * 如果您发现程序中的漏洞，或者有更好的建议和意见，欢迎发送邮件到：afeng_studio@yeah.net
  ******************************************************************************

  ******************************************************************************
  * @brief NRF24L01+ 2.4GHz无线收发器驱动函数使用说明
  *
  * 1. 硬件接口
  *    - 使用硬件SPI接口（SPI），但片选引脚（CSN）采用软件控制
  
  * 2.软件接口
  *    - 驱动函数基于STM32 HAL库的SPI阻塞式传输函数HAL_SPI_Transmit/Receive实现，Timeout=100ms

  * 3. 相关引脚宏定义（已添加到NRF24L01.h中，请勿更改）
  *        #define NRF_CSN_Pin GPIO_PIN_4      // PA4 - 片选引脚，推挽输出模式（初始化为高电平）
  *        #define NRF_CE_Pin GPIO_PIN_1       // PB1 - 使能引脚，推挽输出模式（初始化为低电平）
  *        #define NRF_IRQ_Pin GPIO_PIN_0      // PB0 - 中断引脚，外部中断模式（下降沿触发）
  *
  * 4. 主要使用函数
  *    - 初始化函数：
  *        void NRF24L01_Init(SPI_HandleTypeDef *hspi, NRF_InitTypeDef *init);
  *        void NRF24L01_GetDefaultConfig(NRF_InitTypeDef *init);
  *    - NRF24L01结构体初始化参数：
  *        NRF24L01_GetDefaultConfig()函数默认初始化值：
  *        - InterruptConfig: NRF_INTERRUPT_TX_DS_DISABLE (禁用TX数据发送中断)
  *        - CRCConfig: NRF_CRC_ENABLE (启用CRC校验)
  *        - CRCLength: NRF_CRC_LENGTH_2BYTE (2字节CRC)
  *        - Mode: NRF_MODE_STANDBY_I (待机-I模式)
  *        - AutoAckPipes: NRF_PIPE0_BIT (仅管道0自动应答)
  *        - RxPipes: NRF_PIPE0_BIT (仅启用管道0)
  *        - AddrWidth: NRF_ADDR_WIDTH_5BYTES (5字节地址宽度)
  *        - AutoRetransDelay: NRF_ARD_500US (500us重传延迟)
  *        - AutoRetransCount: NRF_ARC_5_RETRANS (最多5次重传)
  *        - RFChannel: 2 (通道2，2.402GHz)
  *        - DataRate: NRF_DATA_RATE_1MBPS (1Mbps数据速率)
  *        - RFPower: NRF_RF_POWER_0DBM (0dBm输出功率)
  *        - DynamicPayloadPipes: NRF_PIPE0_BIT (管道0动态载荷)
  *        - Features: NRF_FEATURE_EN_DPL (启用动态载荷长度)
  *        - PayloadWidthPipe0: 32 (管道0载荷宽度32字节，此参数在启用动态载荷时无效)
  *        - PayloadWidthPipe1-5: 0 (管道1-5未使用)
  *        - TxAddress: {0xAA, 0xBB, 0xCC, 0xDD, 0xEE}//自动重发模式下TxAddress必须与RxAddressPipe0相同，否则无法接收ACK包
  *        - RxAddressPipe0: {0xAA, 0xBB, 0xCC, 0xDD, 0xEE}
  *        - RxAddressPipe1: {0xC2, 0xC2, 0xC2, 0xC2, 0xC2}
  *        - RxAddressPipe2: 0xC3
  *        - RxAddressPipe3: 0xC4
  *        - RxAddressPipe4: 0xC5
  *        - RxAddressPipe5: 0xC6
  *    - 模式控制：
  *        void NRF24L01_SetMode(SPI_HandleTypeDef *hspi, NRF_ModeTypeDef mode);
  *    - 数据传输（需要开启对应的模式）：
  *        void NRF24L01_WriteTxFIFO(SPI_HandleTypeDef *hspi,uint32_t Timeout);
  *        uint8_t NRF24L01_ReadRxFIFO(SPI_HandleTypeDef *hspi);
  * 
  * 5. 数据缓冲区
  *    - 发送缓冲区：NRF24L01_txbuffer[33] - 32字节数据 + 1字节长度信息
  *                 NRF24L01_rxbuffer_pipe0[33];
  *                 NRF24L01_rxbuffer_pipe1[33];  
  *                 NRF24L01_rxbuffer_pipe2[33];
  *                 NRF24L01_rxbuffer_pipe3[33];  
  *                 NRF24L01_rxbuffer_pipe4[33];  
  *                 NRF24L01_rxbuffer_pipe5[33];  
  *
  * 6. 中断回调函数模板（请复制到main.c中）
  *    - 函数功能：将接收到的数据存储到NRF24L01_rxbuffer_pipe0~5中，并将数据长度存储在NRF24L01_rxbuffer_pipe0~5[32]中
  *    - 中断回调函数模板已在此文件末尾提供
  *
  * 7. 使用示例（接收模式）
  *    @code
  *    // 1. 定义初始化结构体
  *    NRF_InitTypeDef nrf_init;
  *    
  *    // 2. 获取默认配置
  *    NRF24L01_GetDefaultConfig(&nrf_init);
  *    
  *    // 3. 修改配置（可选，一般不用修改）
  *    nrf_init.RFChannel = 40;           // 2.440GHz
  *    nrf_init.DataRate = NRF_DATA_RATE_2MBPS;
  *    nrf_init.RFPower = NRF_RF_POWER_0DBM;
  *    
  *    // 4. 初始化NRF24L01
  *    NRF24L01_Init(&hspi1, &nrf_init);
  *    
  *    // 5. 设置为接收模式
  *    NRF24L01_SetMode(&hspi1, NRF_MODE_RX);
  *    
  *    // 7. 接收数据（接收到数据->产生接收中断->最终调用到中断回调函数）
  *    // 数据接收后存储在NRF24L01_rxbuffer_pipe0~5中
  *    // 数据长度存储在NRF24L01_rxbuffer_pipe0~5[32]中
  *    @endcode
  *
  * 8. 使用示例（发送模式）
  *    @code
  *    // 1. 定义初始化结构体
  *    NRF_InitTypeDef nrf_init;
  *    
  *    // 2. 获取默认配置
  *    NRF24L01_GetDefaultConfig(&nrf_init);
  *    
  *    // 3. 修改配置（可选，一般不用修改）
  *    nrf_init.RFChannel = 40;           // 2.440GHz
  *    nrf_init.DataRate = NRF_DATA_RATE_2MBPS;
  *    nrf_init.RFPower = NRF_RF_POWER_0DBM;
  *    
  *    // 4. 初始化NRF24L01
  *    NRF24L01_Init(&hspi1, &nrf_init);  
  *    
  *    // 5. 设置为发送模式
  *    NRF24L01_SetMode(&hspi1, NRF_MODE_TX);
  *    
  *    // 6. 填充发送缓冲区
  *    memcpy(NRF24L01_txbuffer, data_to_send, data_length);
  *    NRF24L01_txbuffer[32] = data_length; // 设置发送数据长度
  *    
  *    // 7. 发送数据
  *    NRF24L01_WriteTxFIFO(&hspi1, 100);
  *    @endcode
  *
  * 9. 注意事项
  *    - 发送前确保设置为TX模式，接收前确保设置为RX模式
  *    - 发送数据前需要填充NRF24L01_txbuffer缓冲区，数据格式:32字节数据 + 1字节长度信息
  *    - 接收数据后从NRF24L01_rxbuffer读取，数据格式：32字节数据 + 1字节长度信息
  *
  ******************************************************************************
  */

/* 防止递归包含 --------------------------------------------------------------*/
#ifndef __NRF24L01_H
#define __NRF24L01_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* 头文件 --------------------------------------------------------------------*/
#include "main.h"

/* 导出类型 ------------------------------------------------------------------*/

/**
  * @brief  NRF24L01+ 中断配置
  * @note   每个取值互斥，并对应特定的中断屏蔽位
  */
typedef enum
{
    NRF_INTERRUPT_RX_DR_DISABLE  = 0x40,  /*!< 屏蔽 RX 数据就绪中断 (MASK_RX_DR = 1) */
    NRF_INTERRUPT_TX_DS_DISABLE  = 0x20,  /*!< 屏蔽 TX 数据发送完成中断 (MASK_TX_DS = 1) */
    NRF_INTERRUPT_MAX_RT_DISABLE = 0x10,  /*!< 屏蔽最大重传中断 (MASK_MAX_RT = 1) */
    NRF_INTERRUPT_ALL_ENABLE     = 0x00,  /*!< 使能全部中断（所有屏蔽位 = 0） */
} NRF_InterruptConfigTypeDef;

/**
  * @brief  NRF24L01+ CRC 配置
  * @note   这些取值互斥，并对应 CONFIG 寄存器的 EN_CRC 位（bit 3）
  */
typedef enum
{
    NRF_CRC_DISABLE = 0x00,  /*!< 禁用 CRC (EN_CRC = 0) */
    NRF_CRC_ENABLE  = 0x08,  /*!< 启用 CRC (EN_CRC = 1) */
} NRF_CRCConfigTypeDef;

/**
  * @brief  NRF24L01+ CRC 长度配置
  * @note   这些取值互斥，并对应 CONFIG 寄存器的 CRCO 位（bit 2）
  */
typedef enum
{
    NRF_CRC_LENGTH_1BYTE = 0x00,  /*!< 1 字节 CRC (CRCO = 0) */
    NRF_CRC_LENGTH_2BYTE = 0x04,  /*!< 2 字节 CRC (CRCO = 1) */
} NRF_CRCLengthTypeDef;

/**
  * @brief  NRF24L01+ 电源控制
  * @note   这些取值互斥，并对应 CONFIG 寄存器的 PWR_UP 位（bit 1）
  *         NRF_POWER_UP 表示 Standby-I 模式 (PWR_UP = 1, CE = 0)
  */
typedef enum
{
    NRF_POWER_DOWN = 0x00,  /*!< 掉电模式 (PWR_UP = 0) */
    NRF_POWER_UP   = 0x02,  /*!< Standby-I 模式 (PWR_UP = 1, CE = 0) */
} NRF_PowerControlTypeDef;

/**
  * @brief  NRF24L01+ 主模式选择
  * @note   这些取值互斥，并对应 CONFIG 寄存器的 PRIM_RX 位（bit 0）
  */
typedef enum
{
    NRF_PRIMARY_TX = 0x00,  /*!< 主发送模式 (PRIM_RX = 0) */
    NRF_PRIMARY_RX = 0x01,  /*!< 主接收模式 (PRIM_RX = 1) */
} NRF_PrimaryModeTypeDef;

/**
  * @brief  NRF24L01+ 地址宽度配置
  * @note   这些取值互斥，并对应 SETUP_AW 寄存器的 AW 位（bits 1:0）
  */
typedef enum
{
    NRF_ADDR_WIDTH_3BYTES = 0x01,  /*!< 3 字节地址宽度 (AW = 01) */
    NRF_ADDR_WIDTH_4BYTES = 0x02,  /*!< 4 字节地址宽度 (AW = 10) */
    NRF_ADDR_WIDTH_5BYTES = 0x03,  /*!< 5 字节地址宽度 (AW = 11) */
} NRF_AddrWidthTypeDef;

/**
  * @brief  NRF24L01+ 自动重传延迟配置
  * @note   这些取值互斥，并对应 SETUP_RETR 寄存器的 ARD 位（bits 7:4）
  *         延迟 = (ARD 值 + 1) * 250μs
  */
typedef enum
{
    NRF_ARD_250US   = 0x00,  /*!< 250μs 延迟 (ARD = 0000) */
    NRF_ARD_500US   = 0x10,  /*!< 500μs 延迟 (ARD = 0001) */
    NRF_ARD_750US   = 0x20,  /*!< 750μs 延迟 (ARD = 0010) */
    NRF_ARD_1000US  = 0x30,  /*!< 1000μs 延迟 (ARD = 0011) */
    NRF_ARD_1250US  = 0x40,  /*!< 1250μs 延迟 (ARD = 0100) */
    NRF_ARD_1500US  = 0x50,  /*!< 1500μs 延迟 (ARD = 0101) */
    NRF_ARD_1750US  = 0x60,  /*!< 1750μs 延迟 (ARD = 0110) */
    NRF_ARD_2000US  = 0x70,  /*!< 2000μs 延迟 (ARD = 0111) */
    NRF_ARD_2250US  = 0x80,  /*!< 2250μs 延迟 (ARD = 1000) */
    NRF_ARD_2500US  = 0x90,  /*!< 2500μs 延迟 (ARD = 1001) */
    NRF_ARD_2750US  = 0xA0,  /*!< 2750μs 延迟 (ARD = 1010) */
    NRF_ARD_3000US  = 0xB0,  /*!< 3000μs 延迟 (ARD = 1011) */
    NRF_ARD_3250US  = 0xC0,  /*!< 3250μs 延迟 (ARD = 1100) */
    NRF_ARD_3500US  = 0xD0,  /*!< 3500μs 延迟 (ARD = 1101) */
    NRF_ARD_3750US  = 0xE0,  /*!< 3750μs 延迟 (ARD = 1110) */
    NRF_ARD_4000US  = 0xF0,  /*!< 4000μs 延迟 (ARD = 1111) */
} NRF_AutoRetransDelayTypeDef;

/**
  * @brief  NRF24L01+ 自动重传次数配置
  * @note   这些取值互斥，并对应 SETUP_RETR 寄存器的 ARC 位（bits 3:0）
  *         0 = 禁用自动重传，1-15 = 重传次数
  */
typedef enum
{
    NRF_ARC_DISABLE   = 0x00,  /*!< 禁用自动重传 (ARC = 0000) */
    NRF_ARC_1_RETRANS = 0x01,  /*!< 最多 1 次重传 (ARC = 0001) */
    NRF_ARC_2_RETRANS = 0x02,  /*!< 最多 2 次重传 (ARC = 0010) */
    NRF_ARC_3_RETRANS = 0x03,  /*!< 最多 3 次重传 (ARC = 0011) */
    NRF_ARC_4_RETRANS = 0x04,  /*!< 最多 4 次重传 (ARC = 0100) */
    NRF_ARC_5_RETRANS = 0x05,  /*!< 最多 5 次重传 (ARC = 0101) */
    NRF_ARC_6_RETRANS = 0x06,  /*!< 最多 6 次重传 (ARC = 0110) */
    NRF_ARC_7_RETRANS = 0x07,  /*!< 最多 7 次重传 (ARC = 0111) */
    NRF_ARC_8_RETRANS = 0x08,  /*!< 最多 8 次重传 (ARC = 1000) */
    NRF_ARC_9_RETRANS = 0x09,  /*!< 最多 9 次重传 (ARC = 1001) */
    NRF_ARC_10_RETRANS = 0x0A, /*!< 最多 10 次重传 (ARC = 1010) */
    NRF_ARC_11_RETRANS = 0x0B, /*!< 最多 11 次重传 (ARC = 1011) */
    NRF_ARC_12_RETRANS = 0x0C, /*!< 最多 12 次重传 (ARC = 1100) */
    NRF_ARC_13_RETRANS = 0x0D, /*!< 最多 13 次重传 (ARC = 1101) */
    NRF_ARC_14_RETRANS = 0x0E, /*!< 最多 14 次重传 (ARC = 1110) */
    NRF_ARC_15_RETRANS = 0x0F, /*!< 最多 15 次重传 (ARC = 1111) */
} NRF_AutoRetransCountTypeDef;

/**
  * @brief  NRF24L01+ RF 数据速率配置
  * @note   这些取值互斥，并对应 RF_SETUP 寄存器的 RF_DR_LOW 和 RF_DR_HIGH 位
  *         编码：[RF_DR_LOW, RF_DR_HIGH]
  */
typedef enum
{
    NRF_DATA_RATE_1MBPS   = 0x00,  /*!< 1Mbps (RF_DR_LOW=0, RF_DR_HIGH=0) */
    NRF_DATA_RATE_2MBPS   = 0x08,  /*!< 2Mbps (RF_DR_LOW=0, RF_DR_HIGH=1) */
    NRF_DATA_RATE_250KBPS = 0x20,  /*!< 250kbps (RF_DR_LOW=1, RF_DR_HIGH=0) */
} NRF_DataRateTypeDef;

/**
  * @brief  NRF24L01+ RF 输出功率配置
  * @note   这些取值互斥，并对应 RF_SETUP 寄存器的 RF_PWR 位（bits 2:1）
  */
typedef enum
{
    NRF_RF_POWER_N18DBM = 0x00,  /*!< -18dBm 输出功率 (RF_PWR = 00) */
    NRF_RF_POWER_N12DBM = 0x02,  /*!< -12dBm 输出功率 (RF_PWR = 01) */
    NRF_RF_POWER_N6DBM  = 0x04,  /*!< -6dBm 输出功率 (RF_PWR = 10) */
    NRF_RF_POWER_0DBM   = 0x06,  /*!< 0dBm 输出功率 (RF_PWR = 11) */
} NRF_RFPowerTypeDef;

/**
  * @brief  NRF24L01+ 工作模式
  * @note   这些是组合了 PWR_UP、PRIM_RX 和 CE 控制的完整工作模式
  */
typedef enum
{
    NRF_MODE_POWER_DOWN,  /*!< 掉电模式 (PWR_UP = 0) */
    NRF_MODE_STANDBY_I,   /*!< Standby-I 模式 (PWR_UP = 1, CE = 0) */
    NRF_MODE_RX,          /*!< RX 模式 (PWR_UP = 1, PRIM_RX = 1, CE = 1) */
    NRF_MODE_TX,          /*!< TX 模式 (PWR_UP = 1, PRIM_RX = 0, CE = 1) */
} NRF_ModeTypeDef;

/** @defgroup NRF24L01_Data_Pipe_Bit_Masks NRF24L01 数据管道位掩码
  * @brief NRF24L01+ 数据管道位掩码
  * @{
  */
#define NRF_PIPE0_BIT 0x01  /*!< 数据管道 0 位掩码 */
#define NRF_PIPE1_BIT 0x02  /*!< 数据管道 1 位掩码 */
#define NRF_PIPE2_BIT 0x04  /*!< 数据管道 2 位掩码 */
#define NRF_PIPE3_BIT 0x08  /*!< 数据管道 3 位掩码 */
#define NRF_PIPE4_BIT 0x10  /*!< 数据管道 4 位掩码 */
#define NRF_PIPE5_BIT 0x20  /*!< 数据管道 5 位掩码 */
/**
  * @}
  */

/** @defgroup NRF24L01_Feature_Configuration NRF24L01 功能配置
  * @brief NRF24L01+ 功能配置位掩码
  * @{
  */
#define NRF_FEATURE_EN_DYN_ACK  0x01  /*!< 使能 W_TX_PAYLOAD_NOACK 命令 */
#define NRF_FEATURE_EN_ACK_PAY  0x02  /*!< 使能带载荷的 ACK */
#define NRF_FEATURE_EN_DPL      0x04  /*!< 使能动态载荷长度 */
/**
  * @}
  */

/**
  * @brief  NRF24L01+ 初始化结构体
  * @note   此结构体包含 NRF24L01+ 的全部配置参数
  *         每个字段对应特定寄存器配置
  */
typedef struct
{
    /* CONFIG 寄存器 (0x00) 设置 - 每个字段互斥 */
    NRF_InterruptConfigTypeDef InterruptConfig;  /*!< 中断配置
                                                      此参数可取 @ref NRF_InterruptConfigTypeDef 中的值 */
    
    NRF_CRCConfigTypeDef CRCConfig;              /*!< CRC 使能配置
                                                      此参数可取 @ref NRF_CRCConfigTypeDef 中的值 */
    
    NRF_CRCLengthTypeDef CRCLength;              /*!< CRC 长度配置
                                                      此参数可取 @ref NRF_CRCLengthTypeDef 中的值 */
    
    NRF_ModeTypeDef Mode;          /*!< 工作模式配置
                                                      此参数可取 @ref NRF_ModeTypeDef 中的值 */
    
    /* EN_AA 寄存器 (0x01) 设置 - 自动应答使能管道位掩码 */
    uint8_t AutoAckPipes;                        /*!< 启用自动应答的管道                                           
                                                      使用按位或 (|) 组合多个 NRF_PIPEx_BIT 值
                                                      示例：NRF_PIPE0_BIT | NRF_PIPE1_BIT 同时使能管道 0 和管道 1 的自动应答
                                                      此参数是 @ref NRF24L01_Data_Pipe_Bit_Masks 的位掩码 */

    /* EN_RXADDR 寄存器 (0x02) 设置 - 接收管道使能位掩码 */
    uint8_t RxPipes;                             /*!< 启用的接收管道
                                                      使用按位或 (|) 组合多个 NRF_PIPEx_BIT 值
                                                      示例：NRF_PIPE0_BIT | NRF_PIPE1_BIT 同时使能管道 0 和管道 1 接收
                                                      此参数是 @ref NRF24L01_Data_Pipe_Bit_Masks 的位掩码 */
    /* SETUP_AW 寄存器 (0x03) 设置 */
    NRF_AddrWidthTypeDef AddrWidth;              /*!< 地址宽度配置
                                                      此参数可取 @ref NRF_AddrWidthTypeDef 中的值 */
    
    /* SETUP_RETR 寄存器 (0x04) 设置 */
    NRF_AutoRetransDelayTypeDef AutoRetransDelay; /*!< 自动重传延迟配置
                                                      此参数可取 @ref NRF_AutoRetransDelayTypeDef 中的值 */
    
    NRF_AutoRetransCountTypeDef AutoRetransCount; /*!< 自动重传次数配置
                                                      此参数可取 @ref NRF_AutoRetransCountTypeDef 中的值 */
    
    /* RF_CH 寄存器 (0x05) 设置 */
    uint8_t RFChannel;                           /*!< RF 通道频率 (0-125)
                                                      频率 = 2400 + RFChannel (MHz)
                                                      此参数必须设置为 0 到 125 之间的值 */
    
    /* RF_SETUP 寄存器 (0x06) 设置 */
    NRF_DataRateTypeDef DataRate;                /*!< RF 数据速率配置
                                                      此参数可取 @ref NRF_DataRateTypeDef 中的值 */
    
    NRF_RFPowerTypeDef RFPower;                  /*!< RF 输出功率配置
                                                      此参数可取 @ref NRF_RFPowerTypeDef 中的值 */
    
    /* DYNPD 寄存器 (0x1C) 设置 - 动态载荷管道位掩码 */
    uint8_t DynamicPayloadPipes;                 /*!< 启用动态载荷的管道                                          
                                                      使用按位或 (|) 组合多个 NRF_PIPEx_BIT 值
                                                      示例：NRF_PIPE0_BIT | NRF_PIPE1_BIT 同时使能管道 0 和管道 1 的动态载荷
                                                      此参数是 @ref NRF24L01_Data_Pipe_Bit_Masks 的位掩码 */
    
    /* FEATURE 寄存器 (0x1D) 设置 - 已启用功能位掩码 */
    uint8_t Features;                            /*!< 已启用功能
                                                      使用按位或 (|) 组合多个 NRF_FEATURE_xxx 值
                                                      示例：NRF_FEATURE_EN_DPL | NRF_FEATURE_EN_ACK_PAY 同时使能动态载荷和带载荷 ACK
                                                      此参数是 @ref NRF24L01_Feature_Configuration 的位掩码 */
    
    /* 各管道的载荷宽度设置（0-32 字节）
       这些字段对应 RX_PW_Px 寄存器 (0x11-0x16) */
    uint8_t PayloadWidthPipe0;                   /*!< 管道 0 载荷宽度（1-32 字节）
                                                      此值写入 RX_PW_P0 寄存器 (0x11)
                                                      若未使用管道 0，则设置为 0 */
    uint8_t PayloadWidthPipe1;                   /*!< 管道 1 载荷宽度（1-32 字节）
                                                      此值写入 RX_PW_P1 寄存器 (0x12)
                                                      若未使用管道 1，则设置为 0 */
    uint8_t PayloadWidthPipe2;                   /*!< 管道 2 载荷宽度（1-32 字节）
                                                      此值写入 RX_PW_P2 寄存器 (0x13)
                                                      若未使用管道 2，则设置为 0 */
    uint8_t PayloadWidthPipe3;                   /*!< 管道 3 载荷宽度（1-32 字节）
                                                      此值写入 RX_PW_P3 寄存器 (0x14)
                                                      若未使用管道 3，则设置为 0 */
    uint8_t PayloadWidthPipe4;                   /*!< 管道 4 载荷宽度（1-32 字节）
                                                      此值写入 RX_PW_P4 寄存器 (0x15)
                                                      若未使用管道 4，则设置为 0 */
    uint8_t PayloadWidthPipe5;                   /*!< 管道 5 载荷宽度（1-32 字节）
                                                      此值写入 RX_PW_P5 寄存器 (0x16)
                                                      若未使用管道 5，则设置为 0 */
    
    /* 地址设置
       这些字段对应 TX_ADDR 和 RX_ADDR_Px 寄存器 (0x0A-0x10) */
    uint8_t TxAddress[5];                        /*!< 发送地址（1-5 字节，取决于 AddrWidth）
                                                      此值写入 TX_ADDR 寄存器 (0x10) */
    uint8_t RxAddressPipe0[5];                   /*!< 管道 0 接收地址（1-5 字节，取决于 AddrWidth）
                                                      此值写入 RX_ADDR_P0 寄存器 (0x0A) */
    uint8_t RxAddressPipe1[5];                   /*!< 管道 1 接收地址（1-5 字节，取决于 AddrWidth）
                                                      此值写入 RX_ADDR_P1 寄存器 (0x0B) */
    uint8_t RxAddressPipe2;                      /*!< 管道 2 接收地址 LSB（仅 LSB，MSB = 管道 1 地址）
                                                      此值写入 RX_ADDR_P2 寄存器 (0x0C) */
    uint8_t RxAddressPipe3;                      /*!< 管道 3 接收地址 LSB（仅 LSB，MSB = 管道 1 地址）
                                                      此值写入 RX_ADDR_P3 寄存器 (0x0D) */
    uint8_t RxAddressPipe4;                      /*!< 管道 4 接收地址 LSB（仅 LSB，MSB = 管道 1 地址）
                                                      此值写入 RX_ADDR_P4 寄存器 (0x0E) */
    uint8_t RxAddressPipe5;                      /*!< 管道 5 接收地址 LSB（仅 LSB，MSB = 管道 1 地址）
                                                      此值写入 RX_ADDR_P5 寄存器 (0x0F) */
} NRF_InitTypeDef;

/* 导出常量 ------------------------------------------------------------------*/

/** @defgroup NRF24L01_Register_Map NRF24L01 寄存器映射
  * @brief NRF24L01+ 寄存器地址
  * @{
  */
#define NRF_REG_CONFIG          0x00  /*!< 配置寄存器 */
#define NRF_REG_EN_AA           0x01  /*!< 使能自动应答功能 */
#define NRF_REG_EN_RXADDR       0x02  /*!< 使能 RX 地址 */
#define NRF_REG_SETUP_AW        0x03  /*!< 地址宽度设置 */
#define NRF_REG_SETUP_RETR      0x04  /*!< 自动重传设置 */
#define NRF_REG_RF_CH           0x05  /*!< RF 通道 */
#define NRF_REG_RF_SETUP        0x06  /*!< RF 设置寄存器 */
#define NRF_REG_STATUS          0x07  /*!< 状态寄存器 */
#define NRF_REG_OBSERVE_TX      0x08  /*!< 发送观察寄存器 */
#define NRF_REG_RPD             0x09  /*!< 接收功率检测寄存器 */
#define NRF_REG_RX_ADDR_P0      0x0A  /*!< 数据管道 0 接收地址 */
#define NRF_REG_RX_ADDR_P1      0x0B  /*!< 数据管道 1 接收地址 */
#define NRF_REG_RX_ADDR_P2      0x0C  /*!< 数据管道 2 接收地址 */
#define NRF_REG_RX_ADDR_P3      0x0D  /*!< 数据管道 3 接收地址 */
#define NRF_REG_RX_ADDR_P4      0x0E  /*!< 数据管道 4 接收地址 */
#define NRF_REG_RX_ADDR_P5      0x0F  /*!< 数据管道 5 接收地址 */
#define NRF_REG_TX_ADDR         0x10  /*!< 发送地址 */
#define NRF_REG_RX_PW_P0        0x11  /*!< 数据管道 0 的 RX 载荷字节数 */
#define NRF_REG_RX_PW_P1        0x12  /*!< 数据管道 1 的 RX 载荷字节数 */
#define NRF_REG_RX_PW_P2        0x13  /*!< 数据管道 2 的 RX 载荷字节数 */
#define NRF_REG_RX_PW_P3        0x14  /*!< 数据管道 3 的 RX 载荷字节数 */
#define NRF_REG_RX_PW_P4        0x15  /*!< 数据管道 4 的 RX 载荷字节数 */
#define NRF_REG_RX_PW_P5        0x16  /*!< 数据管道 5 的 RX 载荷字节数 */
#define NRF_REG_FIFO_STATUS     0x17  /*!< FIFO 状态寄存器 */
#define NRF_REG_DYNPD           0x1C  /*!< 使能动态载荷长度 */
#define NRF_REG_FEATURE         0x1D  /*!< 功能寄存器 */
/**
  * @}
  */

/** @defgroup NRF24L01_Commands NRF24L01 命令
  * @brief NRF24L01+ SPI 命令
  * @{
  */
#define NRF_CMD_R_REGISTER      0x00  /*!< 读取命令和状态寄存器 */
#define NRF_CMD_W_REGISTER      0x20  /*!< 写入命令和状态寄存器 */
#define NRF_CMD_R_RX_PAYLOAD    0x61  /*!< 读取 RX 载荷：1 - 32 字节 */
#define NRF_CMD_W_TX_PAYLOAD    0xA0  /*!< 写入 TX 载荷：1 - 32 字节 */
#define NRF_CMD_FLUSH_TX        0xE1  /*!< 清空 TX FIFO */
#define NRF_CMD_FLUSH_RX        0xE2  /*!< 清空 RX FIFO */
#define NRF_CMD_REUSE_TX_PL     0xE3  /*!< 复用上一次发送的载荷 */
#define NRF_CMD_R_RX_PL_WID     0x60  /*!< 读取 RX FIFO 顶部 R_RX_PAYLOAD 的 RX 载荷宽度 */
#define NRF_CMD_W_ACK_PAYLOAD   0xA8  /*!< 写入要随 ACK 包一起发送的载荷 */
#define NRF_CMD_W_TX_PAYLOAD_NOACK 0xB0  /*!< 写入不带 ACK 的 TX 载荷 */
#define NRF_CMD_NOP             0xFF  /*!< 空操作 */
/**
  * @}
  */

/** @defgroup NRF24L01_Status_Register_Bits NRF24L01 状态寄存器位
  * @brief NRF24L01+ 状态寄存器位定义
  * @{
  */
#define NRF_STATUS_RX_DR        0x40  /*!< RX FIFO 数据就绪中断 */
#define NRF_STATUS_TX_DS        0x20  /*!< TX FIFO 数据发送完成中断 */
#define NRF_STATUS_MAX_RT       0x10  /*!< TX 最大重传次数中断 */
#define NRF_STATUS_RX_P_NO      0x0E  /*!< RX_FIFO 中可读载荷对应的数据管道编号 */
#define NRF_STATUS_TX_FULL      0x01  /*!< TX FIFO 满标志 */
/**
  * @}
  */

/** @defgroup NRF24L01_FIFO_Status_Register_Bits NRF24L01 FIFO 状态寄存器位
  * @brief NRF24L01+ FIFO 状态寄存器位定义（寄存器 0x17）
  * @{
  */
#define NRF_FIFO_STATUS_TX_REUSE  0x40  /*!< TX 复用标志（bit 6） */
#define NRF_FIFO_STATUS_TX_FULL   0x20  /*!< TX FIFO 满标志（bit 5） */
#define NRF_FIFO_STATUS_TX_EMPTY  0x10  /*!< TX FIFO 空标志（bit 4） */
#define NRF_FIFO_STATUS_RX_FULL   0x02  /*!< RX FIFO 满标志（bit 1） */
#define NRF_FIFO_STATUS_RX_EMPTY  0x01  /*!< RX FIFO 空标志（bit 0） */
/**
  * @}
  */


#define NRF_CSN_Pin GPIO_PIN_4      // PA4 - 片选引脚，推挽输出模式（初始化为高电平）
#define NRF_CE_Pin GPIO_PIN_1       // PB1 - 使能引脚，推挽输出模式（初始化为低电平）
#define NRF_IRQ_Pin GPIO_PIN_0      // PB0 - 中断引脚，外部中断模式（下降沿触发）

/* NRF24L01 全局数据包缓冲区，32字节数据 + 1字节数据长度 */
extern uint8_t NRF24L01_txbuffer[33];       
extern uint8_t NRF24L01_rxbuffer_pipe0[33];
extern uint8_t NRF24L01_rxbuffer_pipe1[33];  
extern uint8_t NRF24L01_rxbuffer_pipe2[33];
extern uint8_t NRF24L01_rxbuffer_pipe3[33];  
extern uint8_t NRF24L01_rxbuffer_pipe4[33];  
extern uint8_t NRF24L01_rxbuffer_pipe5[33];  

/* 初始化函数 */
void NRF24L01_Init(SPI_HandleTypeDef *hspi, NRF_InitTypeDef *init);
void NRF24L01_GetDefaultConfig(NRF_InitTypeDef *init);

/* 模式控制函数 */
void NRF24L01_SetMode(SPI_HandleTypeDef *hspi, NRF_ModeTypeDef mode);

/* 数据发送和接收函数 */
void NRF24L01_WriteTxFIFO(SPI_HandleTypeDef *hspi,uint32_t Timeout);
void NRF24L01_ReadRxFIFO(SPI_HandleTypeDef *hspi);

/* FIFO 控制函数 */
void NRF24L01_FlushRxFIFO(SPI_HandleTypeDef *hspi);
void NRF24L01_FlashTxFIFO(SPI_HandleTypeDef *hspi);

/* 状态寄存器检查函数 */
uint8_t NRF24L01_GetStatus(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_GetFIFOStatus(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_IsRxDataReady(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_IsTxDataSent(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_IsMaxRetransmit(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_GetRxPipeNumber(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_IsTxFIFOFull(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_IsRxFIFOEmpty(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_GetPipeNum(SPI_HandleTypeDef *hspi);
void NRF24L01_ClearInterruptFlags(SPI_HandleTypeDef *hspi, uint8_t flags);

/* 注意：请将此回调函数放入 main.c 文件 */
// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
// {
//   switch (GPIO_Pin) {
//     case NRF_IRQ_Pin:
//         /* TxFIFO 阻塞检查 */
//       if(NRF24L01_IsMaxRetransmit(&hspi1)){
//         /* 用户可添加自己的实现来报告文件名和行号，
//         例如：printf("参数值错误：文件 %s 第 %d 行\r\n", file, line) */
//         NRF24L01_ClearInterruptFlags(&hspi1, NRF_STATUS_MAX_RT);
//         NRF24L01_FlashTxFIFO(&hspi1);
//       }
//       if(NRF24L01_IsRxDataReady(&hspi1)){
//         NRF24L01_ClearInterruptFlags(&hspi1, NRF_STATUS_RX_DR);
//         NRF24L01_ReadRxFIFO(&hspi1);
//       }
//       if(NRF24L01_IsTxDataSent(&hspi1)){
//         /* 发送成功 */
//         NRF24L01_ClearInterruptFlags(&hspi1, NRF_STATUS_TX_DS);
//       }
//       break;
//     default:
//       break;
//   }
// }


#ifdef __cplusplus
}
#endif

#endif /* __NRF24L01_H */
