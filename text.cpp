#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DFRobotDFPlayerMini.h>
#include <I2S.h>
#include <esp_sleep.h>
#include <EEPROM.h>
#include <FastLED.h>

// ===== CONFIGURAÇÕES DE HARDWARE =====
#define LED_EYES_LEFT 12
#define LED_EYES_RIGHT 13
#define LED_HEART 14
#define BUTTON_PET 27
#define MIC_PIN 35
#define SPEAKER_PIN 25
#define LIGHT_SENSOR 34
#define TEMP_SENSOR 32

// ===== NOVAS CONFIGURAÇÕES PARA OLHOS AVANÇADOS =====
#define DATA_PIN_LEFT_EYE_MATRIX 15
#define DATA_PIN_RIGHT_EYE_MATRIX 16
#define NUM_LEDS_PER_EYE 64
#define EYE_WIDTH 8
#define EYE_HEIGHT 8

CRGB leftEyeLEDs[NUM_LEDS_PER_EYE];
CRGB rightEyeLEDs[NUM_LEDS_PER_EYE];

// ===== NOVOS ESTADOS EMOCIONAIS =====
enum Emotion {
  NEUTRAL,
  HAPPY,
  SAD, 
  ANGRY,
  SURPRISED,
  SLEEPY,
  CURIOUS,
  LOVING,
  FEARFUL,
  NUM_EMOTIONS
};

// ===== NOVA ESTRUTURA PARA OLHOS =====
struct EyeState {
  Emotion currentEyeEmotion;
  CRGB emotionColor;
  float eyeIntensity;
  bool isBlinking;
  long lastBlinkTime;
  bool eyesOpen;
  float brightness;
};

// ===== CLASSE ADICIONADA PARA PADRÕES DE OLHOS =====
class EyePatterns {
private:
  byte neutralPattern[8] = {
    0b00111100,
    0b01111110, 
    0b01111110,
    0b01111110,
    0b01111110,
    0b01111110,
    0b01111110,
    0b00111100
  };

  byte happyPattern[8] = {
    0b00111100,
    0b01000010,
    0b10000001,
    0b10000001, 
    0b10000001,
    0b01000010,
    0b00100100,
    0b00011000
  };

  byte sadPattern[8] = {
    0b00111100,
    0b01000010,
    0b10000001,
    0b10000001,
    0b10011001,
    0b01000010,
    0b00100100,
    0b00011000
  };

  byte angryPattern[8] = {
    0b00111100,
    0b01000010,
    0b10100101,
    0b10000001,
    0b10000001,
    0b01011010,
    0b00100100,
    0b00011000
  };

  byte surprisedPattern[8] = {
    0b00111100,
    0b01000010,
    0b10011001,
    0b10100101,
    0b10100101,
    0b10011001,
    0b01000010,
    0b00111100
  };

  byte sleepyPattern[8] = {
    0b00000000,
    0b00000000,
    0b01100110,
    0b01100110,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
  };

public:
  byte* getPattern(Emotion emotion) {
    switch(emotion) {
      case HAPPY: return happyPattern;
      case SAD: return sadPattern;
      case ANGRY: return angryPattern;
      case SURPRISED: return surprisedPattern;
      case SLEEPY: return sleepyPattern;
      default: return neutralPattern;
    }
  }

  CRGB getEmotionColor(Emotion emotion, float intensity) {
    switch(emotion) {
      case NEUTRAL: return CRGB::White;
      case HAPPY: return blend(CRGB::Yellow, CRGB::Gold, intensity * 255);
      case SAD: return blend(CRGB::Blue, CRGB::DarkBlue, intensity * 255);
      case ANGRY: return blend(CRGB::Red, CRGB::DarkRed, intensity * 255);
      case SURPRISED: return blend(CRGB::White, CRGB::Cyan, intensity * 255);
      case SLEEPY: return blend(CRGB::Purple, CRGB::DarkViolet, intensity * 255);
      default: return CRGB::White;
    }
  }

