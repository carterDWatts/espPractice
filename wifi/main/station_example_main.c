#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int s_retry_num = 0;

static const char *TAG = "wifi station";

static void event_handler(void* arg, esp_event_base_t event_base,
														int32_t event_id, void* event_data){

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		
		esp_wifi_connect();

	}else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		
		if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		}else{
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG,"connect to the AP fail");

	}else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

static void initialise_wifi(void){

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK( esp_wifi_start() );
}

void connect_wifi(char *ssid, char *pass){
		
	wifi_config_t wifi_config;
	bzero(&wifi_config, sizeof(wifi_config_t));
	memcpy(wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
	memcpy(wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

	ESP_LOGI(TAG, "Connecting to SSID:%s with PASSWORD: %s", ssid, pass);

	ESP_ERROR_CHECK( esp_wifi_disconnect() );
	ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	ESP_ERROR_CHECK( esp_wifi_connect() );
			
}

void app_main(void){

	ESP_ERROR_CHECK( nvs_flash_init() );

  s_wifi_event_group = xEventGroupCreate();
	initialise_wifi();

	char *ssid =		"1PSSTF"; 		// "HelloThere"; 
	char *pass = 		"page1234"; 	// "generalkenobi";
	//connect_wifi(ssid, pass);

	//vTaskDelay(10000 / portTICK_PERIOD_MS);
	
	//ssid =		"ORBI24";
	//char *pass =		"jaggedcarrot123";
	connect_wifi(ssid, pass);

	//ESP_LOGI("WIFI", "%s", wifi_config.sta);
	
	for(;;){
		if(WIFI_CONNECTED_BIT){
			ESP_LOGI("WIFI", "connected");
			break;
		}else{
			ESP_LOGI("WIFI", "failed to connect");
		}
	}
	
	ESP_LOGI("TEST", "finished");

}

