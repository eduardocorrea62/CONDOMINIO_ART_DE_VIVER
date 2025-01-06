//PROJETO PILOTO DE MONITORAMENTO DE NÍVEL DE CAIXA D'ÁGUA PREDIAL
//TÉCNICO RESPONSÁVEL: Eduardo R Corrêa
//CONDOMÍNIO ART DE VIVER
//Bloco_A_CAIXA_04_Pessoas
//ArduinoOTA - Password = caixaa
//-----------------------------------------------------------------------------------------------------------------------------------
//Habilita a funcionalidade de atualização de firmware Over-The-Air (OTA) no ESP32.
#include <ArduinoOTA.h> 
//-----------------------------------------------------------------------------------------------------------------------------------
#define BLYNK_TEMPLATE_ID "TMPL2D4QwDOwY"
#define BLYNK_TEMPLATE_NAME "Condominio Art de Viver"
#define BLYNK_AUTH_TOKEN "23v2LJENbd0RXHJZ-2GJ36UKMNS_e8O1"
//-----------------------------------------------------------------------------------------------------------------------------------
//Credenciais para as redes Wi-Fi

const char* ssid = "Art de Viver";
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
#include <esp_system.h>            // Para esp_reset_reason()
//---------------------------------------------------------------------------------------------------------------------------------------
// BOT para Alertas de Níveis
String botToken = "8111874052:AAFn_63Sh0hJUuOP4IyzeBtxNOH2dQ2n9No";
String chatIds[] = {"207223980", "975571557"}; // Adicione os IDs aqui
const int totalChats = sizeof(chatIds) / sizeof(chatIds[0]);
//Lista de IDs
 /* Eduardo - "207223980"      //Amábile - "5255435232"     //Prof Tiago - "975571557"
  Síndico -      //Subsíndico -      //Zelador - */
//--------------------------------------------------------------------------------------------------------------------------------------------------
// BOT para controle das mensagens da reinicialização  e reconexão dos ESP32
String botTokenWF = "7396450492:AAEaEsnI2EWWE4hltZyK86_j9MMqyQV9bUI";
String chatIdsWF[] = {"207223980", /*"975571557"*/}; // Adicione os IDs aqui
const int totalChatsWF = sizeof(chatIds) / sizeof(chatIdsWF[0]);
//--------------------------------------------------------------------------------------------------------------------------------------------------
// Configuração da biblioteca
WiFiClientSecure client; // Cliente seguro para comunicação HTTPS
//--------------------------------------------------------------------------------------------------------------------------------------------------
// DEFINIÇÕES DE PINOS
#define pinTrigger 27
#define pinEcho 26
#define VPIN_NIVEL_PERCENTUAL V2
#define VPIN_DISTANCIA V14
#define HISTERESIS 2
//--------------------------------------------------------------------------------------------------------------------------------------------------
// Flag para permitir ou bloquear envio de mensagens
bool mensagensPermitidas = false;  
//--------------------------------------------------------------------------------------------------------------------------------------------------
// Definições do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_SH1106G oled = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1, 400000, 100000);
//--------------------------------------------------------------------------------------------------------------------------------------------------
// Variáveis para o cálculo do nível de água no tanque:
float distancia;                  // Armazena a distância medida pelo sensor ultrassônico.
float nivelPercentual;            // Nível do tanque em porcentagem, com precisão de uma casa decimal.
float emptyTankDistance = 217;   // Distância medida pelo sensor quando o tanque está vazio (em cm).
float fullTankDistance =   37;    // Distância medida pelo sensor quando o tanque está cheio (em cm).
// float emptyTankDistance = 38;   // Medidas para teste de bancada(em cm).
// float fullTankDistance =  30;   // Medidas para teste de bancada(em cm).
//--------------------------------------------------------------------------------------------------------------------------------------------------
//ALERTA do Problema na Bóia a cada 1 segundo
unsigned long lastMessageTime = 0;
const unsigned long messageInterval = 1000; // ---1 minuto
//--------------------------------------------------------------------------------------------------------------------------------------------------
// Flags para cada nível de alerta
bool flag05 = true;
bool flag25 = true;
bool flag50 = true;
bool flag75 = true;
bool flag100 = true;
bool boiaNormal = false;  // Variável de controle para garantir que a mensagem será enviada uma única vez quando o nível estiver normal
//--------------------------------------------------------------------------------------------------------------------------------------------------
// BLYNK AUTENTICAÇÃO
char auth[] = BLYNK_AUTH_TOKEN;
BlynkTimer timer;
//--------------------------------------------------------------------------------------------------------------------------------------------------
// Configuração do cliente NTP para sincronização de data e hora:
WiFiUDP ntpUDP;                                    // Objeto para comunicação via protocolo UDP.
NTPClient timeClient(ntpUDP, "pool.ntp.org",       // Cliente NTP usando o servidor "pool.ntp.org".
                     -3 * 3600,                   // Fuso horário definido para UTC-3 (Brasília).
                     60000);                      // Atualização do horário a cada 60 segundos.
