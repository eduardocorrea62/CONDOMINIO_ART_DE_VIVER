//PROJETO PILOTO DE MONITORAMENTO DE NÍVEL DE CAIXA D'ÁGUA PREDIAL
//TÉCNICO RESPONSÁVEL: Eduardo R Corrêa
//CONDOMÍNIO ART DE VIVER
//Bloco_B_CISTERNA
//ArduinoOTA - Password = cisternab
//-----------------------------------------------------------------------------------------------------------------------------------
//Habilita a funcionalidade de atualização de firmware Over-The-Air (OTA) no ESP32.
#include <ArduinoOTA.h> 
//-----------------------------------------------------------------------------------------------------------------------------------
#define BLYNK_TEMPLATE_ID "TMPL2D4QwDOwY"
#define BLYNK_TEMPLATE_NAME "Condominio Art de Viver"
#define BLYNK_AUTH_TOKEN "23v2LJENbd0RXHJZ-2GJ36UKMNS_e8O1"
//-----------------------------------------------------------------------------------------------------------------------------------
//Credenciais para as redes Wi-Fi

const char* ssid = "Serviço art_2.4G";
const char* pass = "ita130poa@";

// const char* ssid = "AquaMonitor";
// const char* pass = "12111962";

//--------------------------------------------------------------------------------------------------------------------------------
// Bibliotecas utilizadas no projeto para funcionalidades diversas:
#include <Adafruit_GFX.h>          // Biblioteca para gráficos em displays, usada para trabalhar com textos, formas e imagens.
#include <Adafruit_SH110X.h>       // Biblioteca para displays OLED com driver SH110X.
#include <Adafruit_SSD1306.h>      // Biblioteca para displays OLED com driver SSD1306.
#include <BlynkSimpleEsp32.h>      // Biblioteca para integração do ESP32 com a plataforma Blynk.
#include <WiFi.h>                  // Biblioteca para gerenciamento de conexão Wi-Fi.
#include <WiFiClient.h>            // Biblioteca para conexões de cliente Wi-Fi.
#include <HTTPClient.h>            // Biblioteca para realizar requisições HTTP.
#include <UrlEncode.h>             // Biblioteca para codificação de URLs em requisições HTTP.
#include <NTPClient.h>             // Biblioteca para sincronização de hora via protocolo NTP.
#include <WiFiUDP.h>               // Biblioteca para conexões UDP, necessária para o NTPClient.
#include <WiFiClientSecure.h>      // Biblioteca para conexões seguras (SSL/TLS) via Wi-Fi.
#include <UniversalTelegramBot.h>  // Biblioteca para comunicação com o Telegram via bot.
#include <time.h>
//--------------------------------------------------------------------------------------------------------------------------------
// BOT para Alertas de Níveis
String botToken = "8111874052:AAFn_63Sh0hJUuOP4IyzeBtxNOH2dQ2n9No";
String chatIds[] = {"207223980", "975571557", "7490200680", "7700038822", "7157576413"}; // Adicione os IDs aqui
const int totalChats = sizeof(chatIds) / sizeof(chatIds[0]);
//Lista de IDs
 /* Eduardo - "207223980"      //Amábile - "5255435232"     //Prof Tiago - "975571557"
  Síndico - 7700038822      //Subsíndico - 7157576413     //Zelador - 7490200680*/
