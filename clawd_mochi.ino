/*
 * ╔══════════════════════════════════════════════════════════════╗
 *   CLAWD MOCHI — ESP32-C3 Super Mini + ST7789 1.69" 280×240
 *
 *   Wiring:
 *     SDA → GPIO 10  (hardware SPI MOSI)
 *     SCL → GPIO 8   (hardware SPI SCK)
 *     RST → GPIO 2
 *     DC  → GPIO 1
 *     CS  → GPIO 4
 *     BL  → GPIO 3
 *     VCC → 3V3
 *     GND → GND
 *
 *   WiFi: "ClaWD-Mochi"  pw: clawd1234  → http://192.168.4.1
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>

// ── Pins ──────────────────────────────────────────────────────
#define TFT_CS  4
#define TFT_DC  1
#define TFT_RST 2
#define TFT_BLK 3

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ── WiFi ──────────────────────────────────────────────────────
const char* AP_SSID = "ClaWD-Mochi";
const char* AP_PASS = "clawd1234";
WebServer server(80);

// ── Display ───────────────────────────────────────────────────
#define DISP_W 280
#define DISP_H 240

// ── Eye constants (shared by both eye views) ──────────────────
#define EYE_W   30
#define EYE_H   60
#define EYE_GAP 120
#define EYE_OX  0     // horizontal offset
#define EYE_OY  40    // vertical offset upward (subtracted from centre)

// ── Colours ───────────────────────────────────────────────────
uint16_t C_ORANGE, C_DARKBG, C_MUTED, C_GREEN;
#define C_WHITE ST77XX_WHITE
#define C_BLACK ST77XX_BLACK

// ── State ─────────────────────────────────────────────────────
#define VIEW_EYES_NORMAL 0
#define VIEW_EYES_SQUISH 1
#define VIEW_CODE        2
#define VIEW_DRAW        3

uint8_t  currentView  = VIEW_EYES_NORMAL;
bool     busy         = false;
bool     backlightOn  = true;
uint8_t  animSpeed    = 1;   // 1=slow(default) 2=normal 3=fast

uint16_t animBgColor  = 0;   // background for eye/logo animations (set in setup)
uint16_t drawBgColor  = 0;   // background for canvas

String   statusText   = "";  // text shown below eyes
uint8_t  statusSize   = 2;   // font size for status text
uint16_t statusColor  = 0;   // colour for status text (0 = use C_BLACK)

// ── Terminal ──────────────────────────────────────────────────
#define TERM_COLS      18
#define TERM_ROWS       8
#define TERM_CHAR_W    12
#define TERM_CHAR_H    20
#define TERM_PAD_X      8
#define TERM_PAD_Y     18

bool    termMode    = false;
String  termLines[TERM_ROWS];
uint8_t termRow     = 0;
uint8_t termCol     = 0;

// ── Logo data ─────────────────────────────────────────────────
#define LOGO_CX 140
#define LOGO_CY 105

#define LOGO_TRI_COUNT 162
static const int16_t LOGO_TRIS[][6] PROGMEM = {
  {140,105,85,134,120,114},{140,105,120,114,121,113},{140,105,121,113,120,112},
  {140,105,120,112,119,112},{140,105,119,112,113,111},{140,105,113,111,93,111},
  {140,105,93,111,75,110},{140,105,75,110,58,109},{140,105,58,109,54,108},
  {140,105,54,108,50,103},{140,105,50,103,50,100},{140,105,50,100,54,98},
  {140,105,54,98,59,98},{140,105,59,98,70,99},{140,105,70,99,87,100},
  {140,105,87,100,100,101},{140,105,100,101,118,103},{140,105,118,103,121,103},
  {140,105,121,103,121,102},{140,105,121,102,120,101},{140,105,120,101,120,100},
  {140,105,120,100,102,88},{140,105,102,88,83,76},{140,105,83,76,73,69},
  {140,105,73,69,68,65},{140,105,68,65,65,61},{140,105,65,61,64,54},
  {140,105,64,54,69,49},{140,105,69,49,75,49},{140,105,75,49,77,49},
  {140,105,77,49,84,55},{140,105,84,55,98,66},{140,105,98,66,116,79},
  {140,105,116,79,119,81},{140,105,119,81,120,81},{140,105,120,81,120,80},
  {140,105,120,80,119,78},{140,105,119,78,109,60},{140,105,109,60,98,41},
  {140,105,98,41,93,34},{140,105,93,34,92,29},{140,105,92,29,92,28},
  {140,105,92,28,92,27},{140,105,92,27,91,26},{140,105,91,26,91,25},
  {140,105,91,25,91,24},{140,105,91,24,97,16},{140,105,97,16,100,15},
  {140,105,100,15,107,16},{140,105,107,16,111,19},{140,105,111,19,115,29},
  {140,105,115,29,123,46},{140,105,123,46,134,68},{140,105,134,68,138,75},
  {140,105,138,75,139,81},{140,105,139,81,140,83},{140,105,140,83,141,83},
  {140,105,141,83,141,82},{140,105,141,82,142,69},{140,105,142,69,144,54},
  {140,105,144,54,146,34},{140,105,146,34,146,28},{140,105,146,28,149,21},
  {140,105,149,21,155,18},{140,105,155,18,159,20},{140,105,159,20,163,25},
  {140,105,163,25,162,28},{140,105,162,28,160,42},{140,105,160,42,156,64},
  {140,105,156,64,153,78},{140,105,153,78,155,78},{140,105,155,78,156,76},
  {140,105,156,76,164,67},{140,105,164,67,176,51},{140,105,176,51,182,45},
  {140,105,182,45,188,38},{140,105,188,38,192,35},{140,105,192,35,200,35},
  {140,105,200,35,205,43},{140,105,205,43,203,52},{140,105,203,52,195,62},
  {140,105,195,62,188,71},{140,105,188,71,179,83},{140,105,179,83,173,94},
  {140,105,173,94,174,94},{140,105,174,94,175,94},{140,105,175,94,196,90},
  {140,105,196,90,208,88},{140,105,208,88,221,85},{140,105,221,85,228,88},
  {140,105,228,88,228,91},{140,105,228,91,226,97},{140,105,226,97,211,101},
  {140,105,211,101,194,104},{140,105,194,104,168,110},{140,105,168,110,168,111},
  {140,105,168,111,168,111},{140,105,168,111,180,112},{140,105,180,112,185,112},
  {140,105,185,112,197,112},{140,105,197,112,220,114},{140,105,220,114,225,118},
  {140,105,225,118,229,123},{140,105,229,123,228,126},{140,105,228,126,219,131},
  {140,105,219,131,207,128},{140,105,207,128,179,121},{140,105,179,121,169,119},
  {140,105,169,119,167,119},{140,105,167,119,167,120},{140,105,167,120,176,128},
  {140,105,176,128,190,141},{140,105,190,141,209,158},{140,105,209,158,210,163},
  {140,105,210,163,208,166},{140,105,208,166,205,166},{140,105,205,166,189,153},
  {140,105,189,153,182,148},{140,105,182,148,168,136},{140,105,168,136,167,136},
  {140,105,167,136,167,137},{140,105,167,137,170,142},{140,105,170,142,188,168},
  {140,105,188,168,189,176},{140,105,189,176,188,179},{140,105,188,179,183,180},
  {140,105,183,180,178,179},{140,105,178,179,168,165},{140,105,168,165,157,149},
  {140,105,157,149,149,134},{140,105,149,134,148,135},{140,105,148,135,143,189},
  {140,105,143,189,140,192},{140,105,140,192,135,194},{140,105,135,194,130,191},
  {140,105,130,191,128,185},{140,105,128,185,130,174},{140,105,130,174,133,160},
  {140,105,133,160,136,148},{140,105,136,148,138,134},{140,105,138,134,139,129},
  {140,105,139,129,139,129},{140,105,139,129,138,129},{140,105,138,129,127,144},
  {140,105,127,144,111,166},{140,105,111,166,98,180},{140,105,98,180,95,181},
  {140,105,95,181,90,178},{140,105,90,178,90,173},{140,105,90,173,93,169},
  {140,105,93,169,111,146},{140,105,111,146,122,132},{140,105,122,132,129,124},
  {140,105,129,124,129,123},{140,105,129,123,128,123},{140,105,128,123,81,153},
  {140,105,81,153,72,155},{140,105,72,155,69,151},{140,105,69,151,69,146},
  {140,105,69,146,71,144},{140,105,71,144,85,134},{140,105,85,134,85,134},
};

#define LOGO_SEG_COUNT 162
static const int16_t LOGO_SEGS[][4] PROGMEM = {
  {85,134,120,114},{120,114,121,113},{121,113,120,112},{120,112,119,112},
  {119,112,113,111},{113,111,93,111},{93,111,75,110},{75,110,58,109},
  {58,109,54,108},{54,108,50,103},{50,103,50,100},{50,100,54,98},
  {54,98,59,98},{59,98,70,99},{70,99,87,100},{87,100,100,101},
  {100,101,118,103},{118,103,121,103},{121,103,121,102},{121,102,120,101},
  {120,101,120,100},{120,100,102,88},{102,88,83,76},{83,76,73,69},
  {73,69,68,65},{68,65,65,61},{65,61,64,54},{64,54,69,49},
  {69,49,75,49},{75,49,77,49},{77,49,84,55},{84,55,98,66},
  {98,66,116,79},{116,79,119,81},{119,81,120,81},{120,81,120,80},
  {120,80,119,78},{119,78,109,60},{109,60,98,41},{98,41,93,34},
  {93,34,92,29},{92,29,92,28},{92,28,92,27},{92,27,91,26},
  {91,26,91,25},{91,25,91,24},{91,24,97,16},{97,16,100,15},
  {100,15,107,16},{107,16,111,19},{111,19,115,29},{115,29,123,46},
  {123,46,134,68},{134,68,138,75},{138,75,139,81},{139,81,140,83},
  {140,83,141,83},{141,83,141,82},{141,82,142,69},{142,69,144,54},
  {144,54,146,34},{146,34,146,28},{146,28,149,21},{149,21,155,18},
  {155,18,159,20},{159,20,163,25},{163,25,162,28},{162,28,160,42},
  {160,42,156,64},{156,64,153,78},{153,78,155,78},{155,78,156,76},
  {156,76,164,67},{164,67,176,51},{176,51,182,45},{182,45,188,38},
  {188,38,192,35},{192,35,200,35},{200,35,205,43},{205,43,203,52},
  {203,52,195,62},{195,62,188,71},{188,71,179,83},{179,83,173,94},
  {173,94,174,94},{174,94,175,94},{175,94,196,90},{196,90,208,88},
  {208,88,221,85},{221,85,228,88},{228,88,228,91},{228,91,226,97},
  {226,97,211,101},{211,101,194,104},{194,104,168,110},{168,110,168,111},
  {168,111,168,111},{168,111,180,112},{180,112,185,112},{185,112,197,112},
  {197,112,220,114},{220,114,225,118},{225,118,229,123},{229,123,228,126},
  {228,126,219,131},{219,131,207,128},{207,128,179,121},{179,121,169,119},
  {169,119,167,119},{167,119,167,120},{167,120,176,128},{176,128,190,141},
  {190,141,209,158},{209,158,210,163},{210,163,208,166},{208,166,205,166},
  {205,166,189,153},{189,153,182,148},{182,148,168,136},{168,136,167,136},
  {167,136,167,137},{167,137,170,142},{170,142,188,168},{188,168,189,176},
  {189,176,188,179},{188,179,183,180},{183,180,178,179},{178,179,168,165},
  {168,165,157,149},{157,149,149,134},{149,134,148,135},{148,135,143,189},
  {143,189,140,192},{140,192,135,194},{135,194,130,191},{130,191,128,185},
  {128,185,130,174},{130,174,133,160},{133,160,136,148},{136,148,138,134},
  {138,134,139,129},{139,129,139,129},{139,129,138,129},{138,129,127,144},
  {127,144,111,166},{111,166,98,180},{98,180,95,181},{95,181,90,178},
  {90,178,90,173},{90,173,93,169},{93,169,111,146},{111,146,122,132},
  {122,132,129,124},{129,124,129,123},{129,123,128,123},{128,123,81,153},
  {81,153,72,155},{72,155,69,151},{69,151,69,146},{69,146,71,144},
  {71,144,85,134},{85,134,85,134},
};

// ═════════════════════════════════════════════════════════════
//  HELPERS
// ═════════════════════════════════════════════════════════════

int speedMs(int ms) {
  if (animSpeed == 3) return ms / 2;
  if (animSpeed == 1) return ms * 2;
  return ms;
}

uint16_t hexToRgb565(String hex) {
  hex.replace("#", "");
  if (hex.length() != 6) return C_WHITE;
  long v = strtol(hex.c_str(), nullptr, 16);
  return tft.color565((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}

void setBacklight(bool on) {
  backlightOn = on;
  digitalWrite(TFT_BLK, on ? HIGH : LOW);
}

void initColours() {
  // C_ORANGE = tft.color565(170, 72, 28);
  C_ORANGE = tft.color565(219, 99, 0);
  C_DARKBG = tft.color565(10,  12,  16);
  C_MUTED  = tft.color565(90,  88,  86);
  C_GREEN  = tft.color565(80, 220, 130);
  animBgColor = C_ORANGE;
  drawBgColor = C_ORANGE;
}

// ═════════════════════════════════════════════════════════════
//  LOGO
// ═════════════════════════════════════════════════════════════

void drawLogoFilled(uint16_t bg, uint16_t fg) {
  tft.fillScreen(bg);
  for (uint16_t i = 0; i < LOGO_TRI_COUNT; i++) {
    tft.fillTriangle(
      pgm_read_word(&LOGO_TRIS[i][0]), pgm_read_word(&LOGO_TRIS[i][1]),
      pgm_read_word(&LOGO_TRIS[i][2]), pgm_read_word(&LOGO_TRIS[i][3]),
      pgm_read_word(&LOGO_TRIS[i][4]), pgm_read_word(&LOGO_TRIS[i][5]),
      fg);
  }
  tft.setTextColor(fg); tft.setTextSize(2);
  tft.setCursor(LOGO_CX - 54, 210); tft.print("Anthropic");
  tft.setCursor(LOGO_CX - 53, 210); tft.print("Anthropic");
}

// ═════════════════════════════════════════════════════════════
//  VIEWS
// ═════════════════════════════════════════════════════════════

// Eye helpers — shared constants via #define EYE_*
inline int16_t eyeLX(int16_t ox) {
  return (DISP_W - (EYE_W * 2 + EYE_GAP)) / 2 + EYE_OX + ox;
}
inline int16_t eyeRX(int16_t ox) { return eyeLX(ox) + EYE_W + EYE_GAP; }
inline int16_t eyeY()            { return (DISP_H - EYE_H) / 2 - EYE_OY; }
inline int16_t eyeCY()           { return eyeY() + EYE_H / 2; }

void drawStatusText() {
  if (statusText.length() == 0) return;
  tft.setTextSize(statusSize);
  tft.setTextColor(statusColor ? statusColor : C_BLACK);
  int16_t charW = 6 * statusSize;
  int16_t lineH = 8 * statusSize + 2;
  int16_t maxChars = DISP_W / charW;
  if (maxChars < 1) maxChars = 1;

  int16_t len = statusText.length();
  int16_t lines = (len + maxChars - 1) / maxChars;
  int16_t totalH = lines * lineH;
  int16_t startY = DISP_H - totalH - 8;

  for (int16_t i = 0; i < lines; i++) {
    String line = statusText.substring(i * maxChars, min((int)((i + 1) * maxChars), (int)len));
    int16_t tw = line.length() * charW;
    int16_t x = (DISP_W - tw) / 2;
    if (x < 0) x = 0;
    tft.setCursor(x, startY + i * lineH);
    tft.print(line);
  }
}

void drawNormalEyes(int16_t ox = 0, bool blink = false) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  if (!blink) {
    tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
    tft.fillRect(rx, ey, EYE_W, EYE_H, C_BLACK);
  } else {
    tft.fillRect(lx, ey + EYE_H / 2 - 3, EYE_W, 6, C_BLACK);
    tft.fillRect(rx, ey + EYE_H / 2 - 3, EYE_W, 6, C_BLACK);
  }
  drawStatusText();
}

void drawChevron(int16_t cx, int16_t cy, int16_t arm, int16_t reach,
                 uint8_t thk, bool rightFacing, uint16_t col) {
  for (int8_t t = -(int8_t)thk; t <= (int8_t)thk; t++) {
    if (rightFacing) {
      tft.drawLine(cx - reach/2, cy - arm + t, cx + reach/2, cy + t,      col);
      tft.drawLine(cx + reach/2, cy + t,       cx - reach/2, cy + arm + t, col);
    } else {
      tft.drawLine(cx + reach/2, cy - arm + t, cx - reach/2, cy + t,      col);
      tft.drawLine(cx - reach/2, cy + t,       cx + reach/2, cy + arm + t, col);
    }
  }
}

void drawSquishEyes(bool closed = false) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), cy = eyeCY();
  const int16_t arm   = EYE_H / 2;
  const int16_t reach = EYE_W / 2;
  const int16_t lcx   = lx + EYE_W / 2;
  const int16_t rcx   = rx + EYE_W / 2;
  if (!closed) {
    drawChevron(lcx, cy, arm, reach, 10, true,  C_BLACK);
    drawChevron(rcx, cy, arm, reach, 10, false, C_BLACK);
  } else {
    tft.fillRect(lx, cy - 5, EYE_W, 10, C_BLACK);
    tft.fillRect(rx, cy - 5, EYE_W, 10, C_BLACK);
  }
  drawStatusText();
}

void drawCodeView() {
  termMode = false;
  tft.fillScreen(C_DARKBG);
  tft.fillRect(0, 0,          DISP_W, 4, C_ORANGE);
  tft.fillRect(0, DISP_H - 4, DISP_W, 4, C_ORANGE);
  tft.setTextColor(C_ORANGE); tft.setTextSize(4);
  tft.setCursor((DISP_W - 144) / 2, DISP_H / 2 - 52); tft.print("Claude");
  tft.setTextColor(C_WHITE);  tft.setTextSize(4);
  tft.setCursor((DISP_W - 96) / 2,  DISP_H / 2 + 8);  tft.print("Code");
  tft.fillRect((DISP_W - 96) / 2, DISP_H / 2 + 52, 96, 3, C_ORANGE);
}

// ═════════════════════════════════════════════════════════════
//  TERMINAL
// ═════════════════════════════════════════════════════════════

void termClear() {
  for (uint8_t i = 0; i < TERM_ROWS; i++) termLines[i] = "";
  termRow = 0; termCol = 0;
}

void termDrawHeader() {
  tft.fillRect(0, 0, DISP_W, TERM_PAD_Y + 1, C_DARKBG);
  tft.setTextColor(C_ORANGE); tft.setTextSize(1);
  tft.setCursor(TERM_PAD_X, 4); tft.print("clawd@mochi terminal");
  tft.drawFastHLine(0, TERM_PAD_Y, DISP_W, C_ORANGE);
}

// Prefix "clawd:~$ " in green, drawn only when the row has content
void termDrawPrefix(int16_t yy) {
  tft.setTextColor(C_GREEN); tft.setTextSize(1);
  tft.setCursor(TERM_PAD_X, yy + 6);
  tft.print("clawd:~$ ");
}

#define PREFIX_PX 54   // 9 chars × 6px = 54px at textSize 1

void termDrawLine(uint8_t r) {
  const int16_t yy = TERM_PAD_Y + 4 + r * TERM_CHAR_H;
  tft.fillRect(0, yy, DISP_W, TERM_CHAR_H, C_DARKBG);
  // show prefix only on the currently active (cursor) line
  if (r == termRow) termDrawPrefix(yy);
  tft.setTextColor(C_WHITE); tft.setTextSize(2);
  tft.setCursor(TERM_PAD_X + PREFIX_PX, yy + 1);
  tft.print(termLines[r]);
  if (r == termRow) {
    const int16_t cx = TERM_PAD_X + PREFIX_PX + termCol * TERM_CHAR_W;
    tft.fillRect(cx, yy + 1, TERM_CHAR_W - 2, TERM_CHAR_H - 2, C_GREEN);
  }
}

void termDrawLastChar() {
  if (termCol == 0) return;
  const int16_t yy    = TERM_PAD_Y + 4 + termRow * TERM_CHAR_H;
  const int16_t baseX = TERM_PAD_X + PREFIX_PX;
  const uint8_t prev  = termCol - 1;
  // erase prev cell (had cursor block)
  tft.fillRect(baseX + prev * TERM_CHAR_W, yy + 1, TERM_CHAR_W, TERM_CHAR_H - 1, C_DARKBG);
  tft.setTextColor(C_WHITE); tft.setTextSize(2);
  tft.setCursor(baseX + prev * TERM_CHAR_W, yy + 1);
  tft.print(termLines[termRow][prev]);
  // new cursor
  tft.fillRect(baseX + termCol * TERM_CHAR_W, yy + 1, TERM_CHAR_W - 2, TERM_CHAR_H - 2, C_GREEN);
}

void termDrawBackspace() {
  const int16_t yy    = TERM_PAD_Y + 4 + termRow * TERM_CHAR_H;
  const int16_t baseX = TERM_PAD_X + PREFIX_PX;
  // erase deleted char + old cursor
  tft.fillRect(baseX + termCol * TERM_CHAR_W, yy + 1, TERM_CHAR_W * 2, TERM_CHAR_H - 1, C_DARKBG);
  // new cursor
  tft.fillRect(baseX + termCol * TERM_CHAR_W, yy + 1, TERM_CHAR_W - 2, TERM_CHAR_H - 2, C_GREEN);
  // if line now empty, erase the prefix too
  if (termLines[termRow].length() == 0) {
    tft.fillRect(0, yy, TERM_PAD_X + PREFIX_PX, TERM_CHAR_H, C_DARKBG);
  }
}

void termFullRedraw() {
  tft.fillScreen(C_DARKBG);
  termDrawHeader();
  for (uint8_t r = 0; r < TERM_ROWS; r++) termDrawLine(r);
}

void termScroll() {
  for (uint8_t i = 0; i < TERM_ROWS - 1; i++) termLines[i] = termLines[i + 1];
  termLines[TERM_ROWS - 1] = "";
  termRow = TERM_ROWS - 1;
  termFullRedraw();
}

void termAddChar(char c) {
  if (c == '\n' || c == '\r') {
    const int16_t yy = TERM_PAD_Y + 4 + termRow * TERM_CHAR_H;
    // erase cursor on current row
    tft.fillRect(TERM_PAD_X + PREFIX_PX + termCol * TERM_CHAR_W,
                 yy + 1, TERM_CHAR_W, TERM_CHAR_H - 1, C_DARKBG);
    termRow++; termCol = 0;
    if (termRow >= TERM_ROWS) { termScroll(); return; }
    termDrawLine(termRow);  // draws prefix on new line
  } else if (c == '\b' || c == 127) {
    if (termCol > 0) {
      termCol--;
      termLines[termRow].remove(termLines[termRow].length() - 1);
      termDrawBackspace();
    }
  } else if (c >= 32 && c < 127) {
    if (termCol >= TERM_COLS) {
      termRow++; termCol = 0;
      if (termRow >= TERM_ROWS) { termScroll(); return; }
    }
    // draw prefix on first char of this line
    if (termCol == 0) termDrawPrefix(TERM_PAD_Y + 4 + termRow * TERM_CHAR_H);
    termLines[termRow] += c;
    termCol++;
    termDrawLastChar();
  }
}

// ═════════════════════════════════════════════════════════════
//  ANIMATIONS
// ═════════════════════════════════════════════════════════════

void animNormalEyes() {
  busy = true;
  const int16_t offs[] = {-16, 16, -16, 16, 0};
  for (uint8_t i = 0; i < 5; i++) { drawNormalEyes(offs[i]); delay(speedMs(80)); }
  drawNormalEyes(0, true);  delay(speedMs(100));
  drawNormalEyes(0, false); delay(speedMs(70));
  drawNormalEyes(0, true);  delay(speedMs(70));
  drawNormalEyes(0, false);
  busy = false;
}

void animSquishEyes() {
  busy = true;
  for (uint8_t i = 0; i < 3; i++) {
    drawSquishEyes(false); delay(speedMs(160));
    drawSquishEyes(true);  delay(speedMs(100));
  }
  drawSquishEyes(false);
  busy = false;
}

void animLogoReveal() {
  busy = true;
  tft.fillScreen(animBgColor);
  for (uint16_t i = 0; i < LOGO_SEG_COUNT; i++) {
    int16_t x1 = pgm_read_word(&LOGO_SEGS[i][0]);
    int16_t y1 = pgm_read_word(&LOGO_SEGS[i][1]);
    int16_t x2 = pgm_read_word(&LOGO_SEGS[i][2]);
    int16_t y2 = pgm_read_word(&LOGO_SEGS[i][3]);
    tft.drawLine(x1, y1, x2, y2, C_WHITE);
    tft.drawLine(x1 + 1, y1, x2 + 1, y2, C_WHITE);
    if (i % 4 == 0) { server.handleClient(); delay(speedMs(8)); }
  }
  drawLogoFilled(animBgColor, C_WHITE);
  delay(1500);
  busy = false;
}

// ═════════════════════════════════════════════════════════════
//  WEB PAGE
// ═════════════════════════════════════════════════════════════
const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Clawd Mochi</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
body{background:#1c1c20;font-family:'Courier New',monospace;color:#e8e4dc;
  display:flex;flex-direction:column;align-items:center;
  padding:20px 14px 52px;gap:14px;min-height:100vh}

.hdr{text-align:center;padding:2px 0 4px}
.mascot{font-size:15px;color:#c96a3e;line-height:1.3;font-weight:bold;
  font-family:'Courier New',monospace;display:block;letter-spacing:1px}
.sitename{font-size:10px;color:#5a5048;margin-top:8px;letter-spacing:3px}

.sec{width:100%;max-width:390px;font-size:10px;color:#8a8278;
  letter-spacing:2px;font-weight:bold;padding:0 2px}

/* Busy bar */
.busy{width:100%;max-width:390px;height:2px;background:#2e2a28;
  border-radius:1px;overflow:hidden;opacity:0;transition:opacity .2s}
