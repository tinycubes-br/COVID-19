setupMap();

updateDrawColors();

initFilter();

// Recebimento de parametros pela URL
let query = location.search.slice(1);
let partes = query.split('&');
let params = {};
partes.forEach(function (parte) {
	let chaveValor = parte.split('=');
	let chave = chaveValor[0];
	let valor = chaveValor[1];
  params[chave] = valor;
});

initChartLeft();
initChartRight();
initChartBottom();

// Faz a consulta por Schema e as duas por Bounds
initSchema(params);

/**
 * Solicita o schema e os limites geográficos e temporais.
 */
function initLoop() {
	setTimeout(function () {
		let s = getGlobal("schema");
		let t = getGlobal("bounds_time");
		let g = getGlobal("bounds_geo");
		if (s.value && t.value && g.value) {
			request2HeatMap();
			requestFilter2ChartBottom("Map", "#AAAAAA");
			requestMap2ChartLeft();
			requestMap2ChartRight();
		}	else {
			initLoop();			
		}
	}, 1000);
}

initLoop();

$('#start').val(moment.unix(getGlobal("ts_t0").value).format("MM/DD/YYYY"));
$('#end').val(moment.unix(getGlobal("ts_t1").value).format("MM/DD/YYYY"));