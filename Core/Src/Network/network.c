
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "cmsis_os2.h"
#include "Network/network.h"
#include "Network/network_cofing.h"
#include "Network/Mongoose/mongoose.h"
#include "MsgUart/TestUart/TestUart.h"
#include "MsgUart/TxUart/TxUart.h"
#include "Network/PageWeb/web_vars.h"      /* tabla de variables que se ven en la web */
#include "Network/PageWeb/web_demo.h"      /* simulacion de demostracion (borrar al usar tus variables) */
#include "Network/PageWeb/web_sys.h"       /* estado interno del STM32 (hilos, RAM, CPU) */
#include "Network/PageWeb/web_bus.h"       /* visor y forzado de tramas CAN / TRDP */
#include "Network/PageWeb/web_sig.h"       /* señales de comunicacion con forzado individual */
#include "Network/PageWeb/web_err.h"       /* errores: estado, historico, contadores */

extern I_TestUart TestUart_1;

/* Tabla de archivos web (la define packed_fs.c) y puntero que usa mongoose.c.
 * Si tu mongoose.h ya los declara, estas lineas no molestan. */
extern const struct mg_mem_file mg_packed_files[];
extern const struct mg_mem_file *mg_mem_files;

/* Valor de ejemplo que se cambia desde la pagina de configuracion */
static int g_valor_config = 0;



/* Solo letras, numeros, '.', '_' y '-' (sin rutas), maximo 32 caracteres */
static int nombre_valido(const char *n) {
  size_t len = strlen(n);
  if (len == 0 || len > 32 || n[0] == '.') return 0;
  for (size_t i = 0; i < len; i++) {
    char ch = n[i];
    if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
          ch == '.' || ch == '_' || ch == '-')) return 0;
  }
  return 1;
}

// =============================================================================================================================
// MANEJADOR
// =============================================================================================================================

/* =============================================================================
 * Manejador de eventos
 *   - /ws/<pagina>           : WebSocket, una ruta por pagina (ver web_pages[] en web_vars.c)
 *   - /api/valor             : GET lee el valor, POST lo cambia
 *   - cualquier otra ruta    : archivos de la web empaquetada
 *                              (/ -> index.html, /datos.html, /config.html, /style.css)
 * ============================================================================= */
