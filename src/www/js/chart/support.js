/**
 * 
 * Operações sobre o gráfico.
 * 
 */

let chartLeft;
let chartRight;
let chartBottom;
let listBairroClick = [];
let listPoly = [];

/**
 * Difine o intervalo de Y no gráfico inferior.
 * @param {*} mn 
 * @param {*} mx 
 */
function compute_best_unity(mn, mx) {
  let div = 1;
  let potencia = 0;
  for (let i = 0; i < 20; i++) {
    if (mx/div < 100) {
      break;
    }
    potencia ++;
    div *= 10;
  }
  potencia -= potencia % 3;
  div = 1; for (let i=0;i<potencia;i++) div *= 10;
  
  let prefixos = [ "", "K x ", "M x ", "G x ", "T x ", "E x ", "P x " ];
  let prefixo = prefixos[potencia/3];

  let res = { prefix: prefixo, div: div };
  
  // logaritmica ou nao
  if (mn && mx/(mn+1) > 1000) {
    res.log = 1;
  } else {
    res.log = 0;
  }

  return res;
}

/**
 * Formata o gráfico da esquerda.
 */
function initChartLeft() {
  $('#chartLeftContainer').height(window.innerHeight - 300);
  $('#chartLeft').height(window.innerHeight - 350);

  chartLeft = new Bar('chartLeft');
  chartLeft.addLabel('Recovered');
  chartLeft.addLabel('Death');
  chartLeft.addLabel('Active');
  chartLeft.addLabel('Total');
  let backgroundColor = [
    '#AAAAAA',
    '#AAAAAA',
    '#AAAAAA',
    '#AAAAAA'
  ];
  let borderColor = [
    '#AAAAAA',
    '#AAAAAA',
    '#AAAAAA',
    '#AAAAAA'
  ];
  let data = [0,0,0,0];
  chartLeft.addDataset('Map', data, backgroundColor, borderColor);
}

/**
 * Formata o gráfico da direita.
 */
function initChartRight() {
  $('#chartLeftContainer').height(window.innerHeight - 300);
  $('#chartRight').height(window.innerHeight - 350);

  chartRight = new Line('chartRight');
  chartRight.setTicksY({ min: -1, max: 1 });
  chartRight.setTypeX('linear');
  chartRight.setLabelY("Correlation");
  chartRight.setLabelX("Offset");

  let xy = [];
  chartRight.setLabels([]);

  xy.push({x: 1, y: 0.5});
  chartRight.addLabel(1);

  xy.push({x: 2, y: 0.55});
  chartRight.addLabel(2);

  xy.push({x: 3, y: 0.55});
  chartRight.addLabel(3);

  xy.push({x: 4, y: 0.5});
  chartRight.addLabel(4);

  xy.push({x: 5, y: 0.5});
  chartRight.addLabel(5);

  xy.push({x: 6, y: 0.55});
  chartRight.addLabel(6);

  xy.push({x: 7, y: 0.6});
  chartRight.addLabel(7);

  xy.push({x: 8, y: 0.6});
  chartRight.addLabel(8);

  xy.push({x: 9, y: 0.65});
  chartRight.addLabel(9);

  xy.push({x: 10, y: 0.65});
  chartRight.addLabel(10);

  chartRight.removeDataset("Map");
  chartRight.addDataset("Map", xy, "#AAAAAA");
}

/**
 * Apresenta o gráfico inferior.
 */
function initChartBottom() {
  chartBottom = new Line('chartBottom');
}

/**
 * Retorna o gráfico escolhido.
 * @param {*} position 
 */
function getChart(position) {
  if (position == 'left') {
    return chartLeft;
  } else if (position == 'right') {
    return chartRight;
  } else if (position == 'bottom') {
    return chartBottom;
  } else {
    return null;
  }
}

/**
 * Plota os dados do Mapa no gráfico da esquerda.
 * @param {*} responseData 
 */