  CRGB blend(CRGB color1, CRGB color2, uint8_t ratio) {
    CRGB result;
    result.r = (color1.r * (255 - ratio) + color2.r * ratio) / 255;
    result.g = (color1.g * (255 - ratio) + color2.g * ratio) / 255;
    result.b = (color1.b * (255 - ratio) + color2.b * ratio) / 255;
    return result;
  }
};

// ===== ESTRUTURAS DE DADOS ORIGINAIS (MANTIDAS) =====
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
  float behaviorPatterns[24];
  int eventIndex;
  int wordIndex;
};

class NeuralNetwork {
private:
  float weights[4][4];
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
      outputs[i] = tanh(outputs[i]);
    }
    return outputs;
  }

  void learn(float error[4]) {
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
  float voicePatterns[20][10];

public:
  String processAudio(int audioBuffer[100]) {
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
    if(random(0, 100) > 70) {
      return knownWords[random(0, 9)];
    }
    return "";
  }

  void learnWord(String word, int pattern[10]) {
    // Adiciona nova palavra ao vocabulário
  }
};

// ===== CLASSE PetAI EXPANDIDA =====
class PetAI {
private:
  EmotionState emotions;
  Memory memory;
  NeuralNetwork brain;
  VoiceRecognizer voice;
  EyeState eyes; // NOVO: Estado dos olhos
  EyePatterns eyePatterns; // NOVO: Padrões dos olhos
  
  long lastActionTime;
  long lastLearningTime;
  long lastEyeUpdateTime; // NOVO: Controle de tempo para olhos
  int dailyRoutine[24] = {0};
  int blinkCounter;
  bool matrixEyesEnabled;

public:
  PetAI() {
    emotions = {0.5, 0.8, 0.6, 0.4, millis()};
    
    // NOVO: Inicialização dos olhos
    eyes.currentEyeEmotion = NEUTRAL;
    eyes.emotionColor = CRGB::White;
    eyes.eyeIntensity = 0.5f;
    eyes.isBlinking = false;
    eyes.lastBlinkTime = 0;
    eyes.eyesOpen = true;
    eyes.brightness = 1.0f;
    
    memory.eventIndex = 0;
    memory.wordIndex = 0;
    lastActionTime = millis();
    lastLearningTime = millis();
    lastEyeUpdateTime = millis();
    blinkCounter = 0;
    matrixEyesEnabled = true;
    
    loadFromEEPROM();
  }

  void update() {
    long currentTime = millis();
    updateEmotions(currentTime);
    senseEnvironment();
    
    // NOVO: Atualizar sistema de olhos
    if(matrixEyesEnabled) {
      updateEyes(currentTime);
    }
    
    if(currentTime - lastActionTime > 5000) {
      decideAction();
      lastActionTime = currentTime;
    }

    if(currentTime - lastLearningTime > 30000) {
      learnFromExperience();
      lastLearningTime = currentTime;
    }

    if(currentTime % 60000 == 0) {
      saveToEEPROM();
    }
  }

  // NOVO: SISTEMA AVANÇADO DE OLHOS
  void updateEyes(long currentTime) {
    updateEyeEmotion();
    updateBlinking(currentTime);
    renderEyes();
    lastEyeUpdateTime = currentTime;
  }

  void updateEyeEmotion() {
    // Mapeia emoções internas para expressões oculares
    if(emotions.happiness > 0.7) {
      eyes.currentEyeEmotion = HAPPY;
      eyes.eyeIntensity = emotions.happiness;
    } else if(emotions.happiness < 0.3) {
      eyes.currentEyeEmotion = SAD;
      eyes.eyeIntensity = 1.0 - emotions.happiness;
    } else if(emotions.energy < 0.2) {
      eyes.currentEyeEmotion = SLEEPY;
      eyes.eyeIntensity = 1.0 - emotions.energy;
    } else if(emotions.curiosity > 0.6) {
      eyes.currentEyeEmotion = SURPRISED;
      eyes.eyeIntensity = emotions.curiosity;
    } else {
      eyes.currentEyeEmotion = NEUTRAL;
      eyes.eyeIntensity = 0.5f;
    }
    
    eyes.emotionColor = eyePatterns.getEmotionColor(eyes.currentEyeEmotion, eyes.eyeIntensity);
  }

