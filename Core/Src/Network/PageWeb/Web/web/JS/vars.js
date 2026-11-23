// vars.js - Conecta la pagina con la placa y dibuja los widgets. Sin librerias.
//
//  <body data-ws="/ws/widgets">   <- WebSocket de esta pagina (nombre de web_pages[] en web_vars.c)
//  <script src="JS/vars.js"></script>
//
// "ID" = el ID de la variable en web_vars.c.  Todos los widgets son atributos HTML:
//
//  LEER (placa -> web)
//   data-v="ID"            texto con el valor (booleano: SI/NO)
//   data-dot="ID"          numero en pantalla de puntos   (data-digits, data-unit, data-color="red", data-pitch)
//   data-bar="ID"          pone --p (0..1) para dibujar barras con CSS  (data-min, data-max)
//   data-gauge="ID"        reloj de puntos en <canvas>    (data-min, data-max, data-unit)
//   data-chart="ID1,ID2"   grafica en <canvas>            (data-seconds, data-min, data-max)
//   data-barchart="A,B,C"  barras verticales en <canvas>  (data-names, data-min, data-max)
//   data-horizon="pitch,roll"  horizonte artificial en <canvas>
//   data-3d="ID1,ID2,ID3"  pone --rx --ry --rz (grados) para objetos CSS 3D
//   data-table="A,B,C"     tabla                          (data-names, data-units)
//   data-led="ID"          pone data-on="1"/"0" (para dibujar un led con CSS)
//   data-state="ID"        pone data-s="valor" (para cambiar dibujos/SVG con CSS)
//   data-alarm="ID"        pone data-alarm="lo|ok|hi"     (data-lo, data-hi)
//   data-img="ID"          cambia el src segun el valor   (data-map="0:/a.svg,1:/b.svg")
//   data-dial="ID"         medidor de aguja con zonas     (data-min, data-max, data-unit, data-zones="0~60:ok,60~85:warn,85~100:crit")
//   data-steps="ID"        pasos de estado de maquina     (data-names="PARADO,ARRANQUE,MARCHA,FALLO", data-fail="3")
//   data-bits="ID"         un byte como leds de E/S       (data-count="8", data-names="DI0,DI1,...")
//   data-alarmlog="id:lo:hi:nombre,..."  tabla-registro de alarmas con hora   (data-max="10")
//   data-table="A,B,C"     tabla configurable por fila:  data-types="valor|led|barra|alarma,..."
//                          data-limits="min:max,..."  data-texts="1:MARCHA,0:PARADO"
//   data-v + data-fmt="hms|hex|bin"   formatos (segundos a hh:mm:ss, hexadecimal, binario)
//   data-pid               faceplate PID completo (ver abajo)
//   data-bit="ID:n"        pone data-on segun el bit n de una palabra (para leds de salidas/alarmas)
//   data-show-if="ID"      muestra el elemento solo si la variable es distinta de 0 / vacia
//   data-v + data-map="0:PARADO,1:MARCHA"   texto segun el valor (pone tambien data-s para el color)
//   data-confirm="TEXTO"   en un boton: pide una segunda pulsacion para confirmar
//   data-export="#tabla"   boton que descarga una tabla como CSV  (data-name="datos.csv")
//   data-clock="ID"       reloj desde una fecha UNIX (segundos UTC): data-mode="analog|digital|date" (data-tz="minutos" o "ID")
//   data-ring="ID"         anillo de progreso con el valor en el centro   (data-min, data-max, data-unit)
//   data-bytes="lo,hi,len" bytes de una trama como hexadecimal "DE AD BE EF ..."
//   data-v + data-fmt="ip|hex8|size|clock|date|datetime"   formatos extra
//   data-cond="A>B+1"      condicion en vivo (pone data-ok=1/0); lista dentro de data-condlist
//   data-history="ID"      registro en la web de los cambios de una variable (data-map, data-max)
//   data-tip / data-tipbox zonas pulsables con explicacion (dentro de data-explain)
//   data-tabs              pestañas: botones data-tab="x" y paneles data-pane="x"
//   data-report            boton que descarga un informe de soporte con todas las variables
//   <table data-errors="activos|historico|contadores">   errores en vivo / historico / contadores (catalogo /api/errores)
//   <table data-signals="g" data-bus="0">   señales de comunicacion con forzado individual (catalogo /api/senales)
//   data-copy="#id"        boton que copia el texto de un elemento
//   data-settime="ID"     boton que pone la hora de este PC en una variable (segundos UNIX)
//   <input data-bytes-set="lo,hi,len">   editor de bytes de una trama (DE AD BE EF)
//   <input data-set="ID" data-hex>        campo hexadecimal
//  ESCRIBIR (web -> placa)
//   data-set="ID" + data-ip  campo de texto para direcciones IPv4 (se envia como numero)
//   data-set="ID"          en <input>, <select> o <input type=range/checkbox>: modifica la variable
//                          en un <button data-set="ID" data-value="2">: pone ese valor
//   data-cmd="nombre"      boton que ejecuta un comando de la placa   (data-arg="texto")
//   data-terminal          terminal (escribe y recibe texto)
//   data-upload            zona para subir archivos                    (data-max, data-name)
//  OTROS
//   data-estado            texto conectado / desconectado
//   data-msg               texto con la ultima respuesta de la placa
//   VARS.widget('data-mio', function (el, d, todos) {...})   widget propio
//   window.onVars = function (d) {...}                        recibe cada mensaje
var VARS = (function () {
  'use strict';
  var all = {}, widgets = [], ws = null;

  function num(x) { return (typeof x === 'number') ? x : (x === true ? 1 : 0); }
  function attr(el, n, def) { var v = el.getAttribute(n); return (v === null || v === '') ? def : parseFloat(v); }
  function ids(el, a) { return el.getAttribute(a).split(',').map(function (s) { return s.trim(); }); }
  function css(n, def) { return getComputedStyle(document.documentElement).getPropertyValue(n).trim() || def; }
  function widget(attrName, fn) { widgets.push({ attr: attrName, fn: fn }); }
  function enviar(txt) { if (ws && ws.readyState === 1) { ws.send(txt); return true; } return false; }

  // ---------- aviso (respuesta de la placa) ----------
  var toastEl, toastT;
  function aviso(t) {
    document.querySelectorAll('[data-msg]').forEach(function (e) { e.textContent = t; });
    if (!toastEl) { toastEl = document.createElement('div'); toastEl.className = 'aviso'; document.body.appendChild(toastEl); }
    toastEl.textContent = t; toastEl.classList.add('ver');
    clearTimeout(toastT); toastT = setTimeout(function () { toastEl.classList.remove('ver'); }, 2200);
    toastEl.classList.toggle('err', /^error/.test(t));
  }

  // ---------- canvas ----------
  function lienzo(cv) {
    var r = cv.getBoundingClientRect(), dpr = window.devicePixelRatio || 1;
    var w = Math.max(10, Math.round(r.width)), h = Math.max(10, Math.round(r.height));
    if (cv.width !== Math.round(w * dpr) || cv.height !== Math.round(h * dpr)) { cv.width = Math.round(w * dpr); cv.height = Math.round(h * dpr); }
    var c = cv.getContext('2d'); c.setTransform(dpr, 0, 0, dpr, 0, 0); c.clearRect(0, 0, w, h);
    return { c: c, w: w, h: h };
  }

  // ---------- fuente de puntos 5x7 (cada letra = 5 columnas, bit0 = fila de arriba) ----------
  var F = {
    ' ': [0,0,0,0,0], '0': [62,81,73,69,62], '1': [0,66,127,64,0], '2': [66,97,81,73,70], '3': [33,65,69,75,49],
    '4': [24,20,18,127,16], '5': [39,69,69,69,57], '6': [60,74,73,73,48], '7': [1,113,9,5,3], '8': [54,73,73,73,54],
    '9': [6,73,73,41,30], '-': [8,8,8,8,8], '+': [8,8,62,8,8], '.': [0,96,96,0,0], ':': [0,54,54,0,0], '%': [35,19,8,100,98],
    '/': [32,16,8,4,2], '°': [0,7,5,7,0], '>': [65,34,20,8,0], '_': [64,64,64,64,64],
    A: [126,17,17,17,126], B: [127,73,73,73,54], C: [62,65,65,65,34], D: [127,65,65,34,28], E: [127,73,73,73,65],
    F: [127,9,9,9,1], G: [62,65,73,73,122], H: [127,8,8,8,127], I: [0,65,127,65,0], J: [32,64,65,63,1],
    K: [127,8,20,34,65], L: [127,64,64,64,64], M: [127,2,12,2,127], N: [127,4,8,16,127], O: [62,65,65,65,62],
    P: [127,9,9,9,6], Q: [62,65,81,33,94], R: [127,9,25,41,70], S: [70,73,73,73,49], T: [1,1,127,1,1],
    U: [63,64,64,64,63], V: [31,32,64,32,31], W: [63,64,56,64,63], X: [99,20,8,20,99], Y: [7,8,112,8,7], Z: [97,81,73,69,67]
  };
  function limpiar(t) { return String(t).normalize('NFD').replace(/[̀-ͯ]/g, '').toUpperCase(); }

  function puntos(cv, texto, pitch, color) {
    texto = limpiar(texto);
    var n = texto.length, w = Math.max(1, n * 6 - 1) * pitch, h = 7 * pitch;
    var dpr = window.devicePixelRatio || 1;
    cv.style.width = w + 'px'; cv.style.height = h + 'px';
    if (cv.width !== Math.round(w * dpr) || cv.height !== Math.round(h * dpr)) { cv.width = Math.round(w * dpr); cv.height = Math.round(h * dpr); }
    var c = cv.getContext('2d'); c.setTransform(dpr, 0, 0, dpr, 0, 0); c.clearRect(0, 0, w, h);
    var off = css('--dot-off', '#202020'), on = color || css('--fg', '#fff'), r = pitch * 0.36;
    for (var i = 0; i < n; i++) {
      var g = F[texto.charAt(i)] || F[' '];
      for (var col = 0; col < 5; col++) for (var row = 0; row < 7; row++) {
        var lit = (g[col] >> row) & 1;
        c.fillStyle = lit ? on : off;
        c.beginPath(); c.arc((i * 6 + col + 0.5) * pitch, (row + 0.5) * pitch, r, 0, 6.2832); c.fill();
      }
    }
  }
  function colorDe(el) { var k = el.getAttribute('data-color'); return k === 'red' ? css('--acc', '#d71921') : null; }

  function p2(n) { return (n < 10 ? '0' : '') + n; }
  function fecha(v, tzmin, f) {
    var dt = new Date((v + (tzmin || 0) * 60) * 1000);
    var hh = p2(dt.getUTCHours()) + ':' + p2(dt.getUTCMinutes()) + ':' + p2(dt.getUTCSeconds());
    var dd = p2(dt.getUTCDate()) + '/' + p2(dt.getUTCMonth() + 1) + '/' + dt.getUTCFullYear();
    return f === 'clock' ? hh : (f === 'date' ? dd : dd + ' ' + hh);
  }
  function ipNum(t) {
    var m = /^\s*(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})\s*$/.exec(t); if (!m) return null;
    for (var i = 1; i < 5; i++) if (+m[i] > 255) return null;
    return ((+m[1] << 24) >>> 0) + (+m[2] << 16) + (+m[3] << 8) + (+m[4]);
  }
  function fmt(v, f) {
    if (typeof v !== 'number' || !f) return v;
    if (f === 'hms') { var s = Math.max(0, Math.floor(v)); return p2(Math.floor(s / 3600)) + ':' + p2(Math.floor(s % 3600 / 60)) + ':' + p2(s % 60); }
    if (f === 'hex') return '0x' + (v >>> 0).toString(16).toUpperCase();
    if (f === 'ip') { v = v >>> 0; return (v >>> 24) + '.' + ((v >>> 16) & 255) + '.' + ((v >>> 8) & 255) + '.' + (v & 255); }
    if (f === 'hex8') { var h8 = (v >>> 0).toString(16).toUpperCase(); while (h8.length < 8) h8 = '0' + h8; return h8; }
    if (f === 'size') return v >= 1048576 ? (v / 1048576).toFixed(2) + ' MB' : (v >= 1024 ? (v / 1024).toFixed(1) + ' KB' : v + ' B');
    if (f === 'clock' || f === 'date' || f === 'datetime') { return fecha(v, all.rtc_tz || 0, f); }
    if (f === 'bin') { var b = (v >>> 0).toString(2); while (b.length < 8) b = '0' + b; return b; }
    return v;
  }
  function textos(s) { var m = {}; (s || '').split(',').forEach(function (p) { var i = p.indexOf(':'); if (i > 0) m[p.slice(0, i).trim()] = p.slice(i + 1).trim(); }); return m; }
  function lim(s) { var p = (s || '').split(':'); return [p[0] === undefined || p[0] === '' ? NaN : parseFloat(p[0]), p[1] === undefined || p[1] === '' ? NaN : parseFloat(p[1])]; }

  // ---------- LEER ----------
  widget('data-v', function (el, d) {
    var v = d[el.getAttribute('data-v')]; if (v === undefined) return;
    if (el.hasAttribute('data-map')) {                     // texto segun el valor
      var key = typeof v === 'boolean' ? (v ? '1' : '0') : String(v), mp = el._map || (el._map = textos(el.getAttribute('data-map')));
      el.textContent = limpiar(mp[key] !== undefined ? mp[key] : (v === null ? '--' : v)); el.setAttribute('data-s', key); return;
    }
    if (typeof v === 'boolean') { el.textContent = v ? 'SI' : 'NO'; el.setAttribute('data-on', v ? '1' : '0'); }
    else el.textContent = (v === null) ? '--' : fmt(v, el.getAttribute('data-fmt'));
  });

  widget('data-show-if', function (el, d) {
    var v = d[el.getAttribute('data-show-if')]; if (v === undefined) return;
    el.hidden = !(v === true || (typeof v === 'number' && v !== 0) || (typeof v === 'string' && v !== ''));
  });

  widget('data-bit', function (el, d) {
    var a = el.getAttribute('data-bit').split(':'), v = d[a[0]]; if (typeof v !== 'number') return;
    el.setAttribute('data-on', ((v >>> parseInt(a[1], 10)) & 1) ? '1' : '0');
  });

  widget('data-led', function (el, d) {
    var v = d[el.getAttribute('data-led')]; if (v === undefined) return;
    el.setAttribute('data-on', num(v) ? '1' : '0');
  });

  widget('data-state', function (el, d) {
    var v = d[el.getAttribute('data-state')]; if (v === undefined) return;
    el.setAttribute('data-s', typeof v === 'boolean' ? (v ? '1' : '0') : v);
  });

  widget('data-alarm', function (el, d) {
    var v = d[el.getAttribute('data-alarm')]; if (typeof v !== 'number') return;
    var lo = attr(el, 'data-lo', -Infinity), hi = attr(el, 'data-hi', Infinity);
    el.setAttribute('data-alarm-st', v < lo ? 'lo' : v > hi ? 'hi' : 'ok');
  });

  widget('data-bar', function (el, d) {
    var v = d[el.getAttribute('data-bar')]; if (v === undefined || v === null) return;
    var mn = attr(el, 'data-min', 0), mx = attr(el, 'data-max', 100);
    el.style.setProperty('--p', Math.max(0, Math.min(1, (num(v) - mn) / (mx - mn))));
  });

  widget('data-3d', function (el, d) {
    var a = ids(el, 'data-3d'), n = ['--rx', '--ry', '--rz'];
    for (var i = 0; i < a.length && i < 3; i++) { var v = d[a[i]]; if (typeof v === 'number') el.style.setProperty(n[i], v + 'deg'); }
  });

  widget('data-dot', function (el, d) {
    var v = d[el.getAttribute('data-dot')]; if (v === undefined) return;
    var t = (v === null) ? '--' : (typeof v === 'boolean' ? (v ? 'ON' : 'OFF') : String(fmt(v, el.getAttribute('data-fmt'))));
    var u = el.getAttribute('data-unit') || '', dg = attr(el, 'data-digits', 0);
    while (t.length < dg) t = ' ' + t;
    puntos(el, t + (u ? ' ' + u : ''), attr(el, 'data-pitch', 6), colorDe(el));
  });

  widget('data-gauge', function (el, d) {
    var v = d[el.getAttribute('data-gauge')]; if (v === undefined) return;
    var mn = attr(el, 'data-min', 0), mx = attr(el, 'data-max', 100);
    var p = (typeof v === 'number') ? Math.max(0, Math.min(1, (v - mn) / (mx - mn))) : 0;
    var k = lienzo(el), c = k.c, cx = k.w / 2, cy = k.h * 0.56, R = Math.min(k.w / 2, k.h * 0.56) - 12;
    var N = 41, a0 = Math.PI * 0.75, a1 = Math.PI * 2.25;
    for (var i = 0; i < N; i++) {
      var f = i / (N - 1), a = a0 + (a1 - a0) * f, lit = f <= p + 1e-6 && p > 0;
      c.fillStyle = lit ? (f > 0.85 ? css('--acc', '#d71921') : css('--fg', '#fff')) : css('--dot-off', '#262626');
      c.beginPath(); c.arc(cx + Math.cos(a) * R, cy + Math.sin(a) * R, R * (i % 5 === 0 ? 0.045 : 0.032), 0, 6.2832); c.fill();
    }
    c.fillStyle = css('--fg', '#fff'); c.textAlign = 'center'; c.textBaseline = 'middle';
    c.font = 'bold ' + Math.round(R * 0.5) + 'px ui-monospace,Consolas,monospace';
    c.fillText(v === null ? '--' : v, cx, cy);
    c.font = Math.round(R * 0.17) + 'px ui-monospace,Consolas,monospace'; c.fillStyle = css('--dim', '#888');
    c.fillText((el.getAttribute('data-unit') || '').toUpperCase(), cx, cy + R * 0.42);
  });

  var SERIES = ['--fg', '--acc', '--dim', '#999'];
  function colSerie(i) { var s = SERIES[i % 4]; return s.charAt(0) === '-' ? css(s, '#fff') : s; }

  function dibujarGrafica(el) {
    var a = ids(el, 'data-chart'), seg = attr(el, 'data-seconds', 60) * 1000, now = Date.now(), t0 = now - seg;
    var hist = el._h || (el._h = {}), k = lienzo(el), c = k.c, w = k.w, h = k.h, pl = 38, pt = 8, pb = 6;
    var fmin = el.hasAttribute('data-min'), fmax = el.hasAttribute('data-max');
    var mn = fmin ? attr(el, 'data-min', 0) : Infinity, mx = fmax ? attr(el, 'data-max', 1) : -Infinity;
    a.forEach(function (id) { (hist[id] || []).forEach(function (p) { if (!fmin && p.v < mn) mn = p.v; if (!fmax && p.v > mx) mx = p.v; }); });
    var L = [el.hasAttribute('data-lo') ? attr(el, 'data-lo', 0) : null, el.hasAttribute('data-hi') ? attr(el, 'data-hi', 0) : null];
    L.forEach(function (x) { if (x === null) return; if (!fmin && x < mn) mn = x; if (!fmax && x > mx) mx = x; });
    if (!isFinite(mn) || !isFinite(mx)) { mn = 0; mx = 1; }
    if (mx - mn < 1e-6) mx = mn + 1;
    var m = (mx - mn) * 0.08; if (!fmin) mn -= m; if (!fmax) mx += m;
    c.strokeStyle = css('--line', '#2b2b2b'); c.fillStyle = css('--dim', '#888'); c.lineWidth = 1;
    c.font = '10px ui-monospace,Consolas,monospace'; c.textAlign = 'right'; c.textBaseline = 'middle'; c.setLineDash([2, 4]);
    for (var g = 0; g <= 4; g++) {
      var y = pt + (h - pt - pb) * g / 4, val = mx - (mx - mn) * g / 4;
      c.beginPath(); c.moveTo(pl, y); c.lineTo(w, y); c.stroke();
      c.fillText(Math.abs(val) >= 100 ? val.toFixed(0) : val.toFixed(1), pl - 5, y);
    }
    c.setLineDash([6, 4]); c.strokeStyle = css('--acc', '#d71921'); c.lineWidth = 1.2;       // limites
    L.forEach(function (x) { if (x === null) return; var y = pt + (h - pt - pb) * (1 - (x - mn) / (mx - mn)); c.beginPath(); c.moveTo(pl, y); c.lineTo(w, y); c.stroke(); });
    c.setLineDash([]);
    a.forEach(function (id, i) {
      var pts = hist[id] || []; if (!pts.length) return;
      c.strokeStyle = colSerie(i); c.lineWidth = 1.6; c.beginPath();
      var first = true;
      function P(t, v) { var x = pl + (w - pl) * (t - t0) / seg, y = pt + (h - pt - pb) * (1 - (v - mn) / (mx - mn)); if (first) { c.moveTo(x, y); first = false; } else c.lineTo(x, y); }
      pts.forEach(function (p) { P(Math.max(p.t, t0), p.v); });
      P(now, pts[pts.length - 1].v); c.stroke();
    });
  }
  widget('data-chart', function (el, d) {
    var hist = el._h || (el._h = {}), now = Date.now(), lim = now - attr(el, 'data-seconds', 60) * 1000 - 1000;
    ids(el, 'data-chart').forEach(function (id) {
      var v = d[id]; if (typeof v !== 'number' && typeof v !== 'boolean') return;
      var arr = hist[id] || (hist[id] = []); arr.push({ t: now, v: num(v) });
      while (arr.length > 1 && arr[1].t < lim) arr.shift();
    });
    dibujarGrafica(el);
  });
  setInterval(function () { document.querySelectorAll('[data-chart]').forEach(dibujarGrafica); }, 1000);  // avanza aunque no haya cambios

  widget('data-barchart', function (el) {
    var a = ids(el, 'data-barchart'), nm = (el.getAttribute('data-names') || '').split(',');
    var mn = attr(el, 'data-min', 0), mx = attr(el, 'data-max', 100);
    var k = lienzo(el), c = k.c, w = k.w, h = k.h, base = h - 18, top = 6, bw = w / a.length;
    c.textAlign = 'center'; c.font = '10px ui-monospace,Consolas,monospace';
    a.forEach(function (id, i) {
      var v = all[id], p = (typeof v === 'number') ? Math.max(0, Math.min(1, (v - mn) / (mx - mn))) : 0;
      var x = i * bw + bw / 2, segs = Math.max(6, Math.floor((base - top) / 9));
      for (var s = 0; s < segs; s++) {
        var lit = s < Math.round(p * segs);
        c.fillStyle = lit ? css('--fg', '#fff') : css('--dot-off', '#262626');
        c.beginPath(); c.arc(x, base - (s + 0.5) * (base - top) / segs, Math.min(bw * 0.18, 3.4), 0, 6.2832); c.fill();
      }
      c.fillStyle = css('--dim', '#888'); c.fillText(limpiar(nm[i] || id), x, h - 4);
    });
  });

  widget('data-horizon', function (el, d) {
    var a = ids(el, 'data-horizon'), pitch = all[a[0]], roll = all[a[1]];
    if (typeof pitch !== 'number' || typeof roll !== 'number') return;
    var k = lienzo(el), c = k.c, cx = k.w / 2, cy = k.h / 2, R = Math.min(cx, cy) - 6;
    c.save(); c.beginPath(); c.arc(cx, cy, R, 0, 6.2832); c.clip();
    c.translate(cx, cy); c.rotate(-roll * Math.PI / 180);
    var off = Math.max(-R * 2, Math.min(R * 2, pitch * R / 40));
    c.fillStyle = css('--panel2', '#161616'); c.fillRect(-R * 2, off, R * 4, R * 4);          // suelo
    c.strokeStyle = css('--fg', '#fff'); c.lineWidth = 1.5; c.beginPath(); c.moveTo(-R * 2, off); c.lineTo(R * 2, off); c.stroke();
    c.fillStyle = css('--dim', '#888'); c.font = '9px ui-monospace,Consolas,monospace'; c.textAlign = 'center';
    for (var t = -30; t <= 30; t += 10) { if (!t) continue; var y = off - t * R / 40; c.fillRect(-14, y, 28, 1); c.fillText(Math.abs(t), 26, y + 3); }
    c.restore();
    c.strokeStyle = css('--line', '#2b2b2b'); c.lineWidth = 2; c.beginPath(); c.arc(cx, cy, R, 0, 6.2832); c.stroke();
    c.strokeStyle = css('--acc', '#d71921'); c.lineWidth = 2.5; c.beginPath();                  // avion fijo
    c.moveTo(cx - R * 0.5, cy); c.lineTo(cx - R * 0.15, cy); c.lineTo(cx, cy + R * 0.1); c.lineTo(cx + R * 0.15, cy); c.lineTo(cx + R * 0.5, cy); c.stroke();
  });

  // Tabla configurable: cada fila puede ser valor | led | barra | alarma
  widget('data-table', function (el, d) {
    var a = ids(el, 'data-table');
    if (!el._built) {
      var nm = (el.getAttribute('data-names') || '').split(','), un = (el.getAttribute('data-units') || '').split(',');
      var ty = (el.getAttribute('data-types') || '').split(','), li = (el.getAttribute('data-limits') || '').split(',');
      el._tx = textos(el.getAttribute('data-texts')); el._cells = []; el.textContent = '';
      a.forEach(function (id, i) {
        var tp = (ty[i] || 'valor').trim(), tr = el.insertRow(), c1 = tr.insertCell(), c2 = tr.insertCell(), c3 = tr.insertCell();
        c1.textContent = limpiar((nm[i] && nm[i].trim()) || id); c2.className = 'v'; c3.className = 'u'; c3.textContent = (un[i] || '').trim();
        c2._tp = tp; c2._lim = lim(li[i]);
        if (tp === 'led') c2.innerHTML = '<span class="led"></span> <span class="t"></span>';
        else if (tp === 'barra') c2.innerHTML = '<span class="barra mini"><i></i></span><span class="t"></span>';
        else c2.innerHTML = '<span class="t"></span>';
        el._cells.push(c2);
      });
      el._built = 1;
    }
    a.forEach(function (id, i) {
      var v = d[id]; if (v === undefined) return;
      var td = el._cells[i], t = td.querySelector('.t'), key = typeof v === 'boolean' ? (v ? '1' : '0') : String(v);
      if (td._tp === 'led') {
        var on = num(v) !== 0; td.querySelector('.led').setAttribute('data-on', on ? '1' : '0'); td.setAttribute('data-s', key);
        t.textContent = limpiar(el._tx[key] !== undefined ? el._tx[key] : (typeof v === 'boolean' ? (v ? 'SI' : 'NO') : v));
      } else if (td._tp === 'barra') {
        var mn = isNaN(td._lim[0]) ? 0 : td._lim[0], mx = isNaN(td._lim[1]) ? 100 : td._lim[1];
        td.querySelector('.barra').style.setProperty('--p', Math.max(0, Math.min(1, (num(v) - mn) / (mx - mn)))); t.textContent = v;
      } else if (td._tp === 'alarma') {
        t.textContent = (v === null) ? '--' : v;
        var lo = isNaN(td._lim[0]) ? -Infinity : td._lim[0], hi = isNaN(td._lim[1]) ? Infinity : td._lim[1];
        td.setAttribute('data-alarm-st', typeof v !== 'number' ? 'ok' : v < lo ? 'lo' : v > hi ? 'hi' : 'ok');
      } else {
        t.textContent = (v === null) ? '--' : (typeof v === 'boolean' ? (v ? 'SI' : 'NO') : v);
        if (typeof v === 'boolean') td.setAttribute('data-on', v ? '1' : '0');
      }
    });
  });

  // Registro de alarmas: "id:lo:hi:nombre" por variable; anota cada cambio de estado con la hora
  widget('data-alarmlog', function (el, d) {
    if (!el._cfg) {
      el._cfg = ids(el, 'data-alarmlog').map(function (s) {
        var p = s.split(':'); return { id: p[0], lo: (p[1] === undefined || p[1] === '') ? -Infinity : parseFloat(p[1]),
          hi: (p[2] === undefined || p[2] === '') ? Infinity : parseFloat(p[2]), name: p[3] || p[0], st: null };
      });
      el._ev = []; el._max = attr(el, 'data-max', 10); el._dirty = true;
    }
    el._cfg.forEach(function (c) {
      var v = d[c.id]; if (typeof v !== 'number') return;
      var st = v < c.lo ? 'lo' : v > c.hi ? 'hi' : 'ok';
      if (st !== c.st) {
        if (c.st !== null || st !== 'ok') { el._ev.unshift({ t: new Date(), n: c.name, s: st, v: v }); if (el._ev.length > el._max) el._ev.pop(); el._dirty = true; }
        c.st = st;
      }
    });
    if (!el._dirty) return; el._dirty = false; el.textContent = '';
    var h = el.createTHead().insertRow(); ['HORA', 'VARIABLE', 'EVENTO', 'VALOR'].forEach(function (x) { h.insertCell().textContent = x; });
    var tb = el.createTBody();
    if (!el._ev.length) { var r0 = tb.insertRow(), c0 = r0.insertCell(); c0.colSpan = 4; c0.textContent = 'SIN ALARMAS'; return; }
    el._ev.forEach(function (e) {
      var r = tb.insertRow(); r.setAttribute('data-alarm-st', e.s);
      [p2(e.t.getHours()) + ':' + p2(e.t.getMinutes()) + ':' + p2(e.t.getSeconds()), limpiar(e.n), e.s === 'hi' ? 'ALTA' : e.s === 'lo' ? 'BAJA' : 'NORMAL', e.v].forEach(function (x) { r.insertCell().textContent = x; });
    });
  });

  // Medidor de aguja con zonas de color
  widget('data-dial', function (el, d) {
    var v = d[el.getAttribute('data-dial')]; if (v === undefined) return;
    var mn = attr(el, 'data-min', 0), mx = attr(el, 'data-max', 100);
    var k = lienzo(el), c = k.c, cx = k.w / 2, cy = k.h * 0.64, R = Math.min(k.w / 2, k.h * 0.64) - 16, a0 = Math.PI * 0.8, a1 = Math.PI * 2.2;
    function ang(x) { return a0 + (a1 - a0) * Math.max(0, Math.min(1, (x - mn) / (mx - mn))); }
    var zs = (el.getAttribute('data-zones') || '').split(','), col = { ok: css('--ok', '#2fd36b'), warn: css('--warn', '#ffb000'), crit: css('--acc', '#d71921') };
    c.lineWidth = Math.max(6, R * 0.1); c.lineCap = 'butt';
    if (!el.getAttribute('data-zones')) { c.strokeStyle = css('--line', '#2b2b2b'); c.beginPath(); c.arc(cx, cy, R, a0, a1); c.stroke(); }
    zs.forEach(function (z) {
      var m = z.match(/^\s*(-?[\d.]+)~(-?[\d.]+):(\w+)/); if (!m) return;
      c.strokeStyle = col[m[3]] || css('--dim', '#888'); c.beginPath(); c.arc(cx, cy, R, ang(parseFloat(m[1])), ang(parseFloat(m[2]))); c.stroke();
    });
    c.strokeStyle = css('--fg', '#fff'); c.lineWidth = 1.5; c.fillStyle = css('--dim', '#888'); c.font = '10px ui-monospace,Consolas,monospace'; c.textAlign = 'center'; c.textBaseline = 'middle';
    for (var i = 0; i <= 10; i++) {
      var a = a0 + (a1 - a0) * i / 10, r1 = R - R * 0.1 - 4, r2 = r1 - (i % 5 === 0 ? 9 : 5);
      c.beginPath(); c.moveTo(cx + Math.cos(a) * r1, cy + Math.sin(a) * r1); c.lineTo(cx + Math.cos(a) * r2, cy + Math.sin(a) * r2); c.stroke();
      if (i % 5 === 0) c.fillText(Math.round(mn + (mx - mn) * i / 10), cx + Math.cos(a) * (r2 - 11), cy + Math.sin(a) * (r2 - 11));
    }
    if (typeof v === 'number') {
      var an = ang(v); c.strokeStyle = css('--fg', '#fff'); c.lineWidth = 3; c.lineCap = 'round';
      c.beginPath(); c.moveTo(cx, cy); c.lineTo(cx + Math.cos(an) * (R - R * 0.2), cy + Math.sin(an) * (R - R * 0.2)); c.stroke();
    }
    c.fillStyle = css('--acc', '#d71921'); c.beginPath(); c.arc(cx, cy, 6, 0, 6.2832); c.fill();
    c.fillStyle = css('--fg', '#fff'); c.font = 'bold ' + Math.round(R * 0.26) + 'px ui-monospace,Consolas,monospace';
    c.fillText(v === null ? '--' : v, cx, cy + R * 0.5);
    c.fillStyle = css('--dim', '#888'); c.font = Math.round(R * 0.13) + 'px ui-monospace,Consolas,monospace';
    c.fillText((el.getAttribute('data-unit') || '').toUpperCase(), cx, cy + R * 0.5 + R * 0.24);
  });

  // Pasos de estado de maquina
  widget('data-steps', function (el, d) {
    if (!el._b) { el.textContent = ''; (el.getAttribute('data-names') || '').split(',').forEach(function (n) { var s = document.createElement('span'); s.className = 'paso'; s.textContent = limpiar(n.trim()); el.appendChild(s); }); el._b = 1; }
    var v = d[el.getAttribute('data-steps')]; if (typeof v !== 'number') return;
    var fail = attr(el, 'data-fail', -1);
    [].forEach.call(el.children, function (s, i) { s.classList.toggle('sel', i === v); s.classList.toggle('fallo', i === v && i === fail); });
  });

  // Un byte como leds (entradas / salidas digitales)
  widget('data-bits', function (el, d) {
    if (!el._b) {
      var n = attr(el, 'data-count', 8), nm = (el.getAttribute('data-names') || '').split(','); el.textContent = '';
      for (var i = 0; i < n; i++) {
        var w = document.createElement('div'), l = document.createElement('span'), t = document.createElement('span');
        w.className = 'bit'; l.className = 'led'; t.textContent = limpiar(((nm[i] || '') + '').trim() || ('B' + i)); w.appendChild(l); w.appendChild(t); el.appendChild(w);
      }
      el._b = 1;
    }
    var v = d[el.getAttribute('data-bits')]; if (typeof v !== 'number') return;
    [].forEach.call(el.children, function (w, i) { w.firstChild.setAttribute('data-on', ((v >>> i) & 1) ? '1' : '0'); });
  });

  widget('data-img', function (el, d) {
    var v = d[el.getAttribute('data-img')]; if (v === undefined) return;
    var key = typeof v === 'boolean' ? (v ? '1' : '0') : String(v);
    (el.getAttribute('data-map') || '').split(',').forEach(function (p) {
      var i = p.indexOf(':'); if (i > 0 && p.slice(0, i).trim() === key) el.src = p.slice(i + 1).trim();
    });
  });

  // ---------- Faceplate PID: se construye entero a partir de los IDs ----------
  //  <div data-pid data-pv="t_int" data-sp="sp_t" data-out="pid_out" data-mode="pid_mode" data-man="pid_man"
  //       data-kp="pid_kp" data-ki="pid_ki" data-kd="pid_kd" data-unit="C" data-sp-min="10" data-sp-max="30"></div>
  //  Modo: 0 = AUTO, 1 = MANUAL.  sp, mode, man, kp, ki y kd deben ser variables escribibles (WVW).
  widget('data-pid', function (el) {
    if (el._ok) return; el._ok = 1;
    var g = function (n) { return el.getAttribute('data-' + n) || ''; }, u = g('unit') || '';
    var smin = g('sp-min') || 0, smax = g('sp-max') || 100;
    function nodo(t, c, h) { var e = document.createElement(t); if (c) e.className = c; if (h !== undefined) e.innerHTML = h; return e; }
    var top = nodo('div', 'pid-top');
    [['PV', g('pv'), '', ''], ['SP', g('sp'), ' data-color="red"', ''], ['OUT', g('out'), '', '%']].forEach(function (b) {
      var c = nodo('div', 'pid-blq', '<span class="dim">' + b[0] + '</span><canvas class="dot" data-dot="' + b[1] + '" data-unit="' + (b[0] === 'OUT' ? '%' : u) + '" data-pitch="4"' + b[2] + '></canvas>');
      top.appendChild(c);
    });
    el.appendChild(top);
    el.appendChild(nodo('div', 'barra', '<i></i>')); el.lastChild.setAttribute('data-bar', g('out')); el.lastChild.setAttribute('data-min', 0); el.lastChild.setAttribute('data-max', 100);
    var fila = nodo('div', 'fila', '<button class="btn fino" data-set="' + g('mode') + '" data-value="0">AUTO</button><button class="btn fino" data-set="' + g('mode') + '" data-value="1">MANUAL</button>');
    fila.style.margin = '14px 0 4px'; el.appendChild(fila);
    el.appendChild(nodo('div', 'pid-cam',
      '<label>SP<input type="number" step="0.5" min="' + smin + '" max="' + smax + '" data-set="' + g('sp') + '"></label>' +
      '<label>SALIDA MANUAL %<input type="range" min="0" max="100" data-set="' + g('man') + '"></label>' +
      '<label>KP<input type="number" step="0.1" min="0" data-set="' + g('kp') + '"></label>' +
      '<label>KI<input type="number" step="0.05" min="0" data-set="' + g('ki') + '"></label>' +
      '<label>KD<input type="number" step="0.05" min="0" data-set="' + g('kd') + '"></label>'));
    var cv = nodo('canvas', 'grafica'); cv.setAttribute('data-chart', g('pv') + ',' + g('sp')); cv.setAttribute('data-seconds', g('seconds') || 120); cv.style.height = '150px'; el.appendChild(cv);
  });

  // ---------- ESCRIBIR ----------
  function valorDe(el) {
    if (el.type === 'checkbox') return el.checked ? '1' : '0';
    return el.value;
  }
  function mandar(el) {
    var id = el.getAttribute('data-set'), v = el.tagName === 'BUTTON' ? el.getAttribute('data-value') : valorDe(el);
    if (el.hasAttribute('data-hex')) {
      var hx = String(v).replace(/^0x/i, '').trim();
      if (!/^[0-9a-f]{1,8}$/i.test(hx)) { el.classList.add('mal'); aviso('error: valor hexadecimal no valido'); return; }
      el.classList.remove('mal'); v = parseInt(hx, 16) >>> 0;
    }
    if (el.hasAttribute('data-ip')) {
      var n = ipNum(v); el.classList.toggle('mal', n === null);
      if (n === null) { aviso('error: direccion IP no valida (a.b.c.d)'); return; }
      v = n;
    }
    el._hold = Date.now() + 1200;                         // no pisar lo que el usuario acaba de poner
    enviar('S|' + id + '|' + v);
  }
  var tmrs = new WeakMap();
  document.addEventListener('input', function (e) {
    var el = e.target; if (!el.hasAttribute || !el.hasAttribute('data-set') || el.tagName === 'BUTTON') return;
    if (el.type === 'range') {                           // el deslizador manda cada 120 ms mientras se mueve
      el._hold = Date.now() + 1200;
      if (!tmrs.get(el)) tmrs.set(el, setTimeout(function () { tmrs.delete(el); mandar(el); }, 120));
    }
  });
  document.addEventListener('change', function (e) {
    var el = e.target; if (el.hasAttribute && el.hasAttribute('data-set') && el.tagName !== 'BUTTON') mandar(el);
  });
  document.addEventListener('click', function (e) {
    var x = e.target.closest ? e.target.closest('[data-export]') : null;
    if (x) {                                              // descargar una tabla como CSV (separador ; para Excel)
      var t = document.querySelector(x.getAttribute('data-export')); if (!t) return;
      var rows = [].map.call(t.rows, function (r) { return [].map.call(r.cells, function (c) { var tx = c.textContent.trim(), l = c.querySelector('.led'); if (l && !tx) tx = l.getAttribute('data-on') === '1' ? 'ON' : 'OFF'; return '"' + tx.replace(/"/g, '""') + '"'; }).join(';'); });
      var a = document.createElement('a'); a.href = URL.createObjectURL(new Blob(['\ufeff' + rows.join('\r\n')], { type: 'text/csv' }));
      a.download = x.getAttribute('data-name') || 'tabla.csv'; document.body.appendChild(a); a.click();
      setTimeout(function () { URL.revokeObjectURL(a.href); a.remove(); }, 500); return;
    }
    var b = e.target.closest ? e.target.closest('[data-set],[data-cmd]') : null; if (!b || b.tagName !== 'BUTTON') return;
    if (b.hasAttribute('data-confirm')) {                 // primera pulsacion arma, la segunda envia
      if (!b._armed) { b._armed = true; b._orig = b.textContent; b.textContent = b.getAttribute('data-confirm'); clearTimeout(b._tm);
        b._tm = setTimeout(function () { b._armed = false; b.textContent = b._orig; }, 3000); return; }
      b._armed = false; clearTimeout(b._tm); b.textContent = b._orig;
    }
    if (b.hasAttribute('data-cmd')) { enviar('C|' + b.getAttribute('data-cmd') + '|' + (b.getAttribute('data-arg') || '')); }
    else if (b.hasAttribute('data-set')) { mandar(b); }
  });

  widget('data-set', function (el, d) {
    var v = d[el.getAttribute('data-set')]; if (v === undefined || v === null) return;
    if (el.tagName === 'BUTTON') { el.classList.toggle('sel', String(v === true ? 1 : v === false ? 0 : v) === el.getAttribute('data-value')); return; }
    if (document.activeElement === el || (el._hold && Date.now() < el._hold)) return;
    if (el.type === 'checkbox') el.checked = !!v; else el.value = typeof v === 'boolean' ? (v ? '1' : '0') : el.hasAttribute('data-ip') ? fmt(v, 'ip') : (el.hasAttribute('data-hex') ? (v >>> 0).toString(16).toUpperCase() : v);
  });

  // ---------- TERMINAL ----------
  widget('data-terminal', function (el) {
    if (el._ok) return; el._ok = 1;
    var out = document.createElement('pre'), f = document.createElement('form'), inp = document.createElement('input'), b = document.createElement('button');
    out.className = 'term-out'; f.className = 'term-in'; inp.placeholder = '> comando'; inp.autocomplete = 'off'; inp.spellcheck = false;
    b.textContent = 'ENVIAR'; b.className = 'btn'; f.appendChild(inp); f.appendChild(b); el.appendChild(out); el.appendChild(f);
    var hist = [], hi = 0;
    function linea(t) { out.textContent += t + '\n'; var L = out.textContent.split('\n'); if (L.length > 200) out.textContent = L.slice(-200).join('\n'); out.scrollTop = out.scrollHeight; }
    el._linea = linea;
    f.addEventListener('submit', function (e) {
      e.preventDefault(); var t = inp.value.trim(); if (!t) return;
      if (enviar('T|' + t)) { linea('> ' + t); hist.push(t); hi = hist.length; } else linea('(sin conexion)');
      inp.value = '';
    });
    inp.addEventListener('keydown', function (e) {
      if (e.key === 'ArrowUp' && hist.length) { hi = Math.max(0, hi - 1); inp.value = hist[hi]; e.preventDefault(); }
      if (e.key === 'ArrowDown' && hist.length) { hi = Math.min(hist.length, hi + 1); inp.value = hist[hi] || ''; e.preventDefault(); }
    });
  });

  // ---------- SUBIR ARCHIVOS ----------
  widget('data-upload', function (el) {
    if (el._ok) return; el._ok = 1;
    var max = attr(el, 'data-max', 16384), forced = el.getAttribute('data-name');
    var inp = document.createElement('input'), lab = document.createElement('label'), st = document.createElement('div');
    inp.type = 'file'; inp.hidden = true; lab.className = 'btn'; lab.textContent = 'ELEGIR ARCHIVO'; st.className = 'dim';
    st.textContent = 'o arrastralo aqui (max ' + max + ' bytes)';
    lab.appendChild(inp); el.appendChild(lab); el.appendChild(st);
    function subir(f) {
      if (!f) return;
      if (f.size > max) { st.textContent = 'error: ' + f.size + ' bytes, el maximo es ' + max; return; }
      var name = (forced || f.name).replace(/[^A-Za-z0-9._-]/g, '_').slice(0, 32);
      st.textContent = 'subiendo ' + name + '...';
      fetch('/api/upload?name=' + encodeURIComponent(name), { method: 'POST', body: f })
        .then(function (r) { return r.text().then(function (t) { st.textContent = (r.ok ? '' : 'error ' + r.status + ': ') + t.trim(); }); })
        .catch(function () { st.textContent = 'error: sin conexion'; });
    }
    inp.addEventListener('change', function () { subir(inp.files[0]); inp.value = ''; });
    ['dragover', 'dragenter'].forEach(function (n) { el.addEventListener(n, function (e) { e.preventDefault(); el.classList.add('sobre'); }); });
    ['dragleave', 'drop'].forEach(function (n) { el.addEventListener(n, function (e) { e.preventDefault(); el.classList.remove('sobre'); }); });
    el.addEventListener('drop', function (e) { subir(e.dataTransfer.files[0]); });
  });


  // ====================================================================== widgets de datos
  function tzDe(el, todos) {
    var t = el.getAttribute('data-tz'); if (t === null || t === '') return +todos.rtc_tz || 0;
    return isNaN(+t) ? (+todos[t] || 0) : +t;
  }
  widget('data-clock', function (el, d, todos) {
    var v = todos[el.getAttribute('data-clock')]; if (typeof v !== 'number') return;
    var tz = tzDe(el, todos), modo = el.getAttribute('data-mode') || 'analog';
    if (modo === 'digital' || modo === 'date') { puntos(el, fecha(v, tz, modo === 'date' ? 'date' : 'clock'), attr(el, 'data-pitch', 5), colorDe(el)); return; }
    var dt = new Date((v + tz * 60) * 1000), h = dt.getUTCHours() % 12, m = dt.getUTCMinutes(), sg = dt.getUTCSeconds();
    var k = lienzo(el), c = k.c, cx = k.w / 2, cy = k.h / 2, R = Math.min(cx, cy) - 8;
    for (var i = 0; i < 60; i++) {
      var a = i / 60 * 6.2832 - 1.5708, g = i % 5 === 0;
      c.fillStyle = i === sg ? css('--acc', '#d71921') : (g ? css('--fg', '#fff') : css('--dim', '#888'));
      c.beginPath(); c.arc(cx + Math.cos(a) * R, cy + Math.sin(a) * R, R * (g ? 0.045 : 0.028), 0, 6.2832); c.fill();
    }
    function mano(frac, largo, ancho, col) {
      var a = frac * 6.2832 - 1.5708; c.strokeStyle = col; c.lineWidth = ancho; c.lineCap = 'round';
      c.beginPath(); c.moveTo(cx, cy); c.lineTo(cx + Math.cos(a) * R * largo, cy + Math.sin(a) * R * largo); c.stroke();
    }
    mano((h + m / 60) / 12, 0.5, Math.max(3, R * 0.05), css('--fg', '#fff'));
    mano((m + sg / 60) / 60, 0.78, Math.max(2, R * 0.035), css('--fg', '#fff'));
    mano(sg / 60, 0.86, 1.5, css('--acc', '#d71921'));
    c.fillStyle = css('--acc', '#d71921'); c.beginPath(); c.arc(cx, cy, R * 0.05, 0, 6.2832); c.fill();
  });

  widget('data-ring', function (el, d) {
    var v = d[el.getAttribute('data-ring')]; if (v === undefined) return;
    var mn = attr(el, 'data-min', 0), mx = attr(el, 'data-max', 100);
    var p = typeof v === 'number' ? Math.max(0, Math.min(1, (v - mn) / (mx - mn))) : 0;
    var k = lienzo(el), c = k.c, cx = k.w / 2, cy = k.h / 2, R = Math.min(cx, cy) - 10, N = 48;
    for (var i = 0; i < N; i++) {
      var f = i / N, a = f * 6.2832 - 1.5708, lit = f < p;
      c.fillStyle = lit ? (p > 0.85 ? css('--acc', '#d71921') : css('--fg', '#fff')) : css('--dot-off', '#262626');
      c.beginPath(); c.arc(cx + Math.cos(a) * R, cy + Math.sin(a) * R, R * 0.05, 0, 6.2832); c.fill();
    }
    c.fillStyle = css('--fg', '#fff'); c.textAlign = 'center'; c.textBaseline = 'middle';
    c.font = 'bold ' + Math.round(R * 0.42) + 'px ui-monospace,Consolas,monospace'; c.fillText(v === null ? '--' : v, cx, cy - R * 0.06);
    c.font = Math.round(R * 0.16) + 'px ui-monospace,Consolas,monospace'; c.fillStyle = css('--dim', '#888');
    c.fillText((el.getAttribute('data-unit') || '').toUpperCase(), cx, cy + R * 0.36);
  });

  widget('data-bytes', function (el, d, todos) {
    var a = ids(el, 'data-bytes'), lo = todos[a[0]], hi = todos[a[1]], n = a[2] ? todos[a[2]] : 8;
    if (typeof lo !== 'number' || typeof hi !== 'number') return;
    var out = [], w = [lo >>> 0, hi >>> 0];
    for (var i = 0; i < 8; i++) { var b = (w[i >> 2] >>> ((i & 3) * 8)) & 255; out.push(i < n ? (b < 16 ? '0' : '') + b.toString(16).toUpperCase() : '--'); }
    el.textContent = out.join(' ');
  });

  // ---------- condiciones en vivo ----------
  function operando(t, todos) {
    var total = 0, sg = 1, ok = true, partes = t.replace(/\s+/g, '').match(/[+-]?[^+-]+/g) || [];
    partes.forEach(function (p) {
      sg = 1; if (p.charAt(0) === '+') p = p.slice(1); else if (p.charAt(0) === '-') { sg = -1; p = p.slice(1); }
      var x = /^(0x[0-9a-f]+|\d+(\.\d+)?)$/i.test(p) ? Number(p) : todos[p];
      if (typeof x === 'boolean') x = x ? 1 : 0;
      if (typeof x !== 'number') ok = false; else total += sg * x;
    });
    return ok ? total : NaN;
  }
  function evalCond(txt, todos) {
    var m = /^(.*?)(>=|<=|==|!=|!&|>|<|&)(.*)$/.exec(txt); if (!m) return null;
    var a = operando(m[1], todos), b = operando(m[3], todos); if (isNaN(a) || isNaN(b)) return null;
    var r = m[2] === '>' ? a > b : m[2] === '<' ? a < b : m[2] === '>=' ? a >= b : m[2] === '<=' ? a <= b : m[2] === '==' ? a === b :
            m[2] === '!=' ? a !== b : m[2] === '&' ? (a & b) !== 0 : (a & b) === 0;
    return { ok: r, a: a, b: b, op: m[2] };
  }
  widget('data-cond', function (el, d, todos) {
    var r = evalCond(el.getAttribute('data-cond'), todos);
    var sp = el.querySelector('.cv'); if (!sp) { sp = document.createElement('span'); sp.className = 'cv'; el.appendChild(sp); }
    el.setAttribute('data-ok', r === null ? '?' : (r.ok ? '1' : '0'));
    sp.textContent = r === null ? '' : (Math.round(r.a * 100) / 100) + ' ' + r.op + ' ' + (Math.round(r.b * 100) / 100);
    var lista = el.closest('[data-condlist]');
    if (lista) {
      var l = lista.querySelectorAll('[data-cond]'), todas = true, algo = false;
      for (var i = 0; i < l.length; i++) { var o = l[i].getAttribute('data-ok'); if (o === '?') algo = true; else if (o !== '1') todas = false; }
      lista.setAttribute('data-ok', algo ? '?' : (todas ? '1' : '0'));
    }
  });

  // ---------- historial de cambios (en el navegador, mientras la pagina esta abierta) ----------
  function mapaDe(el) { var o = {}; (el.getAttribute('data-map') || '').split(',').forEach(function (p) { var q = p.split(':'); if (q.length > 1) o[q[0].trim()] = q.slice(1).join(':').trim(); }); return o; }
  widget('data-history', function (el, d) {
    var id = el.getAttribute('data-history'), v = d[id]; if (v === undefined || v === null) return;
    if (typeof v === 'boolean') v = v ? 1 : 0;
    var mp = el._mp || (el._mp = mapaDe(el)), lista = el._ev || (el._ev = []);
    if (el._last === v) return;
    var antes = el._last; el._last = v;
    var hora = new Date(), t = p2(hora.getHours()) + ':' + p2(hora.getMinutes()) + ':' + p2(hora.getSeconds());
    lista.unshift({ t: t, a: antes === undefined ? '' : (mp[antes] !== undefined ? mp[antes] : antes), n: mp[v] !== undefined ? mp[v] : v, s: v });
    var mx = attr(el, 'data-max', 10); if (lista.length > mx) lista.length = mx;
    el.innerHTML = '';
    lista.forEach(function (e, i) {
      var li = document.createElement('li'); li.setAttribute('data-s', e.s);
      li.innerHTML = '<b>' + e.t + '</b> <span class="est" data-s="' + e.s + '">' + e.n + '</span>' + (e.a !== '' ? ' <i>desde ' + e.a + '</i>' : ' <i>inicio</i>');
      el.appendChild(li);
    });
  });

  // ---------- zonas pulsables, pestañas, informe, copiar ----------
  function seleccionarTip(h) {
    var cont = h.closest('[data-explain]') || document, id = h.getAttribute('data-tip');
    cont.querySelectorAll('[data-tip]').forEach(function (x) { x.classList.toggle('sel', x === h); });
    cont.querySelectorAll('[data-tipbox]').forEach(function (b) { b.hidden = b.getAttribute('data-tipbox') !== id; });
  }
  function seleccionarTab(b) {
    var cont = b.closest('[data-tabs]'), id = b.getAttribute('data-tab');
    cont.querySelectorAll('[data-tab]').forEach(function (x) { x.classList.toggle('sel', x === b); });
    cont.querySelectorAll('[data-pane]').forEach(function (p) { p.hidden = p.getAttribute('data-pane') !== id; });
    window.dispatchEvent(new Event('resize'));
  }
  function informe(nombre) {
    var f = new Date(), l = ['INFORME DE SOPORTE', 'Fecha del navegador: ' + f.toISOString(), 'Pagina: ' + location.href, 'Navegador: ' + navigator.userAgent, ''];
    Object.keys(all).sort().forEach(function (k) { l.push(k + ' = ' + all[k]); });
    var a = document.createElement('a'); a.href = URL.createObjectURL(new Blob([l.join('\r\n')], { type: 'text/plain' }));
    a.download = nombre || 'informe_soporte.txt'; document.body.appendChild(a); a.click();
    setTimeout(function () { URL.revokeObjectURL(a.href); a.remove(); }, 500);
  }
  document.addEventListener('click', function (e) {
    var t = e.target.closest ? e.target.closest('[data-tip],[data-tab],[data-report],[data-copy]') : null; if (!t) return;
    if (t.hasAttribute('data-tip')) seleccionarTip(t);
    else if (t.hasAttribute('data-tab')) seleccionarTab(t);
    else if (t.hasAttribute('data-report')) informe(t.getAttribute('data-name'));
    else if (t.hasAttribute('data-copy')) {
      var o = document.querySelector(t.getAttribute('data-copy')); if (!o) return;
      var tx = o.value !== undefined && o.tagName === 'INPUT' ? o.value : o.textContent;
      if (navigator.clipboard) navigator.clipboard.writeText(tx).then(function () { aviso('copiado'); }, function () { aviso('error: no se pudo copiar'); });
    }
  });
  document.addEventListener('keydown', function (e) {
    if ((e.key === 'Enter' || e.key === ' ') && e.target.hasAttribute && e.target.hasAttribute('data-tip')) { e.preventDefault(); seleccionarTip(e.target); }
  });


  // ---------- editor de bytes: <input data-bytes-set="lo,hi[,len]"> ----------
  function bytesTxt(todos, a) {
    var lo = todos[a[0]], hi = todos[a[1]], n = a[2] ? todos[a[2]] : 8;
    if (typeof lo !== 'number' || typeof hi !== 'number') return null;
    var o = [], w = [lo >>> 0, hi >>> 0];
    for (var i = 0; i < 8; i++) { if (i >= n) break; var b = (w[i >> 2] >>> ((i & 3) * 8)) & 255; o.push((b < 16 ? '0' : '') + b.toString(16).toUpperCase()); }
    return o.join(' ');
  }
  widget('data-bytes-set', function (el, d, todos) {
    if (document.activeElement === el || (el._hold && Date.now() < el._hold)) return;
    var t = bytesTxt(todos, ids(el, 'data-bytes-set')); if (t !== null) el.value = t;
  });
  document.addEventListener('change', function (e) {
    var el = e.target; if (!el.hasAttribute || !el.hasAttribute('data-bytes-set')) return;
    var a = ids(el, 'data-bytes-set'), tx = el.value.replace(/0x/ig, ' ').replace(/[,;:]/g, ' ').trim(), bytes = [];
    tx.split(/\s+/).forEach(function (tk) { if (tk === '') return; if (tk.length % 2) tk = '0' + tk; for (var i = 0; i < tk.length; i += 2) bytes.push(tk.substr(i, 2)); });
    if (bytes.length > 8 || bytes.some(function (b) { return !/^[0-9a-f]{2}$/i.test(b); })) { el.classList.add('mal'); aviso('error: bytes en hexadecimal, maximo 8 (ej. DE AD BE EF)'); return; }
    el.classList.remove('mal'); el._hold = Date.now() + 1500;
    var w = [0, 0]; bytes.forEach(function (b, i) { w[i >> 2] += parseInt(b, 16) * Math.pow(256, i & 3); });
    enviar('S|' + a[0] + '|' + w[0]); enviar('S|' + a[1] + '|' + w[1]); if (a[2]) enviar('S|' + a[2] + '|' + bytes.length);
  });
  document.addEventListener('click', function (e) {          // poner la hora del PC en la placa
    var b = e.target.closest ? e.target.closest('[data-settime]') : null; if (!b) return;
    enviar('S|' + b.getAttribute('data-settime') + '|' + Math.floor(Date.now() / 1000));
  });


  // ====================================================================== errores y señales (catalogo desde la placa)
  var cats = {};
  function catalogo(url) {
    var c = cats[url];
    if (c && c.data) return c.data;
    if (!c) {
      cats[url] = c = { t: 0 };
      fetch(url).then(function (r) { return r.json(); }).then(function (j) { c.data = j; repintar(); })
        .catch(function () { delete cats[url]; });
    }
    return null;
  }
  function repintar() {
    widgets.forEach(function (w) {
      if (w.attr === 'data-errors' || w.attr === 'data-signals') document.querySelectorAll('[' + w.attr + ']').forEach(function (el) { w.fn(el, all, all); });
    });
  }
  function num2(x) { return typeof x === 'number' ? String(Math.round(x * 100) / 100) : (x === null ? '--' : String(x)); }
  function dur(sg) { sg = Math.max(0, Math.floor(sg)); return sg >= 3600 ? Math.floor(sg / 3600) + 'h ' + p2(Math.floor(sg % 3600 / 60)) + 'm' : (sg >= 60 ? Math.floor(sg / 60) + 'm ' + p2(sg % 60) + 's' : sg + 's'); }
  var SEVN = ['INFO', 'AVISO', 'ALARMA', 'CRITICO'];
  function sevTag(n) { return '<span class="sev" data-sev="' + n + '">' + (SEVN[n] || n) + '</span>'; }
  function escH(t) { return String(t).replace(/[&<>"]/g, function (c) { return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]; }); }
  function varsVivas(txt) {
    return String(txt).split(/\s+/).filter(Boolean).map(function (n) {
      var base = n.split('.')[0], v = all[base];
      return '<span class="vv">' + escH(n) + (v !== undefined ? '=<b>' + num2(v) + '</b>' : '') + '</span>';
    }).join(' ');
  }

  widget('data-errors', function (el, d, todos) {
    var E = catalogo('/api/errores'), modo = el.getAttribute('data-errors');
    if (!E) { el.innerHTML = '<tbody><tr><td class="dim">cargando configuracion...</td></tr></tbody>'; return; }
    if (todos.er_act === undefined) return;
    var act = todos.er_act >>> 0, una = todos.er_una >>> 0, ahora = todos.rtc_e || 0, tz = tzDe(el, todos), h = '';
    if (modo === 'activos') {
      var lista = [];
      E.forEach(function (e, i) { if ((act >>> i) & 1) lista.push({ i: i, e: e }); });
      lista.sort(function (a, b) { return (b.e.s - a.e.s) || ((todos['el' + b.i] || 0) - (todos['el' + a.i] || 0)); });
      h = '<thead><tr><th>ID</th><th>SEV</th><th>NOMBRE</th><th>DESCRIPCION</th><th>VARIABLES</th><th>DESDE</th><th>DURACION</th><th>VECES</th><th></th></tr></thead><tbody>';
      if (!lista.length) h += '<tr><td colspan="9" class="ok-txt">SIN ERRORES ACTIVOS</td></tr>';
      lista.forEach(function (o) {
        var e = o.e, t = todos['el' + o.i] || 0, nuevo = (una >>> o.i) & 1;
        h += '<tr class="' + (nuevo ? 'nuevo' : '') + '" data-sev="' + e.s + '"><td><b>' + e.i + '</b></td><td>' + sevTag(e.s) + '</td><td>' + escH(e.n) + '</td><td class="desc">' + escH(e.d) + '</td><td>' + varsVivas(e.v) +
             '</td><td>' + fecha(t, tz, 'datetime') + '</td><td>' + dur(ahora - t) + '</td><td>' + (todos['ec' + o.i] || 0) + '</td><td>' +
             (nuevo ? '<button class="btn fino" data-cmd="err_ack" data-arg="' + e.i + '">RECONOCER</button>' : '<span class="dim">RECONOCIDO</span>') + '</td></tr>';
      });
      el.innerHTML = h + '</tbody>';
    } else if (modo === 'historico') {
      var porId = {}; E.forEach(function (e) { porId[e.i] = e; });
      h = '<thead><tr><th>HORA</th><th>ID</th><th>SEV</th><th>NOMBRE</th><th>DESCRIPCION</th><th>EVENTO</th><th>DURACION</th><th>VALOR</th></tr></thead><tbody>';
      var n = 0;
      for (var i = 0; i < 20; i++) {
        var t = todos['h' + i + 't']; if (!t) continue; n++;
        var id = todos['h' + i + 'i'], ev = todos['h' + i + 'e'], e = porId[id] || { n: '?', d: '', s: 0 };
        h += '<tr data-ev="' + ev + '"><td>' + fecha(t, tz, 'datetime') + '</td><td><b>' + id + '</b></td><td>' + sevTag(e.s) + '</td><td>' + escH(e.n) + '</td><td class="desc">' + escH(e.d) +
             '</td><td><span class="est" data-s="' + (ev ? 4 : 0) + '">' + (ev ? 'ACTIVA' : 'RESUELTA') + '</span></td><td>' + (ev ? '' : dur(todos['h' + i + 'd'])) + '</td><td>' + (ev ? num2(todos['h' + i + 'v']) : '') + '</td></tr>';
      }
      if (!n) h += '<tr><td colspan="8" class="dim">SIN EVENTOS</td></tr>';
      el.innerHTML = h + '</tbody>';
    } else {                                                   // contadores
      h = '<thead><tr><th>ID</th><th>SEV</th><th>NOMBRE</th><th>DESCRIPCION</th><th>CONDICION</th><th>ESTADO</th><th>CONTADOR</th><th>ULTIMA VEZ</th><th></th></tr></thead><tbody>';
      E.forEach(function (e, i) {
        var on = (act >>> i) & 1, c = todos['ec' + i] || 0, t = todos['el' + i] || 0;
        h += '<tr data-sev="' + e.s + '"><td><b>' + e.i + '</b></td><td>' + sevTag(e.s) + '</td><td>' + escH(e.n) + '</td><td class="desc">' + escH(e.d) + '</td><td><code>' + escH(e.c) + '</code>' + (e.t ? ' <span class="dim">&gt; ' + e.t + ' s</span>' : '') +
             '</td><td><span class="est" data-s="' + (on ? 4 : 0) + '">' + (on ? 'ACTIVO' : 'OK') + '</span></td><td>' + c + '</td><td>' + (t ? fecha(t, tz, 'datetime') : '--') +
             '</td><td><button class="btn fino" data-cmd="err_test" data-arg="' + e.i + '" title="Simula este error 8 s (no toca la maquina)">PROBAR</button></td></tr>';
      });
      el.innerHTML = h + '</tbody>';
    }
  });

  widget('data-signals', function (el, d, todos) {
    if (el._built) return;
    var bus = attr(el, 'data-bus', 0), S = catalogo('/api/senales?bus=' + bus), p = el.getAttribute('data-signals');
    if (!S) return;
    el._built = true;
    var h = '<thead><tr><th>SEÑAL</th><th>DESCRIPCION</th><th>TRAMA</th><th>DIR</th><th>VALOR</th><th>EDAD s</th><th>FORZAR</th><th>VALOR FORZADO</th></tr></thead><tbody>';
    S.forEach(function (g, j) {
      h += '<tr><td><b>' + escH(g.n) + '</b></td><td class="desc">' + escH(g.d) + '</td><td>0x' + g.i.toString(16).toUpperCase() + '</td><td>' + (g.r ? 'RX' : 'TX') + '</td>' +
           '<td><span data-v="' + p + 'v' + j + '"></span> <span class="dim">' + escH(g.u) + '</span></td><td>' + (g.r ? '<span data-v="' + p + 'a' + j + '"></span>' : '') + '</td>' +
           '<td><input type="checkbox" data-set="' + p + 'f' + j + '" aria-label="forzar"></td>' +
           '<td><input type="number" step="any" class="num-in" data-set="' + p + 'x' + j + '" aria-label="valor forzado"></td></tr>';
    });
    if (!S.length) h += '<tr><td colspan="8" class="dim">No hay señales configuradas (web_sig_cfg.h)</td></tr>';
    el.innerHTML = h + '</tbody>';
    repintar2();
  });
  function repintar2() { widgets.forEach(function (w) { document.querySelectorAll('[' + w.attr + ']').forEach(function (e) { if (w.attr !== 'data-signals') w.fn(e, all, all); }); }); }

  // ---------- mensajes de la placa ----------
  function aplicar(d) {
    if (d._t !== undefined) { document.querySelectorAll('[data-terminal]').forEach(function (t) { if (t._linea) t._linea(d._t); }); return; }
    if (d._r !== undefined) { aviso(d._r); return; }
    for (var k in d) all[k] = d[k];
    widgets.forEach(function (w) { document.querySelectorAll('[' + w.attr + ']').forEach(function (el) { w.fn(el, d, all); }); });
    if (window.onVars) window.onVars(d);
  }

  // ---------- conexion ----------
  var path = document.body.getAttribute('data-ws'), estado = document.querySelector('[data-estado]');
  function msg(t) { if (estado) estado.textContent = t; document.body.setAttribute('data-con', t === 'conectado' ? '1' : '0'); }
  function conectar() {
    ws = new WebSocket((location.protocol === 'https:' ? 'wss://' : 'ws://') + location.host + path);
    ws.onopen = function () { msg('conectado'); };
    ws.onclose = function () { msg('desconectado, reintentando...'); setTimeout(conectar, 2000); };
    ws.onmessage = function (e) { var d; try { d = JSON.parse(e.data); } catch (x) { return; } aplicar(d); };
  }

  // ---------- titulos en puntos + arranque ----------
  function titulos() {
    var pit = window.innerWidth < 480 ? 4 : 6;
    document.querySelectorAll('.cont h1').forEach(function (h) {
      if (!h._t) h._t = h.textContent;
      h.setAttribute('aria-label', h._t); h.textContent = '';
      var cv = document.createElement('canvas'); cv.setAttribute('data-title', ''); h.appendChild(cv);
      puntos(cv, h._t, pit, css('--fg', '#fff'));
    });
  }
  function arrancar() {
    titulos();
    document.querySelectorAll('[data-dottext]').forEach(function (el) { puntos(el, el.getAttribute('data-dottext'), attr(el, 'data-pitch', 5), colorDe(el)); });
    ['data-pid', 'data-terminal', 'data-upload', 'data-table', 'data-bits', 'data-steps', 'data-alarmlog'].forEach(function (a) {
      widgets.forEach(function (w) { if (w.attr === a) document.querySelectorAll('[' + a + ']').forEach(function (el) { w.fn(el, {}, all); }); });
    });
    document.querySelectorAll('[data-explain]').forEach(function (c) { var h = c.querySelector('[data-tip]'); if (h) { h.setAttribute('tabindex', 0); c.querySelectorAll('[data-tip]').forEach(function (x) { x.setAttribute('tabindex', 0); }); seleccionarTip(h); } });
    document.querySelectorAll('[data-tabs]').forEach(function (c) { var b = c.querySelector('[data-tab]'); if (b) seleccionarTab(b); });
    if (path) conectar();
  }
  var rt; window.addEventListener('resize', function () { clearTimeout(rt); rt = setTimeout(titulos, 200); });
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', arrancar); else arrancar();

  return { widget: widget, valores: all, enviar: enviar, puntos: puntos };
})();
