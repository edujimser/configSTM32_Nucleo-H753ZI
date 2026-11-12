/* =============================================================================
 * web_vars.h  -  Puente entre las variables de la placa y la web
 *
 *  PLACA -> WEB   variables (tabla web_vars[]), terminal (WebTerm_Print),
 *                 descargas (WebFile_Download)
 *  WEB -> PLACA   variables marcadas como escribibles (WVW), comandos (web_cmds[]),
 *                 terminal (WebTerm_OnLine), subida de archivos (WebFile_Upload)
 *
 * Todo lo que se edita esta en web_vars.c (secciones 1 a 6).
 * ============================================================================= */
#ifndef WEB_VARS_H
#define WEB_VARS_H

#include <stddef.h>
#include <stdint.h>

/* Paginas (maximo 16: son bits de un uint16_t). Cada una es un bit, asi una variable puede estar en varias:
 *   WV("temp", PG_DATOS | PG_SISTEMA, ...)
 * Pagina nueva: anade su PG_xxx aqui y una linea en web_pages[] (web_vars.c).
 * Su WebSocket sera /ws/<nombre>. */
#define PG_DATOS    (1u << 0)
#define PG_CONFIG   (1u << 1)
#define PG_SISTEMA  (1u << 2)
#define PG_WIDGETS  (1u << 3)
#define PG_INDUSTRIA (1u << 4)
#define PG_INTERNO  (1u << 5)      /* estado interno del STM32: hilos, RAM, CPU */
#define PG_HVAC     (1u << 6)      /* climatizacion: circuito frigorifico */
#define PG_COMPON   (1u << 7)      /* climatizacion: componentes, mando manual, electrico */
#define PG_OBJETO   (1u << 8)      /* identificacion, versiones, red, ayuda HVAC, soporte */
#define PG_WDATOS   (1u << 9)      /* catalogo de widgets de datos */
#define PG_CAN      (1u << 10)     /* visor y forzado de tramas CAN */
#define PG_TRDP     (1u << 11)     /* visor y forzado de datos TRDP */
#define PG_ERR      (1u << 12)     /* errores en vivo, historico y contadores */

typedef enum {
  WV_U8,      /* uint8_t  */
  WV_U16,     /* uint16_t */
  WV_U32,     /* uint32_t */
  WV_I32,     /* int32_t  */
  WV_FLOAT,   /* float    */
  WV_BOOL,    /* uint8_t: 0 = false, otro = true */
  WV_STR      /* char[N]: texto; en "decimales" va el tamano maximo del array (solo lectura) */
} WebVarType_t;

typedef struct {
  const char          *key;       /* ID en el JSON y en el HTML (data-v="key") */
  uint16_t             pages;     /* paginas donde se envia (PG_xxx unidos con |) */
  WebVarType_t         type;
  uint8_t              decimals;  /* solo para WV_FLOAT (0 a 6) */
  const volatile void *ptr;       /* direccion de la variable */
  uint8_t              rw;        /* 1 = la web puede modificarla */
  double               min, max;  /* limites al escribir (si min >= max no se limita) */
} WebVar_t;

typedef struct {
  const char *name;                       /* nombre del comando (data-cmd="name") */
  void (*fn)(const char *arg);            /* se ejecuta en el hilo de red: debe ser CORTA */
} WebCmd_t;

/* ---- Paginas y variables ---- */
void    WebVars_Init(void);                               /* llamar una vez al arrancar el hilo de red */
uint16_t WebVars_PageFromUri(const char *uri, size_t len); /* "/ws/datos" -> PG_DATOS, o 0 */
/* Aplica la configuracion de red escrita desde la web (la define tu aplicacion; hay una version vacia "weak") */
void    WebNet_Apply(void);
/* Aplica la configuracion de red escrita desde la web (la define tu aplicacion; hay una version vacia "weak") */
void    WebNet_Apply(void);
size_t  WebVars_BuildData(uint16_t page, char *buf, size_t size);   /* {"clave":valor,...}; 0 si no cabe */

/* Mensaje recibido de la web (formato "S|id|valor", "C|nombre|arg", "T|texto").
 * Escribe la respuesta JSON en reply y devuelve su longitud (0 = sin respuesta). */
size_t  WebVars_OnMessage(uint16_t page, const char *msg, size_t len, char *reply, size_t rsize);

/* ---- Terminal ---- */
void    WebTerm_Print(const char *line);                  /* placa -> web (seguro desde cualquier hilo, NO desde interrupciones) */
size_t  WebTerm_PopJson(char *out, size_t size);          /* lo usa network.c: {"_t":"linea"}; 0 si no hay */
uint16_t WebTerm_Pages(void);                              /* paginas que reciben el terminal */

/* ---- Archivos ---- */
int     WebFile_Upload(const char *name, const uint8_t *data, size_t len, char *reply, size_t rsize);   /* 0 = ok */
int     WebFile_Download(const char *name, const uint8_t **data, size_t *len, const char **mime);       /* 0 = ok */
#define WEBFILE_MAX_UPLOAD  16384u                        /* bytes maximos por subida */

/* Tamano maximo del JSON de una pagina (buffers de network.c). Una pagina con muchas
 * variables puede necesitar mas: se ve en la consola de pruebas o sube este valor. */
#define WEBVARS_JSON_MAX  4096u

/* Acceso al bit de pagina guardado en cada conexion WebSocket (2 bytes en c->data) */
#define WEBPAGE_SET(c, p)  do { (c)->data[0] = (char) ((p) & 0xFF); (c)->data[1] = (char) (((p) >> 8) & 0xFF); } while (0)
#define WEBPAGE_GET(c)     ((uint16_t) (((uint8_t) (c)->data[0]) | (((uint16_t) (uint8_t) (c)->data[1]) << 8)))

#endif