//--------------------------------------------------------------------------------------------------------------------------------------------------
// Variável para o status da rede Wi-Fi
String statusWiFi = "REDE FORA"; 
//-------------------------------------------------------------------------------------------------------------------------------------------
// Inicializa o bot do Telegram - Usando para envio separado de mensagem entre o administrador do sistema e o cliente
UniversalTelegramBot botToken1(botToken, client);
//--------------------------------------------------------------------------------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    // Conexão Wi-Fi com Timeout
    Serial.println("Tentando conectar ao Wi-Fi...");
    WiFi.begin(ssid, pass);
    unsigned long startAttemptTime = millis();
    const unsigned long timeout = 7000; // Tempo limite para tentar conexão (5 segundos)
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(500);         // Aguarda 500ms entre as tentativas
    Serial.print(".");  // Exibe um ponto a cada tentativa
}
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConectado ao Wi-Fi");
        //Mensagem da reinicialização do ESP-32 para o Telegram
          enviarmensagemWiFi("ESP-32 da Caixa BL A foi reinicializado");
        } else {
        Serial.println("\nFalha ao conectar. Continuando sem internet...");
      }
    // Inicialização dos periféricos
    Blynk.config(auth); // Configura o Blynk sem tentar conectar imediatamente
    sonarBegin(pinTrigger, pinEcho);
    oled.begin(0x3C, true);
    oled.clearDisplay();
    oled.display();
    timeClient.begin();
    arduinoOTA();
    // Configuração de temporizadores
    timer.setTimeout(2000L, habilitarMensagens);
    timer.setInterval(1000L, atualizarDados);
    // Configuração de pinos
    pinMode(pinTrigger, OUTPUT);
    pinMode(pinEcho, INPUT);
}
//------------------------------------------------------------------------------------------------------------------------------------------------
void loop() {
    // Verifica e reconecta o Wi-Fi, se necessário
    verificarWiFi();
    // Atualiza rapidamente o status do Wi-Fi
    if (WiFi.status() == WL_CONNECTED) {
        statusWiFi = "WI-FI: OK"; // Conectado
    } else {
        statusWiFi = "WI-FI: FORA"; // Desconectado
    }
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
//-------------------------------------------------------------------------------------------------------------------------------------------
void conectarWiFi() {
    Serial.println("Tentando conectar ao Wi-Fi...");
    WiFi.begin(ssid, pass);
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 10000; // Tempo limite para tentar conexão (10 segundos)
  Serial.print("Tentando conectar ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(500);         // Aguarda 500ms entre as tentativas
    Serial.print(".");  // Exibe um ponto a cada tentativa
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado ao Wi-Fi!");
    enviarmensagemWiFi("REDE Wi-Fi: Caixa BL A conectada à rede prioritária");
  } else {
    Serial.println("\nFalha ao conectar ao Wi-Fi.");
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------
void verificarWiFi() {
    static unsigned long lastWiFiCheck = 0;
    const unsigned long wifiCheckInterval = 10000; // Verificar Wi-Fi a cada 10 segundos
    if (millis() - lastWiFiCheck >= wifiCheckInterval) {
        lastWiFiCheck = millis();

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Wi-Fi desconectado! Tentando reconectar...");
            conectarWiFi();
        }
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------
void arduinoOTA(){                                                               //ATUALIZAÇÃO DO CÓDIGO DO ESP32 VIA WI-FI ATRAVÉS DO OTA(Over-the-Air) 
  ArduinoOTA.setHostname("CAIXA_DO_BLOCO_A");                               // Define o nome do dispositivo para identificação no processo OTA.
  ArduinoOTA.setPassword("caixaa");                                               // Define a senha de autenticação para o processo OTA.
  ArduinoOTA.onStart([]() {                                                       // Callback para o início da atualização OTA.
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
  ArduinoOTA.begin();                                                            // Inicializa o serviço OTA para permitir atualizações.
  }
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void enviarmensagemWiFi(String message) {
  timeClient.update();  // Atualiza a hora com o servidor NTP
  String currentTime = timeClient.getFormattedTime();  // Obtém o horário formatado
  unsigned long epochTime = timeClient.getEpochTime();  // Obtém o tempo em formato de época
  int day, month, year;
  getDateFromEpoch(epochTime, day, month, year);  // Converte o tempo de época para data
  // Formata a mensagem com a data e hora
  String fullMessage = message + " em " + String(day) + "/" + String(month) + "/" + String(year) + " às " + currentTime;
  String encodedMessage = urlEncode(fullMessage);  // Codifica a mensagem para envio via URL
  HTTPClient http;  // Cria uma instância do cliente HTTP
  for (int i = 0; i < totalChatsWF; i++) {
    String url = "https://api.telegram.org/bot" + botTokenWF + "/sendMessage?chat_id=" + chatIdsWF[i] + "&text=" + encodedMessage;
  http.begin(url);  // Inicia a requisição HTTP
   int httpResponseCode = http.GET();  // Envia a requisição GET e obtém o código de resposta
   // Verifica o status da resposta HTTP
   if (httpResponseCode == 200) {
     Serial.println("Mensagem enviada com sucesso para o Bot " + String(i + 1));  // Mensagem de sucesso
    }
    else {
     Serial.print("Erro no envio da mensagem para o Bot " + String(i + 1) + ". Código: ");
     Serial.println(httpResponseCode);  // Imprime o código de erro
    }
    http.end();  // Finaliza a requisição HTTP
  }
}
//---------------------------------------------------------------------------------------------------------------------------------------------------
void sendMessage(String message) {
  if (!mensagensPermitidas) return;  // Evita envio antes da inicialização completa
  timeClient.update();  // Atualiza a hora com o servidor NTP
  String currentTime = timeClient.getFormattedTime();  // Obtém o horário formatado
  unsigned long epochTime = timeClient.getEpochTime();  // Obtém o tempo em formato de época
  int day, month, year;
  getDateFromEpoch(epochTime, day, month, year);  // Converte o tempo de época para data
  // Formata a mensagem com a data e hora
  String fullMessage = message + " em " + String(day) + "/" + String(month) + "/" + String(year) + " às " + currentTime;
  String encodedMessage = urlEncode(fullMessage);  // Codifica a mensagem para envio via URL
  HTTPClient http;  // Cria uma instância do cliente HTTP
  for (int i = 0; i < totalChats; i++) {
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatIds[i] + "&text=" + encodedMessage;
    http.begin(url);  // Inicia a requisição HTTP
    int httpResponseCode = http.GET();  // Envia a requisição GET e obtém o código de resposta
    // Verifica o status da resposta HTTP
    if (httpResponseCode == 200) {
      Serial.println("Mensagem enviada com sucesso para o Bot " + String(i + 1));  // Mensagem de sucesso
    } else {
      Serial.print("Erro no envio da mensagem para o Bot " + String(i + 1) + ". Código: ");
      Serial.println(httpResponseCode);  // Imprime o código de erro
    }
    http.end();  // Finaliza a requisição HTTP
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------
void habilitarMensagens() {
  mensagensPermitidas = true;                                   // Habilita o envio de mensagens
  Serial.println("Mensagens habilitadas após inicialização.");  // Informa que as mensagens estão habilitadas
}
//----------------------------------------------------------------------------------------------------------------------------------------------
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
//---------------------------------------------------------------------------------------------------------------------------------------------------
void displayData(float nivelPercentual) {
  static float lastValue = -1;
  if (nivelPercentual != lastValue) {
    oled.clearDisplay();
    oled.setTextSize(1.9);
    oled.setTextColor(WHITE);
    oled.setCursor(9, 8);  // Define a posição inicial do cursor
    oled.println(statusWiFi);  // Exibe o status da rede Wi-Fi
    oled.setCursor(9, 24);
    oled.println("CAIXA BL A");
    oled.setCursor(3, 44);
    oled.setTextSize(2);
    oled.print(nivelPercentual, 1); // Exibe uma casa decimal no display
    oled.print("%");
    oled.drawRect(0, 0, SCREEN_WIDTH - 51, SCREEN_HEIGHT, WHITE);
    int tankWidth = 22;
    int tankX = SCREEN_WIDTH - tankWidth - 2;
    int tankY = 2;
    int tankHeight = SCREEN_HEIGHT - 10;
    oled.drawRect(tankX, tankY, tankWidth, tankHeight, WHITE);
    int fillHeight = map(nivelPercentual, 0, 100, 0, tankHeight);
    oled.fillRect(tankX + 1, tankY + tankHeight - fillHeight, tankWidth - 2, fillHeight, WHITE);
    oled.setTextSize(1);
    oled.setCursor(tankX - 24, tankY - 2);
    oled.print("100%");
    oled.setCursor(tankX - 18, tankY + tankHeight / 2 - 4);
    oled.print("50%");
    oled.setCursor(tankX - 12, tankY + tankHeight - 8);
    oled.print("0%");
    oled.display();
    lastValue = nivelPercentual;
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------
void sonarBegin(byte trig ,byte echo) { // Função para configurar o sensor ultrassônico (HC-SR04) e inicializar os pinos de disparo e recepção.
                                        // Define as constantes necessárias e prepara os pinos para as medições de distância.
  #define divisor 58.0                  // Fator de conversão para calcular a distância (em cm) com base no tempo de retorno do som
  #define intervaloMedida 10             // Intervalo entre medições (em milissegundos)
  #define qtdMedidas 50                 // Quantidade de medições para calcular a média
  pinMode(trig, OUTPUT);                // Define o pino 'trig' como saída
  pinMode(echo, INPUT);                 // Define o pino 'echo' como entrada
  digitalWrite(trig, LOW);              // Garante que o pino 'trig' comece com nível baixo
  delayMicroseconds(500);               // Aguarda 500 microssegundos para garantir estabilidade nos pinos
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
float calcularDistancia() {   // Função que calcula a distância média com base em várias leituras do sensor ultrassônico.
                              // A média das leituras é calculada para melhorar a precisão da medição e um valor de correção é adicionado no final.
  float leituraSum = 0;                               // Inicializa a variável para somar as leituras
  for (int index = 0; index < qtdMedidas; index++) {  // Loop para realizar múltiplas leituras
    delay(intervaloMedida);                           // Aguarda o intervalo entre as leituras
    leituraSum += leituraSimples();                   // Adiciona o valor da leitura simples à soma total
  }
  return (leituraSum / qtdMedidas) + 2.2;             // Retorna a média das leituras mais um valor de correção
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------
float leituraSimples() {  // Função que realiza uma leitura simples do sensor ultrassônico, 
                          //...calculando o tempo de retorno do sinal e convertendo-o em distância.
  long duracao = 0;                    // Variável para armazenar a duração do pulso recebido
  digitalWrite(pinTrigger, HIGH);      // Envia um pulso de ativação no pino trigger
  delayMicroseconds(10);               // Aguarda 10 microssegundos
  digitalWrite(pinTrigger, LOW);       // Desliga o pulso de ativação
  duracao = pulseIn(pinEcho, HIGH);    // Mede o tempo que o sinal leva para retornar no pino echo
  return ((float) duracao / divisor);  // Converte o tempo de retorno em distância usando o divisor
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------
float calcularPercentual(float distancia) {   // Função que calcula o percentual de nível de água com base na distância medida
  // Calcula o percentual de nível com base na distância
  float nivelPercentual = ((emptyTankDistance - distancia) / (emptyTankDistance - fullTankDistance)) * 100.0;
  // Retorna o percentual, limitado entre 0 e 100
  return constrain(nivelPercentual, 0.0, 100.0);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------
void checkWaterLevel() {
// Alerta para níveis 
 // Nível de 7%
    if (nivelPercentual < (7 - HISTERESIS) && flag05) {
        sendMessage("CAIXA DO BLOCO A: VAZIA");
        flag05 = false; // Bloqueia novas mensagens até o nível subir
    } else if (nivelPercentual > (7 + HISTERESIS) && !flag05) {
        flag05 = true; // Permite envio quando o nível cair novamente
    }
    // Nível de 25%
    if (nivelPercentual > (25 + HISTERESIS) && flag25) {
        sendMessage("CAIXA DO BLOCO A:\nAbastecimento sendo restabelecido.\nNível acima de 25%");
        flag25 = false;
    } else if (nivelPercentual < (25 - HISTERESIS) && !flag25) {
        sendMessage("CAIXA DO BLOCO A:\nNível Crítico: Abaixo de 25%");
        flag25 = true;
    }
    // Nível de 50%
    if (nivelPercentual > (50 + HISTERESIS) && flag50) {
        sendMessage("CAIXA DO BLOCO A:\nAbastecimento sendo restabelecido.\nNível acima de 50%");
        flag50 = false;
    } else if (nivelPercentual < (50 - HISTERESIS) && !flag50) {
        sendMessage("CAIXA DO BLOCO A:\nAtenção: Nível abaixo de 50%");
        flag50 = true;
    }
  // Alerta para nível de 75%
    if (nivelPercentual > (75 + HISTERESIS) && flag75) {
        sendMessage("CAIXA DO BLOCO A:\nAbastecimento sendo restabelecido.\nNível acima de 75%");
        flag75 = false;
    } else if (nivelPercentual < (75 - HISTERESIS) && !flag75) {
      sendMessage("CAIXA DO BLOCO A:\nAtenção: Nível abaixo de 75%");
        flag75 = true;
    }
  // Alerta para nível de 100%
    if (nivelPercentual > (97 + HISTERESIS) && flag100) {
        sendMessage("CAIXA DO BLOCO A: CHEIA.\nNível em 100%");
        flag100 = false;
    } else if (nivelPercentual < (97 - HISTERESIS) && !flag100) {
        flag100 = true;
    }
     // ALERTA SOBRE A BÓIA
      if (distancia <= (28 - HISTERESIS) && boiaNormal && (millis() - lastMessageTime >= messageInterval)) {
      sendMessage("BÓIA DA CAIXA A: \nProblemas na bóia, fazer inspeção.");
      lastMessageTime = millis();
      boiaNormal = false; // Define que já foi enviado o alerta
      Blynk.virtualWrite(V9, "Problemas na bóia, fazer inspeção.");
      Blynk.setProperty(V9, "color", "#FF0000"); // Vermelho
      }
      if (distancia > (28 + HISTERESIS) && !boiaNormal) {
        sendMessage("BÓIA DA CAIXA A: \nBóia operando normalmente.");
        boiaNormal = true; // Impede o envio repetido enquanto a distância for maior que 33
        Blynk.virtualWrite(V9, "Bóia operando normalmente.");
        Blynk.setProperty(V9, "color", "#0000FF"); // Azul
    }
}
//---------------------------------------------------------------------------------------------------------------
void getDateFromEpoch(unsigned long epochTime, int &day, int &month, int &year) {
    // Definições básicas
    const int SECONDS_IN_A_DAY = 86400;
    const int DAYS_IN_YEAR = 365;
    const int DAYS_IN_LEAP_YEAR = 366;
    // Número de dias em cada mês (não bissexto)
    const int DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    // Ajusta o tempo para o fuso horário local (UTC+3)
    epochTime += 3 * 3600;
    // Calcula o ano
    year = 1970;
    while (true) {
        bool leapYear = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
        int daysInYear = leapYear ? DAYS_IN_LEAP_YEAR : DAYS_IN_YEAR;
        if (epochTime >= daysInYear * SECONDS_IN_A_DAY) {
            epochTime -= daysInYear * SECONDS_IN_A_DAY;
            year++;
        } else {
            break;
        }
    }
    // Ajusta os dias para cada mês
    bool leapYear = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    int daysInMonth[12];
    for (int i = 0; i < 12; i++) {
        daysInMonth[i] = DAYS_IN_MONTH[i];
    }
    if (leapYear) {
        daysInMonth[1] = 29; // Ajusta fevereiro para ano bissexto
    }
    // Calcula o mês
    month = 0;
    while (epochTime >= daysInMonth[month] * SECONDS_IN_A_DAY) {
        epochTime -= daysInMonth[month] * SECONDS_IN_A_DAY;
        month++;
    }
    month++; // Ajusta para meses começando em 1
    // Calcula o dia
    day = epochTime / SECONDS_IN_A_DAY + 1;
}
//-----------------------------------------------------------------------------------------------------------------------------------------
