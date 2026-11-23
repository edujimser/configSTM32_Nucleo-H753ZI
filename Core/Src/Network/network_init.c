#include <stdio.h>
#include <string.h>
#include "main.h"
#include "cmsis_os2.h"
#include "Network/network.h"
#include "Network/network_cofing.h"
#include "Network/Mongoose/mongoose.h"
#include "MsgUart/TestUart/TestUart.h"
#include "MsgUart/TxUart/TxUart.h"


extern I_TestUart Uart1;

const  NetConfig_t get_cfg_callback = NET_CONFIG_DEFAULT;
const  NetConfig_t get_cfg          = NET_CONFIG_TYPE1;

struct mg_mgr get_mgr;
struct mg_tcpip_if get_mif;
struct mg_tcpip_driver_stm32h_data get_drv = { .mdc_cr = -1 };





NetStatus_t Network_Init(void){
char   buf_msg [250] = {0};
NetConfig_t get_cfg_active = get_cfg;

	MSG_Enviar(MSG_WEB,
	   "\r\n==========================================================\r\n"
	   " [Init] Inicializando servidor web\r\n"
	   "==========================================================\r\n",
	   Uart1
	);
	

	if(Network_ValidateConfig(&get_cfg_active) != NET_OK){
	  MSG_Enviar(MSG_ERROR,"[NET][ERROR]: configuracion no valida en cfg\r\n", Uart1);
	  return NET_ERR_CONFIG;
	}


    //CONFIG -->
	if(Network_Action(&get_cfg_active) != NET_OK){
		MSG_Enviar(MSG_WEB,"Configurando por defecto, configuracion type no activada\r\n", Uart1);
		

		get_cfg_active = get_cfg_callback;

	  if(Network_ValidateConfig(&get_cfg_active) != NET_OK){
		  MSG_Enviar(MSG_ERROR,"[NET][ERROR]: configuracion no valida en cfg default\r\n", Uart1);
		  
		  return NET_ERR_CONFIG;
		}
	  //CONFIG DEFAULT -->
	   Network_Action(&get_cfg_active);
	}



	//DATA-->
	uint8_t *ip     = (uint8_t *)&get_mif.ip;
	uint8_t *mask   = (uint8_t *)&get_mif.mask;
	uint8_t *getway = (uint8_t *)&get_mif.gw;
	uint8_t *mac    = (uint8_t *)get_mif.mac;

	MSG_Enviar(MSG_WEB,"+-----------+-----------------------+\r\n", Uart1);
	MSG_Enviar(MSG_WEB,"| Parametro | Valor                 |\r\n", Uart1);
	MSG_Enviar(MSG_WEB,"+-----------+-----------------------+\r\n", Uart1);
	
	snprintf(buf_msg, sizeof(buf_msg), "| %-9s | %-21s |\r\n", "Modo",
		  (get_cfg_active.mode == NET_MODE_DHCP) ? "DHCP" : "IP fija");
	MSG_Enviar(MSG_WEB,buf_msg, Uart1);
	snprintf(buf_msg, sizeof(buf_msg), "| %-9s | %3u.%u.%u.%-9u |\r\n", "IP",
		  ip[0], ip[1], ip[2], ip[3]);
	MSG_Enviar(MSG_WEB,buf_msg, Uart1);
	snprintf(buf_msg, sizeof(buf_msg), "| %-9s | %3u.%u.%u.%-9u |\r\n", "Mascara",
		  mask[0], mask[1], mask[2], mask[3]);
	MSG_Enviar(MSG_WEB,buf_msg, Uart1);
	
	snprintf(buf_msg, sizeof(buf_msg), "| %-9s | %3u.%u.%u.%-9u |\r\n", "Gateway",
		  getway[0], getway[1], getway[2], getway[3]);
	MSG_Enviar(MSG_WEB,buf_msg, Uart1);
	snprintf(buf_msg, sizeof(buf_msg), "| %-9s | %02X:%02X:%02X:%02X:%02X:%02X     |\r\n", "MAC",
		  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	MSG_Enviar(MSG_WEB,buf_msg, Uart1);
	snprintf(buf_msg, sizeof(buf_msg), "| %-9s | %-21u |\r\n", "Puerto", get_cfg_active.http_port);
	MSG_Enviar(MSG_WEB,buf_msg, Uart1);
	MSG_Enviar(MSG_WEB,"+-----------+-----------------------+\r\n", Uart1);
	

	MSG_Enviar(MSG_WEB,
	   "\r\n==========================================================\r\n"
	   " [NET][Init] Fin inicialización servidor web\r\n"
	   "==========================================================\r\n",
	   Uart1
	);
	

  return NET_OK;

}

NetStatus_t Network_Action(const NetConfig_t *cfg) {
	char buf_data[250] = {0};

	//MSG CONFIG -->
	mg_log_set_fn(mongoose_log_fn, NULL);
	mg_log_set(cfg->log_level == 0 ? MG_LL_ERROR :
			   cfg->log_level == 1 ? MG_LL_INFO :
			   cfg->log_level == 2 ? MG_LL_DEBUG : MG_LL_VERBOSE);

	mg_mgr_init(&get_mgr);

	//ASIGNACION DE DATOS -->
	memset(&get_mif, 0, sizeof(get_mif));

	 //DRIVER ETH STM -->
	 get_mif.driver = &mg_tcpip_driver_stm32h;
	 get_mif.driver_data = &get_drv;

	 //MAC-->
	 uint32_t *uid = (uint32_t *)UID_BASE;

	 get_mif.mac[0] = 2;
	 get_mif.mac[1] = uid[0] & 255;
	 get_mif.mac[2] = (uid[0] >> 10) & 255;
	 get_mif.mac[3] = (uid[0] >> 19) & 255;
	 get_mif.mac[4] = uid[1] & 255;
	 get_mif.mac[5] = uid[2] & 255;

	 if (cfg->mode == NET_MODE_STATIC) {
	   get_mif.ip   = mg_htonl(MG_U32(cfg->ip[0],   cfg->ip[1],   cfg->ip[2],   cfg->ip[3]));
	   get_mif.mask = mg_htonl(MG_U32(cfg->mask[0], cfg->mask[1], cfg->mask[2], cfg->mask[3]));
	   get_mif.gw   = mg_htonl(MG_U32(cfg->gw[0],   cfg->gw[1],   cfg->gw[2],   cfg->gw[3]));
	 }

	/* DIAGNOSTICO PHY: borrar despues */
	extern ETH_HandleTypeDef heth;
	for (uint32_t a = 0; a < 32; a++) {
		uint32_t id1 = 0xFFFFu;
		if (HAL_ETH_ReadPHYRegister(&heth, a, 2, &id1) == HAL_OK && id1 != 0xFFFFu && id1 != 0u) {
			snprintf(buf_data, sizeof(buf_data), "[PHY] direccion %lu ID1=0x%04lX\r\n",
					 (unsigned long) a, (unsigned long) id1);
			net_print(buf_data);
		}
	}
	net_print("[PHY] fin del escaneo\r\n");

	 //INICIALIZACION TCP -->
	 mg_tcpip_init(&get_mgr, &get_mif);

	 //PORT -->
	 snprintf(buf_data, sizeof(buf_data), "http://0.0.0.0:%u", cfg->http_port);
	 if (mg_http_listen(&get_mgr, buf_data, http_ev_handler, NULL) == NULL) {
	   net_print("[NET] ERROR: no se pudo abrir el servidor web\r\n");
	   return NET_ERR_LISTEN;
	 }

	snprintf(buf_data, sizeof(buf_data), "[NET] Modo: %s, puerto web: %u\r\n",
	cfg->mode == NET_MODE_DHCP ? "DHCP" : "IP FIJA", cfg->http_port);
	net_print(buf_data);

	 //DHCP
	 uint64_t t0 = mg_millis();
	 while (get_mif.state != MG_TCPIP_STATE_READY && cfg->mode != NET_MODE_STATIC) {
	   mg_mgr_poll(&get_mgr, 50);
	   if (mg_millis() - t0 > cfg->link_timeout_ms) {
		 net_print("[NET] ERROR: sin IP tras el tiempo maximo\r\n");
		 return NET_ERR_TIMEOUT;
	   }
	 }


	 return NET_OK;
}



	NetStatus_t Network_ValidateConfig(const NetConfig_t *c) {
	if (c->mode != NET_MODE_DHCP && c->mode != NET_MODE_STATIC) return NET_ERR_CONFIG;
	if (c->http_port == 0) return NET_ERR_CONFIG;
	if (c->mode == NET_MODE_STATIC) {
	if ((c->ip[0] | c->ip[1] | c->ip[2] | c->ip[3]) == 0) return NET_ERR_CONFIG;
	if ((c->mask[0] | c->mask[1] | c->mask[2] | c->mask[3]) == 0) return NET_ERR_CONFIG;
	}
	return NET_OK;
}