function drawMapChartLeft(responseData) { 
  let recuperado = 0;
  let obito = 0;
  let ativo = 0;
  for (let i=0; i<responseData.result.length; i++) {
    let k = responseData.result[i].k[0];
    let v = responseData.result[i].v[0];
    if (k == 1) {
      recuperado = v;
    } else if (k == 2) {
      obito = v;
    } else if (k == 6) {
      ativo = v;
    }
  }

  chartLeft = getChart('left');

//  $('#chartLeft').remove();
//  $('#chartLeftContainer').append('<canvas class="col-12" id="chartLeft"><canvas>');  
//  $('#chartLeft').height(window.innerHeight - 300);
  
  let backgroundColor = [
    '#AAAAAA',
    '#AAAAAA',
    '#AAAAAA',
    '#AAAAAA'
  ];
  let borderColor = [
    '#AAAAAA',
    '#AAAAAA',
    '#AAAAAA',
    '#AAAAAA'
  ];
  let data = [recuperado, obito, ativo, (recuperado + obito + ativo)];
  chartLeft.removeDataset('Map');
  chartLeft.addDataset('Map', data, backgroundColor, borderColor);
}

/**
 * Plota os dados do Mapa no gráfico da direita.
 * @param {*} responseData 
 */
function drawMapChartRight(responseData) { 
  chartRight.setLabels([]);
  let xy = [];
  for (let i=0; i<responseData.result.length; i++) {
    chartRight.addLabel(responseData.result[i][0]);
    xy.push({x: responseData.result[i][0], y: responseData.result[i][1]});
  }
  chartRight.removeDataset('Map');
  chartRight.addDataset('Map', xy, '#AAAAAA'); 
}

/**
 * Plota os dados do Filtro no gráfico da esquerda.
 * @param {*} responseData 
 */
function drawFilterChartLeft(responseData) { 
  let recuperado = 0;
  let obito = 0;
  let ativo = 0;
  for (let i=0; i<responseData.result.length; i++) {
    let k = responseData.result[i].k[0];
    let v = responseData.result[i].v[0];
    if (k == 1) {
      recuperado = v;
    } else if (k == 2) {
      obito = v;
    } else if (k == 6) {
      ativo = v;
    }
  }

  chartLeft = getChart('left');  

//  $('#chartLeft').remove();
//  $('#chartLeftContainer').append('<canvas class="col-12" id="chartLeft"><canvas>');  
//  $('#chartLeft').height(window.innerHeight - 300);
  
  let backgroundColor = [
    '#606060',
    '#606060',
    '#606060',
    '#606060'
  ];
  let borderColor = [
    '#606060',
    '#606060',
    '#606060',
    '#606060'
  ];
  let data = [recuperado, obito, ativo, (recuperado + obito + ativo)];
  chartLeft.removeDataset('Filter');
  chartLeft.addDataset('Filter', data, backgroundColor, borderColor);
}

/**
 * Plota os dados do Filtro no gráfico da direita.
 * @param {*} responseData 
 */
function drawFilterChartRight(responseData) { 
  chartRight.setLabels([]);
  let xy = [];
  for (let i=0; i<responseData.result.length; i++) {
    chartRight.addLabel(responseData.result[i][0]);
    xy.push({x: responseData.result[i][0], y: responseData.result[i][1]});
  }
  chartRight.removeDataset('Filter');
  chartRight.addDataset('Filter', xy, '#606060'); 
}

/**
 * Atualiza o grafico da esquerda após remoção de bairro.
 */
function refreshBairroChartLeft() {
  chartLeft = getChart('left');  

  let backgroundColor = [
    '#AA0000',
    '#AA0000',
    '#AA0000',
    '#AA0000'
  ];
  let borderColor = [
    '#AA0000',
    '#AA0000',
    '#AA0000',
    '#AA0000'
  ];
  let listData = listBairroClick;
  let r = 0;
  let o = 0;
  let a = 0;
  let t = 0;
  for (let i=0; i<listData.length; i++) {
    r = r + listData[i].covid[0];
    o = o + listData[i].covid[1];
    a = a + listData[i].covid[2];
    t = t + listData[i].covid[3];
  }
  let data = [r, o, a, t];
  chartLeft.removeDataset('Neighborhood');
  if (listData.length > 0) {
    chartLeft.addDataset('Neighborhood', data, backgroundColor, borderColor);  
  }
}

/**
 * Atualiza o grafico da direta após remoção de bairro.
 */
function refreshBairroChartRight() {
  console.log("refreshBairroChartRight");
}

/**
 * Remove bairro da lista.
 * @param {*} layer 
 */
function removeBairroClick(codBairro) {
  for (let i=0; i<listBairroClick.length; i++) {
    if (listBairroClick[i].codigo == codBairro) {
      listBairroClick.splice(i, 1);
      break;
    }
  }  
}

