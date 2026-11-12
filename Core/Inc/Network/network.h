#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include "Network/network_cofing.h"
#include "Network/Mongoose/mongoose.h"


#define OSDELAY_UART_MSG     50                      //TIEMPO TERMINAL
#define LOGNET_LINE_MAX      225                     //NO SUPERAR EL MAXIMO DE 250




typedef enum {
  NET_OK = 0,
  NET_ERR_CONFIG,
  NET_ERR_INIT,
  NET_ERR_LISTEN,
  NET_ERR_TIMEOUT
} NetStatus_t;


typedef struct {
  bool init;
  bool run;
  uint8_t  mode;             // NET_MODE_DHCP o NET_MODE_STATIC
  uint8_t  ip[4];            // solo si mode == STATIC
  uint8_t  mask[4];          // Mask
  uint8_t  gw[4];            // GetAwy
  uint16_t http_port;        // puerto del servidor web
  uint8_t  log_level;        // 0 error, 1 info, 2 debug
  uint32_t report_ms;        // periodo del informe de estado
  uint32_t link_timeout_ms;  // espera maxima para obtener IP
} NetConfig_t;



extern const NetConfig_t get_cfg;
extern struct mg_mgr get_mgr;
extern struct mg_tcpip_if get_mif;
extern struct mg_tcpip_driver_stm32h_data get_drv;


// =============================================================================================================================
// THREAD
// =============================================================================================================================
void ThreadFunc_Network(void *argument);

// =============================================================================================================================
// INIT
// =============================================================================================================================
NetStatus_t Network_Init(void);
NetStatus_t Network_Action(const NetConfig_t *cfg);
NetStatus_t Network_ValidateConfig(const NetConfig_t *c);

// =============================================================================================================================
// MANEJADOR
// =============================================================================================================================
void http_ev_handler(struct mg_connection *c, int ev, void *ev_data);

// =============================================================================================================================
// MSG
// =============================================================================================================================
void net_print(const char *s);
void mongoose_log_fn(char ch, void *param);

#endif