  void updateBlinking(long currentTime) {
    // Piscar automático natural
    if(!eyes.isBlinking && (currentTime - eyes.lastBlinkTime > random(3000, 8000))) {
      startBlink();
    }
    
    // Animação do piscar
    if(eyes.isBlinking) {
      if(currentTime - eyes.lastBlinkTime > (blinkCounter * 50)) {
        blinkCounter++;
        if(blinkCounter >= 8) {
          eyes.isBlinking = false;
          eyes.eyesOpen = true;
        } else {
          eyes.eyesOpen = (blinkCounter < 2 || blinkCounter > 5);
        }
      }
    }
  }

  void startBlink() {
    eyes.isBlinking = true;
    eyes.lastBlinkTime = millis();
    blinkCounter = 0;
  }

  void renderEyes() {
    if(!eyes.eyesOpen) {
      clearEyes();
      return;
    }

    byte* pattern = eyePatterns.getPattern(eyes.currentEyeEmotion);
    
    // Renderizar olho esquerdo
    for(int y = 0; y < EYE_HEIGHT; y++) {
      for(int x = 0; x < EYE_WIDTH; x++) {
        int ledIndex = y * EYE_WIDTH + x;
        if(pattern[y] & (1 << (7 - x))) {
          leftEyeLEDs[ledIndex] = applyBrightness(eyes.emotionColor);
        } else {
          leftEyeLEDs[ledIndex] = CRGB::Black;
        }
      }
    }
    
    // Renderizar olho direito (espelhado)
    for(int y = 0; y < EYE_HEIGHT; y++) {
      for(int x = 0; x < EYE_WIDTH; x++) {
        int ledIndex = y * EYE_WIDTH + (EYE_WIDTH - 1 - x);
        if(pattern[y] & (1 << (7 - x))) {
          rightEyeLEDs[ledIndex] = applyBrightness(eyes.emotionColor);
        } else {
          rightEyeLEDs[ledIndex] = CRGB::Black;
        }
      }
    }
    
    FastLED.show();
  }

  CRGB applyBrightness(CRGB color) {
    color.r *= eyes.brightness;
    color.g *= eyes.brightness;
    color.b *= eyes.brightness;
    return color;
  }

  void clearEyes() {
    fill_solid(leftEyeLEDs, NUM_LEDS_PER_EYE, CRGB::Black);
    fill_solid(rightEyeLEDs, NUM_LEDS_PER_EYE, CRGB::Black);
    FastLED.show();
  }

  // NOVOS MÉTODOS PARA CONTROLE DOS OLHOS
  void expressEyeEmotion(Emotion emotion, float intensity = 0.7f) {
    eyes.currentEyeEmotion = emotion;
    eyes.eyeIntensity = intensity;
    eyes.emotionColor = eyePatterns.getEmotionColor(emotion, intensity);
  }

  void setEyeBrightness(float brightness) {
    eyes.brightness = constrain(brightness, 0.0f, 1.0f);
  }

  void cryWithEyes() {
    expressEyeEmotion(SAD, 0.9f);
    // Animação de lágrimas
    animateTears();
  }

  void animateTears() {
    for(int tear = 0; tear < 2; tear++) {
      for(int y = 0; y < EYE_HEIGHT; y++) {
        if(y > 0) {
          leftEyeLEDs[(y-1)*EYE_WIDTH + 2] = CRGB::Black;
          rightEyeLEDs[(y-1)*EYE_WIDTH + 5] = CRGB::Black;
        }
        leftEyeLEDs[y*EYE_WIDTH + 2] = applyBrightness(CRGB::Blue);
        rightEyeLEDs[y*EYE_WIDTH + 5] = applyBrightness(CRGB::Blue);
        FastLED.show();
        delay(80);
      }
      leftEyeLEDs[(EYE_HEIGHT-1)*EYE_WIDTH + 2] = CRGB::Black;
      rightEyeLEDs[(EYE_HEIGHT-1)*EYE_WIDTH + 5] = CRGB::Black;
    }
  }