/**
 * Adiciona bairro a lista.
 * @param {*} codBairro 
 * @param {*} data 
 */
function addBairroClick(codBairro, dataCovid) {
  let achou = false;
  for (let i=0; i<listBairroClick.length; i++) {
    if (listBairroClick[i].codigo == codBairro) {
      listBairroClick[i].covid = dataCovid;
      achou = true;
      break;
    }
  }  
  if (!achou) {
    let bairro = { 
      codigo: codBairro,
      covid: dataCovid
    };
    listBairroClick.push(bairro);
  }
  return listBairroClick;
}

/**
 * Plota os dados dos Bairros no gráfico da esquerda.
 * @param {*} responseData 
 */
function drawBairroChartLeft(codBairro, responseData) { 
  let recuperado = 0;
  let obito = 0;
  let ativo = 0;
  for (let i=0; i<responseData.result.length; i++) {
    let k = responseData.result[i].k[0];
    let v = responseData.result[i].v[0];
    if (k == 1) {
      recuperado = v;
    } else if (k == 2) {
      obito = v;
    } else if (k == 6) {
      ativo = v;
    }
  }

  chartLeft = getChart('left');  

//  $('#chartLeft').remove();
//  $('#chartLeftContainer').append('<canvas class="col-12" id="chartLeft"><canvas>');  
//  $('#chartLeft').height(window.innerHeight - 300);
  
  let id = '#idBairro' + codBairro + 'Recuperado';
  $(id).text(recuperado);
  id = '#idBairro' + codBairro + 'Obito';
  $(id).text(obito);
  id = '#idBairro' + codBairro + 'Ativo';
  $(id).text(ativo);
  id = '#idBairro' + codBairro + 'Total';
  const total = recuperado + obito + ativo;
  $(id).text(total);
  setCovidBairro(codBairro, recuperado, obito, ativo);

  let backgroundColor = [
    '#AA0000',
    '#AA0000',
    '#AA0000',
    '#AA0000'
  ];
  let borderColor = [
    '#AA0000',
    '#AA0000',
    '#AA0000',
    '#AA0000'
  ];
  let listData = addBairroClick(codBairro, [recuperado, obito, ativo, (recuperado + obito + ativo)]);
  let r = 0;
  let o = 0;
  let a = 0;
  let t = 0;
  for (let i=0; i<listData.length; i++) {
    r = r + listData[i].covid[0];
    o = o + listData[i].covid[1];
    a = a + listData[i].covid[2];
    t = t + listData[i].covid[3];
  }
  let data = [r, o, a, t];
  chartLeft.removeDataset('Neighborhood');
  chartLeft.addDataset('Neighborhood', data, backgroundColor, borderColor);
}

/**
 * Plota os dados dos Bairros no gráfico da direita.
 * @param {*} responseData 
 */
function drawBairroChartRight(codBairro, responseData) { 
  chartRight.setLabels([]);
  let xy = [];
  for (let i=0; i<responseData.result.length; i++) {
    chartRight.addLabel(responseData.result[i][0]);
    xy.push({x: responseData.result[i][0], y: responseData.result[i][1]});
  }
  chartRight.removeDataset('Neighborhood');
  chartRight.addDataset('Neighborhood', xy, '#AA0000'); 
}

/**
 * Remove layer da lista.
 * @param {*} layer 
 */
function removePoly(layer) {
  for (let i=0; i<listPoly.length; i++) {
    if ((((layer instanceof L.Rectangle) &&
        (listPoly[i].layer instanceof L.Rectangle)) ||
        ((layer instanceof L.Polygon) &&
        (listPoly[i].layer instanceof L.Polygon))) &&
        (layer.options.color == listPoly[i].layer.options.color)) {
      listPoly.splice(i, 1);
      break;
    }
  }
}

/**
 * Adiciona layer a lista.
 * @param {*} layer 
 * @param {*} data 
 */
function addPoly(layer, dataCovid) {
  let achou = false;
  for (let i=0; i<listPoly.length; i++) {
    if ((((layer instanceof L.Rectangle) &&
        (listPoly[i].layer instanceof L.Rectangle)) ||
        ((layer instanceof L.Polygon) &&
        (listPoly[i].layer instanceof L.Polygon))) &&
        (layer.options.color == listPoly[i].layer.options.color)) {
      listPoly[i].covid = dataCovid;
      achou = true;
      break;
    }
  }  
  if (!achou) {
    let poly = { 
      layer: layer,
      covid: dataCovid
    };
    listPoly.push(poly);
  }
  return listPoly;
}

