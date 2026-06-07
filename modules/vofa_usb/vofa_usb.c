#include "comp_utils.h"
#include "vofa_usb.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

// ==================== 参数表 ====================

//! VOFA USB ASCII 命令参数缓存项。
typedef struct {
  const char *name;
  float value;
  bool received;
} VofaUsbParameter_t;

static VofaUsbParameter_t vofa_usb_parameters[] = {
    {"speed", 0.0f, false},
    {"angle", 0.0f, false},
    {"mit", 0.0f, false},
};

static uint8_t vofa_usb_rx_fifo_[VOFA_USB_RX_LEN];
static char vofa_usb_cmd_buff_[VOFA_USB_RX_CMD_BUF_LEN];
static const uint8_t kVofaUsbTail[VOFA_USB_TAIL_LEN] = {0x00, 0x00, 0x80, 0x7f};

static volatile uint16_t rx_head_ = 0U;
static volatile uint16_t rx_tail_ = 0U;
static volatile bool rx_fifo_overflowed_ = false;
static uint32_t rx_fifo_overflow_count_ = 0U;

static uint32_t cmd_overflow_count_ = 0U;
static uint16_t cmd_len_ = 0U;
static bool cmd_overflowed_ = false;

static VofaUsb_t *instance_ = NULL;

// ==================== FIFO 索引与字符校验工具 ====================

//! 计算环形 FIFO 下一个索引；VOFA_USB_RX_LEN 必须为 2 的幂。
static uint16_t VofaUsb_NextRxIndex(uint16_t index)
{
  return (uint16_t)((index + 1U) & (VOFA_USB_RX_LEN - 1U));
}

