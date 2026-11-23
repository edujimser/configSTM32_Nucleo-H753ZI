// =============================================================================
// modulo12_gpio.js  -  MODULO 12: visor de pines GPIO (GpioInfo_t de modulo12_gpio.h)
//
//  Uso en la pagina (ANTES de vars.js):
//      <script src="JS/modulo12_gpio.js"></script>
//      <script src="JS/modulo12_gpio.js" data-puertos="8"></script>  <- = GPIOINFO_PORTS (8 por defecto)
//
//  Variables que tiene que enviar la placa (ver modulo12_vars.txt): gp_on, ga_en, ga_i, ga_o, ...
//  Contiene: 1) su CSS   2) su HTML   3) widgets propios (mapa, detalle, resumen)
// =============================================================================
(function () {
  'use strict';

  var NPUERTOS = parseInt(document.currentScript.getAttribute('data-puertos'), 10) || 8;

  // ---------------------------------------------------------------------------
  // Nombres de pines (los "User Label" de CubeMX, de main.h). Anade los tuyos:
  //   'PB0': 'STATUS_LED_VERDE'
  // ---------------------------------------------------------------------------
  var NOMBRES = {
    'PB0':  'STATUS_LED_VERDE',
    'PB14': 'STATUS_LED_ROJO',
    'PE1':  'STATUS_LED_AMARILLO',
    'PD8':  'STLINK_RX',
    'PD9':  'STLINK_TX',
    'PD14': 'SPI1_CS',
    'PF11': 'SDMMC1_CD'
  };

  // ---------------------------------------------------------------------------
  // 1) CSS propio del modulo (una sola vez)
  // ---------------------------------------------------------------------------
  if (!document.getElementById('css-mod12')) {
    document.head.insertAdjacentHTML('beforeend', /*html*/`
      <style id="css-mod12">
        .gp-scroll  { overflow-x: auto; margin-top: 12px; }
        .gp-mapa    { display: grid; grid-template-columns: 70px repeat(16, minmax(30px, 1fr)); gap: 5px; min-width: 640px; }
        .gp-cab     { font-size: .55rem; letter-spacing: 1px; color: var(--dim); text-align: center; }
        .gp-port    { font-size: .68rem; letter-spacing: 2px; display: flex; flex-direction: column; justify-content: center; }
        .gp-port i  { font-style: normal; font-size: .5rem; letter-spacing: 1px; color: var(--dim); }
        .gp-pin     { position: relative; height: 34px; border: 1px solid #333; border-radius: 6px; cursor: pointer;
                      display: flex; align-items: center; justify-content: center; font-size: .5rem; letter-spacing: .5px;
                      color: var(--dim); background: #0a0a0a; transition: background .25s, border-color .25s; }
        .gp-pin:hover { border-color: var(--fg); }
        .gp-pin.sel { outline: 2px solid var(--acc); outline-offset: 1px; }
        .gp-pin.m0  { border-radius: 50%; width: 30px; height: 30px; margin: 2px auto; }               /* entrada   */
        .gp-pin.m1  { border-color: #888; border-width: 2px; }                                        /* salida    */
        .gp-pin.m2  { border-style: dashed; border-color: #5b5bff; color: #8f8fff; }                  /* AF        */
        .gp-pin.m3  { opacity: .28; }                                                                 /* analogico */
        .gp-pin.alto            { background: rgba(47,211,107,.18); color: var(--fg); }
        .gp-pin.alto.m0, .gp-pin.alto.m1 { background: var(--ok, #2fd36b); border-color: var(--ok, #2fd36b); color: #000; }
        .gp-pin.nom::after { content: ""; position: absolute; top: 3px; right: 3px; width: 5px; height: 5px; border-radius: 50%; background: var(--acc); }
        .gp-fila-off { opacity: .25; }
        .gp-ley     { display: flex; flex-wrap: wrap; gap: 8px 18px; margin-top: 14px; font-size: .58rem; letter-spacing: 1.5px;
                      color: var(--dim); text-transform: uppercase; align-items: center; }
        .gp-ley span { display: inline-flex; align-items: center; gap: 7px; }
        .gp-ley b   { display: inline-block; width: 14px; height: 14px; border: 1px solid #333; border-radius: 4px; background: #0a0a0a; }
        .gp-kv      { display: grid; grid-template-columns: 1fr auto; gap: 7px 14px; font-size: .78rem; margin-top: 12px; }
        .gp-kv dt   { color: var(--dim); letter-spacing: 1.5px; text-transform: uppercase; font-size: .62rem; align-self: center; }
        .gp-kv dd   { margin: 0; text-align: right; font-variant-numeric: tabular-nums; }
        .gp-big     { font-size: 1.6rem; letter-spacing: 3px; margin-top: 8px; }
        .gp-big small { display: block; font-size: .62rem; letter-spacing: 2px; color: var(--acc); margin-top: 4px; }
        .gp-nivel   { display: inline-block; padding: 2px 10px; border-radius: 999px; border: 1px solid #444; font-size: .66rem; letter-spacing: 1.5px; }
        .gp-nivel.on { color: #000; background: var(--ok, #2fd36b); border-color: var(--ok, #2fd36b); }
        .gp-lista   { display: grid; gap: 8px; margin-top: 12px; }
        .gp-lista div { display: grid; grid-template-columns: 14px 54px 1fr auto; gap: 10px; align-items: center; font-size: .72rem; cursor: pointer; }
        .gp-lista .led { width: 10px; height: 10px; }
        .gp-lista span:nth-child(2) { color: var(--dim); }
      </style>`);
  }

  // ---------------------------------------------------------------------------
  // 2) HTML: 3 tarjetas
  // ---------------------------------------------------------------------------
  var html = /*html*/`
    <div class="rejilla">

      <!-- ===== 1. MAPA DE PINES ===== -->
      <div class="tarjeta ancha">
        <h2>Pines GPIO en vivo</h2>
        <span class="etq">gpio_info</span><span class="etq">IDR</span><span class="etq">MODER</span>
        <div class="gp-scroll"><div class="gp-mapa" data-gpmapa></div></div>
        <div class="gp-ley">
          <span><b style="border-radius:50%"></b>entrada</span>
          <span><b style="border:2px solid #888"></b>salida</span>
          <span><b style="border:1px dashed #5b5bff"></b>funcion alternativa (AF)</span>
          <span><b style="opacity:.3"></b>analogico / sin uso</span>
          <span><b style="background:var(--ok,#2fd36b);border-color:var(--ok,#2fd36b)"></b>nivel alto</span>
          <span><b style="position:relative"><i style="position:absolute;top:2px;right:2px;width:4px;height:4px;border-radius:50%;background:var(--acc)"></i></b>con nombre</span>
        </div>
      </div>

      <!-- ===== 2. DETALLE DEL PIN ===== -->
      <div class="tarjeta">
        <h2>Detalle del pin</h2>
        <span class="etq">pulsa un pin del mapa</span>
        <div data-gpdet></div>
      </div>

      <!-- ===== 3. RESUMEN Y PINES CON NOMBRE ===== -->
      <div class="tarjeta">
        <h2>Resumen</h2>
        <span class="etq">puertos</span><span class="etq">pines con nombre</span>
        <div data-gpres></div>
      </div>

    </div>
  `;

  document.currentScript.insertAdjacentHTML('beforebegin', html);

  // ---------------------------------------------------------------------------
  // 3) Widgets propios
  // ---------------------------------------------------------------------------
  var LETRAS = 'ABCDEFGHIJK';
  var MODOS  = ['Entrada', 'Salida', 'Funcion alternativa', 'Analogico'];
  var PULLS  = ['Ninguna', 'Pull-up', 'Pull-down', '?'];
  var sel = 'PB0';                                         // pin seleccionado

  function esc(s) { return String(s).replace(/[&<>"]/g, function (c) { return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]; }); }

  // datos de un pin a partir de los registros del puerto
  function pin(t, port, n) {
    var k = 'g' + port.toLowerCase() + '_';
    var moder = t[k + 'm'] >>> 0, pupd = t[k + 'p'] >>> 0, af = n < 8 ? t[k + 'al'] >>> 0 : t[k + 'ah'] >>> 0;
    return {
      nombre: 'P' + port + n,
      en:   !!t[k + 'en'],
      modo: (moder >>> (n * 2)) & 3,
      alto: ((t[k + 'i'] | 0) >> n) & 1,
      out:  ((t[k + 'o'] | 0) >> n) & 1,
      od:   ((t[k + 't'] | 0) >> n) & 1,
      pull: (pupd >>> (n * 2)) & 3,
      af:   (af >>> ((n % 8) * 4)) & 15
    };
  }

  function detalle(el, t) {
    var port = sel.charAt(1), n = parseInt(sel.slice(2), 10), p = pin(t, port, n);
    if (!p.en) { el.innerHTML = '<div class="gp-big">' + sel + '</div><p class="dim">El puerto GPIO' + port + ' no tiene el reloj activado: no se usa.</p>'; return; }
    el.innerHTML =
      '<div class="gp-big">' + sel + (NOMBRES[sel] ? '<small>' + esc(NOMBRES[sel]) + '</small>' : '') + '</div>' +
      '<dl class="gp-kv">' +
        '<dt>Modo</dt><dd>' + MODOS[p.modo] + (p.modo === 2 ? ' &middot; AF' + p.af : '') + '</dd>' +
        '<dt>Nivel leido (IDR)</dt><dd><span class="gp-nivel' + (p.alto ? ' on' : '') + '">' + (p.alto ? 'ALTO 1' : 'BAJO 0') + '</span></dd>' +
        (p.modo === 1 ? '<dt>Salida programada (ODR)</dt><dd>' + p.out + (p.out !== p.alto ? ' <span style="color:var(--acc)">(no coincide con el pin)</span>' : '') + '</dd>' : '') +
        (p.modo === 1 || p.modo === 2 ? '<dt>Tipo de salida</dt><dd>' + (p.od ? 'Open-drain' : 'Push-pull') + '</dd>' : '') +
        '<dt>Resistencia</dt><dd>' + PULLS[p.pull] + '</dd>' +
      '</dl>';
  }

  function registrar() {
    if (!window.VARS || VARS._mod12) return;
    VARS._mod12 = true;

    // mapa: fila por puerto, columna por pin
    VARS.widget('data-gpmapa', function (el, d, t) {
      if (t.ga_en === undefined) return;
      var h = '<div></div>';
      for (var c = 0; c < 16; c++) h += '<div class="gp-cab">' + c + '</div>';
      for (var i = 0; i < NPUERTOS; i++) {
        var L = LETRAS.charAt(i), en = !!t['g' + L.toLowerCase() + '_en'];
        h += '<div class="gp-port' + (en ? '' : ' gp-fila-off') + '">GPIO' + L + '<i>' + (en ? 'activo' : 'sin reloj') + '</i></div>';
        for (var n = 0; n < 16; n++) {
          var p = pin(t, L, n), nom = NOMBRES[p.nombre];
          var cls = 'gp-pin' + (en ? ' m' + p.modo : ' m3') + (en && p.alto ? ' alto' : '') + (nom ? ' nom' : '') + (p.nombre === sel ? ' sel' : '');
          var txt = !en ? '' : p.modo === 2 ? 'AF' + p.af : p.modo === 3 ? '~' : '';
          var tip = p.nombre + (nom ? ' · ' + nom : '') + (en ? ' · ' + MODOS[p.modo] + ' · ' + (p.alto ? 'alto' : 'bajo') : ' · sin reloj');
          h += '<div class="' + cls + '" data-gppin="' + p.nombre + '" title="' + esc(tip) + '">' + txt + '</div>';
        }
      }
      el.innerHTML = h;
    });

    // detalle del pin seleccionado
    VARS.widget('data-gpdet', function (el, d, t) { if (t.ga_en !== undefined) detalle(el, t); });

    // resumen + pines con nombre
    VARS.widget('data-gpres', function (el, d, t) {
      if (t.ga_en === undefined) return;
      var cuenta = [0, 0, 0, 0], altos = 0;
      for (var i = 0; i < NPUERTOS; i++) {
        var L = LETRAS.charAt(i); if (!t['g' + L.toLowerCase() + '_en']) continue;
        for (var n = 0; n < 16; n++) { var p = pin(t, L, n); cuenta[p.modo]++; if (p.modo === 1 && p.alto) altos++; }
      }
      var lista = Object.keys(NOMBRES).map(function (k) {
        var p = pin(t, k.charAt(1), parseInt(k.slice(2), 10));
        return '<div data-gppin="' + k + '"><span class="led" data-on="' + (p.en && p.alto ? 1 : 0) + '"></span><span>' + k + '</span>' +
               '<span>' + esc(NOMBRES[k]) + '</span><span class="dim" style="margin:0">' + (p.en ? MODOS[p.modo].split(' ')[0] : '-') + '</span></div>';
      }).join('');
      el.innerHTML =
        '<dl class="gp-kv">' +
          '<dt>Puertos activos</dt><dd>' + (t.gp_on | 0) + ' de ' + NPUERTOS + '</dd>' +
          '<dt>Salidas</dt><dd>' + cuenta[1] + ' (' + altos + ' en alto)</dd>' +
          '<dt>Entradas</dt><dd>' + cuenta[0] + '</dd>' +
          '<dt>Funcion alternativa</dt><dd>' + cuenta[2] + '</dd>' +
          '<dt>Analogico / sin uso</dt><dd>' + cuenta[3] + '</dd>' +
        '</dl>' +
        '<div class="gp-lista">' + lista + '</div>';
    });

    // pulsar un pin (en el mapa o en la lista) lo selecciona
    document.addEventListener('click', function (e) {
      var x = e.target.closest && e.target.closest('[data-gppin]'); if (!x) return;
      sel = x.getAttribute('data-gppin');
      document.querySelectorAll('.gp-pin.sel').forEach(function (p) { p.classList.remove('sel'); });
      document.querySelectorAll('.gp-pin[data-gppin="' + sel + '"]').forEach(function (p) { p.classList.add('sel'); });
      document.querySelectorAll('[data-gpdet]').forEach(function (el) { detalle(el, VARS.valores); });
    });
  }

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', registrar);
  else registrar();
})();
