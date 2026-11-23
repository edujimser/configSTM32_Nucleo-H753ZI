# MANUAL: como crear una pagina, anadir variables y widgets

Hay 3 sitios donde se toca, siempre en este orden:

1. **Variables** (C): `web_vars.c` -> las registras con un ID.
2. **Pagina** (C): `web_vars.h` + `web_pages[]` -> le das un nombre y un bit.
3. **HTML**: `web/mipagina.html` -> usas el ID en atributos `data-...`.

Despues se vuelve a empaquetar la web (`packed_fs.c`) y se compila.

---

## 1. Registrar una variable

En `web_vars.c`, dentro de la tabla `web_vars[]`:

```c
// SOLO LECTURA:  WV(ID, paginas, tipo, decimales, variable)
WV ("temp_motor", PG_MIPAGINA,              WV_FLOAT, 1, mi_temperatura),

// ESCRIBIBLE:    WVW(ID, paginas, tipo, decimales, variable, minimo, maximo)
WVW("consigna",   PG_MIPAGINA | PG_DATOS,   WV_FLOAT, 1, mi_consigna, 10, 35),
```

| Campo | Significado |
|---|---|
| **ID** | nombre que usaras en el HTML. Solo letras, numeros y `_`. Corto = menos bytes por mensaje. |
| **paginas** | en que paginas se envia. Une varias con `\|`. La variable solo viaja a las paginas que la piden y que estan abiertas. |
| **tipo** | `WV_U8`, `WV_U16`, `WV_U32`, `WV_I32`, `WV_FLOAT`, `WV_BOOL`, `WV_STR` (texto, solo lectura; en `decimales` va el tamano del array). |
| **decimales** | solo para `WV_FLOAT`: cuantos decimales se muestran. |
| **variable** | tu variable global (el tipo C debe coincidir con `tipo`). No puede ser `static` dentro de una funcion. |
| **min / max** | solo en `WVW`: si la web manda un valor fuera de rango, la placa lo rechaza. |

Reglas:
- La variable tiene que ser **global** (o `static` de fichero) y estar declarada **antes** de la tabla (`extern` en el `.c` donde la tengas).
- Un `float` o entero de hasta 32 bits se lee sin problema desde el hilo de red. Una `struct` o un `uint64_t` **no**: copia antes el valor a una variable simple.
- Un sensor con fallo: escribe `NAN` en el `float` y la web mostrara `--`.
- Palabras de bits (entradas, salidas, alarmas): una `uint8_t`/`uint16_t` con un bit por señal; en el HTML se usan con `data-bits` o `data-bit="ID:n"`.
- Limite: cada pagina puede enviar como maximo **1024 bytes** de JSON (`WEBVARS_JSON_MAX`). Unas 40 variables por pagina es razonable. Si te pasas, la pagina deja de enviar: reparte las variables en dos paginas o sube el limite.

### Texto (WV_STR)
```c
char estado_txt[16];
WV ("estado_txt", PG_MIPAGINA, WV_STR, 16, estado_txt),   // 16 = tamano del array
```

---

## 2. Crear la pagina (parte C)

**a)** En `web_vars.h` anade un bit nuevo (maximo 16 paginas: bits 0 a 15):

```c
#define PG_MIPAGINA  (1u << 8)
```

**b)** En `web_vars.c`, en `web_pages[]`, anade el nombre (es el que va en la URL del WebSocket):

```c
{ "mipagina", PG_MIPAGINA },
```

**c)** Si quieres que la pagina tenga terminal (`data-terminal`), anade `PG_MIPAGINA` a `TERM_PAGES`.

**d)** En `network.c` no hay que tocar nada: la ruta `/ws/<nombre>` sale de `web_pages[]`.

---

## 3. Crear la pagina (parte HTML)

Copia cualquiera de las paginas de `web/` (por ejemplo `sistema.html`) a `web/mipagina.html`. El esqueleto minimo es:

