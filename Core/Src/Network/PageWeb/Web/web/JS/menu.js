

(function () {

var PAGINAS = [
    ['/index.html' , 'Inicio'],
    ['/datos.html' ,  'Datos'],
    ['/config.html' , 'Configuración'],
    ['/sistema.html' , 'Sistema'],
    ['/widgets.html' , 'Widgets'],
    ['/industria.html' , 'Industria'],
    ['/interno.html' , 'Interno'],
    ['/hvac.html' , 'HVAC'],
    ['/compon.html' , 'Componentes'],
    ['/objeto.html' , 'Objeto'],
    ['/wdatos.html' , 'Widget Datos'],
    ['/can.html' , 'CAN'],
    ['/trdp.html' , 'TRDP'],
    ['/errores.html' , 'Errores'],
    ['/stm32.html' , 'STM32'],
]

 var paginaActual = location.pathname;
 if (paginaActual == '/') {paginaActual = '/index.html';};

 //HTML -->
 var html = '<button class="burger" onclick="document.getElementById(\'menu\').classList.toggle(\'abierto\')">&#9776;</button>';
     html += '<nav class="menu" id="menu">';
     html += '<div class="logo">WEB SYSTEM STM32</div>';

 PAGINAS.forEach(function (pagina) {
    var paginaActualCSS = (pagina[0] === paginaActual) ? ' class="act"' : '';
    html += '<a' + paginaActualCSS + ' href="' + pagina[0] + '">' + pagina[1] + '</a>';
  });

  html += '</nav>';
  document.currentScript.insertAdjacentHTML('beforebegin', html);
})();