void http_ev_handler(struct mg_connection *c, int ev, void *ev_data) {

  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    uint16_t page = WebVars_PageFromUri(hm->uri.buf, hm->uri.len);   /* /ws/<pagina> */

    if (hm->uri.len >= 4 && memcmp(hm->uri.buf, "/ws/", 4) == 0) {
      if (page == 0) {
        mg_http_reply(c, 404, "", "Pagina desconocida\n");
      } else {
        WEBPAGE_SET(c, page);          /* ANTES del upgrade: WS_OPEN se dispara dentro */
        mg_ws_upgrade(c, hm, NULL);
      }

    } else if (mg_match(hm->uri, mg_str("/api/upload"), NULL)) {
      /* Subida de archivo: POST /api/upload?name=archivo.ext  con el archivo como cuerpo.
       * Mongoose entrega el mensaje cuando ya tiene el cuerpo completo en RAM. */
      char name[40] = {0};
      mg_http_get_var(&hm->query, "name", name, sizeof(name));
      if (mg_strcmp(hm->method, mg_str("POST")) != 0) {
        mg_http_reply(c, 405, "", "Usa POST\n");
      } else if (hm->body.len > WEBFILE_MAX_UPLOAD) {
        mg_http_reply(c, 413, "", "Archivo demasiado grande (max %u bytes)\n", (unsigned) WEBFILE_MAX_UPLOAD);
      } else if (!nombre_valido(name)) {
        mg_http_reply(c, 400, "", "Nombre no valido\n");
      } else {
        char rep[80] = {0};
        int rc = WebFile_Upload(name, (const uint8_t *) hm->body.buf, hm->body.len, rep, sizeof(rep));
        mg_http_reply(c, rc == 0 ? 200 : 500, "", "%s\n", rep);
      }

    } else if (mg_match(hm->uri, mg_str("/api/download"), NULL)) {
      /* Descarga: GET /api/download?name=datos.csv */
      char name[40] = {0};
      const uint8_t *data = NULL; size_t len = 0; const char *mime = "application/octet-stream";
      mg_http_get_var(&hm->query, "name", name, sizeof(name));
      if (!nombre_valido(name) || WebFile_Download(name, &data, &len, &mime) != 0) {
        mg_http_reply(c, 404, "", "No existe\n");
      } else {
        mg_printf(c, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n"
                     "Content-Disposition: attachment; filename=\"%s\"\r\n"
                     "Content-Length: %lu\r\n\r\n", mime, name, (unsigned long) len);
        mg_send(c, data, len);
      }

    } else if (mg_match(hm->uri, mg_str("/api/errores"), NULL) || mg_match(hm->uri, mg_str("/api/senales"), NULL)) {
      /* Catalogo (textos de la configuracion en C) en JSON, enviado por trozos para no necesitar un buffer grande:
       *   GET /api/errores        lista de web_err_cfg.h
       *   GET /api/senales?bus=0  señales de web_sig_cfg.h (0 CAN, 1 TRDP) */
      int es_err = mg_match(hm->uri, mg_str("/api/errores"), NULL);
      char bq[4] = {0};
      mg_http_get_var(&hm->query, "bus", bq, sizeof(bq));
      int bus = (bq[0] == '1');
      char ent[420];
      mg_printf(c, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nCache-Control: no-store\r\n"
                   "Transfer-Encoding: chunked\r\n\r\n");
      mg_http_printf_chunk(c, "[");
      for (unsigned k = 0; ; k++) {
        int n = es_err ? WebErr_Catalog(k, ent, sizeof(ent)) : WebSig_Catalog(bus, k, ent, sizeof(ent));
        if (n <= 0) break;
        mg_http_printf_chunk(c, "%s%s", k ? "," : "", ent);
      }
      mg_http_printf_chunk(c, "]");
      mg_http_write_chunk(c, "", 0);

    } else if (mg_match(hm->uri, mg_str("/api/valor"), NULL)) {
      if (mg_strcmp(hm->method, mg_str("POST")) == 0) {
        char tmp[16] = {0};
        mg_http_get_var(&hm->query, "v", tmp, sizeof(tmp));
        g_valor_config = atoi(tmp);
        mg_http_reply(c, 200, "", "Valor guardado: %d\n", g_valor_config);
      } else {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                      "{\"valor\":%d}\n", g_valor_config);
      }

    } else {
      /* Archivos de la web empaquetada (packed_fs.c).
       * root_dir debe coincidir con el prefijo de las rutas de packed_fs.c:
       * "/PageWeb/index.html", "/PageWeb/style.css", ... */
      struct mg_http_serve_opts opts = {
        .root_dir = "/PageWeb",
        .fs = &mg_fs_packed
      };
      mg_http_serve_dir(c, hm, &opts);
    }

  } else if (ev == MG_EV_WS_OPEN) {
    /* handshake terminado: se manda el estado actual de las variables de esa
     * pagina, para que el navegador no espere al siguiente cambio */
    static char json[WEBVARS_JSON_MAX];   /* static: solo se usa desde este hilo (no ocupa pila) */
    size_t n = WebVars_BuildData(WEBPAGE_GET(c), json, sizeof(json));
    if (n > 0) mg_ws_send(c, json, n, WEBSOCKET_OP_TEXT);
  } else if (ev == MG_EV_WS_MSG) {
    /* Mensaje del navegador: "S|id|valor" (escribir variable), "C|nombre|arg" (comando),
     * "T|texto" (terminal). WebVars_OnMessage valida y contesta si hace falta. */
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    char rep[160];
    size_t n = WebVars_OnMessage(WEBPAGE_GET(c), wm->data.buf, wm->data.len, rep, sizeof(rep));
    if (n > 0) mg_ws_send(c, rep, n, WEBSOCKET_OP_TEXT);
  } else if (ev == MG_EV_CLOSE) {
    /* conexion cerrada */
  }
}



