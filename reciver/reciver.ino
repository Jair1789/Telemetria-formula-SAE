#include <SPI.h>
#include <LoRa.h>

#define SS_PIN 18    // Pin do ESP32 conectado ao pino SS do módulo LoRa
#define RST_PIN 14   // Pin do ESP32 conectado ao pino RST do módulo LoRa
#define DI0_PIN 26   // Pin do ESP32 conectado ao pino DIO0 do módulo LoRa

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Inicializa o módulo LoRa
  LoRa.setPins(SS_PIN, RST_PIN, DI0_PIN);
  if (!LoRa.begin(915E6)) {
    Serial.println("Falha na inicialização do módulo LoRa");
    while (1);
  }
}

void loop() {
  // Aguarda a recepção de dados
  if (LoRa.parsePacket()) {
    // Lê o valor recebido via LoRa
    String receivedMessage = LoRa.readString();

    // Imprime o valor recebido
    Serial.println(receivedMessage);
  }
}
