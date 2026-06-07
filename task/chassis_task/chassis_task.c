#include "chassis_task.h"

#include "FreeRTOS.h"
#include "task.h"

#include "can.h"
#include "chassis_control.h"
#include "dj_motor_ctrl.h"

#define CHASSIS_TASK_PERIOD_MS (2U)

err_t chassis_status[4] = {PENDING, PENDING, PENDING, PENDING};

static err_t chassis_can2_config_filter(STM32CAN_t *can) {
  if (can == NULL) {
    return PTR_NULL;
  }

  CAN_FilterTypeDef filter = {0};
  filter.FilterBank = 14U;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;
  filter.FilterIdHigh = 0U;
  filter.FilterIdLow = 0U;
  filter.FilterMaskIdHigh = 0U;
  filter.FilterMaskIdLow = 0U;
  filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  filter.FilterActivation = ENABLE;
  filter.SlaveStartFilterBank = 14U;

  return STM32CAN_ConfigFilter(can, &filter);
}

static err_t chassis_can2_init(STM32CAN_t *can) {
  err_t error = STM32CAN_Init(can, &hcan2, NULL);
  if (error != OK) {
    return error;
  }

  error = chassis_can2_config_filter(can);
  if (error != OK) {
    return error;
  }

  error = dj_motor_attach_can(can);
  if (error != OK) {
    return error;
  }

  return STM32CAN_Start(can);
}

void start_chassis_task(void *argument) {
  RM_UNUSED(argument);

  static STM32CAN_t chassis_can2;

  dj_motor_system_init();
  chassis_status[0] = dj_motor_feedback_queue_init();
  chassis_status[1] = chassis_can2_init(&chassis_can2);
  dj_motor_speed_pid_init();

  if ((chassis_status[0] != OK) || (chassis_status[1] != OK)) {
    vTaskDelete(NULL);
    return;
  }

  TickType_t previous_wake_time = xTaskGetTickCount();

  for (;;) {
    chassis_status[2] = dj_motor_update_feedback(CHASSIS_TASK_PERIOD_MS);
    Chassis_Mode();
    chassis_status[3] = dj_motor_control_send_can(&chassis_can2);
    vTaskDelayUntil(&previous_wake_time,
                    pdMS_TO_TICKS(CHASSIS_TASK_PERIOD_MS));
  }
}
