#include <Wire.h>
#include <MPU6050.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <LoRa.h>


#define SENSOR_PIN  13 // ESP32 pin GPIO17 connected to DS18B20 sensor's DATA pin
#define THROTTLE_PIN      37
#define BRAKE_PIN         39
#define SS_PIN 18    // Pin do ESP32 conectado ao pino SS do módulo LoRa
#define RST_PIN 14   // Pin do ESP32 conectado ao pino RST do módulo LoRa
#define DI0_PIN 26   // Pin do ESP32 conectado ao pino DIO0 do módulo LoRa
#define PIN_CALIBRATE 12
#define MAX_SAMPLES 100



OneWire oneWire(SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

float tempC;

MPU6050 mpu;

int cont = 0;
//int contAc = 0;
bool restartDone = false;
int zeroTH= 1, totalTH=4095; 
int zeroBR= 1, totalBR=4095;


// Função de média móvel
float mediaMovel(float leitura[], int numAmostras) {
  float soma = 0.0;
  for (int i = 0; i < numAmostras; i++) {
    soma += leitura[i];
  }
  return soma / numAmostras;
}

// Função para atualizar a leitura e retornar o valor filtrado
float filtrar(float novaLeitura) {
  static float leituras[MAX_SAMPLES];
  static int indice = 0;
  static int numAmostras = 0;

  // Adicionar a nova leitura ao array de leituras
  leituras[indice] = novaLeitura;

  // Atualizar o índice circularmente
  indice = (indice + 1) % MAX_SAMPLES;

  // Incrementar o número de amostras se for menor que o máximo
  if (numAmostras < MAX_SAMPLES) {
    numAmostras++;
  }

  // Retornar o valor filtrado pela média móvel
  return mediaMovel(leituras, numAmostras);
}

void setup() {
  
  Serial.begin(115200);
  Wire.begin();
  DS18B20.begin(); 

  while (!Serial);

  // Inicializa o módulo LoRa
  LoRa.setPins(SS_PIN, RST_PIN, DI0_PIN);
  if (!LoRa.begin(915E6)) {
    Serial.println("Falha na inicialização do módulo LoRa");
    while (1);
  }
  
  mpu.initialize();
  
  Serial.println("Inicialização do MPU6050");
  Serial.println("Ajustando o giroscópio...");
  mpu.setFullScaleGyroRange(0); // +/- 250 graus por segundo
  delay(100);
  
  Serial.println("Ajustando o acelerômetro...");
  mpu.setFullScaleAccelRange(0); // +/- 2g (use 3 para 2g)
  pinMode(PIN_CALIBRATE, INPUT);
  delay(100);
}

void loop() {
  int16_t ax, ay, az, gx, gy, gz;
  bool buttonState = digitalRead(PIN_CALIBRATE);
  float accelXOffset, accelYOffset,accelZOffset;
 
  DS18B20.requestTemperatures();       // send the command to get temperatures
//
//  if (contAc == 0) { // Realiza calibração automática na primeira medição
//    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
//    accelXOffset = -ax / 16384.0; // Calcula o offset para a leitura do acelerômetro
//    accelYOffset = -ay / 16384.0;
//    accelZOffset = -az / 16384.0;
//    cont += 1;
//  }

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  float accelX = (ax / 16384.0);
  float accelY = (ay / 16384.0);
  float accelZ = (az / 16384.0);
  accelX = filtrar(accelX);
  accelY = filtrar(accelY);
  accelY = filtrar(accelZ);

  String Ax ="AX" + String(accelX,2)+ ";" + String(accelY, 2);
  String Az ="AZ" + String(accelZ,2);

  Serial.println(Ax);
  Serial.println(Az);

  float gyroX = gx / 131.0; // 131 LSB por grau por segundo (quando configurado para +/- 250 graus por segundo)
  float gyroY = gy / 131.0;
  float gyroZ = gz / 131.0;
  gyroX = filtrar(gyroX);
  gyroY = filtrar(gyroY);
  gyroZ = filtrar(gyroZ);
  
  String Gx ="GX" + String(gyroX,2)+ ";" + String(gyroY, 2);
  String Gz ="GZ" + String(gyroZ,2);

  Serial.println(Gx);
  Serial.println(Gz);

 
  //---------------------Calibração de pedais--------------------------------------------------

  
  bool calibrate = digitalRead(PIN_CALIBRATE);
  int ThrottleValue = analogRead(THROTTLE_PIN);
  int BrakeValue = analogRead(BRAKE_PIN);
  if (cont == 0){
    zeroTH =  ThrottleValue;
    zeroBR =  BrakeValue;
    cont=+1;
  }
  if (calibrate == true){
    totalTH = ThrottleValue;
    totalBR = BrakeValue;
  }

  //---------------------Leitura acelerador--------------------------------------------------

  float Throttle = map(ThrottleValue,zeroTH, totalTH, 0, 100);
  Throttle = filtrar(Throttle);
  Throttle = constrain(Throttle, 0.00, 100.00);
  String Th ="TH" + String(Throttle,2);

  Serial.println(Th);

  //---------------------Leitura Freio--------------------------------------------------

  float Brake = map(BrakeValue,zeroBR, totalBR, 0, 100);
  Brake = filtrar(Brake);
  Brake = constrain(Brake, 0.00, 100.00);
  String Br ="BR" + String(Brake,2);

  Serial.println(Br);
  
  //---------------------Leitura Temperatura--------------------------------------------------

  tempC = DS18B20.getTempCByIndex(0);
  tempC =tempC+18;
  String Tm ="TM" + String(tempC,2);
  
  Serial.println(Tm);

//---------------------Envio de Pacote--------------------------------------------------

  LoRa.beginPacket();
  
  LoRa.println(Ax);
  LoRa.println(Az);
  LoRa.println(Gx);
  LoRa.println(Gz);
  LoRa.println(Th);
  LoRa.println(Br);
  LoRa.println(Tm);
  
  LoRa.endPacket();
  
  delay(200);

}
