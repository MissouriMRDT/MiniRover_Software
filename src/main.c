#include "net.h"
#include "web.h"
#include <esp_log.h>
#include <nvs_flash.h>

static const char *TAG_MAIN = "main.c";

void app_main(void)
{
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG_MAIN, "ESP_WIFI_MODE_AP");
  wifi_init_softap();
  start_webserver();

  motor_control_main();
}
