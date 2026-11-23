#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <stdint.h>

/*      [    LIST CONFIG    ]        [    NUMBER CONFIG    ]       */
#define   NET_MODE_DHCP                         0
#define   NET_MODE_STATIC                       1



#define NET_CONFIG_DEFAULT {                    \
  .mode            		= NET_MODE_STATIC,      \
  .ip              		= {192, 168, 1, 100},   \
  .mask            		= {255, 255, 255, 0},   \
  .gw              		= {192, 168, 1, 1},     \
  .http_port       		= 80,                   \
  .log_level       		= 1,                    \
  .report_ms       		= 5000,                 \
  .link_timeout_ms 		= 0,                    \
  };

#define NET_CONFIG_TYPE1 {                      \
  .mode            		= NET_MODE_DHCP,        \
  .ip              		= {192, 168, 1, 100},   \
  .mask            		= {255, 255, 255, 0},   \
  .gw              		= {192, 168, 1, 1},     \
  .http_port       		= 80,                   \
  .log_level       		= 1,                    \
  .report_ms       		= 5000,                 \
  .link_timeout_ms 		= 30000,                \
  };
#endif