```html
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Mi pagina</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body data-ws="/ws/mipagina">      <!-- mismo nombre que en web_pages[] -->

  <nav class="menu" id="menu"> ...copia el menu de otra pagina... </nav>

  <main class="cont">
    <h1>Mi pagina</h1>             <!-- los h1 se dibujan solos en puntos -->

    <div class="rejilla">
      <div class="tarjeta">
        <h2>Temperatura</h2>
        <canvas class="dial" data-dial="temp_motor" data-min="0" data-max="120"
                data-unit="C" data-zones="0~80:ok,80~100:warn,100~120:crit"></canvas>
      </div>
    </div>
  </main>

  <script src="JS/vars.js"></script>   <!-- siempre al final -->
</body>
</html>
```

Puntos clave:
- **`<body data-ws="/ws/mipagina">`** es lo que conecta la pagina con la placa. Sin esto no llega ningun dato.
- **El menu esta copiado en cada HTML.** Para que la pagina aparezca en el menu, anade una linea `<a href="/mipagina.html">Mi pagina</a>` en el `<nav>` de **todas** las paginas.
- **Un widget es cualquier etiqueta con un atributo `data-...` cuyo valor es el ID de la variable.** El diseno (tamanos, colores, posicion) es HTML y CSS normales: puedes poner un widget donde quieras.
- Para el CSS propio: anade tus clases al final de `web/style.css` o un `<style>` en la pagina. Los colores estan en variables: `--bg --fg --dim --line --acc (rojo) --ok --warn --frio`.

---

## 4. Catalogo de widgets (los atributos)

La pagina **widgets.html** tiene cada uno funcionando, con su codigo HTML y C listo para copiar. Resumen:

### Leer (placa -> web)
| Atributo | Que hace |
|---|---|
| `data-v="ID"` | texto con el valor. Extras: `data-fmt="hms"` (segundos a hh:mm:ss), `hex`, `bin`; `data-map="0:PARADO,1:MARCHA"` texto segun valor |
| `data-dot="ID"` | numero en pantalla de puntos (`data-digits`, `data-unit`, `data-color="red"`, `data-pitch`) |
| `data-bar="ID"` | pone `--p` (0 a 1) en el elemento para dibujar barras con CSS (`data-min`, `data-max`) |
| `data-gauge="ID"` / `data-dial="ID"` | reloj / medidor de aguja con zonas de color |
| `data-chart="A,B"` | grafica en el tiempo (`data-seconds`, `data-min`, `data-max`, `data-lo`, `data-hi`) |
| `data-barchart="A,B,C"` | barras verticales |
| `data-horizon="pitch,roll"` | horizonte artificial |
| `data-3d="A,B,C"` | pone `--rx --ry --rz` (grados) para objetos 3D en CSS |
| `data-table="A,B,C"` | tabla. `data-names`, `data-units`, `data-types="valor,led,barra,alarma"` (por fila), `data-limits`, `data-texts` |
| `data-led="ID"` | pone `data-on="1/0"` para dibujar un led con CSS |
| `data-state="ID"` | pone `data-s="valor"` para cambiar dibujos SVG con CSS (colores, animaciones) |
| `data-alarm="ID"` | pone `data-alarm="lo/ok/hi"` (`data-lo`, `data-hi`) |
| `data-steps="ID"` | pasos de maquina de estados (`data-names`, `data-fail`) |
| `data-bits="ID"` | un byte/word como fila de leds (`data-count`, `data-names`; clases `inv`, `alarma`, `compacto`) |
| `data-bit="ID:n"` | pone `data-on` segun el bit n de una palabra |
| `data-show-if="ID"` | muestra el elemento solo si la variable es distinta de 0 / vacia (pon `hidden` por defecto) |
| `data-alarmlog="id:lo:hi:nombre,..."` | registro de alarmas con hora |
| `data-img="ID"` | cambia el `src` de una imagen segun el valor |
| `data-pid` | faceplate PID completo (`data-pv`, `data-sp`, `data-out`, `data-mode`, `data-man`, `data-kp`, `data-ki`, `data-kd`) |