/**
 * Plota os dados das Geometrias no gráfico da esquerda.
 * @param {*} responseData 
 */
function drawPolyChartLeft(layer, responseData) { 
  let recuperado = 0;
  let obito = 0;
  let ativo = 0;
  for (let i=0; i<responseData.result.length; i++) {
    let k = responseData.result[i].k[0];
    let v = responseData.result[i].v[0];
    if (k == 1) {
      recuperado = v;
    } else if (k == 2) {
      obito = v;
    } else if (k == 6) {
      ativo = v;
    }
  }

  chartLeft = getChart('left');  

//  $('#chartLeft').remove();
//  $('#chartLeftContainer').append('<canvas class="col-12" id="chartLeft"><canvas>');  
//  $('#chartLeft').height(window.innerHeight - 300);
  
  let backgroundColor = [
    '#00AA00',
    '#00AA00',
    '#00AA00',
    '#00AA00'
  ];
  let borderColor = [
    '#00AA00',
    '#00AA00',
    '#00AA00',
    '#00AA00'
  ];
  let listData = addPoly(layer, [recuperado, obito, ativo, (recuperado + obito + ativo)]);
  let r = 0;
  let o = 0;
  let a = 0;
  let t = 0;
  for (let i=0; i<listData.length; i++) {
    r = r + listData[i].covid[0];
    o = o + listData[i].covid[1];
    a = a + listData[i].covid[2];
    t = t + listData[i].covid[3];
  }
  let data = [r, o, a, t];
  chartLeft.removeDataset('Geometries');
  chartLeft.addDataset('Geometries', data, backgroundColor, borderColor);
}

/**
 * Plota os dados das Geometrias no gráfico da direita.
 * @param {*} responseData 
 */
function drawPolyChartRight(layer, responseData) { 
  chartRight.setLabels([]);
  let xy = [];
  for (let i=0; i<responseData.result.length; i++) {
    chartRight.addLabel(responseData.result[i][0]);
    xy.push({x: responseData.result[i][0], y: responseData.result[i][1]});
  }
  chartRight.removeDataset('Geometries');
  chartRight.addDataset('Geometries', xy, '#00AA00'); 
}

/**
 * Atualiza o grafico da esquerda após remoção de layer.
 */
function refreshPolyChartLeft() {
  chartLeft = getChart('left');  

  let backgroundColor = [
    '#00AA00',
    '#00AA00',
    '#00AA00',
    '#00AA00'
  ];
  let borderColor = [
    '#00AA00',
    '#00AA00',
    '#00AA00',
    '#00AA00'
  ];
  let listData = listPoly;
  let r = 0;
  let o = 0;
  let a = 0;
  let t = 0;
  for (let i=0; i<listData.length; i++) {
    r = r + listData[i].covid[0];
    o = o + listData[i].covid[1];
    a = a + listData[i].covid[2];
    t = t + listData[i].covid[3];
  }
  let data = [r, o, a, t];
  chartLeft.removeDataset('Geometries');
  if (listData.length > 0) {
    chartLeft.addDataset('Geometries', data, backgroundColor, borderColor);  
  }
}

/**
 * Atualiza o grafico da direta após remoção de layer.
 */
function refreshPolyChartRight() {
  console.log("refreshPolyChartRight");
}

/**
 * Remove um poligono do gráfico inferior.
 * @param {*} layer 
 */
function removePolyInChartBottom(layer) {
  let datasetLabel = getLayerType(layer);
  let datasetColor = layer.options.color;
  chartBottom = getChart('bottom');
  chartBottom.removePolyDataset(datasetLabel, datasetColor);
}

/**
 * Atualiza grafico da direita após mudança temporal.
 */
function updatePoly() {
  for (let i=0; i<listPoly.length; i++) {
    let layer = listPoly[i].layer;
    removePoly(layer);
    requestPoly2ChartLeft(layer);
    requestPoly2ChartRight(layer);
  }
}

/**
 * Atualiza grafico da direita após mudança temporal.
 */
function updateBairro() {
  for (let i=0; i<listBairroClick.length; i++) {
    let codBairro = listBairroClick[i].codigo;
    removeBairroClick(codBairro);
    requestBairro2ChartLeft(codBairro);  
    requestBairro2ChartRight(codBairro);  
  }
}