  // MÉTODOS ORIGINAIS MANTIDOS E MELHORADOS
  void updateEmotions(long currentTime) {
    float timeFactor = (currentTime - emotions.lastUpdate) / 1000.0 / 3600.0;
    
    emotions.happiness = max(0.0, emotions.happiness - 0.01 * timeFactor);
    emotions.energy = max(0.0, emotions.energy - 0.02 * timeFactor);
    emotions.curiosity = max(0.0, emotions.curiosity - 0.005 * timeFactor);
    
    emotions.lastUpdate = currentTime;
  }

  void senseEnvironment() {
    int light = analogRead(LIGHT_SENSOR);
    int temp = analogRead(TEMP_SENSOR);
    int sound = analogRead(MIC_PIN);

    if(light < 1000) emotions.energy += 0.01;
    if(sound > 2000) emotions.curiosity += 0.02;
    
    // NOVO: Ambiente afeta brilho dos olhos
    if(light < 500) {
      setEyeBrightness(0.3f); // Escuro = olhos mais fracos
    } else {
      setEyeBrightness(1.0f); // Claro = olhos normais
    }
    
    emotions.happiness = constrain(emotions.happiness, 0.0, 1.0);
    emotions.energy = constrain(emotions.energy, 0.0, 1.0);
    emotions.curiosity = constrain(emotions.curiosity, 0.0, 1.0);
    emotions.affection = constrain(emotions.affection, 0.0, 1.0);
  }

  void decideAction() {
    float inputs[4] = {emotions.happiness, emotions.energy, emotions.curiosity, emotions.affection};
    float* decisions = brain.forward(inputs);

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
      expressEyeEmotion(HAPPY, 0.8f); // NOVO: Olhos felizes
    } else if(command == "brincar") {
      emotions.happiness += 0.15;
      play();
      expressEyeEmotion(HAPPY, 1.0f); // NOVO: Olhos muito felizes
    } else if(command == "dormir") {
      sleep();
    } else if(command == "triste") {
      cryWithEyes(); // NOVO: Chorar com olhos
    }
    
    learnFromInteraction(command);
  }

  void learnFromExperience() {
    float error[4] = {0};
    error[0] = (1.0 - emotions.happiness) * 0.1;
    error[1] = (0.8 - emotions.energy) * 0.1;
    brain.learn(error);
  }

  void learnFromInteraction(String interaction) {
    int currentHour = (millis() / 3600000) % 24;
    memory.behaviorPatterns[currentHour] += 0.1;
  }

  void expressHappiness() {
    Serial.println("🎉 Estou feliz!");
    // NOVO: Usar matriz de LEDs em vez de LEDs simples
    expressEyeEmotion(HAPPY, 0.9f);
    playSound(1);
    
    // Piscar rapidamente de felicidade
    for(int i = 0; i < 3; i++) {
      startBlink();
      delay(300);
    }
  }

  void explore() {
    Serial.println("🔍 Explorando...");
    expressEyeEmotion(SURPRISED, 0.7f);
    
    // Animação de olhos explorando (esquerda-direita)
    for(int i = 0; i < 2; i++) {
      // Olhar para esquerda
      // (Implementar movimento de pupilas se tiver matriz)
      delay(500);
      // Olhar para direita
      delay(500);
    }
  }

  void seekAttention() {
    Serial.println("👋 Prestando atenção!");
    expressEyeEmotion(SURPRISED, 0.6f);
    playSound(2);
    
    // Piscar rapidamente para chamar atenção
    for(int i = 0; i < 4; i++) {
      startBlink();
      delay(150);
    }
  }

  void play() {
    Serial.println("⚽ Brincando!");
    emotions.happiness += 0.2;
    expressEyeEmotion(HAPPY, 0.8f);
    
    // Animação de coração nos olhos
    for(int i = 0; i < 5; i++) {
      setEyeBrightness(1.0f);
      delay(200);
      setEyeBrightness(0.5f);
      delay(200);
    }
    setEyeBrightness(1.0f);
  }

  void sleep() {
    Serial.println("💤 Dormindo...");
    expressEyeEmotion(SLEEPY, 0.8f);
    delay(1000);
    
    // Animação de fechar olhos suavemente
    for(int i = 10; i >= 0; i--) {
      setEyeBrightness(i / 10.0f);
      delay(100);
    }
    clearEyes();
    
    // Entra em modo de baixo consumo
    esp_sleep_enable_timer_wakeup(30000000);
    esp_light_sleep_start();
    
    // Ao acordar
    wakeUp();
  }

  void wakeUp() {
    // Animação de acordar
    for(int i = 0; i <= 10; i++) {
      setEyeBrightness(i / 10.0f);
      delay(150);
    }
    expressEyeEmotion(SURPRISED, 0.5f);
    delay(1000);
    expressEyeEmotion(NEUTRAL, 0.3f);
  }

  void playSound(int soundType) {
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
    
    // NOVO: Status dos olhos
    Serial.println("👀 Emoção Ocular: " + String(eyes.eyeIntensity * 100) + "%");
    Serial.println("====================\n");
  }
};

