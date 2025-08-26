# ===PagerWiFi===

## Översikt

PagerWiFi är en IoT-enhet som automatiskt hämtar och visar POCSAG ALPHA-meddelanden från pocsag2025 webbserver.
Enheten loggar in på servern, hämtar filtrerade meddelanden i JSON-format och visar dem på en LCD-skärm med ljudalarm för nya meddelanden.

## Funktioner

- **Automatisk inloggning** - Loggar in på pocsag2025 servern med sessionhantering
- **JSON-datahantering** - Hämtar och parsar meddelanden från `/messages` endpoint
- **Filtrerade meddelanden** - Visar endast meddelanden från `filtered` array
- **LCD-display** - 16x2 tecken LCD med I2C-gränssnitt
- **Svenskt språkstöd** - Anpassade tecken för å, ä, ö
- **Scrollande text** - Automatisk scrollning för meddelanden längre än 16 tecken
- **Ljudalarm** - Buzzer som spelar alarm för nya meddelanden
- **WiFi-återanslutning** - Automatisk återanslutning vid WiFi-avbrott
- **Sessionhantering** - Automatisk förnyelse av inloggningssession (60 min)

## Hårdvarukrav

### Komponenter som behövs:
- **1x WEMOS D1 Mini Pro** (ESP8266-baserad mikrokontroller)
- **1x 16x2 LCD med I2C-gränssnitt** (adress 0x27)
- **1x Aktiv buzzer** (3.3V/5V kompatibel)
- **1x Micro USB-kabel** (för strömförsörjning och programmering)
- **3D-printad låda** (valfritt, för snyggt hölje)

### Kopplingsschema:
```
WEMOS D1 Mini Pro -> LCD I2C
D1 (SCL)           -> SCL
D2 (SDA)           -> SDA
3.3V/5V            -> VCC
GND                -> GND

WEMOS D1 Mini Pro -> Buzzer
D5                 -> Buzzer +
GND                -> Buzzer -
```

## Installation och konfiguration

### 1. Arduino IDE-inställningar
Se till att du har följande bibliotek installerade:
- ESP8266WiFi
- ESP8266HTTPClient  
- WiFiClient
- Wire
- LiquidCrystal_I2C

### 2. Konfiguration av koden
Innan du laddar upp koden måste du ändra följande inställningar i början av filen:

#### WiFi-inställningar:
```cpp
const char* ssid = "DITT_WIFI_NAMN";
const char* password = "DITT_WIFI_LÖSENORD";
```

#### Server-inställningar:
```cpp
const char* serverAddress = "http://DIN_SERVER_IP:PORT";
```
Exempel: `"http://192.168.1.100:5000"`

#### Inloggningsuppgifter:
```cpp
const char* loginUsername = "DITT_ANVÄNDARNAMN";
const char* loginPassword = "DITT_LÖSENORD";
```

#### LCD I2C-adress (om nödvändigt):
```cpp
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Ändra 0x27 om din LCD har annan adress
```

### 3. Upptäck LCD I2C-adress
Om LCD:n inte fungerar, kör denna kod för att hitta rätt I2C-adress:
```cpp
#include <Wire.h>
void setup() {
  Wire.begin(D2, D1);
  Serial.begin(115200);
  Serial.println("Scanning for I2C devices...");
  for(byte i = 8; i < 120; i++) {
    Wire.beginTransmission(i);
    if(Wire.endTransmission() == 0) {
      Serial.println("Found I2C device at: 0x" + String(i, HEX));
    }
  }
}
void loop() {}
```

## Användning

1. **Strömförsörjning** - Anslut enheten via USB-kabel
2. **Uppstart** - LCD:n visar "PagerWiFi..." och "Starting..."
3. **WiFi-anslutning** - Enheten ansluter automatiskt till WiFi
4. **Inloggning** - Automatisk inloggning på Flask-servern
5. **Övervakningsläge** - Enheten kollar efter nya meddelanden var 30:e sekund
6. **Nya meddelanden** - LCD visar meddelandet och buzzern spelar alarm

### Status-meddelanden på LCD:
- `"Starting..."` - Enheten startar upp
- `"Connecting WiFi"` - Ansluter till WiFi
- `"WiFi Connected!"` - WiFi-anslutning lyckades
- `"Ready"` - Redo att övervaka meddelanden
- `"Login failed"` - Inloggning misslyckades
- `"No Alpha messages"` - Inga meddelanden tillgängliga

## Tekniska specifikationer

- **Uppdateringsfrekvens**: 30 sekunder
- **Session-timeout**: 60 minuter (automatisk förnyelse)
- **Återförsöksintervall**: 5 minuter vid misslyckad inloggning
- **Maximal meddelandelängd**: Obegränsad (med scrollning)
- **Strömförbrukning**: ~80mA (WEMOS + LCD + Buzzer)
- **Temperaturområde**: 0°C till +70°C

## Felsökning

### LCD visar inget:
- Kontrollera I2C-kopplingar (SDA/SCL)
- Verifiera LCD-adress med I2C-scanner
- Kontrollera strömförsörjning till LCD

### WiFi ansluter inte:
- Kontrollera SSID och lösenord
- Säkerställ att WiFi använder 2.4GHz (inte 5GHz)
- Kontrollera signalstyrka

### Inloggning misslyckas:
- Verifiera användarnamn och lösenord
- Kontrollera server-IP och port
- Säkerställ att Flask-servern körs och är tillgänglig

### Inga meddelanden visas:
- Kontrollera att `/messages` endpoint returnerar JSON
- Verifiera att `"filtered"` array innehåller data
- Kontrollera serial monitor för debug-meddelanden

## 3D-printning

En 3D-printad låda!

https://makerworld.com/sv

Utvecklare: Daniel Andersson

## Kontakt 

Utvecklare: SA7BNB Anders Isaksson

E-post: sa7bnb(@)gmail.com

GitHub: https://github.com/sa7bnb/PagerWiFi

![Enhetsbild](https://github.com/sa7bnb/PagerWiFI/blob/main/enhets-bild.jpg)