### Escribir (web -> placa)
La variable debe estar registrada con **WVW** (escribible).
| Atributo | Que hace |
|---|---|
| `data-set="ID"` | en `<input>`, `<select>`, `range`, `checkbox`: modifica la variable. En `<button data-set="ID" data-value="2">` pone ese valor |
| `data-cmd="nombre"` | boton que ejecuta un comando de la placa (`data-arg="texto"`, `data-confirm="TEXTO"` pide 2 pulsaciones) |
| `data-terminal` | terminal: escribes y recibes texto |
| `data-upload` | zona para subir archivos |
| `data-export="#tabla"` | boton que descarga una tabla como CSV |
| `data-estado`, `data-msg` | estado de la conexion / ultima respuesta de la placa |

### Widget propio
```js
VARS.widget('data-mio', function (el, d, todos) {
  // d = solo lo que ha cambiado, todos = todas las variables de la pagina
  var v = todos[el.getAttribute('data-mio')];
  el.textContent = 'Valor: ' + v;
});
```
```html
<span data-mio="temp_motor"></span>
```
Tambien existe `window.onVars = function (d) {...}` que recibe cada mensaje.

---

## 5. Dibujos propios con SVG (data-state / data-bar)

Es la forma de hacer esquemas (valvulas, circuitos, compresores) con diseno libre:

```html
<svg class="compresor" data-state="c1_st" data-bar="c1_hz" data-min="0" data-max="90"> ... </svg>
```
- `data-state="ID"` pone `data-s="0|1|2..."` en el elemento. En el CSS: `.compresor[data-s="2"] .anillo { stroke: var(--ok); }`
- `data-bar="ID"` pone `--p` (0 a 1). En el CSS: `animation-duration: calc(2s - var(--p) * 1.8s);`
- Dentro del SVG puedes poner valores vivos: `<text>HP <tspan data-v="c1_hp"></tspan> bar</text>`.
- Mira `style.css` (secciones HVAC e Industria) para ejemplos de todo esto.

---

## 6. Comandos (botones que ejecutan codigo en la placa)

En `web_vars.c`:
```c
static void cmd_arrancar(const char *arg) { (void) arg; marcha = 1; WebTerm_Print("arrancando"); }

static const WebCmd_t web_cmds[] = {
  ...
  { "arrancar", cmd_arrancar },
};
```
En el HTML:
```html
<button class="btn" data-cmd="arrancar">ARRANCAR</button>
<button class="btn rojo" data-cmd="emergencia" data-confirm="CONFIRMAR?">EMERGENCIA</button>
```
Los comandos se ejecutan en el **hilo de red**: tienen que ser cortos (nada de `osDelay` largos ni bucles bloqueantes). Si hace falta algo lento, el comando solo levanta una bandera y tu hilo hace el trabajo.

## 7. Terminal (UART / texto)

- Placa -> web: `WebTerm_Print("texto");` (desde cualquier hilo, **no** desde interrupciones). Cola de 8 lineas de 96 caracteres.
- Web -> placa: lo que se escribe llega a `WebTerm_OnLine(const char *line)` en `web_vars.c` (seccion 5). Ahi decides que hacer: reenviarlo a una UART, interpretarlo como comando, etc.

## 8. Archivos (subir / descargar)

- Subir: `<div data-upload data-name="firmware.bin" data-max="16384"></div>` -> llega a `WebFile_Upload` (maximo 16 KB, llega entero a RAM).
- Descargar: `<a href="/api/download?name=datos.csv">Descargar</a>` -> se genera en `WebFile_Download`.

---

## 9. Empaquetar la web y compilar

1. Pon el `.html` (y cualquier `.css`, `.js`, `.svg`) en la carpeta `web/`.
2. Abre **empaquetar_web.html** en el navegador, selecciona los ficheros de `web/` y descarga **packed_fs.c**.
3. Reemplaza `packed_fs.c` en el proyecto (ruta raiz `/PageWeb`) y compila.
4. Recuerda: el empaquetado cambia solo cuando regeneras `packed_fs.c`. Si cambias `web_vars.c` pero no regeneras la web, las paginas viejas siguen funcionando con los IDs que existan.

