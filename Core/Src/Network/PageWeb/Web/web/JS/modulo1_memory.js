// =============================================================================
// mod_ram.js  -  MODULO RAM: toda la estructura RamInfo_t (ram_info.h) en tarjetas
//
//  Uso en la pagina (dentro de <div class="rejilla">, ANTES de vars.js):
//      <script src="/mod_ram.js"></script>
//
//  Variables que tiene que enviar la placa (tabla web en C, ver mod_ram_vars.txt):
//      ram_reg  ram_regsz ram_usado ram_libre ram_pct ram_total
//      ram_data ram_bss   ram_stat
//      ram_rt_tot ram_rt_lib ram_rt_min ram_rt_may ram_rt_alloc ram_rt_free ram_rt_pct
//      ram_lc_arena ram_lc_uso ram_lc_lib
//      ram_pi_res ram_pi_act ram_pi_max ram_pi_pct
//
//  Contiene: 1) su CSS   2) su HTML   3) un widget propio (mapa del bloque de RAM)
// =============================================================================
(function () {
  'use strict';

  // ---------------------------------------------------------------------------
  // 1) CSS propio del modulo (se mete una sola vez aunque el modulo se use 2 veces)
  // ---------------------------------------------------------------------------
  if (!document.getElementById('css-mod-ram')) {
    document.head.insertAdjacentHTML('beforeend', /*html*/`
      <style id="css-mod-ram">
        .ram-kv      { display: grid; grid-template-columns: 1fr auto; gap: 7px 14px; font-size: .78rem; margin-top: 12px; }
        .ram-kv dt   { color: var(--dim); letter-spacing: 1.5px; text-transform: uppercase; font-size: .62rem; align-self: center; }
        .ram-kv dd   { margin: 0; text-align: right; font-variant-numeric: tabular-nums; }
        .ram-kv dd.r { color: var(--acc); }
        .ram-dos     { display: grid; grid-template-columns: minmax(200px, 1fr) 1.3fr; gap: 10px 26px; align-items: center; }
        @media (max-width: 700px) { .ram-dos { grid-template-columns: 1fr; } }

        /* mapa del bloque principal:  [data|bss|heapRTOS|malloc|....libre....|pila] */
        .ram-mapa    { display: flex; height: 26px; border-radius: 6px; overflow: hidden; margin-top: 18px;
                       border: 1px solid var(--line);
                       background: radial-gradient(#1c1c1c 1px, transparent 1.2px) 0 0 / 5px 5px; }
        .ram-mapa i  { display: block; height: 100%; min-width: 2px; transition: width .4s; }
        .ram-mapa .d { background: #e8e8e8; }
        .ram-mapa .b { background: #8a8a8a; }
        .ram-mapa .h { background: var(--acc); }
        .ram-mapa .m { background: var(--warn, #ffb000); }
        .ram-mapa .l { background: transparent; flex: 1; }
        .ram-mapa .p { background: #5b5bff; }
        .ram-ley     { display: flex; flex-wrap: wrap; gap: 6px 16px; margin-top: 10px; font-size: .6rem;
                       letter-spacing: 1.5px; color: var(--dim); text-transform: uppercase; }
        .ram-ley span::before { content: ""; display: inline-block; width: 9px; height: 9px; border-radius: 2px;
                       margin-right: 6px; vertical-align: -1px; background: var(--c); }
        .ram-aviso   { margin-top: 10px; font-size: .7rem; letter-spacing: 1px; color: var(--acc); }

        /* tablas: bloques del micro y todos los campos */
        .ram-tab     { width: 100%; border-collapse: collapse; font-size: .74rem; margin-top: 8px; }
        .ram-tab th  { text-align: left; font-weight: normal; font-size: .58rem; letter-spacing: 2px; color: var(--dim);
                       text-transform: uppercase; padding: 6px 8px; border-bottom: 1px solid var(--line); }
        .ram-tab td  { padding: 7px 8px; border-bottom: 1px dotted var(--line); font-variant-numeric: tabular-nums; }
        .ram-tab tr:last-child td { border-bottom: 0; }
        .ram-tab td.n, .ram-tab th.n { text-align: right; }
        .ram-tab code { color: var(--fg); font-family: inherit; }
        .ram-tab .id  { color: var(--dim); }
        .ram-tab .grp td { color: var(--acc); font-size: .58rem; letter-spacing: 2.5px; text-transform: uppercase;
                       padding-top: 14px; border-bottom: 1px solid var(--line); }
        .ram-tab tr.prin td { background: rgba(215,25,33,.08); }
        .ram-tab tr.prin td:first-child { box-shadow: inset 2px 0 0 var(--acc); }
        .ram-tab .barra.mini { width: 90px; }
        @media (max-width: 700px) { .ram-tab .id, .ram-tab .desc { display: none; } }
      </style>`);
  }

  // ---------------------------------------------------------------------------
  // Tabla ram_regions[] (copia de ram_info.c: datos FIJOS del STM32H753, RM0433)
  // No se envian por el WebSocket porque no cambian nunca.
  // ---------------------------------------------------------------------------
  var REGIONES = [
    ['ITCM',          0x00000000,  64, 'solo CPU, codigo/datos rapidos'],
    ['DTCM',          0x20000000, 128, 'solo CPU, muy rapida, sin DMA'],
    ['AXI SRAM (D1)', 0x24000000, 512, 'principal'],
    ['SRAM1 (D2)',    0x30000000, 128, 'DMA, Ethernet'],
    ['SRAM2 (D2)',    0x30020000, 128, 'DMA'],
    ['SRAM3 (D2)',    0x30040000,  32, 'DMA'],
    ['SRAM4 (D3)',    0x38000000,  64, 'dominio de bajo consumo'],
    ['Backup SRAM',   0x38800000,   4, 'se mantiene con bateria (VBAT)']
  ];
  function hex8(n) { return '0x' + ('0000000' + n.toString(16).toUpperCase()).slice(-8); }

  // ---------------------------------------------------------------------------
  // TODOS los campos de RamInfo_t, en el mismo orden que ram_info.h
  //   [campo en C, ID web, formato, descripcion]   formato: size | pct | num | txt
  // ---------------------------------------------------------------------------
  var CAMPOS = [
    ['Bloque principal'],
    ['main_region',    'ram_reg',      'txt',  'nombre del bloque donde estan .data/.bss'],
    ['region_txt',     'ram_reg',      'txt',  'el mismo nombre en un array (para la web)'],
    ['main_size',      'ram_regsz',    'size', 'tamano del bloque'],
    ['Variables globales'],
    ['data',           'ram_data',     'size', '.data: globales con valor inicial'],
    ['bss',            'ram_bss',      'size', '.bss: globales a 0 (incluye heap FreeRTOS)'],
    ['statics',        'ram_stat',     'size', 'data + bss'],
    ['Heap FreeRTOS'],
    ['rtos_total',     'ram_rt_tot',   'size', 'configTOTAL_HEAP_SIZE'],
    ['rtos_free',      'ram_rt_lib',   'size', 'libre ahora'],
    ['rtos_min_free',  'ram_rt_min',   'size', 'minimo libre desde el arranque'],
    ['rtos_largest',   'ram_rt_may',   'size', 'bloque libre mas grande'],
    ['rtos_allocs',    'ram_rt_alloc', 'num',  'pvPortMalloc correctos'],
    ['rtos_frees',     'ram_rt_free',  'num',  'vPortFree correctos'],
    ['rtos_pct',       'ram_rt_pct',   'pct',  '% usado'],
    ['Heap malloc (libc)'],
    ['libc_arena',     'ram_lc_arena', 'size', 'RAM pedida al sistema (sbrk)'],
    ['libc_used',      'ram_lc_uso',   'size', 'en uso por malloc'],
    ['libc_free',      'ram_lc_lib',   'size', 'libre dentro de la arena'],
    ['Pila principal (MSP)'],
    ['stack_reserved', 'ram_pi_res',   'size', '_Min_Stack_Size del .ld'],
    ['stack_now',      'ram_pi_act',   'size', 'uso en este momento'],
    ['stack_max',      'ram_pi_max',   'size', 'uso maximo historico'],
    ['stack_pct',      'ram_pi_pct',   'pct',  'stack_max / reservada'],
    ['Resumen del bloque principal'],
    ['main_used',      'ram_usado',    'size', 'globales + malloc + pila reservada'],
    ['main_free',      'ram_libre',    'size', 'sin usar en ese bloque'],
    ['main_pct',       'ram_pct',      'pct',  '% usado del bloque'],
    ['Total del micro'],
    ['ram_total',      'ram_total',    'size', 'suma de bloques (sin Backup SRAM)']
  ];

  function filaCampo(c) {
    if (c.length === 1) return '<tr class="grp"><td colspan="4">' + c[0] + '</td></tr>';
    var val = c[2] === 'size' ? '<span data-v="' + c[1] + '" data-fmt="size">--</span>'
            : c[2] === 'pct'  ? '<span data-v="' + c[1] + '">--</span> %'
            :                   '<span data-v="' + c[1] + '">--</span>';
    return '<tr><td><code>ram_info.' + c[0] + '</code></td><td class="id">' + c[1] + '</td>' +
           '<td class="n">' + val + '</td><td class="desc dim" style="margin:0">' + c[3] + '</td></tr>';
  }

  function filaRegion(r) {
    return '<tr data-region="' + r[0] + '"><td>' + r[0] + '</td><td class="id">' + hex8(r[1]) + '</td>' +
           '<td class="n">' + r[2] + ' KB</td><td class="desc dim" style="margin:0">' + r[3] + '</td>' +
           '<td class="n" data-ramuso></td></tr>';
  }

  // ---------------------------------------------------------------------------
  // 2) HTML: 4 tarjetas, una por parte de RamInfo_t
  //    data-fmt="size" -> 1234 se ve como "1.2 KB" (formato de vars.js)
  // ---------------------------------------------------------------------------
  var html = /*html*/`

    <!-- ===== 1. BLOQUE PRINCIPAL + MAPA ===== -->
    <div class="tarjeta ancha">
      <h2>RAM &middot; bloque principal</h2>
      <span class="etq">ram_info</span><span class="etq">lee</span>
      <div class="ram-dos">
        <canvas class="dial" data-dial="ram_pct" data-min="0" data-max="100" data-unit="% usado"
                data-zones="0~70:ok,70~90:warn,90~100:crit"></canvas>
        <div>
          <dl class="ram-kv">
            <dt>Bloque</dt>          <dd data-v="ram_reg">--</dd>
            <dt>Tamano</dt>          <dd data-v="ram_regsz" data-fmt="size">--</dd>
            <dt>Usado</dt>           <dd data-v="ram_usado" data-fmt="size">--</dd>
            <dt>Libre</dt>           <dd data-v="ram_libre" data-fmt="size" class="r">--</dd>
            <dt>% usado</dt>         <dd><span data-v="ram_pct">--</span> %</dd>
            <dt>RAM total micro</dt> <dd data-v="ram_total" data-fmt="size">--</dd>
          </dl>
        </div>
      </div>
      <div class="ram-mapa" data-rammapa></div>
      <div class="ram-ley">
        <span style="--c:#e8e8e8">.data</span>
        <span style="--c:#8a8a8a">.bss</span>
        <span style="--c:var(--acc)">heap FreeRTOS (dentro de .bss)</span>
        <span style="--c:var(--warn,#ffb000)">malloc</span>
        <span style="--c:transparent;outline:1px solid var(--line)">libre</span>
        <span style="--c:#5b5bff">pila principal</span>
      </div>
    </div>

    <!-- ===== 2. HEAP DE FREERTOS ===== -->
    <div class="tarjeta">
      <h2>Heap FreeRTOS</h2>
      <span class="etq">pvPortMalloc</span>
      <canvas data-ring="ram_rt_pct" data-min="0" data-max="100" data-unit="% usado"
              style="width:150px;height:150px;display:block;margin:0 auto"></canvas>
      <dl class="ram-kv">
        <dt>Total</dt>            <dd data-v="ram_rt_tot" data-fmt="size">--</dd>
        <dt>Libre ahora</dt>      <dd data-v="ram_rt_lib" data-fmt="size">--</dd>
        <dt>% usado</dt>          <dd><span data-v="ram_rt_pct">--</span> %</dd>
        <dt>Minimo historico</dt> <dd data-v="ram_rt_min" data-fmt="size" class="r">--</dd>
        <dt>Bloque libre mayor</dt><dd data-v="ram_rt_may" data-fmt="size">--</dd>
        <dt>Fragmentacion</dt>    <dd data-ramfrag>--</dd>
        <dt>malloc / free</dt>    <dd><span data-v="ram_rt_alloc">-</span> / <span data-v="ram_rt_free">-</span></dd>
      </dl>
    </div>

    <!-- ===== 3. PILA PRINCIPAL (interrupciones) ===== -->
    <div class="tarjeta">
      <h2>Pila principal</h2>
      <span class="etq">MSP</span><span class="etq">interrupciones</span>
      <div class="fila" style="margin-top:10px">
        <div class="barra" style="flex:1" data-bar="ram_pi_pct" data-min="0" data-max="100"
             data-alarm="ram_pi_pct" data-hi="80"><i></i></div>
        <b><span data-v="ram_pi_pct">--</span> %</b>
      </div>
      <dl class="ram-kv">
        <dt>Reservada (.ld)</dt>  <dd data-v="ram_pi_res" data-fmt="size">--</dd>
        <dt>Uso ahora</dt>        <dd data-v="ram_pi_act" data-fmt="size">--</dd>
        <dt>Uso maximo</dt>       <dd data-v="ram_pi_max" data-fmt="size" class="r">--</dd>
      </dl>
      <div class="ram-aviso" data-rampila></div>
    </div>

    <!-- ===== 4. GLOBALES Y MALLOC ===== -->
    <div class="tarjeta">
      <h2>Globales y malloc</h2>
      <span class="etq">.data</span><span class="etq">.bss</span><span class="etq">libc</span>
      <dl class="ram-kv">
        <dt>.data (con valor)</dt> <dd data-v="ram_data" data-fmt="size">--</dd>
        <dt>.bss (a cero)</dt>     <dd data-v="ram_bss"  data-fmt="size">--</dd>
        <dt>Total globales</dt>    <dd data-v="ram_stat" data-fmt="size">--</dd>
      </dl>
      <dl class="ram-kv" style="border-top:1px dotted var(--line);padding-top:12px">
        <dt>malloc reservado</dt>  <dd data-v="ram_lc_arena" data-fmt="size">--</dd>
        <dt>malloc en uso</dt>     <dd data-v="ram_lc_uso"   data-fmt="size">--</dd>
        <dt>malloc libre</dt>      <dd data-v="ram_lc_lib"   data-fmt="size">--</dd>
      </dl>
      <p class="dim">El heap de FreeRTOS es un array global: ya esta contado dentro de .bss.</p>
    </div>

    <!-- ===== 5. MAPA DE RAM DEL MICRO: ram_regions[] ===== -->
    <div class="tarjeta ancha">
      <h2>Bloques de RAM del STM32H753</h2>
      <span class="etq">ram_regions[]</span><span class="etq">fijo</span>
      <table class="ram-tab" data-ramregiones>
        <tr><th>Bloque</th><th class="id">Direccion</th><th class="n">Tamano</th><th class="desc">Uso tipico</th><th class="n">Ocupado</th></tr>
        ${REGIONES.map(filaRegion).join('')}
      </table>
      <p class="dim">Solo se puede medir el bloque donde el linker pone .data/.bss (marcado en rojo).
        Lo que ocupan los demas se ve en el Build Analyzer de STM32CubeIDE.</p>
    </div>

    <!-- ===== 6. TODOS LOS CAMPOS DE ram_info ===== -->
    <div class="tarjeta ancha">
      <h2>ram_info &middot; todos los campos</h2>
      <span class="etq">RamInfo_t</span><span class="etq">ram_info.h</span>
      <details open>
        <summary class="dim" style="cursor:pointer">${CAMPOS.filter(function (c) { return c.length > 1; }).length} campos</summary>
        <table class="ram-tab">
          <tr><th>Campo en C</th><th class="id">ID web</th><th class="n">Valor</th><th class="desc">Que es</th></tr>
          ${CAMPOS.map(filaCampo).join('')}
        </table>
      </details>
    </div>
  `;

  document.currentScript.insertAdjacentHTML('beforebegin', html);

  // ---------------------------------------------------------------------------
  // 3) Widgets propios del modulo.
  //    vars.js todavia no esta cargado (va despues), asi que se registran
  //    cuando la pagina termina de leerse.  t = todos los valores recibidos.
  // ---------------------------------------------------------------------------
  function registrar() {
    if (!window.VARS || VARS._modRam) return;
    VARS._modRam = true;

    // mapa del bloque: anchos proporcionales al tamano del bloque
    VARS.widget('data-rammapa', function (el, d, t) {
      var tam = t.ram_regsz; if (!tam) return;
      var bssOtros = Math.max(0, (t.ram_bss || 0) - (t.ram_rt_tot || 0));
      var partes = [
        ['d', t.ram_data || 0,       '.data'],
        ['b', bssOtros,              '.bss (otras globales)'],
        ['h', t.ram_rt_tot || 0,     'heap FreeRTOS'],
        ['m', t.ram_lc_arena || 0,   'malloc'],
        ['l', 0,                     'libre'],
        ['p', t.ram_pi_res || 0,     'pila principal']
      ];
      if (!el.children.length) {                       // crear los trozos la primera vez
        el.innerHTML = partes.map(function (p) { return '<i class="' + p[0] + '"></i>'; }).join('');
      }
      partes.forEach(function (p, i) {
        var seg = el.children[i];
        if (p[0] !== 'l') seg.style.width = (p[1] * 100 / tam) + '%';
        var bytes = p[0] === 'l' ? (t.ram_libre || 0) : p[1];
        seg.title = p[2] + ': ' + (bytes / 1024).toFixed(1) + ' KB';
      });
    });

    // fragmentacion del heap: cuanto del libre NO esta en el bloque mayor
    VARS.widget('data-ramfrag', function (el, d, t) {
      if (!t.ram_rt_lib) return;
      var f = Math.round(100 - (t.ram_rt_may || 0) * 100 / t.ram_rt_lib);
      el.textContent = f + ' %';
      el.className = f > 50 ? 'r' : '';
    });

    // tabla de bloques: marca el bloque principal y pone su barra de uso
    VARS.widget('data-ramregiones', function (el, d, t) {
      if (!t.ram_reg) return;
      el.querySelectorAll('tr[data-region]').forEach(function (tr) {
        var prin = tr.getAttribute('data-region') === t.ram_reg, celda = tr.querySelector('[data-ramuso]');
        tr.classList.toggle('prin', prin);
        celda.innerHTML = prin
          ? '<span class="barra mini" style="--p:' + ((t.ram_pct || 0) / 100) + '"><i></i></span>' + (t.ram_pct || 0) + ' %'
          : '<span class="dim" style="margin:0">no medido</span>';
      });
    });

    // aviso de la pila principal
    VARS.widget('data-rampila', function (el, d, t) {
      var p = t.ram_pi_pct, max = t.ram_pi_max;
      if (p === undefined) return;
      el.textContent = !max ? 'sin medir: falta RamInfo_PaintMainStack() en main()'
                     : p >= 100 ? '(!) DESBORDADA: sube _Min_Stack_Size en el .ld'
                     : p > 80   ? 'casi llena: conviene subir _Min_Stack_Size'
                     : '';
    });
  }

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', registrar);
  else registrar();
})();