#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

File myFile;
int pinoSS = 5;
int LED = 22;

#define BUFFER_SIZE 1024   // Definindo o tamanho do buffer para armazenar os dados (ajuste conforme necessário)
#define INTERVALO_GRAVACAO 100  // Intervalo para gravação em milissegundos (100ms)

String buffer = "";       // Buffer para armazenar dados
String serial_0_inputString = "";  // String para armazenar os dados recebidos
String serial_2_inputString = "";  // String para armazenar os dados recebidos
bool stringComplete = false;
unsigned long lastWriteTime = 0;  // Para armazenar o tempo da última gravação
int terminatorIndex = 0; // Índice para verificar sequência de terminadores
String nome_arquivo = "";

// Declaração antecipada da função
void serial2Event();

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  pinMode(pinoSS, OUTPUT);
  pinMode(LED, OUTPUT);
  if (SD.begin(pinoSS)) {
    Serial.println("SD Card pronto para uso.");
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);

  } else {
    Serial.println("Falha na inicialização do SD Card.");
    return;
  }
}

void loop() {
    // Chama a função serial2Event manualmente
  serial2Event();
  // Verifica se a string está completa para adicionar ao buffer
  if (stringComplete) {
    if (buffer.length() + serial_0_inputString.length() <= BUFFER_SIZE) {
      buffer += serial_0_inputString;  // Adiciona dados ao buffer
      serial_0_inputString = "";       // Limpa a string de entrada
      stringComplete = false;
      digitalWrite(LED, !digitalRead(LED));
    } else {
      Serial.println("Buffer cheio, aguardando gravação...");
    }
  }

  // Verifica se é hora de gravar os dados no SD (com base no intervalo)
  if (millis() - lastWriteTime >= INTERVALO_GRAVACAO) {
    if (buffer.length() > 0) {  // Somente grava se houver dados no buffer           
      myFile = SD.open(nome_arquivo, FILE_WRITE);
      if (myFile) {
        myFile.print(buffer);  // Grava o conteúdo do buffer
        myFile.close();
        buffer = "";  // Limpa o buffer após a gravação
        Serial.println("Dados gravados no SD.");
      } else {
        Serial.println("Erro ao abrir arquivo .txt");
      }
    }
    lastWriteTime = millis();  // Atualiza o tempo da última gravação
  }
}

void serial2Event() {
  while (Serial2.available()) {
    char inChar = (char)Serial2.read();
    //digitalWrite(LED, !digitalRead(LED));
    // Verifica se a sequência de terminadores (255, 254, 253) foi recebida
    if (terminatorIndex == 0 && inChar == 255) {
      terminatorIndex++;
    } else if (terminatorIndex == 1 && inChar == 254) {
      terminatorIndex++;
    } else if (terminatorIndex == 2 && inChar == 253) {
      // Sequência completa: substitui por \n e \r
      serial_2_inputString += '\n';
      serial_2_inputString += '\r';
      stringComplete = true; // Marca a string como completa
      terminatorIndex = 0;   // Reseta o índice de terminadores
      //digitalWrite(LED, !digitalRead(LED));

    } else {
      // Se um dos caracteres não faz parte da sequência, grava os anteriores e o atual
      if (terminatorIndex > 0) {
        // Grava os caracteres 255 e/ou 254 recebidos antes
        for (int i = 0; i < terminatorIndex; i++) {
          serial_2_inputString += (char)(255 - i);
        }
      }
      // Adiciona o caractere atual ao buffer
      serial_2_inputString += inChar;
      terminatorIndex = 0;  // Reseta o índice de terminadores
    }
  }
}

// void serial2Event() {
//   // Função manual para leitura da Serial2
//   while (Serial2.available()) {
//     digitalWrite(LED, !digitalRead(LED));  // Alterna o estado do LED
//     char inChar = (char)Serial2.read();
//     serial_2_inputString += inChar;
//     if (inChar == '\n' || inChar == '\r') {
//       stringComplete = true;
//     }
//   }
// }

// void serialEvent() {
//   static String inputString = "";  // a String to hold incoming data
//   while (Serial.available()) {
//     char inChar = (char)Serial.read();
//     inputString += inChar;
//     if (inChar == '\n' || inChar == '\r') {
//       stringComplete = true;
//     }
//   }
// }