## 10. Receta completa: pagina "Motor" en 5 minutos

**C** (`web_vars.h`):
```c
#define PG_MOTOR (1u << 8)
```
**C** (`web_vars.c`):
```c
extern float temp_motor, rpm_motor;       // tus variables globales
extern uint8_t motor_on;
...
WV ("tm",  PG_MOTOR, WV_FLOAT, 1, temp_motor),
WV ("rpm", PG_MOTOR, WV_FLOAT, 0, rpm_motor),
WVW("on",  PG_MOTOR, WV_BOOL,  0, motor_on, 0, 1),
...
{ "motor", PG_MOTOR },                    // dentro de web_pages[]
```
**HTML** (`web/motor.html`, copia de `sistema.html` cambiando `data-ws="/ws/motor"`):
```html
<h1>Motor</h1>
<div class="rejilla">
  <div class="tarjeta"><h2>Temperatura</h2>
    <canvas class="dial" data-dial="tm" data-min="0" data-max="120" data-unit="C"
            data-zones="0~80:ok,80~100:warn,100~120:crit"></canvas></div>
  <div class="tarjeta"><h2>Velocidad</h2>
    <canvas class="dot" data-dot="rpm" data-digits="4" data-unit="RPM"></canvas>
    <canvas class="grafica" data-chart="rpm" data-seconds="60"></canvas></div>
  <div class="tarjeta"><h2>Marcha</h2>
    <input type="checkbox" data-set="on"></div>
</div>
```
Empaqueta, compila y abre `http://IP-DE-LA-PLACA/motor.html`.

---

## 11. Problemas tipicos

| Sintoma | Causa |
|---|---|
| La pagina carga pero no hay datos | falta `data-ws` en `<body>`, o el nombre no esta en `web_pages[]` (la placa responde 404 al WebSocket) |
| Un widget no se actualiza | el ID no existe en la tabla o no incluye el bit de esa pagina en `paginas` |
| "estado: desconectado" | el hilo de red no esta corriendo, IP incorrecta, o hay mas de las conexiones permitidas |
| No deja escribir una variable | esta registrada con `WV` en lugar de `WVW`, o el valor esta fuera de min/max (mira `data-msg` para el motivo) |
| Se dejan de enviar datos de una pagina | has superado 1024 bytes de JSON: reparte variables en otra pagina |
| 404 al abrir `/mipagina.html` | no regeneraste `packed_fs.c` despues de anadir el HTML |
| CPU del STM32 siempre 0 | faltan las estadisticas de FreeRTOS (ver LEEME.txt) |
| Valores `--` en pantalla | la variable vale `NAN` (sensor con fallo) |

## 12. Seguridad

La web no tiene autenticacion. Cualquiera en la red puede leer, escribir variables, ejecutar comandos y reiniciar el micro. No la expongas fuera de una red de confianza y no la uses como unica proteccion de seguridad de la maquina.


---

## 13. Widgets nuevos (paginas Objeto y Widgets de datos)

