# Ändringar för 280x240 skärm + Serial CLI

## 1. Display 240x240 → 280x240

### Header-kommentar
```
1.54" 240×240  →  1.69" 280×240
```

### Display-dimensioner
```c
#define DISP_W 240  →  #define DISP_W 280
```
`DISP_H` förblir 240.

### TFT init (i setup)
```c
tft.init(240, 240);  →  tft.init(240, 280);
```

### Logo-centrum
```c
#define LOGO_CX 120  →  #define LOGO_CX 140
```

### LOGO_TRIS och LOGO_SEGS
Alla X-koordinater i `LOGO_TRIS[][6]` och `LOGO_SEGS[][4]` offsettas med **+20**.

- LOGO_TRIS: varje `{x1,y1,x2,y2,x3,y3}` — x1, x2, x3 får +20
- LOGO_SEGS: varje `{x1,y1,x2,y2}` — x1, x2 får +20

### Terminal-kolumner
```c
#define TERM_COLS 15  →  #define TERM_COLS 18
```

### HTML canvas (inbäddad i webserver)
```html
<canvas id="cvs" width="240" height="240">  →  width="280" height="240"
```

### Orange färg
```c
C_ORANGE = tft.color565(218, 17, 0);  →  C_ORANGE = tft.color565(219, 99, 0);
```

### Default bakgrundsfärg (i initColours)
```c
animBgColor = C_ORANGE;
drawBgColor = C_ORANGE;
```

---

## 2. Status-text under ögon

### Nya globala variabler (efter drawBgColor)
```c
String   statusText   = "";  // text shown below eyes
uint8_t  statusSize   = 2;   // font size for status text
uint16_t statusColor  = 0;   // colour for status text (0 = use C_BLACK)
```

### Ny funktion drawStatusText (före drawNormalEyes)
```c
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
```

### Anropa drawStatusText() i slutet av:
- `drawNormalEyes()` — efter ögonen ritats
- `drawSquishEyes()` — efter ögonen ritats

---

## 3. Serial CLI-kommandon (handleSerial i loop)

### Ny global + funktion (före loop)
```c
String serialBuf = "";

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      serialBuf.trim();
      if (serialBuf.length() == 0) { serialBuf = ""; return; }
      // ... kommandoparsning ...
      serialBuf = "";
    } else {
      serialBuf += c;
    }
  }
}
```

### loop() ändras
```c
void loop() {
  server.handleClient();
  handleSerial();
}
```

### Serial-kommandon som parsas:

**Enkelkommando (1 tecken):**
- `w` — VIEW_EYES_NORMAL + animNormalEyes()
- `s` — VIEW_EYES_SQUISH + animSquishEyes()
- `d` — VIEW_CODE + terminal-läge
- `a` — animLogoReveal()
- `q` — avsluta terminal-läge

**Text-kommandon:**
- `t<text>` — skriv text i terminal (byter till terminal-läge om ej aktivt)
- `bg#RRGGBB` — ändra bakgrundsfärg, rita om nuvarande vy
- `speed1`/`speed2`/`speed3` — ändra animationshastighet
- `logo` — visa logo-animation
- `canvas` — byt till draw-läge
- `line x1,y1,x2,y2,#RRGGBB` — rita linje på canvas (2px tjock)
- `img` — ta emot RGB565-bild (280x240, 134400 bytes binärt efter "ready")
- `status[N] [-c#RRGGBB] <text>` — visa text under ögon
  - N = fontstorlek 1-4 (valfritt, default 2)
  - -c#RRGGBB = textfärg (valfritt, default svart)
  - tom text = rensa

### Exempel serial-parsning för status:
```c
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
```

### Exempel serial-parsning för img:
```c
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
```

### Exempel serial-parsning för line:
```c
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
```

---

## 4. Serial port: undvik reset

Vid öppning av serial-porten, sätt DTR och RTS till false för att undvika att ESP32 startar om:
```python
ser = serial.Serial()
ser.port = port
ser.baudrate = 115200
ser.timeout = 2
ser.dtr = False
ser.rts = False
ser.open()
```

---

## 5. Python CLI (mochi.py)

Se separat fil `mochi.py` för komplett CLI med kommandon:
- `text`, `eyes`, `squish`, `terminal`, `logo`, `quit`
- `bg`, `speed`, `status`, `canvas`, `line`, `img`
- `raw`, `ports`

Bild-stöd (img) kräver `pip install pyserial Pillow`.
RGB565 skickas i **little-endian** byte-ordning.
Tint-stöd: `mochi.py img -t "#db6300" bild.png`

---

## 6. Claude Code hooks (~/.claude/settings.json)

```json
{
  "hooks": {
    "UserPromptSubmit": [{
      "hooks": [{
        "type": "command",
        "command": "DIR=$(basename \"$PWD\") && python3 \"<sökväg>/mochi.py\" squish && python3 \"<sökväg>/mochi.py\" status -s2 -c#dddddd \"$DIR\"",
        "timeout": 10,
        "async": true
      }]
    }],
    "Stop": [{
      "hooks": [{
        "type": "command",
        "command": "DIR=$(basename \"$PWD\") && python3 \"<sökväg>/mochi.py\" eyes && python3 \"<sökväg>/mochi.py\" status -s2 -c#dddddd \"$DIR\"",
        "timeout": 10,
        "async": true
      }]
    }]
  }
}
```
