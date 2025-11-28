#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DFRobotDFPlayerMini.h>
#include <I2S.h>
#include <esp_sleep.h>
#include <EEPROM.h>

// ===== CONFIGURAÇÕES DE HARDWARE =====
#define LED_EYES_LEFT 12
#define LED_EYES_RIGHT 13
#define LED_HEART 14
#define BUTTON_PET 27
#define MIC_PIN 35
#define SPEAKER_PIN 25
#define LIGHT_SENSOR 34
#define TEMP_SENSOR 32

// ===== ESTRUTURAS DE DADOS =====
struct EmotionState {
  float happiness;
  float energy;
  float curiosity;
  float affection;
  long lastUpdate;
};

struct Memory {
  String events[100];
  String learnedWords[50];
  float behaviorPatterns[24]; // Padrões por hora do dia
  int eventIndex;
  int wordIndex;
};

class NeuralNetwork {
private:
  float weights[4][4]; // Rede neural simples para tomada de decisão
  float biases[4];

public:
  NeuralNetwork() {
    randomizeWeights();
  }

  void randomizeWeights() {
    for(int i = 0; i < 4; i++) {
      for(int j = 0; j < 4; j++) {
        weights[i][j] = random(-100, 100) / 100.0;
      }
      biases[i] = random(-50, 50) / 100.0;
    }
  }

  float* forward(float inputs[4]) {
    static float outputs[4];
    for(int i = 0; i < 4; i++) {
      outputs[i] = biases[i];
      for(int j = 0; j < 4; j++) {
        outputs[i] += inputs[j] * weights[i][j];
      }
      outputs[i] = tanh(outputs[i]); // Função de ativação
    }
    return outputs;
  }

  void learn(float error[4]) {
    // Aprendizado por reforço simples
    for(int i = 0; i < 4; i++) {
      biases[i] += error[i] * 0.1;
      for(int j = 0; j < 4; j++) {
        weights[i][j] += error[i] * 0.05;
      }
    }
  }
};

class VoiceRecognizer {
private:
  String knownWords[20] = {"ola", "bom", "ruim", "fome", "sede", "brincar", "dormir", "sim", "nao"};
  float voicePatterns[20][10]; // Padrões de voz armazenados

public:
  String processAudio(int audioBuffer[100]) {
    // Simulação de processamento de áudio
    float volume = calculateVolume(audioBuffer);
    if(volume > 500) {
      return recognizeWord(audioBuffer);
    }
    return "";
  }

  float calculateVolume(int buffer[100]) {
    long sum = 0;
    for(int i = 0; i < 100; i++) {
      sum += abs(buffer[i]);
    }
    return sum / 100.0;
  }

  String recognizeWord(int buffer[100]) {
    // Simulação de reconhecimento de palavra
    if(random(0, 100) > 70) { // 30% de chance de reconhecer
      return knownWords[random(0, 9)];
    }
    return "";
  }

  void learnWord(String word, int pattern[10]) {
    // Adiciona nova palavra ao vocabulário
  }
};

class PetAI {
private:
  EmotionState emotions;
  Memory memory;
  NeuralNetwork brain;
  VoiceRecognizer voice;
  long lastActionTime;
  long lastLearningTime;
  int dailyRoutine[24] = {0}; // Rotina diária aprendida

public:
  PetAI() {
    emotions = {0.5, 0.8, 0.6, 0.4, millis()};
    memory.eventIndex = 0;
    memory.wordIndex = 0;
    lastActionTime = millis();
    lastLearningTime = millis();
    loadFromEEPROM();
  }

  void update() {
    long currentTime = millis();
    updateEmotions(currentTime);
    senseEnvironment();
    
    if(currentTime - lastActionTime > 5000) { // A cada 5 segundos
      decideAction();
      lastActionTime = currentTime;
    }

    if(currentTime - lastLearningTime > 30000) { // A cada 30 segundos
      learnFromExperience();
      lastLearningTime = currentTime;
    }

    if(currentTime % 60000 == 0) { // A cada minuto
      saveToEEPROM();
    }
  }

