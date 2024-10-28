#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

File myFile;
int pinoSS = 5;
int LED = 22;

#define NUM_PACOTES 500           // Número de pacotes a serem armazenados antes de gravar
#define TAMANHO_PACOTE 11         // Tamanho de cada pacote em bytes
uint8_t buffer1[NUM_PACOTES][TAMANHO_PACOTE];  // Primeiro buffer
uint8_t buffer2[NUM_PACOTES][TAMANHO_PACOTE];  // Segundo buffer
int pacoteIndex1 = 0;              // Índice atual do pacote para o buffer 1
int pacoteIndex2 = 0;              // Índice atual do pacote para o buffer 2
int byteIndex = 0;                 // Índice do byte dentro do pacote
String nome_arquivo = "/datalog.txt";  // Nome do arquivo
bool gravandoBuffer1 = true;       // Flag para controlar qual buffer está sendo gravado
String serial_0_inputString = "";  // String temporária para os dados recebidos
String serial_2_inputString = "";  // String temporária para os dados recebidos
bool stringComplete = false;
int terminatorIndex = 0;   // Índice para verificar sequência de terminadores

void serial2Event();
bool gravarNoSD(uint8_t buffer[][TAMANHO_PACOTE], int pacoteIndex);

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
    serial2Event();  // Chama a função para capturar dados da Serial2

    // Se um dos buffers estiver cheio, grave o que está cheio
    if (pacoteIndex1 >= NUM_PACOTES || pacoteIndex2 >= NUM_PACOTES) {
        if (gravandoBuffer1) {
            if (gravarNoSD(buffer1, pacoteIndex1)) {
                Serial.println("Pacotes do Buffer 1 gravados no SD.");
                pacoteIndex1 = 0;  // Reinicia o índice do buffer 1 após a gravação
            } else {
                Serial.println("Erro ao gravar pacotes do Buffer 1 no SD.");
            }
        } else {
            if (gravarNoSD(buffer2, pacoteIndex2)) {
                Serial.println("Pacotes do Buffer 2 gravados no SD.");
                pacoteIndex2 = 0;  // Reinicia o índice do buffer 2 após a gravação
            } else {
                Serial.println("Erro ao gravar pacotes do Buffer 2 no SD.");
            }
        }

        // Alterna entre os buffers
        gravandoBuffer1 = !gravandoBuffer1;
    }
}

void serial2Event() {
    while (Serial2.available()) {
        char inChar = (char)Serial2.read();

        // Verifica se a sequência de terminadores (255, 254, 253) foi recebida
        if (terminatorIndex == 0 && inChar == 255) {
            terminatorIndex++;
        } else if (terminatorIndex == 1 && inChar == 254) {
            terminatorIndex++;
        } else if (terminatorIndex == 2 && inChar == 253) {
            // Sequência completa: substitui por \n e \r
            if (gravandoBuffer1) {
                if (pacoteIndex1 < NUM_PACOTES) { // Verifica se ainda há espaço no buffer 1
                    buffer1[pacoteIndex1][0] = '\n';  // Armazena \n no início do pacote
                    buffer1[pacoteIndex1][1] = '\r';  // Armazena \r no início do pacote
                    // Grava a string no buffer a partir da posição 2
                    for (int i = 0; i < serial_2_inputString.length() && i < TAMANHO_PACOTE - 2; i++) {
                        buffer1[pacoteIndex1][i + 2] = serial_2_inputString[i];
                    }
                    pacoteIndex1++;
                } else {
                    Serial.println("Buffer 1 cheio. Ignorando dados adicionais.");
                }
            } else {
                if (pacoteIndex2 < NUM_PACOTES) { // Verifica se ainda há espaço no buffer 2
                    //buffer2[pacoteIndex2][0] = '\n';  // Armazena \n no início do pacote
                    //buffer2[pacoteIndex2][1] = '\r';  // Armazena \r no início do pacote
                    // Grava a string no buffer a partir da posição 2
                    for (int i = 0; i < serial_2_inputString.length() && i < TAMANHO_PACOTE - 2; i++) {
                        buffer2[pacoteIndex2][i + 2] = serial_2_inputString[i];
                    }
                    pacoteIndex2++;
                } else {
                    Serial.println("Buffer 2 cheio. Ignorando dados adicionais.");
                }
            }

            // Marca a string como completa e reseta
            stringComplete = true; // Marca a string como completa
            terminatorIndex = 0;   // Reseta o índice de terminadores
            serial_2_inputString = ""; // Limpa a string para receber novos dados

        } else {
            // Se um dos caracteres não faz parte da sequência, grava os anteriores e o atual
            if (terminatorIndex > 0) {
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


bool gravarNoSD(uint8_t buffer[][TAMANHO_PACOTE], int pacoteIndex) {
    myFile = SD.open(nome_arquivo.c_str(), FILE_APPEND);
    if (myFile) {
        for (int i = 0; i < pacoteIndex; i++) {
            myFile.write((const uint8_t*)buffer[i], TAMANHO_PACOTE);
        }
        myFile.close();
        
        // Reabre o arquivo para conferir o tamanho atualizado
        myFile = SD.open(nome_arquivo.c_str(), FILE_READ);
        if (myFile) {
            Serial.print("Tamanho atual do arquivo: ");
            Serial.println(myFile.size());
            myFile.close();
        }
        digitalWrite(LED, !digitalRead(LED));
        return true;
    } else {
        Serial.println("Erro ao abrir o arquivo para gravação.");
        return false;
    }
}