#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD Display inställningar (16x2 med I2C)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Buzzer pin
#define BUZZER_PIN D5

// I2C pinnar för WEMOS D1 Mini
#define SDA_PIN D2
#define SCL_PIN D1

// WiFi credentials - Change this!
const char* ssid = "Wifi";
const char* password = "Password";

// Web server details - Change this!
const char* serverAddress = "http://192.168.1.104:5000";
const char* loginEndpoint = "/login";

// Login credentials - Change this!
const char* loginUsername = "sa7bnb";
const char* loginPassword = "pocsag2025";

// Session management
String sessionCookie = "";
bool isLoggedIn = false;
unsigned long lastLoginTime = 0;
const unsigned long sessionTimeout = 3600000; // 60 minuter
const unsigned long loginRetryInterval = 300000; // 5 minuter

// Variables to track state
String lastMessage = "";
unsigned long lastFetchTime = 0;
const unsigned long fetchInterval = 60000; // 60 sekunder
bool lcdInitialized = false;
unsigned long messageScrollTime = 0;
int scrollPosition = 0;
String currentDisplayMessage = "";
bool isScrolling = false;
int consecutiveFailures = 0;
const int maxFailures = 5;

// Anpassade tecken för svenska bokstäver - från Arduino forum exempel
byte char_a_ring[8] = {   // å
  B00100,
  B01010,
  B01110,
  B00001,
  B01111,
  B10001,
  B01111,
  B00000
};

byte char_a_dots[8] = {   // ä
  B01010,
  B00000,
  B01110,
  B00001,
  B01111,
  B10001,
  B01111,
  B00000
};

byte char_o_dots[8] = {   // ö
  B01010,
  B00000,
  B01110,
  B10001,
  B10001,
  B10001,
  B01110,
  B00000
};

byte char_A_ring[8] = {   // Å
  B00100,
  B01010,
  B01110,
  B10001,
  B11111,
  B10001,
  B10001,
  B00000
};

byte char_A_dots[8] = {   // Ä
  B01010,
  B00000,
  B01110,
  B10001,
  B11111,
  B10001,
  B10001,
  B00000
};

byte char_O_dots[8] = {   // Ö
  B01010,
  B01110,
  B10001,
  B10001,
  B10001,
  B10001,
  B01110,
  B00000
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP8266 Pager...");
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  Wire.begin(SDA_PIN, SCL_PIN);
  initializeLCD();
  showStartupScreen();
  connectToWiFi();
  
  Serial.println("System ready - will login and check for messages");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Hantera scrollning
  if (isScrolling && currentDisplayMessage.length() > 0) {
    handleMessageScrolling();
  }
  
  // Logga in vid start eller efter timeout
  if (!isLoggedIn) {
    Serial.println("Need to login - attempting once...");
    
    if (performLogin()) {
      Serial.println("Login successful - valid for 60 minutes");
      updateDisplay("Ready");
      // Behåll lastMessage så vi inte får falska alerts efter reconnect
      if (lastMessage.length() == 0) {
        lastMessage = "Ready";
      }
      currentDisplayMessage = "Ready";
    } else {
      Serial.println("Login failed - will retry in 5 minutes");
      updateDisplay("Login failed");
      delay(loginRetryInterval);
      return;
    }
  }
  
  // Kontrollera session timeout
  if (isLoggedIn && lastLoginTime > 0 && (currentTime > lastLoginTime)) {
    unsigned long sessionAge = currentTime - lastLoginTime;
    if (sessionAge > sessionTimeout) {
      Serial.println("Session expired after " + String(sessionAge/60000) + " minutes");
      isLoggedIn = false;
      lastLoginTime = 0;
      return;
    }
  }
  
  // Kolla efter nya meddelanden
  if (isLoggedIn && (currentTime - lastFetchTime >= fetchInterval || lastFetchTime == 0)) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("=== Checking for messages ===");
      checkForNewMessage();
      lastFetchTime = currentTime;
    } else {
      Serial.println("WiFi disconnected - reconnecting...");
      connectToWiFi();
    }
  }
  
  delay(50);
}

void initializeLCD() {
  lcd.init();
  lcd.backlight();
  
  // Ladda anpassade tecken för svenska bokstäver - förbättrade definitioner
  lcd.createChar(1, char_a_ring);  // å
  lcd.createChar(2, char_a_dots);  // ä
  lcd.createChar(3, char_o_dots);  // ö
  lcd.createChar(4, char_A_ring);  // Å
  lcd.createChar(5, char_A_dots);  // Ä
  lcd.createChar(6, char_O_dots);  // Ö
  
  lcdInitialized = true;
  Serial.println("LCD initialized with improved Swedish characters");
}

void showStartupScreen() {
  if (!lcdInitialized) return;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  lcd.setCursor(0, 1);
  lcd.print("PagerWiFi...");
  delay(2000);
}