  void updateEmotions(long currentTime) {
    float timeFactor = (currentTime - emotions.lastUpdate) / 1000.0 / 3600.0; // Horas
    
    // Emoções decaem naturalmente com o tempo
    emotions.happiness = max(0.0, emotions.happiness - 0.01 * timeFactor);
    emotions.energy = max(0.0, emotions.energy - 0.02 * timeFactor);
    emotions.curiosity = max(0.0, emotions.curiosity - 0.005 * timeFactor);
    
    emotions.lastUpdate = currentTime;
  }

  void senseEnvironment() {
    int light = analogRead(LIGHT_SENSOR);
    int temp = analogRead(TEMP_SENSOR);
    int sound = analogRead(MIC_PIN);

    // Ambiente influencia emoções
    if(light < 1000) emotions.energy += 0.01; // Escuro = mais energia
    if(sound > 2000) emotions.curiosity += 0.02; // Sons altos = curiosidade
    
    emotions.happiness = constrain(emotions.happiness, 0.0, 1.0);
    emotions.energy = constrain(emotions.energy, 0.0, 1.0);
    emotions.curiosity = constrain(emotions.curiosity, 0.0, 1.0);
    emotions.affection = constrain(emotions.affection, 0.0, 1.0);
  }

  void decideAction() {
    float inputs[4] = {emotions.happiness, emotions.energy, emotions.curiosity, emotions.affection};
    float* decisions = brain.forward(inputs);

    // Executa ação baseada na decisão da rede neural
    if(decisions[0] > 0.7) {
      expressHappiness();
    } else if(decisions[1] > 0.6) {
      explore();
    } else if(decisions[2] > 0.5) {
      seekAttention();
    } else if(decisions[3] < 0.3) {
      sleep();
    }
  }

  void processVoiceCommand(String command) {
    addToMemory("Voice: " + command);
    
    if(command == "ola") {
      emotions.affection += 0.1;
      expressHappiness();
    } else if(command == "brincar") {
      emotions.happiness += 0.15;
      play();
    } else if(command == "dormir") {
      sleep();
    }
    
    learnFromInteraction(command);
  }

  void learnFromExperience() {
    // Aprendizado por reforço
    float error[4] = {0};
    error[0] = (1.0 - emotions.happiness) * 0.1; // Busca felicidade
    error[1] = (0.8 - emotions.energy) * 0.1; // Mantém energia
    brain.learn(error);
  }

  void learnFromInteraction(String interaction) {
    // Aprende padrões de interação
    int currentHour = (millis() / 3600000) % 24;
    memory.behaviorPatterns[currentHour] += 0.1;
  }

  void expressHappiness() {
    Serial.println("🎉 Estou feliz!");
    digitalWrite(LED_EYES_LEFT, HIGH);
    digitalWrite(LED_EYES_RIGHT, HIGH);
    playSound(1); // Som feliz
    delay(1000);
    digitalWrite(LED_EYES_LEFT, LOW);
    digitalWrite(LED_EYES_RIGHT, LOW);
  }

  void explore() {
    Serial.println("🔍 Explorando...");
    // Simulação de movimento de exploração
    for(int i = 0; i < 3; i++) {
      digitalWrite(LED_EYES_LEFT, HIGH);
      delay(200);
      digitalWrite(LED_EYES_LEFT, LOW);
      digitalWrite(LED_EYES_RIGHT, HIGH);
      delay(200);
      digitalWrite(LED_EYES_RIGHT, LOW);
    }
  }

  void seekAttention() {
    Serial.println("👋 Prestando atenção!");
    playSound(2); // Som de chamada
    blinkEyes(5, 100);
  }

  void play() {
    Serial.println("⚽ Brincando!");
    emotions.happiness += 0.2;
    for(int i = 0; i < 10; i++) {
      digitalWrite(LED_HEART, HIGH);
      delay(100);
      digitalWrite(LED_HEART, LOW);
      delay(100);
    }
  }

  void sleep() {
    Serial.println("💤 Dormindo...");
    digitalWrite(LED_EYES_LEFT, LOW);
    digitalWrite(LED_EYES_RIGHT, LOW);
    
    // Entra em modo de baixo consumo
    esp_sleep_enable_timer_wakeup(30000000); // Acorda após 30 segundos
    esp_light_sleep_start();
  }