.busy.show{opacity:1}
.busy-i{height:100%;width:30%;background:#c96a3e;border-radius:1px;
  animation:sl 1s linear infinite}
@keyframes sl{0%{margin-left:-30%}100%{margin-left:100%}}

/* Controls */
.ctrl{display:flex;gap:8px;width:100%;max-width:390px}
.cbtn{flex:1;background:#252428;border:1.5px solid #38343a;border-radius:10px;
  color:#b8b4ac;font-family:'Courier New',monospace;font-size:11px;font-weight:bold;
  padding:12px 4px;cursor:pointer;text-align:center;transition:all .12s}
.cbtn:active:not(:disabled){transform:scale(.94)}
.cbtn:disabled{opacity:.3;cursor:default}
.cbtn.on{border-color:#c96a3e;color:#c96a3e;background:#201408}
.cbtn.dim{border-color:#2e2a28;color:#4a4540}

/* View grid */
.vgrid{display:grid;grid-template-columns:1fr 1fr;gap:8px;width:100%;max-width:390px}
.vbtn{background:#252428;border:1.5px solid #38343a;border-radius:12px;
  color:#d8d4cc;font-family:'Courier New',monospace;
  padding:14px 6px 10px;cursor:pointer;text-align:center;
  transition:all .12s;user-select:none}
.vbtn:active:not(:disabled){transform:scale(.94)}
.vbtn:disabled{opacity:.3;cursor:default}
.vbtn .ic{font-size:20px;display:block;margin-bottom:4px;line-height:1;color:#c96a3e}
.vbtn .nm{font-size:12px;font-weight:bold;color:#e8e4dc}
.vbtn .ht{font-size:9px;color:#8a8278;margin-top:3px}
.vbtn.active{border-color:#c96a3e;background:#201408}
.vbtn[data-v="1"].active{border-color:#c96a3e;background:#201408}
.vbtn[data-v="2"].active{border-color:#4a8acd;background:#0c1628}
.vbtn[data-v="3"].active{border-color:#38343a;background:#201c18}

/* Speed slider */
.speed-row{width:100%;max-width:390px;display:flex;align-items:center;gap:10px}
.sl{font-size:10px;color:#6a6058;white-space:nowrap;min-width:36px}
input[type=range]{flex:1;accent-color:#c96a3e;cursor:pointer;height:20px}
.sv{font-size:11px;color:#c96a3e;min-width:44px;text-align:right;font-weight:bold}

/* Terminal */
.twrap{width:100%;max-width:390px;display:none;flex-direction:column;gap:8px}
.twrap.open{display:flex}
.thdr{display:flex;justify-content:space-between;align-items:center}
.tttl{font-size:11px;color:#28b878;letter-spacing:1px;font-weight:bold}
.tx{background:#0c1e12;border:2px solid #1a4828;border-radius:9px;
  color:#28b878;font-family:'Courier New',monospace;font-size:13px;
  font-weight:bold;padding:10px 18px;cursor:pointer}
.tx:active{background:#081410}
.trow{display:flex;gap:6px}
.tin{flex:1;background:#0c1018;border:1.5px solid #1a2820;border-radius:9px;
  color:#40d880;font-family:'Courier New',monospace;font-size:15px;
  padding:11px;outline:none}
.tin::placeholder{color:#2a3828}
.tgo{background:#1a9060;border:none;border-radius:9px;color:#fff;
  font-family:'Courier New',monospace;font-size:22px;font-weight:bold;
  padding:11px 16px;cursor:pointer;min-width:52px}
.tgo:active{background:#0f6040}

/* Canvas */
.cwrap{width:100%;max-width:390px;background:#222028;border:1.5px solid #38343a;
  border-radius:12px;padding:12px;flex-direction:column;gap:10px;display:none}
.cwrap.open{display:flex}
.crow{display:flex;gap:8px}
.ci{display:flex;flex-direction:column;align-items:center;gap:4px;flex:1}
.cl{font-size:10px;color:#7a7068;letter-spacing:1px;font-weight:bold}
.cs{width:100%;height:38px;border-radius:7px;border:1.5px solid #38343a;cursor:pointer;padding:0}
.dacts{display:flex;gap:7px}
.db{flex:1;background:#1c1820;border:1.5px solid #38343a;border-radius:9px;
  color:#c0bab8;font-family:'Courier New',monospace;font-size:11px;
  font-weight:bold;padding:11px 4px;cursor:pointer;transition:all .12s}
.db:active{transform:scale(.95);background:#281838}
.db.hi{border-color:#c96a3e;color:#c96a3e}
canvas{width:100%;border-radius:8px;border:1.5px solid #38343a;
  touch-action:none;cursor:crosshair;display:block}

/* Toast */
.toast{position:fixed;bottom:18px;left:50%;transform:translateX(-50%);
  background:#252428;border:1.5px solid #38343a;border-radius:9px;
  font-size:12px;color:#d8d4cc;padding:7px 16px;opacity:0;
  transition:opacity .18s;pointer-events:none;white-space:nowrap;z-index:99}
.toast.show{opacity:1}
</style>
</head>
<body>

<div class="hdr">
  <span class="mascot">&#x2590;&#x259B;&#x2588;&#x2588;&#x2588;&#x259C;&#x258C;<br>&#x259C;&#x2588;&#x2588;&#x2588;&#x2588;&#x2588;&#x259B;<br>&#x2598;&#x2598;&nbsp;&#x259D;&#x259D;</span>
  <div class="sitename">CLAWD &middot; MOCHI &middot; CONTROLLER</div>
</div>

<div class="busy" id="busy"><div class="busy-i"></div></div>

<div class="sec">// controls</div>
<div class="ctrl">
  <button class="cbtn on" id="blBtn" onclick="toggleBL()">&#9728; display on</button>
</div>

<div class="sec">// views</div>
<div class="vgrid">
  <button class="vbtn active" data-v="0" onclick="setView(0)">
    <span class="ic">&#9632; &#9632;</span>
    <span class="nm">Normal eyes</span>
    <span class="ht">wiggle + blink</span>
  </button>
  <button class="vbtn" data-v="1" onclick="setView(1)">
    <span class="ic">&gt; &lt;</span>
    <span class="nm">Squish eyes</span>
    <span class="ht">open / close</span>
  </button>
  <button class="vbtn" data-v="2" onclick="setView(2)">
    <span class="ic">{ }</span>
    <span class="nm">Claude Code</span>
    <span class="ht">opens terminal</span>
  </button>
  <button class="vbtn" data-v="3" onclick="toggleCanvas()">
    <span class="ic">&#11035;</span>
    <span class="nm">Canvas</span>
    <span class="ht">draw on display</span>
  </button>
</div>

<div class="sec">// speed</div>
<div class="speed-row">
  <span class="sl">slow</span>
  <input type="range" id="spd" min="1" max="3" value="1" step="1" oninput="setSpeed(this.value)">
  <span class="sv" id="spdV">slow</span>
</div>

<div class="ctrl">
  <div class="ci" style="flex:1;display:flex;flex-direction:column;gap:4px;align-items:stretch">
    <span class="cl" style="font-size:10px;color:#8a8278;letter-spacing:1px;font-weight:bold;text-align:center">BACKGROUND</span>
    <input type="color" class="cs" id="bgCol" value="#aa4818" oninput="onBgChange(this.value)">
  </div>
  <div class="ci" style="flex:1;display:flex;flex-direction:column;gap:4px;align-items:stretch">
    <span class="cl" style="font-size:10px;color:#8a8278;letter-spacing:1px;font-weight:bold;text-align:center">PEN COLOR</span>
    <input type="color" class="cs" id="penCol" value="#000000">
  </div>
</div>

<div class="sec">// terminal</div>
<div class="twrap" id="twrap">
  <div class="thdr">
    <span class="tttl">&#9658; clawd:~$</span>
    <button class="tx" onclick="closeTerm()">&#x2715; exit terminal</button>
  </div>
  <div class="trow">
    <input class="tin" id="tin" type="text" placeholder="type here..."
           autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false">
    <button class="tgo" onclick="termEnter()">&#8629;</button>
  </div>
</div>

<div class="cwrap" id="cwrap">
  <div class="dacts">
    <button class="db hi" onclick="clearAll()">&#11035; clear</button>
    <button class="db" style="border-color:#28b878;color:#28b878" onclick="toggleCanvas()">&#10003; done</button>
  </div>
  <canvas id="cvs" width="280" height="240"></canvas>
</div>

<div class="toast" id="toast"></div>

<script>
let activeView  = 0;
let termOpen    = false;
let canvasOpen  = false;
let blOn        = true;
let isBusy      = false;
let drawing     = false;
let lastX = 0, lastY = 0;
let tt;

const spdLabels = ['','slow','normal','fast'];

// ── Toast ──────────────────────────────────────────────────────
function toast(msg, ok=true) {
  const el = document.getElementById('toast');
  el.textContent = msg;
  el.style.borderColor = ok ? '#28b878' : '#c96a3e';
  el.classList.add('show');
  clearTimeout(tt);
  tt = setTimeout(() => el.classList.remove('show'), 1300);
}

// ── Busy ────────────────────────────────────────────────────────
function setBusy(b) {
  isBusy = b;
  document.getElementById('busy').classList.toggle('show', b);
  const locked = b || termOpen;
  document.querySelectorAll('.vbtn').forEach(el => {
    // when canvas open, keep canvas btn (data-v=3) active so user can exit
    el.disabled = canvasOpen ? parseInt(el.dataset.v) !== 3 : locked;
  });
  document.querySelectorAll('.lbtn').forEach(el => el.disabled = locked || canvasOpen);
  document.querySelectorAll('.cbtn').forEach(el => {
    if (el.id !== 'blBtn') el.disabled = locked;
  });
}

// ── HTTP ────────────────────────────────────────────────────────
async function req(path) {
  try { const r = await fetch(path); return r.ok; }
  catch(e) { toast('no connection', false); return false; }
}

async function waitNotBusy() {
  for (let i = 0; i < 100; i++) {
    try {
      const r = await fetch('/state');
      const j = await r.json();
      if (!j.busy) return;
    } catch(e) {}
    await new Promise(r => setTimeout(r, 150));
  }
}

// ── Background colour ───────────────────────────────────────────
async function onBgChange(hex) {
  if (canvasOpen) {
    await req('/draw/clear?bg=' + encodeURIComponent(hex));
  } else {
    await req('/redraw?bg=' + encodeURIComponent(hex));
  }
  redrawCanvas(hex);
}

// ── Speed ───────────────────────────────────────────────────────
async function setSpeed(v) {
  document.getElementById('spdV').textContent = spdLabels[v];
  await req('/speed?v=' + v);
}

// ── Views ───────────────────────────────────────────────────────
async function setView(v) {
  if (isBusy || termOpen || canvasOpen) return;
  if (v === 3) { toggleCanvas(); return; }  // canvas button in grid
  const keys = ['w','s','d'];
  if (!await req('/cmd?k=' + keys[v])) return;
  activeView = v;
  document.querySelectorAll('.vbtn').forEach(b =>
    b.classList.toggle('active', parseInt(b.dataset.v) === v));
  if (v === 2) {
    termOpen = true;
    document.getElementById('twrap').classList.add('open');
    setBusy(false);   // re-run to apply termOpen lock
    setBusy(false);
    document.querySelectorAll('.vbtn,.lbtn').forEach(b => b.disabled = true);
    const cvb = document.getElementById('cvBtn'); if (cvb) cvb.disabled = true;
    document.getElementById('tin').focus();
    toast('terminal open');
    return;
  }
  setBusy(true);
  await waitNotBusy();
  setBusy(false);
}

// ── Logo animations (kept for startup, not exposed in UI) ──────

// ── Backlight ───────────────────────────────────────────────────
async function toggleBL() {
  blOn = !blOn;
  await req('/backlight?on=' + (blOn ? 1 : 0));
  const b = document.getElementById('blBtn');
  b.textContent = blOn ? '\u2600 display on' : '\u25cb display off';
  b.classList.toggle('on', blOn);
  b.classList.toggle('dim', !blOn);
}

// ── Canvas toggle ───────────────────────────────────────────────
async function toggleCanvas() {
  canvasOpen = !canvasOpen;
  document.getElementById('cwrap').classList.toggle('open', canvasOpen);
  const b = document.getElementById('cvBtn');
  if (b) { b.classList.toggle('on', canvasOpen); b.textContent = canvasOpen ? '\u2b1b canvas on' : '\u2b1b canvas'; }
  // highlight the canvas vbtn (data-v=3) in the grid
  document.querySelectorAll('.vbtn').forEach(btn =>
    btn.classList.toggle('active', canvasOpen && parseInt(btn.dataset.v) === 3));
  await req('/canvas?on=' + (canvasOpen ? 1 : 0));
  if (canvasOpen) {
    const bg = document.getElementById('bgCol').value;
    redrawCanvas(bg);
    await req('/draw/clear?bg=' + encodeURIComponent(bg));
    // lock all other buttons
    document.querySelectorAll('.vbtn,.lbtn').forEach(b => b.disabled = true);
    toast('canvas active');
  } else {
    setBusy(false);   // re-evaluate locks
    toast('canvas off');
  }
}

// ── Terminal ────────────────────────────────────────────────────
const tin = document.getElementById('tin');
let lastVal = '';
tin.addEventListener('input', async () => {
  const cur = tin.value, prev = lastVal;
  if (cur.length > prev.length) {
    await req('/char?c=' + encodeURIComponent(cur[cur.length - 1]));
  } else if (cur.length < prev.length) {
    await req('/char?c=%08');
  }
  lastVal = cur;
});
async function termEnter() {
  await req('/char?c=%0A');
  tin.value = ''; lastVal = ''; tin.focus();
}
tin.addEventListener('keydown', e => {
  if (e.key === 'Enter') { e.preventDefault(); termEnter(); }
});
async function closeTerm() {
  await req('/cmd?k=q');
  termOpen = false;
  document.getElementById('twrap').classList.remove('open');
  setBusy(false);
  toast('terminal closed');
}

// ── Canvas drawing — send full stroke on finger lift ────────────
const cvs = document.getElementById('cvs');
const ctx = cvs.getContext('2d');
let strokePts = [];

function getPos(e) {
  const r = cvs.getBoundingClientRect();
  const sx = cvs.width / r.width, sy = cvs.height / r.height;
  const s = e.touches ? e.touches[0] : e;
  return { x: (s.clientX - r.left) * sx, y: (s.clientY - r.top) * sy };
}

function redrawCanvas(hex) {
  ctx.fillStyle = hex;
  ctx.fillRect(0, 0, cvs.width, cvs.height);
}

function startDraw(e) {
  e.preventDefault();
  drawing = true;
  strokePts = [];
  const p = getPos(e); lastX = p.x; lastY = p.y;
  strokePts.push({ x: Math.round(p.x), y: Math.round(p.y) });
  // draw dot on canvas preview only — no display send yet
  ctx.beginPath(); ctx.arc(p.x, p.y, 2, 0, Math.PI * 2);
  ctx.fillStyle = document.getElementById('penCol').value; ctx.fill();
}
function moveDraw(e) {
  if (!drawing) return; e.preventDefault();
  const p = getPos(e);
  ctx.beginPath(); ctx.moveTo(lastX, lastY); ctx.lineTo(p.x, p.y);
  ctx.strokeStyle = document.getElementById('penCol').value;
  ctx.lineWidth = 4; ctx.lineCap = 'round'; ctx.stroke();
  strokePts.push({ x: Math.round(p.x), y: Math.round(p.y) });
  lastX = p.x; lastY = p.y;
}
async function endDraw(e) {
  if (!drawing) return; drawing = false;
  if (!canvasOpen || strokePts.length < 1) return;
  const pen = document.getElementById('penCol').value.replace('#', '');
  const pts = strokePts.map(p => p.x + ',' + p.y).join(';');
  await req('/draw/stroke?pen=' + pen + '&pts=' + encodeURIComponent(pts));
  strokePts = [];
}

cvs.addEventListener('mousedown',  startDraw);
cvs.addEventListener('mousemove',  moveDraw);
cvs.addEventListener('mouseup',    endDraw);
cvs.addEventListener('mouseleave', endDraw);
cvs.addEventListener('touchstart', startDraw, {passive:false});
cvs.addEventListener('touchmove',  moveDraw,  {passive:false});
cvs.addEventListener('touchend',   endDraw);

// Clear = clear both web canvas and display
async function clearAll() {
  const bg = document.getElementById('bgCol').value;
  redrawCanvas(bg);
  await req('/draw/clear?bg=' + encodeURIComponent(bg));
  toast('cleared');
}

// Init: sync speed and backlight from ESP32, reset bg to default
(async () => {
  try {
    const r = await fetch('/state');
    const j = await r.json();
    // Sync speed
    const spd = j.speed || 1;
    document.getElementById('spd').value = spd;
    document.getElementById('spdV').textContent = spdLabels[spd];
    // Sync backlight
    if (j.bl === false) {
      blOn = false;
      const b = document.getElementById('blBtn');
      b.textContent = '\u25cb display off';
      b.classList.remove('on'); b.classList.add('dim');
    }
  } catch(e) {}
  // Always reset bg picker to default orange on page load
  document.getElementById('bgCol').value = '#aa4818';
  redrawCanvas('#aa4818');
})();
</script>
</body>
</html>
)rawhtml";

// ═════════════════════════════════════════════════════════════
//  WEB ROUTES
// ═════════════════════════════════════════════════════════════

void routeRoot() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.send_P(200, "text/html", INDEX_HTML);
}

void routeCmd() {
  if (!server.hasArg("k") || server.arg("k").isEmpty()) {
    server.send(400, "application/json", "{\"e\":1}"); return;
  }
  const char c = server.arg("k")[0];

  if (termMode) {
    if (c == 'q') { termMode = false; drawCodeView(); }
    server.send(200, "application/json", "{\"ok\":1}"); return;
  }

  server.send(200, "application/json", "{\"ok\":1}");
  switch (c) {
    case 'w': currentView = VIEW_EYES_NORMAL; animNormalEyes(); break;
    case 's': currentView = VIEW_EYES_SQUISH; animSquishEyes(); break;
    case 'd':
      currentView = VIEW_CODE; drawCodeView();
      termMode = true; termClear(); termFullRedraw(); break;
    case 'a':
      currentView = VIEW_EYES_NORMAL;
      animLogoReveal();
      break;
  }
}

void routeChar() {
  if (!termMode) { server.send(200, "application/json", "{\"ok\":1}"); return; }
  const String val = server.arg("c");
  if (val.length() > 0) termAddChar(val[0]);
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeSpeed() {
  if (server.hasArg("v")) animSpeed = constrain(server.arg("v").toInt(), 1, 3);
  server.send(200, "application/json", "{\"ok\":1}");
}

// /redraw?bg=hex — set animBg and immediately redraw current view
void routeRedraw() {
  if (server.hasArg("bg")) {
    animBgColor = hexToRgb565(server.arg("bg"));
    drawBgColor = animBgColor;
  }
  switch (currentView) {
    case VIEW_EYES_NORMAL: drawNormalEyes(); break;
    case VIEW_EYES_SQUISH: drawSquishEyes(); break;
    case VIEW_CODE:        drawCodeView();   break;
    case VIEW_DRAW:        tft.fillScreen(drawBgColor); break;
  }
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeCanvas() {
  const bool on = server.hasArg("on") && server.arg("on") == "1";
  if (on) { currentView = VIEW_DRAW; tft.fillScreen(drawBgColor); }
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeDrawClear() {
  const String bg = server.hasArg("bg") ? server.arg("bg") : "#aa4818";
  drawBgColor = hexToRgb565(bg);
  animBgColor = drawBgColor;  // keep in sync
  currentView = VIEW_DRAW; termMode = false;
  tft.fillScreen(drawBgColor);
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeDrawStroke() {
  if (!server.hasArg("pts") || !server.hasArg("pen")) {
    server.send(200, "application/json", "{\"ok\":1}"); return;
  }
  const uint16_t color = hexToRgb565(server.arg("pen"));
  const String   data  = server.arg("pts");
  currentView = VIEW_DRAW;

  struct Pt { int16_t x, y; };
  Pt prev = {-1, -1};
  int start = 0;
  while (start < (int)data.length()) {
    int semi = data.indexOf(';', start);
    if (semi == -1) semi = data.length();
    String entry = data.substring(start, semi);
    const int comma = entry.indexOf(',');
    if (comma > 0) {
      const int16_t x = entry.substring(0, comma).toInt();
      const int16_t y = entry.substring(comma + 1).toInt();
      if (prev.x >= 0) {
        tft.drawLine(prev.x, prev.y, x, y, color);
        tft.drawLine(prev.x + 1, prev.y, x + 1, y, color);
        tft.drawLine(prev.x, prev.y + 1, x, y + 1, color);
      } else {
        tft.fillCircle(x, y, 2, color);
      }
      prev = {x, y};
    }
    start = semi + 1;
  }
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeBacklight() {
  setBacklight(server.hasArg("on") && server.arg("on") == "1");
  server.send(200, "application/json", "{\"ok\":1}");
}

// Convert RGB565 back to #RRGGBB for state endpoint
String rgb565ToHex(uint16_t c) {
  uint8_t r = ((c >> 11) & 0x1F) << 3;
  uint8_t g = ((c >> 5)  & 0x3F) << 2;
  uint8_t b = (c & 0x1F) << 3;
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
  return String(buf);
}

void routeState() {
  String j = "{\"view\":"; j += currentView;
  j += ",\"busy\":";   j += busy        ? "true" : "false";
  j += ",\"term\":";   j += termMode    ? "true" : "false";
  j += ",\"bl\":";     j += backlightOn ? "true" : "false";
  j += ",\"speed\":";  j += animSpeed;
  j += "}";
  server.send(200, "application/json", j);
}

void routeNotFound() { server.send(404, "text/plain", "not found"); }

// ═════════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BLK, OUTPUT);
  setBacklight(true);

  SPI.begin(8, -1, 10, TFT_CS);   // SCK=8, MOSI=10
  tft.init(240, 280);
  tft.setSPISpeed(40000000);
  tft.setRotation(1);
  initColours();

  // ── Boot splash ────────────────────────────────────────────
  tft.fillScreen(animBgColor);
  tft.setTextColor(C_WHITE); tft.setTextSize(3);
  tft.setCursor(DISP_W / 2 - 54, DISP_H / 2 - 22); tft.print("Clawd");
  tft.setCursor(DISP_W / 2 - 54, DISP_H / 2 + 14); tft.print("Mochi");
  delay(1200);

  // ── Logo shown once at startup ─────────────────────────────
  animLogoReveal();

  // ── Start WiFi ─────────────────────────────────────────────
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  // ── WiFi info screen (stays until first web request) ───────
  tft.fillScreen(C_DARKBG);
  tft.fillRect(0, 0, DISP_W, 4, C_ORANGE);
  tft.setTextColor(C_WHITE);  tft.setTextSize(2);
  tft.setCursor(12, 16);  tft.print("WiFi: ClaWD-Mochi");
  tft.setTextColor(C_MUTED);  tft.setTextSize(1);
  tft.setCursor(12, 44);  tft.print("password: clawd1234");
  tft.setTextColor(C_WHITE);  tft.setTextSize(2);
  tft.setCursor(12, 68);  tft.print("Open browser:");
  tft.setTextColor(C_ORANGE); tft.setTextSize(2);
  tft.setCursor(12, 94);  tft.print("192.168.4.1");
  tft.setTextColor(C_MUTED);  tft.setTextSize(1);
  tft.setCursor(12, 124); tft.print("press any button to start");
  delay(10000);

  currentView = VIEW_EYES_NORMAL;
  animNormalEyes();

  // ── Register routes ────────────────────────────────────────
  server.on("/",            HTTP_GET, routeRoot);
  server.on("/cmd",         HTTP_GET, routeCmd);
  server.on("/char",        HTTP_GET, routeChar);
  server.on("/speed",       HTTP_GET, routeSpeed);
  server.on("/redraw",      HTTP_GET, routeRedraw);
  server.on("/canvas",      HTTP_GET, routeCanvas);
  server.on("/draw/clear",  HTTP_GET, routeDrawClear);
  server.on("/draw/stroke", HTTP_GET, routeDrawStroke);
  server.on("/backlight",   HTTP_GET, routeBacklight);
  server.on("/state",       HTTP_GET, routeState);
  server.onNotFound(routeNotFound);
  server.begin();

  // WiFi info stays on screen — first button press triggers setView/cmd
  // which will replace it with the correct view
}

// ═════════════════════════════════════════════════════════════
//  LOOP
// ═════════════════════════════════════════════════════════════

// ── Serial command handler ───────────────────────────────────
String serialBuf = "";

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      serialBuf.trim();
      if (serialBuf.length() == 0) { serialBuf = ""; return; }

      // single-char commands: w s d a q
      if (serialBuf.length() == 1) {
        char k = serialBuf[0];
        if (termMode && k == 'q') { termMode = false; drawCodeView(); }
        else switch (k) {
          case 'w': currentView = VIEW_EYES_NORMAL; animNormalEyes(); break;
          case 's': currentView = VIEW_EYES_SQUISH; animSquishEyes(); break;
          case 'd': currentView = VIEW_CODE; drawCodeView();
                    termMode = true; termClear(); termFullRedraw(); break;
          case 'a': currentView = VIEW_EYES_NORMAL; animLogoReveal(); break;
        }
        Serial.println("ok");
      }
      // t<text> — type in terminal
      else if (serialBuf[0] == 't') {
        if (!termMode) {
          currentView = VIEW_CODE; drawCodeView();
          termMode = true; termClear(); termFullRedraw();
        }
        for (uint16_t i = 1; i < serialBuf.length(); i++) termAddChar(serialBuf[i]);
        Serial.println("ok");
      }
      // bg#RRGGBB
      else if (serialBuf.startsWith("bg")) {
        String hex = serialBuf.substring(2);
        animBgColor = hexToRgb565(hex);
        drawBgColor = animBgColor;
        switch (currentView) {
          case VIEW_EYES_NORMAL: drawNormalEyes(); break;
          case VIEW_EYES_SQUISH: drawSquishEyes(); break;
          case VIEW_CODE:        drawCodeView();   break;
          case VIEW_DRAW:        tft.fillScreen(drawBgColor); break;
        }
        Serial.println("ok");
      }
      // speed1 / speed2 / speed3
      else if (serialBuf.startsWith("speed")) {
        animSpeed = constrain(serialBuf.substring(5).toInt(), 1, 3);
        Serial.println("ok");
      }
      // canvas — switch to draw mode
      else if (serialBuf == "canvas") {
        currentView = VIEW_DRAW; termMode = false;
        tft.fillScreen(drawBgColor);
        Serial.println("ok");
      }
      // line x1,y1,x2,y2,#RRGGBB — draw line on canvas
      else if (serialBuf.startsWith("line ")) {
        String args = serialBuf.substring(5);
        int c1 = args.indexOf(',');
        int c2 = args.indexOf(',', c1+1);
        int c3 = args.indexOf(',', c2+1);
        int c4 = args.indexOf(',', c3+1);
        if (c4 > 0) {
          int16_t x1 = args.substring(0, c1).toInt();
          int16_t y1 = args.substring(c1+1, c2).toInt();
          int16_t x2 = args.substring(c2+1, c3).toInt();
          int16_t y2 = args.substring(c3+1, c4).toInt();
          uint16_t col = hexToRgb565(args.substring(c4+1));
          tft.drawLine(x1, y1, x2, y2, col);
          tft.drawLine(x1+1, y1, x2+1, y2, col);
        }
        Serial.println("ok");
      }
      // logo
      else if (serialBuf == "logo") {
        currentView = VIEW_EYES_NORMAL;
        animLogoReveal();
        Serial.println("ok");
      }
      // img — receive raw RGB565 image (280x240, 134400 bytes)
      else if (serialBuf == "img") {
        currentView = VIEW_DRAW; termMode = false;
        tft.startWrite();
        tft.setAddrWindow(0, 0, DISP_W, DISP_H);
        Serial.println("ready");
        uint32_t total = (uint32_t)DISP_W * DISP_H * 2;
        uint32_t received = 0;
        uint8_t buf[512];
        unsigned long lastData = millis();
        while (received < total && millis() - lastData < 10000) {
          int avail = Serial.available();
          if (avail > 0) {
            int toRead = min((int)sizeof(buf), min(avail, (int)(total - received)));
            Serial.readBytes(buf, toRead);
            tft.writePixels((uint16_t*)buf, toRead / 2);
            received += toRead;
            lastData = millis();
          }
        }
        tft.endWrite();
        Serial.printf("done %lu\n", received);
      }
      // status[N] [-c#RRGGBB] <text> — show text below eyes
      else if (serialBuf.startsWith("status")) {
        uint8_t idx = 6;
        if (idx < serialBuf.length() && serialBuf[idx] >= '1' && serialBuf[idx] <= '4') {
          statusSize = serialBuf[idx] - '0';
          idx++;
        }
        String rest = serialBuf.substring(idx);
        rest.trim();
        if (rest.startsWith("-c")) {
          int sp = rest.indexOf(' ', 2);
          if (sp > 0) {
            statusColor = hexToRgb565(rest.substring(2, sp));
            rest = rest.substring(sp + 1);
          } else {
            statusColor = hexToRgb565(rest.substring(2));
            rest = "";
          }
        }
        statusText = rest;
        statusText.trim();
        if (currentView == VIEW_EYES_NORMAL) drawNormalEyes();
        else if (currentView == VIEW_EYES_SQUISH) drawSquishEyes();
        Serial.println("ok");
      }
      else {
        Serial.println("unknown cmd");
      }
      serialBuf = "";
    } else {
      serialBuf += c;
    }
  }
}

void loop() {
  server.handleClient();
  handleSerial();
}