// Funktion för att konvertera svenska tecken till anpassade LCD-tecken
String convertSwedishChars(String text) {
  // Ersätt svenska tecken med anpassade tecken (1-6)
  text.replace("å", String(char(1)));   // å -> anpassat tecken 1
  text.replace("ä", String(char(2)));   // ä -> anpassat tecken 2  
  text.replace("ö", String(char(3)));   // ö -> anpassat tecken 3
  text.replace("Å", String(char(4)));   // Å -> anpassat tecken 4
  text.replace("Ä", String(char(5)));   // Ä -> anpassat tecken 5
  text.replace("Ö", String(char(6)));   // Ö -> anpassat tecken 6
  
  // Hantera även UTF-8 byte-sekvenser direkt
  text.replace("\xC3\xA5", String(char(1))); // å UTF-8 bytes
  text.replace("\xC3\xA4", String(char(2))); // ä UTF-8 bytes  
  text.replace("\xC3\xB6", String(char(3))); // ö UTF-8 bytes
  text.replace("\xC3\x85", String(char(4))); // Å UTF-8 bytes
  text.replace("\xC3\x84", String(char(5))); // Ä UTF-8 bytes
  text.replace("\xC3\x96", String(char(6))); // Ö UTF-8 bytes
  
  return text;
}

void connectToWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi:");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.println("IP: " + WiFi.localIP().toString());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());
    delay(3000);
    
    isLoggedIn = false;
    sessionCookie = "";
  } else {
    Serial.println("\nWiFi connection failed!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi FAILED");
    delay(5000);
  }
}

bool performLogin() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No WiFi connection for login");
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  
  String loginUrl = String(serverAddress) + loginEndpoint;
  http.begin(client, loginUrl);
  http.setTimeout(15000);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("User-Agent", "ESP8266-ALPHA-Monitor/1.0");
  
  http.collectHeaders(new const char*[10]{"Set-Cookie", "Location", "Content-Type", "Server", "Date", "Cache-Control", "Expires", "Pragma", "Connection", "Content-Length"}, 10);
  
  String postData = "username=" + String(loginUsername) + "&password=" + String(loginPassword);
  
  Serial.println("Attempting login to: " + loginUrl);
  
  int httpResponseCode = http.POST(postData);
  Serial.println("Login response code: " + String(httpResponseCode));
  
  if (httpResponseCode == 302) {
    if (http.hasHeader("Set-Cookie")) {
      sessionCookie = http.header("Set-Cookie");
      Serial.println("*** FOUND SESSION COOKIE! ***");
      
      http.end();
      
      // Testa cookien
      http.begin(client, String(serverAddress) + "/");
      http.setTimeout(10000);
      http.addHeader("User-Agent", "ESP8266-ALPHA-Monitor/1.0");
      http.addHeader("Cookie", sessionCookie);
      
      int testResponse = http.GET();
      Serial.println("Cookie test response: " + String(testResponse));
      
      if (testResponse == HTTP_CODE_OK) {
        Serial.println("SUCCESS! Cookie works!");
        isLoggedIn = true;
        lastLoginTime = millis();
        consecutiveFailures = 0;
        http.end();
        return true;
      }
      
      http.end();
    }
  }
  
  Serial.println("Login failed");
  consecutiveFailures++;
  isLoggedIn = false;
  sessionCookie = "";
  
  http.end();
  return false;
}

void checkForNewMessage() {
  if (!isLoggedIn) {
    Serial.println("Not logged in - cannot fetch messages");
    return;
  }

  WiFiClient client;
  HTTPClient http;
  
  String dataUrl = String(serverAddress) + "/messages";
  
  http.begin(client, dataUrl);
  http.setTimeout(15000);
  http.addHeader("User-Agent", "ESP8266-ALPHA-Monitor/1.0");
  http.addHeader("Accept", "application/json");
  
  if (sessionCookie.length() > 0) {
    http.addHeader("Cookie", sessionCookie);
  }
  
  int httpResponseCode = http.GET();
  Serial.println("Messages endpoint response: " + String(httpResponseCode));
  
  if (httpResponseCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Got JSON payload, length: " + String(payload.length()));
    
    consecutiveFailures = 0;
    
    String alphaText = extractAlphaFromJson(payload);
    
    if (alphaText.length() > 0) {
      Serial.println("*** FOUND ALPHA TEXT: '" + alphaText + "' ***");
      
      // Konvertera både det nya meddelandet och det lagrade för korrekt jämförelse
      String convertedAlphaText = convertSwedishChars(alphaText);
      String convertedLastMessage = convertSwedishChars(lastMessage);
      
      if (convertedAlphaText != convertedLastMessage) {
        Serial.println("*** NEW ALPHA MESSAGE! ***");
        updateDisplay(alphaText);
        playAlert();
        lastMessage = alphaText;
        currentDisplayMessage = convertedAlphaText;
        resetScrolling();
      } else {
        Serial.println("Same message as before - no alert");
        // Uppdatera display ändå för att säkerställa korrekt visning efter reconnect
        updateDisplay(alphaText);
        currentDisplayMessage = convertedAlphaText;
      }
    } else {
      Serial.println("No Alpha content found in JSON");
      if (lastMessage.length() == 0) {
        updateDisplay("No Alpha messages");
      }
    }
    
    http.end();
    return;
  }
  
  consecutiveFailures++;
  Serial.println("Messages endpoint request failed - failure count: " + String(consecutiveFailures));
  
  if (consecutiveFailures >= maxFailures) {
    Serial.println("Too many failures - session expired");
    isLoggedIn = false;
    sessionCookie = "";
    consecutiveFailures = 0;
  }
  
  http.end();
}

