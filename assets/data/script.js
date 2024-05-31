/*Eu sei que não é assim a forma correta de se documentar um arquivo JS, mas o que importa nesse momento é o meu entendimento do código */


/*********************************************************PARTE DE CONEXÃO DO SERVIDOR**********************************************/
var gateway = `ws://${window.location.hostname}/ws`; // ponto de entrada p/ websocket e obtém o ip da página atual
var websocket;

window.addEventListener('load', onload); //ouvinte de evento que chama o onload enquanto a página é carregada

function onload(event) { // função que inicia a conexão do websocket com o servidor
  initWebSocket();
  initToggleSwitch(); //parte do botão
}
function initWebSocket() {
  console.log('Trying to open a WebSocket connection…');
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getValues(); // envia mensagem para obter os valores atuais do controle deslizante
    getReadings(); //envia mensagem para obter o valor da temperatura do bico
}

function onClose(event) { //se o websocket ficar desconectado, ele conecta novamente após 2 segundos
    console.log('Connection closed');
    setTimeout(initWebSocket, 200);
}

function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);
  
    for (var i = 0; i < keys.length; i++){ //parte do controle deslizante 
      var key = keys[i];
      document.getElementById(key).innerHTML = myObj[key]; //pega as informações das chaves do json do bico e do motor
      document.getElementById("sliderValue1" .toString()).value = myObj[key]; // parte do controle do motor
    }
    //parte do acionamento do botão bico
   /*
    var estado; //parte do botão liga desliga bico
    //if (event.data == "1"){
      if (myObj.data === "toggle"){
      estado = "ON";
    }
    else{
      estado = "OFF";
    }
    document.getElementById('estado').innerHTML = estado;
    */
   /*
      // Verifica o estado do botão liga/desliga do bico
      if (key === "buttonState") {
       var estado;
        if (myObj[key] === "ON") {
           estado = "Ligado";
        } else {
            estado = "Desligado";
        }
        document.getElementById('estado').innerHTML = estado;
   }
   */
}

function getValues(){ // envia uma mensagem para o servidor para obter o valor do controle deslizante
    websocket.send("getValues");
}
function getReadings(){ // envia uma mensagem para o servidor para obter o valor do controle on off
    websocket.send("getReadings");
}

/**********************************************************FUNÇÕES PARA CONEXÃO COM O HARDWARE******************************************************************************/
function updateValue(value){ //escreve no id  da temperatura atual o valor da temperatura atual
    document.getElementById("bico").innerHTML = value;
}

function updateSliderPWM(element) {
  var sliderNumber = element.id.charAt(element.id.length-1);
  var sliderValue = document.getElementById(element.id).value;
  document.getElementById("sliderValue"+sliderNumber).innerHTML = sliderValue;
  console.log(sliderValue);
  websocket.send(sliderNumber+"s"+sliderValue.toString());
  }

function initToggleSwitch() { //funcionamento on off do botão
    const toggleSwitch = document.getElementById('toogle');

    toggleSwitch.addEventListener('change', function() {
        if (toggleSwitch.checked) {
            // Aqui você pode executar ações quando o switch estiver ligado
            console.log('Switch ligado');
        } else {
            // Aqui você pode executar ações quando o switch estiver desligado
            console.log('Switch desligado');
        }
    });
  }


  // Função para enviar a mensagem ao servidor quando o botão é pressionado
function buttonPressed() {
  websocket.send("buttonPressed");
}
  /*function updateTemperature(temperature) {
    var temperatureElement = document.getElementById('tempattbico');
    temperatureElement.innerHTML = 'Temperatura Atual: ' + temperature.toFixed(2) + ' °C';
  }

  // Função para lidar com mensagens WebSocket
  function handleMessage(event) {
    var data = JSON.parse(event.data);
    if (data.temperature_read !== undefined) {
      updateTemperature(data.temperature_read);
    }
  }
*/

/**********************************************************************************************************************************************/