//! 命令名必须以字母或下划线开头。
static bool IsCommandNameStart(char ch)
{
  return ((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z')) ||
         (ch == '_');
}

//! 命令名后续字符允许字母、数字或下划线。
static bool IsCommandNameChar(char ch)
{
  return IsCommandNameStart(ch) || ((ch >= '0') && (ch <= '9'));
}

//! 检查命令名是否合法，避免把异常输入写入参数表。
static bool IsCommandNameValid(const char *name)
{
  if ((name == NULL) || (name[0] == '\0') || !IsCommandNameStart(name[0])) {
    return false;
  }
  for (size_t i = 1U; name[i] != '\0'; ++i) {
    if (!IsCommandNameChar(name[i])) {
      return false;
    }
  }
  return true;
}

//! 命令值不允许出现空白字符，便于保持 name=value! 格式简单明确。
static bool HasWhitespace(const char *text)
{
  for (size_t i = 0U; text[i] != '\0'; ++i) {
    const char ch = text[i];
    if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n')) {
      return true;
    }
  }
  return false;
}

// ==================== 参数表操作 ====================

//! 查找参数表项，不存在则返回 NULL。
static VofaUsbParameter_t *FindParameter(const char *name)
{
  if (name == NULL) {
    return NULL;
  }

  for (size_t i = 0U; i < ARRAY_LEN(vofa_usb_parameters); ++i) {
    if (strcmp(vofa_usb_parameters[i].name, name) == 0) {
      return &vofa_usb_parameters[i];
    }
  }
  return NULL;
}

//! 设置参数值，并标记该参数已经收到过。
static void SetParameter(const char *name, float value)
{
  VofaUsbParameter_t *parameter = FindParameter(name);
  if (parameter == NULL) {
    return;
  }

  taskENTER_CRITICAL();
  parameter->value = value;      // 更新参数值。
  parameter->received = true;    // 标记参数已收到过。
  taskEXIT_CRITICAL();
}

// ==================== USB 回调到任务环形 FIFO ====================

//! 从 USB CDC 回调写入接收 FIFO，满时标记溢出并丢弃当前字节。
static void VofaUsb_PushRxByteFromCallback(uint8_t byte)
{
  const uint16_t head = rx_head_;
  const uint16_t next_head = VofaUsb_NextRxIndex(head);

  if (next_head == rx_tail_) {
    rx_fifo_overflowed_ = true;  // 留一个空位区分 FIFO 空/满。
    ++rx_fifo_overflow_count_;
    return;
  }

  vofa_usb_rx_fifo_[head] = byte;
  rx_head_ = next_head;
}

//! 在任务上下文批量取出 FIFO 数据，返回是否取到数据。
static bool VofaUsb_PopRxBatch(uint8_t *buf, uint16_t *count)
{
  *count = 0U;
  taskENTER_CRITICAL();
  while (rx_tail_ != rx_head_) {
    buf[*count] = vofa_usb_rx_fifo_[rx_tail_];
    rx_tail_ = VofaUsb_NextRxIndex(rx_tail_);
    ++(*count);
  }
  taskEXIT_CRITICAL();
  return (*count > 0U);
}

//! 消费并清除 RX FIFO 溢出标志。
static bool VofaUsb_ConsumeRxOverflow(void)
{
  taskENTER_CRITICAL();
  const bool overflowed = rx_fifo_overflowed_;
  rx_fifo_overflowed_ = false;
  taskEXIT_CRITICAL();
  return overflowed;
}

//! 清空软件 FIFO：直接追平 tail 到当前 head。
static void VofaUsb_ClearRxFifo(void)
{
  taskENTER_CRITICAL();
  rx_tail_ = rx_head_;
  taskEXIT_CRITICAL();
}

// ==================== ASCII 命令解析 ====================

//! 重置命令累积缓冲状态。
static void VofaUsb_ResetCommand(void)
{
  cmd_len_ = 0U;
  cmd_overflowed_ = false;
}

//! 解析命令缓冲区中的 name=value! 命令，并更新参数表。
static err_t VofaUsb_ParseCommand(void)
{
  if (cmd_len_ == 0U) {
    return FAILED;
  }

  vofa_usb_cmd_buff_[cmd_len_] = '\0';  // 添加字符串结束符，供 strtof/strcmp 使用。
  char *separator = (char *)memchr(vofa_usb_cmd_buff_, '=', (size_t)cmd_len_);
  if ((separator == NULL) || (separator == vofa_usb_cmd_buff_) ||
      (separator == &vofa_usb_cmd_buff_[cmd_len_ - 1U])) {
    return FAILED;
  }

  *separator = '\0';
  const char *name = vofa_usb_cmd_buff_;
  const char *value_text = separator + 1;

  if (!IsCommandNameValid(name) || HasWhitespace(value_text)) {
    return FAILED;
  }

  char *value_end = NULL;
  const float value = strtof(value_text, &value_end);
  if ((value_end == value_text) || (value_end == NULL) ||
      (*value_end != '\0') || !isfinite(value)) {
    return FAILED;
  }

  SetParameter(name, value);
  if ((instance_ != NULL) && (instance_->command_callback_ != NULL)) {
    instance_->command_callback_(name, value);
  }

  return OK;
}

//! 逐字节累积命令，遇到 ! 作为命令结束符；CR/LF 被忽略。
static void VofaUsb_ProcessRxByte(uint8_t byte)
{
  if ((byte == '\r') || (byte == '\n')) {
    return;
  }

  if (byte == '!') {  // VOFA 下行命令结束符。
    if (!cmd_overflowed_ && (cmd_len_ > 0U)) {
      (void)VofaUsb_ParseCommand();
    }
    VofaUsb_ResetCommand();
    return;
  }

  if (cmd_overflowed_) {
    return;
  }

  if (cmd_len_ >= (VOFA_USB_RX_CMD_BUF_LEN - 1U)) {
    cmd_overflowed_ = true;  // 当前命令过长，丢弃到下一个 !。
    ++cmd_overflow_count_;
    cmd_len_ = 0U;
    return;
  }

  vofa_usb_cmd_buff_[cmd_len_] = (char)byte;
  ++cmd_len_;
}

// ==================== USB CDC 接收入口 ====================

//! USB CDC 接收回调：写入 FIFO 后通知任务解析。
static void VofaUsb_RxCallback(uint8_t *data, size_t size)
{
  if ((data == NULL) || (size == 0U)) {
    return;
  }

  for (size_t i = 0U; i < size; ++i) {
    VofaUsb_PushRxByteFromCallback(data[i]);
  }

  if ((instance_ != NULL) && (instance_->thread_alert != NULL)) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(instance_->thread_alert, VOFA_USB_SIGNAL_RAW_READY,
                       eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

// ==================== 初始化与任务周期处理 ====================

//! 初始化 VOFA USB 对象，绑定 USB CDC 通道。
err_t VofaUsb_Init(VofaUsb_t *self, USBD_HandleTypeDef *usb_handle)
{
  if (self == NULL) {
    return PTR_NULL;
  }

  memset(self, 0, sizeof(*self));
  self->init_error_ = STM32USB_Init(&self->usb_, usb_handle,
                                    VofaUsb_RxCallback, NULL);
  instance_ = self;

  return self->init_error_;
}

//! 启动 VOFA USB；USB Device 的底层启动由 MX_USB_DEVICE_Init() 负责。
err_t VofaUsb_Start(VofaUsb_t *self)
{
  if (self == NULL) {
    return PTR_NULL;
  }

  return self->init_error_;
}

//! 任务周期入口：处理溢出恢复、排空 FIFO，并解析 ASCII 命令。
void VofaUsb_Update(VofaUsb_t *self, uint32_t timeout_ms)
{
  if (self == NULL) {
    return;
  }

  xTaskNotifyWait(0U, VOFA_USB_SIGNAL_RAW_READY, NULL, pdMS_TO_TICKS(timeout_ms));

  if (VofaUsb_ConsumeRxOverflow()) {
    VofaUsb_ClearRxFifo();
    VofaUsb_ResetCommand();
  }

  uint8_t local_buf[VOFA_USB_RX_LEN];
  uint16_t count = 0U;
  VofaUsb_PopRxBatch(local_buf, &count);

  for (uint16_t i = 0U; i < count; ++i) {
    VofaUsb_ProcessRxByte(local_buf[i]);  // 逐字节推进命令解析。
  }

  if (VofaUsb_ConsumeRxOverflow()) {
    VofaUsb_ClearRxFifo();
    VofaUsb_ResetCommand();
  }
}

// ==================== 公有接口 ====================

//! 设置命令回调函数。
void VofaUsb_SetCommandCallback(VofaUsb_t *self,
                                VofaUsb_CommandCallback_t callback)
{
  if (self == NULL) {
    return;
  }
  self->command_callback_ = callback;
}

//! 获取参数值：参数存在且已收到过则返回 true。
bool VofaUsb_GetParameter(const char *name, float *value)
{
  if (value == NULL) {
    return false;
  }

  VofaUsbParameter_t *parameter = FindParameter(name);
  if (parameter == NULL) {
    return false;
  }

  taskENTER_CRITICAL();
  const bool received = parameter->received;  // 只在收到过有效命令后输出参数值。
  if (received) {
    *value = parameter->value;
  }
  taskEXIT_CRITICAL();

  return received;
}

//! 获取 speed 参数。
bool VofaUsb_GetSpeed(float *speed)
{
  return VofaUsb_GetParameter("speed", speed);
}

//! 获取 angle 参数。
bool VofaUsb_GetAngle(float *angle)
{
  return VofaUsb_GetParameter("angle", angle);
}

//! 发送 VOFA firewater 数据帧，每个 float 占 4 字节，尾部追加 00 00 80 7F。
err_t VofaUsb_Send(VofaUsb_t *self, const float *data, size_t size)
{
  if ((self == NULL) || (data == NULL)) {
    return PTR_NULL;
  }
  if ((size == 0U) || (size > VOFA_USB_MAX_FLOATS)) {
    return FAILED;
  }

  const size_t payload_len = size * sizeof(float);          // 纯 float 数据长度。
  const size_t frame_len = payload_len + VOFA_USB_TAIL_LEN; // 完整 firewater 帧长度。
  uint8_t frame[VOFA_USB_TX_LEN];
  memcpy(frame, data, payload_len);
  memcpy(frame + payload_len, kVofaUsbTail, VOFA_USB_TAIL_LEN);

  taskENTER_CRITICAL();
  const err_t err = STM32USB_Write(&self->usb_, frame, frame_len);
  taskEXIT_CRITICAL();

  return err;
}
