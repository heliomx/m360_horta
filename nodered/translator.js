// ======================
// MySensors Decoder 2.3.x
// ======================

const COMMANDS = {
    0: "PRESENTATION",
    1: "SET",
    2: "REQ",
    3: "INTERNAL",
    4: "STREAM"
};

const PRESENTATION = {
    0:"S_DOOR",1:"S_MOTION",2:"S_SMOKE",3:"S_BINARY",4:"S_DIMMER",
    5:"S_COVER",6:"S_TEMP",7:"S_HUM",8:"S_BARO",9:"S_WIND",
    10:"S_RAIN",11:"S_UV",12:"S_WEIGHT",13:"S_POWER",14:"S_HEATER",
    15:"S_DISTANCE",16:"S_LIGHT_LEVEL",17:"S_ARDUINO_NODE",
    18:"S_ARDUINO_REPEATER_NODE",19:"S_LOCK",20:"S_IR",21:"S_WATER",
    22:"S_AIR_QUALITY",23:"S_CUSTOM",24:"S_DUST",
    25:"S_SCENE_CONTROLLER",26:"S_RGB_LIGHT",27:"S_RGBW_LIGHT",
    28:"S_COLOR_SENSOR",29:"S_HVAC",30:"S_MULTIMETER",
    31:"S_SPRINKLER",32:"S_WATER_LEAK",33:"S_SOUND",
    34:"S_VIBRATION",35:"S_MOISTURE",36:"S_INFO",
    37:"S_GAS",38:"S_GPS",39:"S_WATER_QUALITY"
};

const VARIABLES = {
    0:["V_TEMP","Temperatura","°C"],
    1:["V_HUM","Umidade","%"],
    2:["V_STATUS","Estado",""],
    3:["V_PERCENTAGE","Percentual","%"],
    4:["V_PRESSURE","Pressão","hPa"],
    5:["V_FORECAST","Previsão",""],
    6:["V_RAIN","Chuva","mm"],
    7:["V_RAINRATE","Taxa de chuva","mm/h"],
    8:["V_WIND","Vento","m/s"],
    9:["V_GUST","Rajada","m/s"],
    10:["V_DIRECTION","Direção","°"],
    11:["V_UV","UV",""],
    12:["V_WEIGHT","Peso","kg"],
    13:["V_DISTANCE","Distância","cm"],
    14:["V_IMPEDANCE","Impedância","ohm"],
    15:["V_ARMED","Armado",""],
    16:["V_TRIPPED","Disparado",""],
    17:["V_WATT","Potência","W"],
    18:["V_KWH","Energia","kWh"],
    19:["V_SCENE_ON","Cena ON",""],
    20:["V_SCENE_OFF","Cena OFF",""],
    21:["V_HVAC_FLOW_STATE","HVAC Estado",""],
    22:["V_HVAC_SPEED","HVAC Velocidade",""],
    23:["V_LIGHT_LEVEL","Luminosidade","%"],
    24:["V_VAR1","VAR1",""],
    25:["V_VAR2","VAR2",""],
    26:["V_VAR3","VAR3",""],
    27:["V_VAR4","VAR4",""],
    28:["V_VAR5","VAR5",""],
    29:["V_UP","Subir",""],
    30:["V_DOWN","Descer",""],
    31:["V_STOP","Parar",""],
    32:["V_IR_SEND","IR Enviar",""],
    33:["V_IR_RECEIVE","IR Recebido",""],
    34:["V_FLOW","Vazão","L/min"],
    35:["V_VOLUME","Volume","L"],
    36:["V_LOCK_STATUS","Fechadura",""],
    37:["V_LEVEL","Nível",""],
    38:["V_VOLTAGE","Tensão","V"],
    39:["V_CURRENT","Corrente","A"],
    40:["V_RGB","RGB",""],
    41:["V_RGBW","RGBW",""],
    42:["V_ID","ID",""],
    43:["V_UNIT_PREFIX","Prefixo",""],
    44:["V_HVAC_SETPOINT_COOL","Setpoint Frio","°C"],
    45:["V_HVAC_SETPOINT_HEAT","Setpoint Quente","°C"],
    46:["V_HVAC_FLOW_MODE","Modo HVAC",""],
    47:["V_TEXT","Texto",""],
    48:["V_CUSTOM","Custom",""],
    49:["V_POSITION","Posição",""],
    50:["V_IR_RECORD","IR Gravado",""],
    51:["V_PH","pH",""],
    52:["V_ORP","ORP","mV"],
    53:["V_EC","Condutividade","µS/cm"],
    54:["V_VAR","Variável",""],
    55:["V_VA","Potência Aparente","VA"],
    56:["V_POWER_FACTOR","Fator de Potência",""]
};

