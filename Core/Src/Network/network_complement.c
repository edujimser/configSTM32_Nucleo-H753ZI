#include <stdio.h>
#include <string.h>
#include "main.h"
#include "cmsis_os2.h"
#include "Network/network.h"
#include "Network/network_cofing.h"
#include "Network/Mongoose/mongoose.h"
#include "MsgUart/TestUart/TestUart.h"
#include "MsgUart/TxUart/TxUart.h"

extern I_TestUart TestUart_1;



// =============================================================================================================================
// MSG
// =============================================================================================================================
void net_print(const char *s) { MSG_Enviar(s, TestUart_1); }

void mongoose_log_fn(char ch, void *param) {
  static char line[LOGNET_LINE_MAX];
  static uint32_t n = 0;
  (void)param;
  if (ch == '\r') return;
  if (ch == '\n' || n >= LOGNET_LINE_MAX - 3) {
    line[n++] = '\r'; line[n++] = '\n'; line[n] = '\0';
    net_print(line);
    n = 0;
    return;
  }
  if ((unsigned char)ch >= 32) line[n++] = ch;
}
