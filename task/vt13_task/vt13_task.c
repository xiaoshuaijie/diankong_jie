

#include "vt13_task.h"
#include "bsp_log.h"

#include "FreeRTOS.h"
#include <stdbool.h>

#include "task.h"

vt13_t vt13;
vt13_cmd_rc_t vt13_cmd_rc;
err_t vt13_status[4] = {ERR_NULL, ERR_NULL, ERR_NULL, ERR_NULL};

void Start_vt13_task(void *argument) {
  static bool last_online = false;
  static bool dma_start_ok = true;

  RM_UNUSED(argument);
  LOGINFO("[vt13_task] start");

  /* 初始化VT13接收模块。 */
  vt13_status[0] = vt13_init(&vt13);
  if (vt13_status[0] == RM_OK) {
    LOGINFO("[vt13_task] ready");
  } else {
    LOGERROR("[vt13_task] init failed status=%d", (int)vt13_status[0]);
  }

  while (1) {
    /* 启动一次DMA接收。 */
    vt13_status[1] = vt13_start_dma_recv(&vt13);
    if (vt13_status[1] != RM_OK) {
      if (dma_start_ok) {
        LOGERROR("[vt13_task] dma start failed status=%d", (int)vt13_status[1]);
        dma_start_ok = false;
      }
      vTaskDelay(2);
      continue;
    }
    if (!dma_start_ok) {
      LOGINFO("[vt13_task] dma start recovered");
      dma_start_ok = true;
    }

    /* 等待接收完成并解析；超时按离线处理。 */
    if (vt13_wait_dma_cplt(10)) {
      vt13_status[2] = vt13_parse_rc(&vt13, &vt13_cmd_rc);
      if (vt13_status[2] == RM_OK) {
        if (!last_online) {
          LOGINFO("[vt13_task] online");
          last_online = true;
        }
      } else if (last_online) {
        LOGWARNING("[vt13_task] data corrupted");
        last_online = false;
      }

    } else {
      vt13_status[3] = vt13_handle_offline(&vt13, &vt13_cmd_rc);
      if (last_online) {
        LOGWARNING("[vt13_task] offline");
        last_online = false;
      }
    }

    vTaskDelay(2);
  }
}