  void blinkEyes(int times, int delayMs) {
    for(int i = 0; i < times; i++) {
      digitalWrite(LED_EYES_LEFT, HIGH);
      digitalWrite(LED_EYES_RIGHT, HIGH);
      delay(delayMs);
      digitalWrite(LED_EYES_LEFT, LOW);
      digitalWrite(LED_EYES_RIGHT, LOW);
      delay(delayMs);
    }
  }

  void playSound(int soundType) {
    // Implementar síntese de som ou usar DFPlayer
    tone(SPEAKER_PIN, 1000 + (soundType * 500), 500);
  }

  void addToMemory(String event) {
    memory.events[memory.eventIndex] = event;
    memory.eventIndex = (memory.eventIndex + 1) % 100;
  }

  void saveToEEPROM() {
    EEPROM.put(0, emotions);
    EEPROM.put(sizeof(EmotionState), memory);
    EEPROM.commit();
  }

  void loadFromEEPROM() {
    EEPROM.get(0, emotions);
    EEPROM.get(sizeof(EmotionState), memory);
  }

  void printStatus() {
    Serial.println("\n=== STATUS DO PET ===");
    Serial.println("😊 Felicidade: " + String(emotions.happiness * 100) + "%");
    Serial.println("⚡ Energia: " + String(emotions.energy * 100) + "%");
    Serial.println("🔍 Curiosidade: " + String(emotions.curiosity * 100) + "%");
    Serial.println("❤️ Afeição: " + String(emotions.affection * 100) + "%");
    Serial.println("====================\n");
  }
};

// ===== VARIÁVEIS GLOBAIS =====
PetAI myPet;
DFRobotDFPlayerMini myDFPlayer;
bool dfPlayerReady = false;

// Buffer para áudio
int audioBuffer[100];
int audioIndex = 0;

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  
  // Inicializar pinos
  pinMode(LED_EYES_LEFT, OUTPUT);
  pinMode(LED_EYES_RIGHT, OUTPUT);
  pinMode(LED_HEART, OUTPUT);
  pinMode(BUTTON_PET, INPUT_PULLUP);
  pinMode(SPEAKER_PUT, OUTPUT);

  // Inicializar EEPROM
  EEPROM.begin(512);

  // Inicializar I2S para áudio (se disponível)
  I2S.begin(I2S_PHILIPS_MODE, 16000, 16);

  // Inicializar DFPlayer
  Serial1.begin(9600, SERIAL_8N1, 16, 17);
  if(myDFPlayer.begin(Serial1)) {
    dfPlayerReady = true;
    myDFPlayer.volume(20);
  }

  Serial.println("🤖 Pet AI Inicializado!");
  Serial.println("Comandos de voz: ola, bom, ruim, fome, sede, brincar, dormir");
  
  myPet.printStatus();
  
  // Piscar olhos na inicialização
  digitalWrite(LED_EYES_LEFT, HIGH);
  digitalWrite(LED_EYES_RIGHT, HIGH);
  delay(1000);
  digitalWrite(LED_EYES_LEFT, LOW);
  digitalWrite(LED_EYES_RIGHT, LOW);
}

// ===== LOOP PRINCIPAL =====
void loop() {
  myPet.update();

  // Processar comandos de voz
  processVoiceInput();

  // Processar botão de carinho
  if(digitalRead(BUTTON_PET) == LOW) {
    myPet.processVoiceCommand("carinho");
    delay(500); // Debounce
  }

  // Exibir status a cada 30 segundos
  static unsigned long lastStatusTime = 0;
  if(millis() - lastStatusTime > 30000) {
    myPet.printStatus();
    lastStatusTime = millis();
  }

  delay(100);
}

void processVoiceInput() {
  // Coletar amostras de áudio
  if(audioIndex < 100) {
    audioBuffer[audioIndex] = analogRead(MIC_PIN);
    audioIndex++;
  } else {
    String command = myPet.processVoiceCommand(audioBuffer);
    if(command != "") {
      Serial.println("🎤 Comando reconhecido: " + command);
      myPet.processVoiceCommand(command);
    }
    audioIndex = 0;
  }
}

// Função de interrupção para eventos externos
void IRAM_ATTR externalEvent() {
  myPet.processVoiceCommand("evento");
}
