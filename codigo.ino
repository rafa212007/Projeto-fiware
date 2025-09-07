/*
Autores adicionais nos comentários:
- Enzo Hort Ramos
- Rafael Augusto Carmona
- Eduardo Tolentino
*/

//Autor: Fábio Henrique Cabrini
//Resumo: Esse programa possibilita ligar e desligar o led onboard, além de mandar o status para o Broker MQTT possibilitando o Helix saber
//se o led está ligado ou desligado.
//Revisões:
//Rev1: 26-08-2023 Código portado para o ESP32 e para realizar a leitura de luminosidade e publicar o valor em um tópico apropriado do broker 
//Autor Rev1: Lucas Demetrius Augusto 
//Rev2: 28-08-2023 Ajustes para o funcionamento no FIWARE Descomplicado
//Autor Rev2: Fábio Henrique Cabrini
//Rev3: 1-11-2023 Refinamento do código e ajustes para o funcionamento no FIWARE Descomplicado
//Autor Rev3: Fábio Henrique Cabrini
//link da simulação wokwi: https://wokwi.com/projects/441203962116738049

#include <WiFi.h>           // Biblioteca para conexão Wi-Fi
#include <PubSubClient.h>   // Biblioteca para MQTT

// Configurações - variáveis editáveis
const char* default_SSID = "Wokwi-GUEST";         // Nome da rede Wi-Fi
const char* default_PASSWORD = "";                // Senha da rede Wi-Fi
const char* default_BROKER_MQTT = "20.116.216.196";// IP do Broker MQTT
const int default_BROKER_PORT = 1883;             // Porta do Broker MQTT
const char* default_TOPICO_SUBSCRIBE = "/TEF/lamp001/cmd";     // Tópico MQTT para escuta
const char* default_TOPICO_PUBLISH_1 = "/TEF/lamp001/attrs";   // Tópico para enviar status do LED
const char* default_TOPICO_PUBLISH_2 = "/TEF/lamp001/attrs/l"; // Tópico para enviar luminosidade
const char* default_ID_MQTT = "fiware_001";       // ID de identificação MQTT
const int default_D4 = 2;                         // Pino do LED onboard

// Prefixo do tópico (ajuda a padronizar as mensagens)
const char* topicPrefix = "lamp001";

// Variáveis para configurações (podem ser alteradas em tempo de execução se necessário)
char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
int D4 = default_D4;

// Objetos necessários para comunicação
WiFiClient espClient;      
PubSubClient MQTT(espClient);

// Variável que guarda o estado atual do LED
char EstadoSaida = '0';

// Função para inicializar o Serial Monitor
void initSerial() {
    Serial.begin(115200);
}

// Função para conectar no Wi-Fi
void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconectWiFi(); // Tenta reconectar caso não esteja conectado
}

// Função para configurar o cliente MQTT
void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   // Define broker e porta
    MQTT.setCallback(mqtt_callback);            // Define a função de callback para receber mensagens
}

// Setup inicial do ESP32
void setup() {
    InitOutput();   // Inicializa LED com efeito de "pisca"
    initSerial();   // Inicia comunicação serial
    initWiFi();     // Conecta ao Wi-Fi
    initMQTT();     // Configura conexão MQTT
    delay(5000);
    MQTT.publish(TOPICO_PUBLISH_1, "s|on"); // Envia status inicial ao broker
}

// Loop principal
void loop() {
    VerificaConexoesWiFIEMQTT(); // Mantém conexões ativas
    EnviaEstadoOutputMQTT();     // Envia status do LED
    handleLuminosity();          // Lê luminosidade e envia ao broker
    MQTT.loop();                 // Mantém o cliente MQTT ativo
}

// Reconexão Wi-Fi se necessário
void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED)
        return;
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());

    // Garante que o LED inicie desligado
    digitalWrite(D4, LOW);
}

// Função callback: executa quando uma mensagem chega via MQTT
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        char c = (char)payload[i];
        msg += c;
    }
    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);

    // Monta padrões de mensagens esperadas
    String onTopic = String(topicPrefix) + "@on|";
    String offTopic = String(topicPrefix) + "@off|";

    // Liga LED
    if (msg.equals(onTopic)) {
        digitalWrite(D4, HIGH);
        EstadoSaida = '1';
    }

    // Desliga LED
    if (msg.equals(offTopic)) {
        digitalWrite(D4, LOW);
        EstadoSaida = '0';
    }
}

// Verifica e mantém conexões Wi-Fi e MQTT
void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected())
        reconnectMQTT();
    reconectWiFi();
}

// Publica no broker o estado atual do LED
void EnviaEstadoOutputMQTT() {
    if (EstadoSaida == '1') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|on");
        Serial.println("- Led Ligado");
    }

    if (EstadoSaida == '0') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|off");
        Serial.println("- Led Desligado");
    }
    Serial.println("- Estado do LED onboard enviado ao broker!");
    delay(1000);
}

// Inicializa o LED com pisca para indicar inicialização
void InitOutput() {
    pinMode(D4, OUTPUT);
    digitalWrite(D4, HIGH);
    boolean toggle = false;

    for (int i = 0; i <= 10; i++) {
        toggle = !toggle;
        digitalWrite(D4, toggle);
        delay(200);
    }
}

// Tenta reconectar ao broker MQTT caso desconecte
void reconnectMQTT() {
    while (!MQTT.connected()) {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); // Se conecta ao tópico
        } else {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Haverá nova tentativa de conexão em 2s");
            delay(2000);
        }
    }
}

// Lê luminosidade pelo pino analógico e publica no broker
void handleLuminosity() {
    const int potPin = 34; // Pino analógico para leitura do sensor
    int sensorValue = analogRead(potPin); 
    int luminosity = map(sensorValue, 0, 4095, 0, 100); // Normaliza para 0–100%
    String mensagem = String(luminosity);
    Serial.print("Valor da luminosidade: ");
    Serial.println(mensagem.c_str());
    MQTT.publish(TOPICO_PUBLISH_2, mensagem.c_str()); // Publica valor
}