// =============================================================================================================================
// HILO DE RED
// =============================================================================================================================
void ThreadFunc_Network(void *argument) {
  (void) argument;
  char buf[128];

  /* Activa la web empaquetada: mongoose.c busca los archivos en mg_mem_files.
   * Sin esta linea la tabla esta vacia y "/" responde Forbidden. */
  mg_mem_files = mg_packed_files;
  WebVars_Init();                /* crea el bloqueo del terminal */

  /* Network_Init abre el puerto y espera link_timeout_ms a tener IP.
   * Si solo falla por tiempo (NET_ERR_TIMEOUT), el servidor ya esta abierto
   * y la red puede aparecer despues al conectar el cable: se sigue adelante.
   * Con cualquier otro error (config, listen) el hilo termina. */
  NetStatus_t st = Network_Init();
  if (st != NET_OK && st != NET_ERR_TIMEOUT) {
    net_print("[NET] Fallo en el arranque, hilo de red detenido\r\n");
    osThreadExit();
  }

  int      last_state = -1;
  uint64_t t_envio    = 0;
  uint32_t ult_hash[16] = {0};    /* hash del ultimo JSON enviado a cada pagina */

  for (;;) {

    /* 1. Motor de Mongoose: atiende conexiones y llama a http_ev_handler */
    mg_mgr_poll(&get_mgr, 100);

    /* 2. Aviso por UART cuando cambia el estado de la interfaz */
    if ((int) get_mif.state != last_state) {
      last_state = (int) get_mif.state;

      if (get_mif.state == MG_TCPIP_STATE_READY) {
        uint8_t *ip = (uint8_t *) &get_mif.ip;
        snprintf(buf, sizeof(buf), "[NET] Red lista, IP %u.%u.%u.%u\r\n",
                 ip[0], ip[1], ip[2], ip[3]);
        net_print(buf);
      } else {
        net_print("[NET] Red no lista (sin cable o esperando IP)\r\n");
      }
    }

    /* 3. Terminal: las lineas de WebTerm_Print salen hacia las paginas que lo tienen */
    {
      char tl[160];
      size_t tn;
      while ((tn = WebTerm_PopJson(tl, sizeof(tl))) > 0) {
        for (struct mg_connection *c = get_mgr.conns; c != NULL; c = c->next) {
          if (c->is_websocket && (WEBPAGE_GET(c) & WebTerm_Pages())) {
            mg_ws_send(c, tl, tn, WEBSOCKET_OP_TEXT);
          }
        }
      }
    }

    /* 4. Cada segundo: solo se construye y se manda el JSON de las paginas que
     *    tienen algun navegador conectado, y solo si algun valor ha cambiado. */
    if (mg_millis() - t_envio >= 1000) {
      t_envio = mg_millis();
      WebBus_Tick1s();               /* contadores de tramas y caducidad del forzado */
      WebVars_DemoTick();            /* solo demo: BORRAR al usar tus variables */
      WebSig_Tick1s();               /* edad de las señales recibidas y mantenimiento del forzado */
      WebErr_Tick1s();               /* evalua los errores de web_err_cfg.h */
      WebSys_Update();               /* hilos, RAM y CPU del STM32 (pagina interno) */

      uint16_t activas = 0;          /* bit por pagina con algun navegador conectado */
      uint8_t  nws = 0;
      for (struct mg_connection *c = get_mgr.conns; c != NULL; c = c->next) {
        if (c->is_websocket && WEBPAGE_GET(c) != 0) { activas |= WEBPAGE_GET(c); nws++; }
      }
      sys_ws = nws;

      static char json[WEBVARS_JSON_MAX];
      for (int p = 0; p < 16; p++) {
        uint16_t page = (uint16_t) (1u << p);
        if (!(activas & page)) { ult_hash[p] = 0; continue; }  /* nadie mira: no se calcula nada */

        size_t n = WebVars_BuildData(page, json, sizeof(json));
        if (n == 0) continue;

        uint32_t h = 2166136261u;                              /* hash FNV-1a del JSON */
        for (size_t i = 0; i < n; i++) { h ^= (uint8_t) json[i]; h *= 16777619u; }
        if (h == ult_hash[p]) continue;                        /* nada ha cambiado */
        ult_hash[p] = h;

        for (struct mg_connection *c = get_mgr.conns; c != NULL; c = c->next) {
          if (c->is_websocket && WEBPAGE_GET(c) == page) {
            mg_ws_send(c, json, n, WEBSOCKET_OP_TEXT);
          }
        }
      }
    }
  }
}
