const String unitID = "3"; //"* * * set this before compile * * *"; // 'n' for giga, 'Nano-Iot-n" for Nano 33 Io

#define LOCAL_BUILD

#ifdef LOCAL_BUILD
#define CLOUD_TIME time_t
class CloudWrapper {
  public:
    void setup() {}
    void loop() {}
    bool isConnected() { return false; }
    CLOUD_TIME getLocalTime() { return 0; }
};
#else
#define CLOUD_TIME CloudTime
#include "thingProperties.h"
class CloudWrapper {
  public:
    void setup() {
      initProperties();
      ArduinoCloud.begin(ArduinoIoTPreferredConnection);
      setDebugMessageLevel(2);
      ArduinoCloud.printDebugInfo();
    }
    void loop() {
      ArduinoCloud.update();
    }
    bool isConnected() {
      return ArduinoCloud.connected();
    }
    CLOUD_TIME getLocalTime() {
      return ArduinoCloud.getLocalTime();
    }
};
#endif
CloudWrapper cloudWrapper;

class Utils {
  private:
    static unsigned long lastPrintln;
  public:
    // Slows everything down to make Serial output readable.
    static void publish(String s) {
      if (millis() < lastPrintln + 1000) {
        delay(1000);
      }
      Serial.println(s);
      lastPrintln = millis();
    }
    static void waitForSerial(String s) {
        delay(1000);
        String msg(s);
        msg.concat(", Press any character + Enter to continue: ");
        Utils::publish(msg);
        while (Serial.read() == -1) {
          delay(100);
        }
    }
    static String msToString(unsigned long ms) {
      int totalSeconds = ms / 1000;
      int secs = totalSeconds % 60;
      int minutes = (totalSeconds / 60) % 60;
      int hours = (totalSeconds / 60) / 60;
    
      char buf[100];
      sprintf(buf, "%02u:%02u:%02u", hours, minutes, secs);
      return String(buf);
    }
};
unsigned long Utils::lastPrintln = 0;

const int COLOR_WHITE = 0x65535;
const int COLOR_BLACK = 0x0;
const int COLOR_RED = 0xF800;
#include "Arduino_GigaDisplay_GFX.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSans24pt7b.h"

GigaDisplay_GFX display_;

class OLEDWrapper {
  private:
    uint16_t currentColor = COLOR_WHITE;
    const int DEFAULT_FONT_SIZE = 1;
  public:
    void clear() {
      display_.fillScreen(COLOR_BLACK);
    }
    void startup() {
      delay(1000);
      display_.begin(); //init library
      clear();
      display_.setRotation(1);
    }
    void display(String s, const GFXfont* font, int textSize, uint16_t x, uint16_t y) {
      display_.setCursor(x, y);
      display_.setFont(font);
      display_.setTextSize(textSize);
      display_.print(s);
    }
    void display(String s, int textSize, uint16_t x, uint16_t y) {
      display(s, &FreeSans24pt7b, textSize, x, y);
    }
    void display(String s) {
      display(s, DEFAULT_FONT_SIZE, 10, getHeight() / 2);
    }
    void display(String s[], int nStrings) {
      for (int i = 0; i < nStrings; i++) {
        display(s[i], DEFAULT_FONT_SIZE, 10, 48 + (i * 48));
      }
    }
    void setDrawColor(int color) {
      currentColor = color;
    }
    void setFont(const GFXfont* font) {
      display_.setFont(font);
    }
    void getTextBox(const GFXfont* font, String string, int textSize, int16_t x0, int16_t y0,
                    int16_t* x, int16_t* y, uint16_t* w, uint16_t* h) {
      display_.setFont(font);
      display_.setTextSize(textSize);
      display_.getTextBounds(string, x0, y0, x, y, w, h);
      // https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/issues/43
      *w += (textSize * 2);
      *h -= (textSize * 2);
    }
    void drawLine(int x0, int y0, int x1, int y1) {
      // Rotate not happening automatically?
      display_.drawLine(y0, x0, y1, x1, currentColor);
    }
    void fillRect(int x0, int y0, int x1, int y1, int color) {
      display_.fillRect(x0, y0, x1 - x0, y1 - y0, color); // is there an off-by-one error here?
    }  
    void fillRectWH(int x0, int y0, int w, int h, int color) {
      display_.fillRect(x0, y0, w, h, color);
    }  
    int getHeight() {
      return display_.height();
    }
    int getWidth() {
      return display_.width();
    }
    void dump() {
      String s("OLEDWrapper: getHeight(): ");
      s.concat(getHeight());
      s.concat(", getWidth(): ");
      s.concat(getWidth());
      Utils::publish(s);
    }
};
OLEDWrapper* oledWrapper = nullptr;

#include <time.h>
class TimeSupport {
  private:
    const unsigned long  NO_TIME = 0;
  public:
                TimeSupport();
    CLOUD_TIME  getCurrentTime();
    String      timeStr(CLOUD_TIME t);
    String      now();
    String      dump();
    void        getDump(String s[3]);
};

TimeSupport::TimeSupport() {}
CLOUD_TIME TimeSupport::getCurrentTime() {
  return cloudWrapper.getLocalTime();
}
String TimeSupport::timeStr(CLOUD_TIME t) {
  if ((unsigned long)t == NO_TIME) {
    return "unknown time, no time from cloud";
  }
  time_t rawTime = (time_t)t;
  char buffer[128];
  char* timeStr = ctime_r(&rawTime, buffer);
  if (timeStr == NULL) {
    return "unknown time, ctime_r failed";
  }
  String s(buffer);
  s.trim();
  return s;
}

String TimeSupport::now() {
  return timeStr(getCurrentTime());
}

String TimeSupport::dump() {
  String s("getCurrentTime(): ");
  s.concat((unsigned long)getCurrentTime());
  s.concat(", now(): ");
  s.concat(now());
  s.concat(", NO_TIME: ");
  s.concat(NO_TIME);
  return s;
}
void TimeSupport::getDump(String s[3]) {
    s[0] = "getCurrentTime(): ";
    s[0].concat((unsigned long)getCurrentTime());
    s[1] = "now(): ";
    s[1].concat(now());
    s[2] = "NO_TIME: ";
    s[2].concat(NO_TIME);
}
TimeSupport*    timeSupport = nullptr;

void setup() {
  Serial.begin(115200);
  cloudWrapper.setup();
  oledWrapper = new OLEDWrapper();
  oledWrapper->startup();
  timeSupport = new TimeSupport();
}

void loop() {
  cloudWrapper.loop();
  oledWrapper->clear();
  String s[3];
  timeSupport->getDump(s);
  oledWrapper->display(s, 3);
  delay(3000);
}