const INTERNAL = {
    0:"I_BATTERY_LEVEL",
    1:"I_TIME",
    2:"I_VERSION",
    3:"I_ID_REQUEST",
    4:"I_ID_RESPONSE",
    5:"I_INCLUSION_MODE",
    6:"I_CONFIG",
    7:"I_FIND_PARENT",
    8:"I_FIND_PARENT_RESPONSE",
    9:"I_LOG_MESSAGE",
    10:"I_CHILDREN",
    11:"I_SKETCH_NAME",
    12:"I_SKETCH_VERSION",
    13:"I_REBOOT",
    14:"I_GATEWAY_READY",
    15:"I_SIGNING_PRESENTATION",
    16:"I_NONCE_REQUEST",
    17:"I_NONCE_RESPONSE",
    18:"I_HEARTBEAT_REQUEST",
    19:"I_PRESENTATION",
    20:"I_DISCOVER_REQUEST",
    21:"I_DISCOVER_RESPONSE",
    22:"I_HEARTBEAT_RESPONSE",
    23:"I_LOCKED",
    24:"I_PING",
    25:"I_PONG",
    26:"I_REGISTRATION_REQUEST",
    27:"I_REGISTRATION_RESPONSE",
    28:"I_DEBUG",
    29:"I_SIGNAL_REPORT_REQUEST",
    30:"I_SIGNAL_REPORT_REVERSE",
    31:"I_SIGNAL_REPORT_RESPONSE",
    32:"I_PRE_SLEEP_NOTIFICATION",
    33:"I_POST_SLEEP_NOTIFICATION"
};

// Entrada MQTT
let m = msg.payload;

let comando = COMMANDS[m.command] || "DESCONHECIDO";
let tipo = "";
let descricao = "";
let unidade = "";

switch(m.command){

    case 0:
        tipo = PRESENTATION[m.type] || "DESCONHECIDO";
        descricao = "Apresentação de sensor";
        break;

    case 1:
    case 2:
        if(VARIABLES[m.type]){
            tipo = VARIABLES[m.type][0];
            descricao = VARIABLES[m.type][1];
            unidade = VARIABLES[m.type][2];
        }
        break;

    case 3:
        tipo = INTERNAL[m.type] || "DESCONHECIDO";
        descricao = "Mensagem interna";
        break;

    case 4:
        tipo = "STREAM";
        descricao = "Transferência de dados";
        break;
}

let valor = m.payload;

if(valor === null || valor === undefined)
    valor = "(vazio)";

let texto =
`══════════════════════════
MySensors MQTT Decoder
══════════════════════════
Nó.............: ${m.nodeId}
Sensor.........: ${m.sensorId}
Direção........: ${m.direction || "-"}

Comando........: ${comando}
Tipo...........: ${tipo}
Descrição......: ${descricao}

ACK............: ${m.ack}
Timestamp......: ${m.timestamp}

Payload........: ${valor} ${unidade}
══════════════════════════`;

msg.decoded = {
    nodeId: m.nodeId,
    sensorId: m.sensorId,
    command: comando,
    type: tipo,
    description: descricao,
    unit: unidade,
    value: valor
};

msg.payload = texto;

return msg;