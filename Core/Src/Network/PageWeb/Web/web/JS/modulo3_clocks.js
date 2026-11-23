// =============================================================================
// modulo6_clocks.js  -  MODULO 6: relojes y temperatura (ClockInfo_t de modulo6_clocks.h)
//
//  Uso en la pagina (ANTES de vars.js):
//      <script src="JS/modulo6_clocks.js"></script>
//
//  Variables que tiene que enviar la placa (ver modulo6_vars.txt): ck_...
//  Contiene: 1) su CSS   2) su HTML   3) widgets propios (MHz, arbol de relojes, estado)
// =============================================================================
(function () {
  'use strict';

  // ---------------------------------------------------------------------------
  // 1) CSS propio del modulo (una sola vez)
  // ---------------------------------------------------------------------------
  if (!document.getElementById('css-mod6')) {
    document.head.insertAdjacentHTML('beforeend', /*html*/`
      <style id="css-mod6">
        .ck-kv       { display: grid; grid-template-columns: 1fr auto; gap: 7px 14px; font-size: .78rem; margin-top: 12px; }
        .ck-kv dt    { color: var(--dim); letter-spacing: 1.5px; text-transform: uppercase; font-size: .62rem; align-self: center; }
        .ck-kv dd    { margin: 0; text-align: right; font-variant-numeric: tabular-nums; }
        .ck-kv dd.r  { color: var(--acc); }
        .ck-dos      { display: grid; grid-template-columns: minmax(200px, 1fr) 1.2fr; gap: 10px 26px; align-items: center; }
        @media (max-width: 700px) { .ck-dos { grid-template-columns: 1fr; } }
        .ck-nota     { margin-top: 10px; font-size: .7rem; line-height: 1.6; color: var(--warn, #ffb000); }

        /* arbol de relojes: 3 columnas */
        .ck-arbol    { display: grid; grid-template-columns: 1fr 1.25fr 1fr; gap: 18px; margin-top: 14px; align-items: start; }
        @media (max-width: 900px) { .ck-arbol { grid-template-columns: 1fr; } }
        .ck-col h4   { margin: 0 0 10px; font-weight: normal; font-size: .58rem; letter-spacing: 2.5px; color: var(--dim); text-transform: uppercase; }
        .ck-caja     { position: relative; border: 1px solid var(--line); border-radius: 10px; padding: 9px 12px; margin-bottom: 8px;
                       background: #0a0a0a; font-size: .74rem; transition: border-color .3s, opacity .3s; }
        .ck-caja.off { opacity: .35; }
        .ck-caja.act { border-color: var(--acc); box-shadow: 0 0 0 1px rgba(215,25,33,.35), 0 0 18px rgba(215,25,33,.18); }
        .ck-caja .t  { display: flex; justify-content: space-between; align-items: center; letter-spacing: 1.5px; }
        .ck-caja .f  { color: var(--fg); font-variant-numeric: tabular-nums; }
        .ck-caja .s  { display: grid; grid-template-columns: auto 1fr; gap: 3px 10px; margin-top: 6px; font-size: .66rem; color: var(--dim); }
        .ck-caja .s b{ font-weight: normal; color: var(--fg); text-align: right; font-variant-numeric: tabular-nums; }
        .ck-led      { display: inline-block; width: 8px; height: 8px; border-radius: 50%; background: #333; margin-right: 8px; }
        .ck-led.on   { background: var(--ok, #2fd36b); box-shadow: 0 0 8px var(--ok, #2fd36b); }
        .ck-flecha   { position: absolute; right: -16px; top: 50%; transform: translateY(-50%); color: var(--acc); font-size: .9rem; }
        @media (max-width: 900px) { .ck-flecha { display: none; } }
        .ck-sys      { border-color: #555; }
        .ck-bus      { padding-left: 22px; }
        .ck-bus::before { content: ""; position: absolute; left: 10px; top: -9px; bottom: 50%; border-left: 1px dotted #444; border-bottom: 1px dotted #444; width: 7px; }
      </style>`);
  }

  // ---------------------------------------------------------------------------
  // 2) HTML: 3 tarjetas
  // ---------------------------------------------------------------------------
  var html = /*html*/`
    <div class="rejilla">

      <!-- ===== 1. TEMPERATURA ===== -->
      <div class="tarjeta">
        <h2>Temperatura del chip</h2>
        <span class="etq">clock_info</span><span class="etq">ADC3</span>
        <canvas class="dial" data-dial="ck_t" data-min="0" data-max="125" data-unit="&deg;C"
                data-zones="0~85:ok,85~105:warn,105~125:crit"></canvas>
        <dl class="ck-kv">
          <dt>Minima / maxima</dt><dd><span data-v="ck_tmin">--</span> / <span data-v="ck_tmax">--</span> &deg;C</dd>
          <dt>Vref+ (analogico)</dt><dd data-ckv="ck_vref">--</dd>
          <dt>Pila RTC (VBAT)</dt> <dd data-ckv="ck_vbat">--</dd>
        </dl>
        <div class="ck-nota" data-cknota></div>
      </div>

      <!-- ===== 2. CONFIGURACION ===== -->
      <div class="tarjeta">
        <h2>Configuracion de reloj</h2>
        <span class="etq">RCC</span><span class="etq">PWR</span><span class="etq">FLASH</span>
        <dl class="ck-kv">
          <dt>CPU</dt>                 <dd data-mhz="ck_cpu" class="r">--</dd>
          <dt>Origen del sistema</dt>  <dd data-cksrc>--</dd>
          <dt>Cristal HSE</dt>         <dd data-mhz="ck_hse">--</dd>
          <dt>Oscilador HSI</dt>       <dd data-mhz="ck_hsi">--</dd>
          <dt>Escala de tension</dt>   <dd data-ckvos>--</dd>
          <dt>Esperas de la flash</dt> <dd><span data-v="ck_ws">--</span> WS</dd>
          <dt>Chip</dt>                <dd data-ckchip>--</dd>
        </dl>
      </div>

      <!-- ===== 3. ARBOL DE RELOJES ===== -->
      <div class="tarjeta ancha">
        <h2>Arbol de relojes</h2>
        <span class="etq">frecuencias reales</span>
        <div data-ckarbol></div>
        <p class="dim">En rojo, el camino que alimenta al sistema. Apagado = oscilador o PLL parado.</p>
      </div>

    </div>
  `;

  document.currentScript.insertAdjacentHTML('beforebegin', html);

  // ---------------------------------------------------------------------------
  // 3) Widgets propios
  // ---------------------------------------------------------------------------
  var OSC = { HSI: 1, CSI: 2, HSE: 4, HSI48: 8, LSI: 16, LSE: 32, PLL1: 64, PLL2: 128, PLL3: 256 };
  var SRC = ['HSI', 'CSI', 'HSE', 'PLL1'];

  function mhz(v) {
    if (!v) return '--';
    if (v < 1e6) return (v / 1e3).toFixed(v % 1e3 ? 3 : 0) + ' kHz';
    var m = v / 1e6;
    return (m % 1 ? m.toFixed(2) : m.toFixed(0)) + ' MHz';
  }

  function registrar() {
    if (!window.VARS || VARS._mod6) return;
    VARS._mod6 = true;

    VARS.widget('data-mhz', function (el, d, t) {
      var v = t[el.getAttribute('data-mhz')]; if (v !== undefined) el.textContent = mhz(v);
    });
    VARS.widget('data-ckv', function (el, d, t) {            // mV -> "3.29 V"
      var v = t[el.getAttribute('data-ckv')];
      el.textContent = (t.ck_adc && v) ? (v / 1000).toFixed(2) + ' V' : '--';
    });
    VARS.widget('data-cksrc', function (el, d, t) {
      if (t.ck_src !== undefined) el.textContent = SRC[t.ck_src] || '?';
    });
    VARS.widget('data-ckvos', function (el, d, t) {
      if (t.ck_vos === undefined) return;
      el.textContent = 'VOS' + t.ck_vos + (t.ck_vos === 0 ? ' (maxima velocidad)' : t.ck_vos === 3 ? ' (bajo consumo)' : '');
    });
    VARS.widget('data-ckchip', function (el, d, t) {
      if (t.ck_dev === undefined) return;
      el.textContent = (t.ck_dev === 0x450 ? 'STM32H74x/75x' : '0x' + t.ck_dev.toString(16)) + ' rev ' + (t.ck_revt || '?');
    });
    VARS.widget('data-cknota', function (el, d, t) {
      el.textContent = t.ck_adc ? '' :
        'Temperatura sin medir: activa el ADC3 en CubeMX con los canales Temperature Sensor, Vrefint y Vbat (ver instrucciones).';
    });

    // arbol de relojes
    VARS.widget('data-ckarbol', function (el, d, t) {
      if (t.ck_sys === undefined) return;
      var on = t.ck_osc | 0, src = SRC[t.ck_src];
      var pllsrc = ['HSI', 'CSI', 'HSE', ''][t.ck_psrc | 0];   // entrada de los PLL

      function caja(nombre, f, encendido, activo, extra, flecha) {
        return '<div class="ck-caja' + (encendido ? '' : ' off') + (activo ? ' act' : '') + '">' +
          '<div class="t"><span><i class="ck-led' + (encendido ? ' on' : '') + '"></i>' + nombre + '</span>' +
          '<span class="f">' + f + '</span></div>' + (extra || '') +
          (flecha ? '<span class="ck-flecha">&rarr;</span>' : '') + '</div>';
      }
      function pll(n) {
        var k = 'ck_p' + n, ok = on & OSC['PLL' + n];
        var s = '<div class="s"><span>P</span><b>' + mhz(t[k + 'p']) + '</b><span>Q</span><b>' + mhz(t[k + 'q']) +
                '</b><span>R</span><b>' + mhz(t[k + 'r']) + '</b></div>';
        return caja('PLL' + n, '', ok, n === 1 && src === 'PLL1', s, n === 1 && src === 'PLL1');
      }
      function bus(nombre, v) {
        return '<div class="ck-caja ck-bus"><div class="t"><span>' + nombre + '</span><span class="f">' + mhz(v) + '</span></div></div>';
      }
      var actOsc = function (o) { return src === o || (src === 'PLL1' && o === pllsrc); };

      el.innerHTML =
        '<div class="ck-arbol">' +
          '<div class="ck-col"><h4>Osciladores</h4>' +
            caja('HSE',   mhz(t.ck_hse), on & OSC.HSE,   actOsc('HSE'), '', actOsc('HSE')) +
            caja('HSI',   mhz(t.ck_hsi), on & OSC.HSI,   actOsc('HSI'), '', actOsc('HSI')) +
            caja('CSI',   '4 MHz',       on & OSC.CSI,   actOsc('CSI'), '', actOsc('CSI')) +
            caja('HSI48', '48 MHz',      on & OSC.HSI48, false) +
            caja('LSE',   '32.768 kHz',  on & OSC.LSE,   false) +
            caja('LSI',   '32 kHz',      on & OSC.LSI,   false) +
          '</div>' +
          '<div class="ck-col"><h4>PLL</h4>' + pll(1) + pll(2) + pll(3) + '</div>' +
          '<div class="ck-col"><h4>Sistema</h4>' +
            '<div class="ck-caja ck-sys act"><div class="t"><span>SYSCLK &middot; ' + (src || '?') + '</span><span class="f">' + mhz(t.ck_sys) + '</span></div></div>' +
            bus('CPU (M7)', t.ck_cpu) + bus('AHB', t.ck_hclk) + bus('APB1', t.ck_apb1) +
            bus('APB2', t.ck_apb2) + bus('APB3', t.ck_apb3) + bus('APB4', t.ck_apb4) +
          '</div>' +
        '</div>';
    });
  }

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', registrar);
  else registrar();
})();