| Atributo | Que hace |
|---|---|
| `data-clock="rtc" data-mode="analog\|digital\|date"` | reloj desde segundos UNIX UTC; zona con `data-tz="minutos"` o `data-tz="ID"` (por defecto la variable `rtc_tz`) |
| `data-ring="ID"` | anillo de progreso con el valor en el centro |
| `data-v data-fmt="ip\|hex8\|size\|clock\|date\|datetime"` | formatos extra |
| `data-bytes="lo,hi,len"` | bytes de una trama en hexadecimal |
| `<input data-bytes-set="lo,hi,len">` | editor de bytes (escribe `DE AD BE EF`) |
| `<input data-set="ID" data-ip>` | campo de direccion IPv4 (la placa recibe un numero uint32) |
| `<input data-set="ID" data-hex>` | campo hexadecimal |
| `data-cond="A>B+1"` dentro de `data-condlist` | condicion en vivo (verde/rojo); operadores `> < >= <= == != & !&` |
| `data-history="ID" data-map="0:A,1:B"` | historial de cambios de una variable (vive en el navegador) |
| `data-explain` + `data-tip="x"` + `data-tipbox="x"` | zonas pulsables con explicacion (sirve para SVG o imagenes) |
| `data-tabs` + `data-tab` + `data-pane` | pestañas |
| `data-report` | boton que descarga un informe con todas las variables |
| `data-copy="#id"` | boton que copia un texto |
| `data-settime="rtc"` | boton que escribe la hora del PC en una variable |
| `.termo` `.pila` `.senal` `.brujula` + `data-bar` | termometro, bateria, señal, brujula (solo CSS) |

## 14. Visor y forzado de buses (CAN / TRDP)

Esta en `web_bus.c/.h`. Las variables de la web (`cr0i`, `cx0a`, `cb_st`...) apuntan a las estructuras `web_bus_rx/tx/st`, asi que **no hay que registrar nada**. Solo:

```c
// recepcion (tu driver CAN / pila TRDP)
WebBus_Rx(WEBBUS_CAN, id, len, datos);

// tu aplicacion lee SIEMPRE asi (respeta el forzado de recepcion):
uint8_t d[8], n;  if (WebBus_GetRx(WEBBUS_CAN, 0x100, d, &n)) { ... }

// transmision (tu hilo de envio)
if (WebBus_TxForced(WEBBUS_CAN)) {
    while (WebBus_TxPoll(WEBBUS_CAN, HAL_GetTick(), &id, &len, d)) enviar_can(id, len, d);
} else { /* tu transmision normal */ }
```
Para añadir mas filas cambia `WEBBUS_RX_N` / `WEBBUS_TX_N` y duplica las lineas correspondientes en la tabla de `web_vars.c` (o en `gen/spec.py` si usas el generador).


---

## 15. Errores (web_err_cfg.h)

Una linea por error en `web_err_cfg.h`; no se toca HTML ni la tabla de variables:

```c
#define WEBERR_NOW()  (rtc_epoch)          // hora UNIX para fechar los errores
#define WEBERR_LIST(X) \
  X( 101, AP1_C1, ERR_ALARMA, 0, (al_w & 0x0001u), "al_w c1_hp", "Alta presion circuito 1", c_hp[0]) \
  X( 108, FILTRO, ERR_AVISO,  5, (dp_filt > 250u), "dp_filt",    "Filtro de aire sucio",     dp_filt)
//    ID    NOMBRE  SEVERIDAD  RETARDO  CONDICION       VARIABLES      DESCRIPCION             VALOR (historico)
```
Severidades: `ERR_INFO`, `ERR_AVISO`, `ERR_ALARMA`, `ERR_CRITICO`. Retardo en segundos (0 = en el acto). Maximo 32 errores.
Para mostrar el valor en vivo de las variables de la columna VARIABLES, esas variables deben tener `PG_ERR` en su campo de paginas.

## 16. Señales de comunicacion (web_sig_cfg.h)

```c
#define WEBSIG_CAN_RX(X) \
  X( CAN_T_INT, 0x100u, 0, 16, 0, 0.1f, 0.0f, "C", "Temperatura interior", mi_temp = v)
#define WEBSIG_CAN_TX(X) \
  X( CAN_SP,    0x200u, 0, 16, 0, 0.1f, 0.0f, "C", "Consigna", consigna)
#define WEBSIG_CAN_FRAMES(X) \
  X( 0x200u, 8, 500 )                      // id, dlc, periodo ms
```
Numeracion de bits Intel; valor fisico = raw * factor + offset; maximo 16 señales por bus. Idem para TRDP (`WEBSIG_TRDP_*`).
En la web, pestaña SEÑALES: casilla FORZAR + valor forzado por señal; actua solo con el boton FORZAR SEÑALES del bus activo.
