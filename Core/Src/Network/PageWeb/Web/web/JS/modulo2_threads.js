// =============================================================================
// mod_hilos.js  -  MODULO 2: hilos de FreeRTOS (estructura RtosInfo_t de Modulo2_threads.h)
//
//  Uso en la pagina (dentro de <div class="rejilla">, ANTES de vars.js):
//      <script src="/mod_hilos.js"></script>
//      <script src="/mod_hilos.js" data-max="10"></script>   <- = RTOSINFO_MAX_THREADS (10 por defecto)
//
//  Variables que tiene que enviar la placa (tabla web en C, ver mod_hilos_vars.txt):
//      resumen:  hl_n hl_vis hl_ok hl_cpu hl_up hl_stot hl_suso hl_spct hl_tcb hl_mem hl_peor hl_peorp
//      por hilo: h0_n h0_s h0_p h0_c h0_sz h0_lib h0_pct ... (h1_, h2_, ...)
//
//  Contiene: 1) su CSS   2) su HTML   3) widgets propios (tabla, reparto de CPU, pila por hilo)
// =============================================================================
(function () {
  'use strict';

  var MAX = parseInt(document.currentScript.getAttribute('data-max'), 10) || 10;

  // ---------------------------------------------------------------------------
  // 1) CSS propio del modulo (una sola vez)
  // ---------------------------------------------------------------------------
  if (!document.getElementById('css-mod-hilos')) {
    document.head.insertAdjacentHTML('beforeend', /*html*/`
      <style id="css-mod-hilos">
        .hl-kv       { display: grid; grid-template-columns: 1fr auto; gap: 7px 14px; font-size: .78rem; margin-top: 12px; }
        .hl-kv dt    { color: var(--dim); letter-spacing: 1.5px; text-transform: uppercase; font-size: .62rem; align-self: center; }
        .hl-kv dd    { margin: 0; text-align: right; font-variant-numeric: tabular-nums; }
        .hl-kv dd.r  { color: var(--acc); }
        .hl-dos      { display: grid; grid-template-columns: minmax(200px, 1fr) 1.3fr; gap: 10px 26px; align-items: center; }
        @media (max-width: 700px) { .hl-dos { grid-template-columns: 1fr; } }
        .hl-nota     { margin-top: 10px; font-size: .7rem; letter-spacing: .5px; color: var(--warn, #ffb000); line-height: 1.5; }

        /* tabla de hilos */
        .hl-tab      { width: 100%; border-collapse: collapse; font-size: .74rem; margin-top: 8px; }
        .hl-tab th   { text-align: left; font-weight: normal; font-size: .58rem; letter-spacing: 2px; color: var(--dim);
                       text-transform: uppercase; padding: 6px 8px; border-bottom: 1px solid var(--line); white-space: nowrap; }
        .hl-tab td   { padding: 8px; border-bottom: 1px dotted var(--line); font-variant-numeric: tabular-nums; white-space: nowrap; }
        .hl-tab tr:last-child td { border-bottom: 0; }
        .hl-tab .n   { text-align: right; }
        .hl-tab .dim { color: var(--dim); }
        .hl-tab .nom { color: var(--fg); letter-spacing: 1px; }
        .hl-mini     { display: inline-block; width: 64px; height: 8px; border-radius: 3px; vertical-align: middle;
                       margin-right: 8px; background: #1b1b1b; overflow: hidden; }
        .hl-mini i   { display: block; height: 100%; width: calc(var(--p, 0) * 100%); background: var(--fg); }
        .hl-mini.warn i { background: var(--warn, #ffb000); }
        .hl-mini.crit i { background: var(--acc); }
        .hl-est      { display: inline-block; font-size: .56rem; letter-spacing: 1.5px; text-transform: uppercase;
                       padding: 3px 8px; border-radius: 999px; border: 1px solid var(--line); color: var(--dim); }
        .hl-est.e0   { color: var(--ok, #2fd36b); border-color: var(--ok, #2fd36b); }
        .hl-est.e1   { color: var(--fg); border-color: #555; }
        .hl-est.e3   { color: var(--warn, #ffb000); border-color: var(--warn, #ffb000); }
        .hl-est.e4   { color: var(--acc); border-color: var(--acc); }
        .hl-scroll   { overflow-x: auto; }
        @media (max-width: 700px) { .hl-tab .op { display: none; } }

        /* reparto de CPU */
        .hl-cpu      { display: flex; height: 26px; border-radius: 6px; overflow: hidden; margin-top: 14px;
                       border: 1px solid var(--line);
                       background: radial-gradient(#1c1c1c 1px, transparent 1.2px) 0 0 / 5px 5px; }
        .hl-cpu i    { display: block; height: 100%; transition: width .4s; }
        .hl-ley      { display: flex; flex-wrap: wrap; gap: 6px 16px; margin-top: 10px; font-size: .6rem;
                       letter-spacing: 1.5px; color: var(--dim); text-transform: uppercase; }
        .hl-ley span::before { content: ""; display: inline-block; width: 9px; height: 9px; border-radius: 2px;
                       margin-right: 6px; vertical-align: -1px; background: var(--c); }

        /* pila de cada hilo: barra = tamano (relativo al mayor), relleno = usado */
        .hl-pilas    { display: grid; gap: 10px; margin-top: 12px; }
        .hl-fila     { display: grid; grid-template-columns: 110px 1fr 120px; gap: 12px; align-items: center; font-size: .72rem; }
        .hl-fila .nm { letter-spacing: 1px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
        .hl-fila .tx { text-align: right; color: var(--dim); font-variant-numeric: tabular-nums; }
        .hl-caja     { height: 14px; border-radius: 4px; border: 1px solid #333; overflow: hidden;
                       background: radial-gradient(#1c1c1c 1px, transparent 1.2px) 0 0 / 4px 4px; }
        .hl-caja i   { display: block; height: 100%; width: calc(var(--p, 0) * 100%); background: var(--fg); }
        .hl-caja.warn i { background: var(--warn, #ffb000); }
        .hl-caja.crit i { background: var(--acc); }
        .hl-caja.desc { border-style: dashed; }
      </style>`);
  }

  // ---------------------------------------------------------------------------
  // 2) HTML: 4 tarjetas
  // ---------------------------------------------------------------------------
  var html = /*html*/`

    <!-- ===== 1. RESUMEN ===== -->
    <div class="tarjeta ancha">
      <h2>Hilos de FreeRTOS</h2>
      <span class="etq">rtos_info</span><span class="etq">lee</span>
      <div class="hl-dos">
        <canvas class="dial" data-dial="hl_cpu" data-min="0" data-max="100" data-unit="% CPU"
                data-zones="0~70:ok,70~90:warn,90~100:crit"></canvas>
        <div>
          <dl class="hl-kv">
            <dt>Hilos</dt>                 <dd><span data-v="hl_n">--</span> <span class="dim" data-hlvis></span></dd>
            <dt>Tiempo encendido</dt>      <dd data-v="hl_up" data-fmt="hms">--</dd>
            <dt>Pila reservada</dt>        <dd data-v="hl_stot" data-fmt="size">--</dd>
            <dt>Pila usada (maximo)</dt>   <dd><span data-v="hl_suso" data-fmt="size">--</span> &middot; <span data-v="hl_spct">--</span> %</dd>
            <dt>Bloques de control (TCB)</dt><dd data-v="hl_tcb" data-fmt="size">--</dd>
            <dt>Memoria total de hilos</dt><dd data-v="hl_mem" data-fmt="size">--</dd>
            <dt>Pila mas llena</dt>        <dd class="r"><span data-v="hl_peor">--</span> &middot; <span data-v="hl_peorp">--</span> %</dd>
          </dl>
        </div>
      </div>
      <div class="hl-nota" data-hlnota></div>
    </div>

    <!-- ===== 2. REPARTO DE CPU ===== -->
    <div class="tarjeta ancha">
      <h2>Reparto de CPU</h2>
      <span class="etq">ultimo segundo</span>
      <div class="hl-cpu" data-hlcpu></div>
      <div class="hl-ley" data-hlcpuley></div>
    </div>

    <!-- ===== 3. TABLA DE HILOS ===== -->
    <div class="tarjeta ancha">
      <h2>Estado de cada hilo</h2>
      <span class="etq">uxTaskGetSystemState</span>
      <div class="hl-scroll">
        <table class="hl-tab">
          <thead><tr>
            <th>Hilo</th><th>Estado</th><th class="n">Prio</th><th>CPU</th>
            <th class="n">Pila</th><th class="n op">Usada max</th><th class="n op">Libre min</th><th>Pila usada</th>
          </tr></thead>
          <tbody data-hltabla></tbody>
        </table>
      </div>
    </div>

    <!-- ===== 4. MEMORIA DE CADA HILO ===== -->
    <div class="tarjeta ancha">
      <h2>Pila de cada hilo</h2>
      <span class="etq">tamano</span><span class="etq">usado</span>
      <div class="hl-pilas" data-hlpilas></div>
      <p class="dim">Largo de la barra = pila del hilo (respecto al mayor). Relleno = maximo usado.
        Borde discontinuo = tamano desconocido (pila estatica: anadela en pilas_manuales[] de Modulo2_threads.c).</p>
    </div>
  `;

  document.currentScript.insertAdjacentHTML('beforebegin', html);

  // ---------------------------------------------------------------------------
  // 3) Widgets propios (se registran cuando vars.js ya esta cargado)
  // ---------------------------------------------------------------------------
  var ESTADOS = ['ejecutando', 'listo', 'bloqueado', 'suspendido', 'borrado', '?'];
  var COLORES = ['#e8e8e8', '#d71921', '#ffb000', '#2fd36b', '#5b5bff', '#c05bff', '#4fd1ff', '#ff7ab6', '#9a9a9a', '#b5e853'];

  function esc(s) { return String(s).replace(/[&<>"]/g, function (c) { return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]; }); }
  function kb(b) { return b >= 1024 ? (b / 1024).toFixed(1) + ' KB' : b + ' B'; }
  function nivel(p) { return p >= 90 ? 'crit' : p >= 70 ? 'warn' : ''; }

  function hilos(t) {                                  // lista de hilos validos con sus datos
    var l = [];
    for (var i = 0; i < MAX; i++) {
      var n = t['h' + i + '_n'];
      if (!n) continue;
      l.push({ i: i, n: n, s: t['h' + i + '_s'] | 0, p: t['h' + i + '_p'] | 0, c: t['h' + i + '_c'] | 0,
               sz: t['h' + i + '_sz'] | 0, lib: t['h' + i + '_lib'] | 0, pct: t['h' + i + '_pct'] | 0 });
    }
    return l;
  }

  function registrar() {
    if (!window.VARS || VARS._modHilos) return;
    VARS._modHilos = true;

    // tabla de hilos
    VARS.widget('data-hltabla', function (el, d, t) {
      el.innerHTML = hilos(t).map(function (h) {
        var usado = h.sz ? h.sz - h.lib : 0;
        return '<tr>' +
          '<td class="nom">' + esc(h.n) + '</td>' +
          '<td><span class="hl-est e' + h.s + '">' + (ESTADOS[h.s] || '?') + '</span></td>' +
          '<td class="n">' + h.p + '</td>' +
          '<td>' + (t.hl_ok ? '<span class="hl-mini" style="--p:' + h.c / 100 + '"><i></i></span>' + h.c + ' %' : '<span class="dim">--</span>') + '</td>' +
          '<td class="n">' + (h.sz ? kb(h.sz) : '<span class="dim">?</span>') + '</td>' +
          '<td class="n op">' + (h.sz ? kb(usado) : '<span class="dim">?</span>') + '</td>' +
          '<td class="n op">' + kb(h.lib) + '</td>' +
          '<td>' + (h.sz ? '<span class="hl-mini ' + nivel(h.pct) + '" style="--p:' + h.pct / 100 + '"><i></i></span>' + h.pct + ' %'
                         : '<span class="dim">libre ' + kb(h.lib) + '</span>') + '</td>' +
          '</tr>';
      }).join('');
    });

    // reparto de CPU: un trozo por hilo con CPU > 0 (IDLE incluido = tiempo libre)
    VARS.widget('data-hlcpu', function (el, d, t) {
      var ley = el.parentNode.querySelector('[data-hlcpuley]');
      if (!t.hl_ok) {
        el.innerHTML = '';
        ley.innerHTML = '<span style="--c:transparent">sin estadisticas de CPU (ver aviso arriba)</span>';
        return;
      }
      var l = hilos(t).filter(function (h) { return h.c > 0; });
      el.innerHTML = l.map(function (h) {
        var col = h.n === 'IDLE' ? 'transparent' : COLORES[h.i % COLORES.length];
        return '<i style="width:' + h.c + '%;background:' + col + '" title="' + esc(h.n) + ': ' + h.c + ' %"></i>';
      }).join('');
      ley.innerHTML = l.map(function (h) {
        var col = h.n === 'IDLE' ? 'transparent;outline:1px solid var(--line)' : COLORES[h.i % COLORES.length];
        return '<span style="--c:' + col + '">' + esc(h.n === 'IDLE' ? 'IDLE (libre)' : h.n) + ' ' + h.c + ' %</span>';
      }).join('');
    });

    // pila de cada hilo: barras proporcionales al hilo con mas pila
    VARS.widget('data-hlpilas', function (el, d, t) {
      var l = hilos(t), mayor = 1;
      l.forEach(function (h) { mayor = Math.max(mayor, h.sz || h.lib); });
      el.innerHTML = l.map(function (h) {
        var largo = (h.sz || h.lib) * 100 / mayor;
        var txt = h.sz ? kb(h.sz - h.lib) + ' / ' + kb(h.sz) : 'libre ' + kb(h.lib);
        return '<div class="hl-fila"><span class="nm">' + esc(h.n) + '</span>' +
          '<div style="width:' + largo + '%"><div class="hl-caja ' + (h.sz ? nivel(h.pct) : 'desc') + '" style="--p:' + (h.sz ? h.pct / 100 : 0) + '"><i></i></div></div>' +
          '<span class="tx">' + txt + '</span></div>';
      }).join('');
    });

    // "de N" si hay mas hilos de los que caben, y avisos
    VARS.widget('data-hlvis', function (el, d, t) {
      el.textContent = (t.hl_vis !== undefined && t.hl_n > t.hl_vis) ? '(se ven ' + t.hl_vis + ')' : '';
    });
    VARS.widget('data-hlnota', function (el, d, t) {
      var a = [];
      if (t.hl_ok === false || t.hl_ok === 0)
        a.push('CPU sin medir: activa configGENERATE_RUN_TIME_STATS en FreeRTOSConfig.h (ver LEEME).');
      if (t.hl_peorp >= 90) a.push('Pila casi llena en ' + t.hl_peor + ': aumenta su stack_size.');
      if (t.hl_n > t.hl_vis) a.push('Hay mas hilos que RTOSINFO_MAX_THREADS: sube el valor y data-max.');
      el.textContent = a.join('  ');
    });
  }

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', registrar);
  else registrar();
})();