//--------------------------------------------------------------------------------------------------------------------------------------------------
// BOT para controle das mensagens da reinicialização  e reconexão dos ESP32
String botTokenWF = "7396450492:AAEaEsnI2EWWE4hltZyK86_j9MMqyQV9bUI";
String chatIdsWF[] = {"207223980", "975571557"}; // Adicione os IDs aqui
const int totalChatsWF = sizeof(chatIdsWF) / sizeof(chatIdsWF[0]);
//-----------------------------------------------------------------------------------------------------------------------------------------
// Configuração da biblioteca
WiFiClientSecure client; // Cliente seguro para comunicação HTTPS
//-----------------------------------------------------------------------------------------------------------------------------------------
// DEFINIÇÕES DE PINOS
#define pinTrigger 27
#define pinEcho 26
#define VPIN_NIVEL_PERCENTUAL V5
#define VPIN_DISTANCIA V17
#define HISTERESIS 2
//-----------------------------------------------------------------------------------------------------------------------------------------
// Flag para permitir ou bloquear envio de mensagens
bool mensagensPermitidas = false;
//-----------------------------------------------------------------------------------------------------------------------------------------
// Definições do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_SH1106G oled = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1, 400000, 100000);
//-----------------------------------------------------------------------------------------------------------------------------------------
// Variáveis para o cálculo do nível de água no tanque:
float distancia;                 // Armazena a distância medida pelo sensor ultrassônico.
float nivelPercentual;           // Nível do tanque em porcentagem, com precisão de uma casa decimal.
float emptyTankDistance = 271;    // Distância medida pelo sensor quando o tanque está vazio (em cm).
float fullTankDistance =   40;    // Distância medida pelo sensor quando o tanque está cheio (em cm).
float ultimaLeitura = 100.0;  // Valor inicial da última leitura válida
#define TOLERANCIA 2  // Tolerância em cm para oscilações aceitáveis
// float emptyTankDistance = 38;   // Medidas para teste de bancada(em cm).
// float fullTankDistance =  30;   // Medidas para teste de bancada(em cm).
//-----------------------------------------------------------------------------------------------------------------------------------------
//ALERTA do Problema na Bóia a cada 60 segundos/1 min
unsigned long lastMessageTime = 0;
const unsigned long messageInterval = 1000; // 10 segundos--- 1 minuto
//-----------------------------------------------------------------------------------------------------------------------------------------
// Flags para cada nível de alerta
bool flag05 = true;
bool flag25 = true;
bool flag50 = true;
bool flag75 = true;
bool flag100 = true;
bool boiaNormal = false;  // Variável de controle para garantir que a mensagem será enviada uma única vez quando o nível estiver normal
//-----------------------------------------------------------------------------------------------------------------------------------------
// BLYNK AUTENTICAÇÃO
char auth[] = BLYNK_AUTH_TOKEN;
BlynkTimer timer;
//-----------------------------------------------------------------------------------------------------------------------------------------
// Configuração do cliente NTP para sincronização de data e hora:
WiFiUDP ntpUDP;                                    // Objeto para comunicação via protocolo UDP.
NTPClient timeClient(ntpUDP, "pool.ntp.org",       // Cliente NTP usando o servidor "pool.ntp.org".
                     -3 * 3600,                   // Fuso horário definido para UTC-3 (Brasília).
                     60000);                      // Atualização do horário a cada 60 segundos.