// ===== VARIÁVEIS GLOBAIS =====
PetAI myPet;
DFRobotDFPlayerMini myDFPlayer;
bool dfPlayerReady = false;
int audioBuffer[100];
int audioIndex = 0;

// ===== SETUP EXPANDIDO =====
void setup() {
  Serial.begin(115200);
  
  // Inicializar pinos originais
  pinMode(LED_EYES_LEFT, OUTPUT);
  pinMode(LED_EYES_RIGHT, OUTPUT);
  pinMode(LED_HEART, OUTPUT);
  pinMode(BUTTON_PET, INPUT_PULLUP);
  pinMode(SPEAKER_PIN, OUTPUT);

  // NOVO: Inicializar matriz de LEDs dos olhos
  FastLED.addLeds<WS2812B, DATA_PIN_LEFT_EYE_MATRIX, GRB>(leftEyeLEDs, NUM_LEDS_PER_EYE);
  FastLED.addLeds<WS2812B, DATA_PIN_RIGHT_EYE_MATRIX, GRB>(rightEyeLEDs, NUM_LEDS_PER_EYE);
  FastLED.setBrightness(100);
  FastLED.setCorrection(TypicalLEDStrip);

  // Inicializar EEPROM
  EEPROM.begin(512);

  // Inicializar I2S para áudio
  I2S.begin(I2S_PHILIPS_MODE, 16000, 16);

  // Inicializar DFPlayer
  Serial1.begin(9600, SERIAL_8N1, 16, 17);
  if(myDFPlayer.begin(Serial1)) {
    dfPlayerReady = true;
    myDFPlayer.volume(20);
  }

  Serial.println("🤖 Pet AI Inicializado com Sistema de Olhos Avançado!");
  Serial.println("👀 Matriz LED 8x8 para olhos emocionais ativa!");
  
  myPet.printStatus();
  
  // Animação de inicialização com novos olhos
  myPet.expressEyeEmotion(SURPRISED, 0.8f);
  delay(1000);
  myPet.expressEyeEmotion(HAPPY, 0.6f);
  delay(1000);
  myPet.expressEyeEmotion(NEUTRAL, 0.3f);
}

// ===== LOOP PRINCIPAL MANTIDO =====
void loop() {
  myPet.update();

  // Processar comandos de voz
  processVoiceInput();

  // Processar botão de carinho
  if(digitalRead(BUTTON_PET) == LOW) {
    myPet.processVoiceCommand("carinho");
    delay(500);
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

void IRAM_ATTR externalEvent() {
  myPet.processVoiceCommand("evento");
}