String extractAlphaFromJson(const String& jsonPayload) {
  Serial.println("=== PARSING JSON FOR FILTERED MESSAGES ===");
  
  // Leta efter "filtered" array
  int filteredPos = jsonPayload.indexOf("\"filtered\":[");
  if (filteredPos == -1) {
    Serial.println("No 'filtered' array found in JSON");
    return "";
  }
  
  Serial.println("Found filtered array at position " + String(filteredPos));
  
  // Hitta första meddelandet i filtered array
  int arrayStart = jsonPayload.indexOf("[", filteredPos) + 1;
  int firstQuote = jsonPayload.indexOf("\"", arrayStart);
  
  if (firstQuote == -1) {
    Serial.println("No messages in filtered array");
    return "";
  }
  
  int secondQuote = jsonPayload.indexOf("\"", firstQuote + 1);
  if (secondQuote == -1) {
    Serial.println("Malformed JSON - missing closing quote");
    return "";
  }
  
  String latestMessage = jsonPayload.substring(firstQuote + 1, secondQuote);
  Serial.println("=== RAW MESSAGE FROM JSON ===");
  Serial.println(latestMessage);
  Serial.println("=== END RAW MESSAGE ===");
  
  // Leta efter "Alpha:" i meddelandet
  int alphaPos = latestMessage.indexOf("Alpha:");
  if (alphaPos == -1) {
    Serial.println("No 'Alpha:' found in message");
    return "";
  }
  
  // Extrahera Alpha-texten (allt efter "Alpha: ")
  String alphaText = latestMessage.substring(alphaPos + 6);
  alphaText.trim();
  
  // Rensa JSON escape-tecken och konvertera specifika Unicode sekvenser
  alphaText.replace("\\\"", "\"");
  alphaText.replace("\\n", " ");
  alphaText.replace("\\t", " ");
  alphaText.replace("\\r", "");
  
  // Konvertera Unicode escape-sekvenser för svenska tecken till rätt format
  alphaText.replace("\\u00d6", "Ö");  // Ö
  alphaText.replace("\\u00D6", "Ö");  // Ö
  alphaText.replace("\\u00c4", "Ä");  // Ä
  alphaText.replace("\\u00C4", "Ä");  // Ä
  alphaText.replace("\\u00c5", "Å");  // Å
  alphaText.replace("\\u00C5", "Å");  // Å
  alphaText.replace("\\u00f6", "ö");  // ö
  alphaText.replace("\\u00F6", "ö");  // ö
  alphaText.replace("\\u00e4", "ä");  // ä
  alphaText.replace("\\u00E4", "ä");  // ä
  alphaText.replace("\\u00e5", "å");  // å
  alphaText.replace("\\u00E5", "å");  // å
  
  // Ta bort extra whitespace
  while (alphaText.indexOf("  ") != -1) {
    alphaText.replace("  ", " ");
  }
  
  Serial.println("Extracted Alpha text (before Swedish conversion): '" + alphaText + "'");
  
  // Konvertera svenska tecken för LCD-visning
  alphaText = convertSwedishChars(alphaText);
  
  return alphaText;
}

void updateDisplay(String message) {
  if (!lcdInitialized) return;
  
  Serial.println("Updating display with: " + message);
  
  // Konvertera svenska tecken innan visning
  message = convertSwedishChars(message);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("===PagerWiFi===");
  
  if (message.length() > 16) {
    isScrolling = true;
    currentDisplayMessage = message;
    resetScrolling();
    Serial.println("Starting scrolling mode");
  } else {
    isScrolling = false;
    lcd.setCursor(0, 1);
    lcd.print(message);
    Serial.println("Static display mode");
  }
}

void resetScrolling() {
  scrollPosition = 0;
  messageScrollTime = millis();
}

void playAlert() {
  Serial.println("*** PLAYING ALERT - NEW MESSAGE! ***");
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

void handleMessageScrolling() {
  if (!lcdInitialized || currentDisplayMessage.length() == 0 || !isScrolling) {
    return;
  }
  
  if (millis() - messageScrollTime > 400) {
    messageScrollTime = millis();
    
    lcd.setCursor(0, 1);
    lcd.print("                "); // Rensa raden
    lcd.setCursor(0, 1);
    
    String displayText;
    int textLength = currentDisplayMessage.length();
    
    if (scrollPosition < textLength) {
      displayText = currentDisplayMessage.substring(scrollPosition, 
                                                   min(scrollPosition + 16, textLength));
    } else {
      displayText = "";
    }
    
    // Fyll ut med mellanslag
    while (displayText.length() < 16) {
      displayText += " ";
    }
    
    lcd.print(displayText);
    
    scrollPosition++;
    
    // Starta om scrollningen
    if (scrollPosition >= textLength + 4) {
      scrollPosition = 0;
      delay(1000);
    }
  }
}