//-----------------------------------------------------------------------------------------------------------------------------------------
// Variável para o status da rede Wi-Fi
String statusWiFi = "Wi-Fi: ...";
//--------------------------------------------------------------------------------------------------------------------------------
// Inicializa o bot do Telegram - Usando para envio separado de mensagem entre o administrador do sistema e o cliente
UniversalTelegramBot botToken1(botToken, client);
//--------------------------------------------------------------------------------------------------------------------------------
const char* firmwareURL = "https://eduardocorrea62.github.io/CONDOMINIO_ART_DE_VIVER/Bloco_B_CISTERNA.ino.bin";
bool firmwareUpdated = false;  // Flag global para controle de atualização de firmware
//----------------------------------------------------------------------------------------------------------------------------------------------
const char* ntpServer = "pool.ntp.org";  // Servidor NTP
const long gmtOffset_sec = -3 * 3600;   // Offset UTC-3 para o Brasil
const int daylightOffset_sec = 0;      // Sem horário de verão
unsigned long startMillis;  // Marca o momento em que o dispositivo iniciou
time_t baseTime = 0;        // Tempo base para a sincronização inicial
const char* tz = "BRT3"; // BRT é o fuso horário de Brasília (UTC-3)
String statusWiFiAnterior = "";
//-----------------------------------------------------------------------------------------------------------------------------
void setup() {
   Serial.begin(115200);

       // Conectar ao Wi-Fi
    Serial.println("Tentando conectar ao Wi-Fi...");
    WiFi.begin(ssid, pass);
    
    // Configuração do fuso horário
    configTzTime(tz, "pool.ntp.org");  // Ajuste do fuso horário e NTP
    
    startMillis = millis();

    // Tentar obter o tempo local
    struct tm timeInfo;
    if (getLocalTime(&timeInfo, 5000)) {
        Serial.println("Tempo local obtido!");
        baseTime = mktime(&timeInfo);  // Define o tempo base
    } else {
        Serial.println("Falha ao obter tempo local. Usando RTC interno.");
        // Usando o tempo atual do RTC interno se o NTP falhar
        struct tm defaultTime;
        time_t rtcTime = time(nullptr); // Obtém o tempo atual do RTC
        localtime_r(&rtcTime, &defaultTime);
        baseTime = rtcTime; // Usa o tempo do RTC como base
    }

    unsigned long startAttemptTime = millis();
    const unsigned long timeout = 7000;

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConectado ao Wi-Fi");
        enviarmensagemWiFi("O ESP32 da Cisterna B foi reinicializado com sucesso");
    } else {
        Serial.println("\nFalha ao conectar ao Wi-Fi.");
    }

    // Inicialização de outros periféricos
    Blynk.config(auth);
    sonarBegin(pinTrigger, pinEcho);
    oled.begin(0x3C, true);
    oled.clearDisplay();
    oled.display();
    arduinoOTA();

    // Configuração de temporizadores
    timer.setTimeout(5000L, habilitarMensagens);
    timer.setInterval(60000L, blynkVirtualWrite);
    timer.setInterval(5500L, verificaStatusWifi);
    // timer.setInterval(10000L, sendWiFiSignal);   // Enviar o RSSI a cada 10 segundos
    timer.setInterval(1000L, []() {
    atualizarDados();
    enviarStatusBlynk();
    });
    
    // Configuração de pinos
    pinMode(pinTrigger, OUTPUT);
    pinMode(pinEcho, INPUT);
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void loop() {
    // Verifica e reconecta o Wi-Fi, se necessário
    verificarWiFi();
    // Atualiza o display com as informações
    displayData(nivelPercentual); // Mostra nível de água e status do Wi-Fi
    // Processa as atualizações OTA
    ArduinoOTA.handle();
    // Verifica o nível de água
    checkWaterLevel();
    // Mantém a conexão com o Blynk
    Blynk.run();
    // Executa funções programadas pelo timer
    timer.run();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
// void sendWiFiSignal() {
//     int rssi = WiFi.RSSI();  // Obtém o sinal Wi-Fi
//     Blynk.virtualWrite(V17, String(rssi) + "db");  // Envia para o Blynk no Virtual Pin V14
//     Serial.print("Sinal Wi-Fi (RSSI): ");
//     Serial.println(rssi);
// }
//-------------------------------------------------------------------------------------------------------
void verificaStatusWifi() {
    if (WiFi.status() == WL_CONNECTED) {
        statusWiFi = "Wi-Fi: OK"; // Conectado
    } else {
        statusWiFi = "Wi-Fi: FORA"; // Desconectado
    }
    Serial.println(statusWiFi); // Exibe no monitor serial
}
//----------------------------------------------------------------------------------------------------------------------------------------
// Função para enviar status ao Blynk
void enviarStatusBlynk() {
    String statusESP32;
    if (WiFi.status() == WL_CONNECTED) {
        statusESP32 = "ONLINE";
    } else {
        statusESP32 = "OFFLINE";
    }

    struct tm timeInfo;
    if (!getLocalTime(&timeInfo)) {
        Serial.println("Falha ao obter o tempo local");
        return;
    }

    // Obtém a data e hora formatadas
    char dateBuffer[11]; // Formato: DD/MM/YYYY
    strftime(dateBuffer, sizeof(dateBuffer), "%d/%m/%Y", &timeInfo);
    char timeBuffer[9]; // Formato: HH:MM:SS
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeInfo);

    // Formata a mensagem com status, data e hora
    String fullMessage = statusESP32 + " em " + String(dateBuffer) + " às " + String(timeBuffer);

    // Envia o status ao Blynk pelo pino virtual V7
    Blynk.virtualWrite(V7, fullMessage);
}
//-------------------------------------------------------------------------------------------------------
// Função chamada pelo botão no Blynk
   BLYNK_WRITE(V24) { // Substitua V1 pelo pino virtual configurado no Blynk
  int buttonState = param.asInt(); // Estado do botão (1 = pressionado, 0 = solto)
  if (buttonState == 1) {
    Serial.println("Botão pressionado no Blynk. Atualizando firmware...");
    enviarmensagemWiFi("CISTERNA BL-B: Botão acionado no Blynk. Atualizando firmware do ESP32. Aguarde...");
    performFirmwareUpdate(firmwareURL);
  }
}
//--------------------------------------------------------------------------------------------------------
void performFirmwareUpdate(const char* firmwareURL) {
    Serial.println("Iniciando atualização de firmware...");

    HTTPClient http;
    http.begin(firmwareURL);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        WiFiClient* client = http.getStreamPtr();

        if (contentLength > 0) {
            if (Update.begin(contentLength)) {
                size_t written = Update.writeStream(*client);

                if (written == contentLength) {
                    if (Update.end() && Update.isFinished()) {
                        Serial.println("Atualização concluída com sucesso. Reiniciando...");
                        Serial.println("");
                        enviarmensagemWiFi("Atualização concluída com sucesso");
                        delay(500);
                        ESP.restart();  // Reinicia após atualização bem-sucedida
                    } else {
                        Serial.println("Falha ao finalizar a atualização.");
                    }
                } else {
                    Serial.println("Falha na escrita do firmware.");
                }
            } else {
                Serial.println("Espaço insuficiente para OTA.");
            }
        } else {
            Serial.println("Tamanho do conteúdo inválido.");
        }
    } else {
        Serial.printf("Erro HTTP ao buscar firmware. Código: %d\n", httpCode);
    }
      http.end();
}
//--------------------------------------------------------------------------------------------------------
// Função para gerenciar a conexão Wi-Fi
void conectarWiFi() {
    Serial.println("Tentando conectar ao Wi-Fi...");
    WiFi.begin(ssid, pass);
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 10000; // Tempo limite para tentar conexão (10 segundos)
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(500);         // Aguarda 500ms entre as tentativas
    Serial.print(".");  // Exibe um ponto a cada tentativa
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado ao Wi-Fi!");
    enviarmensagemWiFi("REDE Wi-Fi: Cisterna BL-B conectada à rede " + String(ssid));
  } else {
    Serial.println("\nFalha ao conectar ao Wi-Fi.");
  }
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void verificarWiFi() {
    static unsigned long lastWiFiCheck = 0;
    const unsigned long wifiCheckInterval = 20000; // Verificar Wi-Fi a cada 10 segundos
    if (millis() - lastWiFiCheck >= wifiCheckInterval) {
        lastWiFiCheck = millis();

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Wi-Fi desconectado! Tentando reconectar...");
            conectarWiFi();
        }
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void arduinoOTA() {                                                              //ATUALIZAÇÃO DO CÓDIGO DO ESP32 VIA WI-FI ATRAVÉS DO OTA(Over-the-Air)
 //ATUALIZAÇÃO DO CÓDIGO DO ESP32 VIA WI-FI ATRAVÉS DO OTA(Over-the-Air)
  ArduinoOTA.setHostname("CISTERNA_BL_B");                                 // Define o nome do dispositivo para identificação no processo OTA.
  ArduinoOTA.setPassword("cisternab");                                           // Define a senha de autenticação para o processo OTA.
  ArduinoOTA.onStart([]() {                                                      // Callback para o início da atualização OTA.
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem"; // Identifica o tipo de atualização.
    Serial.println("Iniciando atualização de " + type);                          // Exibe o tipo de atualização no Serial Monitor.
  });
  ArduinoOTA.onEnd([]() {                                                        // Callback para o final da atualização OTA.
    Serial.println("\nAtualização concluída.");                                  // Confirma que a atualização foi finalizada com sucesso.
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {          // Callback para progresso da atualização.
    Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));              // Mostra o progresso em porcentagem.
  });
  ArduinoOTA.onError([](ota_error_t error) {                                     // Callback para tratamento de erros durante a atualização.
    Serial.printf("Erro [%u]: ", error);                                         // Exibe o código do erro no Serial Monitor.
    if (error == OTA_AUTH_ERROR) Serial.println("Falha de autenticação");        // Mensagem para erro de autenticação.
    else if (error == OTA_BEGIN_ERROR) Serial.println("Falha ao iniciar");       // Mensagem para erro de inicialização.
    else if (error == OTA_CONNECT_ERROR) Serial.println("Falha de conexão");     // Mensagem para erro de conexão.
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha de recebimento"); // Mensagem para erro de recebimento.
    else if (error == OTA_END_ERROR) Serial.println("Falha ao finalizar");       // Mensagem para erro ao finalizar.
  });
  ArduinoOTA.begin();  // Inicializa o serviço OTA para permitir atualizações.
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void enviarmensagemWiFi(String message) {
    struct tm timeInfo;
    if (!getLocalTime(&timeInfo)) {
        Serial.println("Falha ao obter o tempo local");
        return;
    }

    // Obtém a data e hora formatadas
    char dateBuffer[11]; // Formato: DD/MM/YYYY
    strftime(dateBuffer, sizeof(dateBuffer), "%d/%m/%Y", &timeInfo);
    char timeBuffer[9]; // Formato: HH:MM:SS
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeInfo);

    // Formata a mensagem
    String fullMessage = message + " em " + String(dateBuffer) + " às " + String(timeBuffer);
    String encodedMessage = urlEncode(fullMessage);  

    for (int i = 0; i < totalChatsWF; i++) {
        HTTPClient http;
        String url = "https://api.telegram.org/bot" + botTokenWF + "/sendMessage?chat_id=" + chatIdsWF[i] + "&text=" + encodedMessage;
        http.begin(url);
        int httpResponseCode = http.GET();

        if (httpResponseCode == 200) {
            Serial.println("Mensagem enviada com sucesso para o Chat ID: " + chatIdsWF[i]);
        } else {
            Serial.println("Erro ao enviar para o Chat ID: " + chatIdsWF[i] + ". Código: " + String(httpResponseCode));
        }
        http.end();  // Finaliza a conexão HTTP
        delay(500);  // Atraso para evitar bloqueio por excesso de requisições
    }
}
//--------------------------------------------------------------------------------------------------------------------------------
void sendMessage(String message) {
    if (!mensagensPermitidas) return;  // Evita envio antes da inicialização completa

    struct tm timeInfo;
    if (!getLocalTime(&timeInfo)) {
        Serial.println("Falha ao obter o tempo local");
        return;
    }

    // Obtém a data e hora formatadas
    char dateBuffer[11]; // Formato: DD/MM/YYYY
    strftime(dateBuffer, sizeof(dateBuffer), "%d/%m/%Y", &timeInfo);
    char timeBuffer[9]; // Formato: HH:MM:SS
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeInfo);

    // Formata a mensagem
    String fullMessage = message + " em " + String(dateBuffer) + " às " + String(timeBuffer);
    String encodedMessage = urlEncode(fullMessage);  

    for (int i = 0; i < totalChats; i++) {
        HTTPClient http;  // Cria uma nova instância do cliente HTTP para cada iteração
        String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatIds[i] + "&text=" + encodedMessage;

        http.begin(url);  // Inicia a requisição HTTP
        int httpResponseCode = http.GET();  // Envia a requisição GET

        // Verifica o código de resposta HTTP
        if (httpResponseCode == 200) {
            Serial.println("Mensagem enviada com sucesso para o Chat ID: " + chatIds[i]);
        } else {
            Serial.print("Erro no envio da mensagem para o Chat ID: " + chatIds[i] + ". Código: ");
            Serial.println(httpResponseCode);
        }

        http.end();  // Finaliza a requisição HTTP
        delay(500);  // Atraso para evitar bloqueio por excesso de requisições
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void habilitarMensagens() {
  mensagensPermitidas = true;                                   // Habilita o envio de mensagens
  Serial.println("Mensagens habilitadas após inicialização.");  // Informa que as mensagens estão habilitadas
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void atualizarDados() {
  distancia = calcularDistancia();                                         // Calcula a distância usando o sensor ultrassônico
  nivelPercentual = calcularPercentual(distancia);                         // Calcula o percentual do nível com base na distância
  Blynk.virtualWrite(VPIN_NIVEL_PERCENTUAL, String(nivelPercentual, 1));   // Atualiza o valor do nível percentual no Blynk com uma casa decimal
  Blynk.virtualWrite(VPIN_DISTANCIA, String(distancia));
  displayData(nivelPercentual);                                            // Exibe os dados no display OLED
  Serial.print("Distância: ");
  Serial.print(distancia);                                                // Exibe a distância no Serial Monitor
  Serial.println(" cm");
  Serial.print("Nível: ");
  Serial.print(nivelPercentual, 1);                                      // Exibe o nível com uma casa decimal no Serial Monitor
  Serial.println(" %");
  Serial.println("");                                                    // Adiciona uma linha em branco para separar as leituras
  delay(500);                                                            // Aguarda 500 ms antes de realizar a próxima leitura
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void displayData(float nivelPercentual) {
    static float lastNivel = -1;       // Armazena o último valor do nível para evitar atualização desnecessária
    static String lastStatusWiFi = ""; // Armazena o último status do Wi-Fi

    // Atualiza o display se o nível ou o status Wi-Fi mudar
    if (nivelPercentual != lastNivel || statusWiFi != lastStatusWiFi) {
        oled.clearDisplay();  // Limpa o display OLED
        oled.setTextSize(1.9);  // Define o tamanho do texto
        oled.setTextColor(WHITE);  // Define a cor do texto como branco
        oled.setCursor(9, 8);  // Define a posição inicial do cursor
        oled.println(statusWiFi);  // Exibe o status da rede Wi-Fi
        oled.setCursor(9, 24);  // Define nova posição do cursor
        oled.println("CISTERNA B");  // Exibe o título "CAIXA"
        oled.setCursor(3, 44);  // Define a posição do cursor para o valor do nível
        oled.setTextSize(2);  // Aumenta o tamanho do texto
        oled.print(nivelPercentual, 1);  // Exibe o valor do nível com uma casa decimal
        oled.print("%");
        oled.drawRect(0, 0, SCREEN_WIDTH - 51, SCREEN_HEIGHT, WHITE);  // Desenha o retângulo da borda do display
        int tankWidth = 22;  // Largura do tanque
        int tankX = SCREEN_WIDTH - tankWidth - 2;  // Posição X do tanque
        int tankY = 2;  // Posição Y do tanque
        int tankHeight = SCREEN_HEIGHT - 10;  // Altura do tanque
        oled.drawRect(tankX, tankY, tankWidth, tankHeight, WHITE);  // Desenha o contorno do tanque
        int fillHeight = map(nivelPercentual, 0, 100, 0, tankHeight);  // Calcula a altura da água no tanque
        oled.fillRect(tankX + 1, tankY + tankHeight - fillHeight, tankWidth - 2, fillHeight, WHITE);  // Preenche o tanque com base no nível
        oled.setTextSize(1);  // Restaura o tamanho do texto
        oled.setCursor(tankX - 24, tankY - 2);  // Define a posição do texto "100%"
        oled.print("100%");
        oled.setCursor(tankX - 18, tankY + tankHeight / 2 - 4);  // Define a posição do texto "50%"
        oled.print("50%");
        oled.setCursor(tankX - 12, tankY + tankHeight - 8);  // Define a posição do texto "0%"
        oled.print("0%");
        oled.display();  // Atualiza o display com os dados
        lastNivel = nivelPercentual;  // Atualiza o valor do nível armazenado
        lastStatusWiFi = statusWiFi; // Atualiza o último status do Wi-Fi armazenado
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void sonarBegin(byte trig ,byte echo) { // Função para configurar o sensor ultrassônico (HC-SR04) e inicializar os pinos de disparo e recepção.
                                        // Define as constantes necessárias e prepara os pinos para as medições de distância.
  #define divisor 57.74                  // Fator de conversão para calcular a distância (em cm) com base no tempo de retorno do som
  #define intervaloMedida 200          // Intervalo entre medições (em milissegundos)
  #define qtdMedidas 50                 // Quantidade de medições para calcular a média
  pinMode(trig, OUTPUT);                // Define o pino 'trig' como saída
  pinMode(echo, INPUT);                 // Define o pino 'echo' como entrada
  digitalWrite(trig, LOW);              // Garante que o pino 'trig' comece com nível baixo
  delayMicroseconds(500);               // Aguarda 500 microssegundos para garantir estabilidade nos pinos
}
//-----------------------------------------------------------------------------------------------------------------------------------------
float calcularDistancia() {
  float leituraSum = 0;
  int leiturasValidas = 0;

  // Definir faixa dinâmica com base na última leitura válida
  float minDistancia = ultimaLeitura - TOLERANCIA;
  float maxDistancia = ultimaLeitura + TOLERANCIA;

  // Garante que os limites não ultrapassem os valores extremos da caixa d'água
  minDistancia = max(minDistancia, 40.0f);  // Adicionamos 'f' para garantir que seja float
  maxDistancia = min(maxDistancia, 271.0f); // Adicionamos 'f' para garantir que seja float

  for (int i = 0; i < qtdMedidas; i++) {
    delay(intervaloMedida);
    float leitura = leituraSimples();

    // Filtra leituras inválidas
    if (leitura >= minDistancia && leitura <= maxDistancia) {
      leituraSum += leitura;
      leiturasValidas++;
    }
  }

  if (leiturasValidas == 0) return ultimaLeitura; // Se nenhuma leitura for válida, mantém o último valor

  ultimaLeitura = leituraSum / leiturasValidas;  // Atualiza última leitura válida
  return ultimaLeitura + 2.2; // Média das leituras válidas com correção
}
//-----------------------------------------------------------------------------------------------------------------------------------------
float leituraSimples() {
  long duracao = 0;
  digitalWrite(pinTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrigger, LOW);
  duracao = pulseIn(pinEcho, HIGH);

  float distancia = ((float) duracao / divisor);

  // Definir os limites dinâmicos com base na última leitura válida
  float minDistancia = max(ultimaLeitura - TOLERANCIA, 40.0f);  // Adicionamos 'f' para indicar float
  float maxDistancia = min(ultimaLeitura + TOLERANCIA, 271.0f); // Adicionamos 'f' para indicar float

  // Verifica se a distância está dentro do intervalo aceitável
  if (distancia >= minDistancia && distancia <= maxDistancia) {
    ultimaLeitura = distancia; // Atualiza a última leitura válida
    return distancia;
  } else {
    return ultimaLeitura; // Retorna a última leitura válida se o valor for inválido
  }
}
//-----------------------------------------------------------------------------------------------------------------------------------------
float calcularPercentual(float distancia) {   // Função que calcula o percentual de nível de água com base na distância medida
  // Calcula o percentual de nível com base na distância
  float nivelPercentual = ((emptyTankDistance - distancia) / (emptyTankDistance - fullTankDistance)) * 100.0;
  // Retorna o percentual, limitado entre 0 e 100
  return constrain(nivelPercentual, 0.0, 100.0);
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void checkWaterLevel() {
// Alerta para níveis 
 // Nível de 7%
    if (nivelPercentual < (7 - HISTERESIS) && flag05) {
        sendMessage("CISTERNA DO BL-B VAZIA");
        flag05 = false; // Bloqueia novas mensagens até o nível subir
    } else if (nivelPercentual > (7 + HISTERESIS) && !flag05) {
        flag05 = true; // Permite envio quando o nível cair novamente
    }
    // Nível de 25%
    if (nivelPercentual > (25 + HISTERESIS) && flag25) {
        sendMessage("CISTERNA DO BL-B:\nAbastecimento sendo restabelecido.\nNível acima de 25%");
        flag25 = false;
    } else if (nivelPercentual < (25 - HISTERESIS) && !flag25) {
        sendMessage("CISTERNA DO BL-B:\nNÍVEL CRÍTICO: Abaixo de 25%");
        flag25 = true;
    }
    // Nível de 40%
    if (nivelPercentual > (40 + HISTERESIS) && flag50) {
        sendMessage("CISTERNA DO BL-B:\nAbastecimento sendo restabelecido.\nNível acima de 40%");
        flag50 = false;
    } else if (nivelPercentual < (40 - HISTERESIS) && !flag50) {
        sendMessage("CISTERNA DO BL-B:\nATENÇÃO: Nível abaixo de 40%");
        flag50 = true;
    }
  // Alerta para nível de 65%
    if (nivelPercentual > (65 + HISTERESIS) && flag75) {
        sendMessage("CISTERNA DO BL-B:\nAbastecimento sendo restabelecido.\nNível acima de 65%");
        flag75 = false;
    } else if (nivelPercentual < (65 - HISTERESIS) && !flag75) {
      sendMessage("CISTERNA DO BL-B:\nATENÇÃO: Nível abaixo de 65%");
        flag75 = true;
    }
  // Alerta para nível de 100%
    if (nivelPercentual > (97.5 + HISTERESIS) && flag100) {
        sendMessage("CISTERNA DO BL-B: CHEIA.\nNível em praticamente 100%");
        flag100 = false;
    } else if (nivelPercentual < (97.5 - HISTERESIS) && !flag100) {
        flag100 = true;
    }
  }
//-----------------------------------------------------------------------------------------------------------------------------------------
void blynkVirtualWrite(){
    //ALERTA SOBRE A BÓIA
    if (distancia <= (33 - HISTERESIS) && boiaNormal && (millis() - lastMessageTime >= messageInterval)) {
      sendMessage("BÓIA DA CISTERNA DO BL-B: \nProblemas na bóia, fazer inspeção");
      lastMessageTime = millis();
      boiaNormal = false; // Define que já foi enviado o alerta
      Blynk.virtualWrite(V13, "Problemas na bóia, fazer inspeção.");
      Blynk.setProperty(V13, "color", "#FF0000"); // Vermelho
      }
    if (distancia > (33 + HISTERESIS) && !boiaNormal) {
      sendMessage("BÓIA DA CISTERNA DO BL-B: \nBóia operando normalmente.");
      boiaNormal = true; // Impede o envio repetido enquanto a distância for maior que 33
      Blynk.virtualWrite(V13, "Bóia operando normalmente.");
      Blynk.setProperty(V13, "color", "#0000FF"); // Azul
    }
}
//---------------------------------------------------------------------------------------------------------------
