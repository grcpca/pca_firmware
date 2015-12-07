#include <Adafruit_NeoPixel.h>

#define PIN 0

#define NUM_PIXELS  4
// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define BUTTON  2

uint32_t x, y, z, w;

void setupRNG() {
  x = millis();
  delay(5);
  y = micros();
  z = micros()*2;
  w = micros()+100;
}

uint32_t rdm(void) { //xorshift128
    uint32_t t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

void setAll(uint32_t c) {
  uint16_t i;
  for(i = 0; i<NUM_PIXELS; i++) 
    strip.setPixelColor(i, c);
  strip.show();
}

void setOne(uint16_t num, uint32_t c) {
  setAll(0);
  strip.setPixelColor(num, c);  
  strip.show();
}

void safeSet(uint8_t num, uint32_t c) {
  if(num >=0 && num <NUM_PIXELS) strip.setPixelColor(num, c);
}

uint32_t scaleCol(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
  uint8_t rt = ((uint16_t)r * brightness) >> 8;
  uint8_t gt = ((uint16_t)g * brightness) >> 8;
  uint8_t bt = ((uint16_t)b * brightness) >> 8;
  return strip.Color(rt,gt,bt);
}

class Effect {
  public:
  Effect() {  }
  virtual void init() { }
  virtual void update() {}
};

class Rainbow:public Effect {
  uint8_t idx;
  public:
  Rainbow():idx(0) {}
  void init() {idx=0;}
  void update() {
    rainbow(20, idx++);
  }
};

uint8_t cycle_order[] = {0,1,2,3};
class Cycle:public Effect {
  uint8_t r, g, b;
  uint8_t idx;
  byte clr_idx;
  public:
  Cycle(uint8_t _r, uint8_t _g, uint8_t _b) : r(_r), g(_g), b(_b), idx(0),clr_idx(0) {
  }
  void init() {
    idx = 0;
    clr_idx = 0;
  }
  void update() {
    idx++;
    if(idx >=strip.numPixels()) idx = 0;
//    setOne(cycle_order[idx], strip.Color(r,g,b));
    clr_idx+=10;
    setOne(cycle_order[idx], Wheel(clr_idx));
    delay(300);
  }
};

class Larson:public Effect {
  uint8_t r, g, b;
  uint8_t idx;
  byte dir;
  uint8_t mode;
  public:
  Larson(uint8_t _r, uint8_t _g, uint8_t _b): r(_r), g(_g), b(_b), idx(0), dir(1), mode(0) {
  }
  
  void init() {
    idx = 0; dir = 1;
  }
  
  void update() {
    idx+=dir;
    if(idx < 0 || idx >= NUM_PIXELS) dir = -dir;
    setAll(0);
    safeSet(idx-1, scaleCol(r,g,b, 50));
    safeSet(idx, strip.Color(r,g,b));
    safeSet(idx+1, scaleCol(r,g,b, 50));
    strip.show();
    delay(100);
  }
};

uint8_t red[] = {255,0,0};
uint8_t yellow[] = {255, 255, 0};

class Candle :public Effect { 
  
  public:
  Candle() { }
  
  void init() {
  }  
  
  void update() {
    uint8_t br1 = (rdm()&0x7F)+128;
    uint8_t br2 = (rdm()&0x7F)+64;  
    
    uint32_t cc1 = scaleCol(red[0], red[1], red[2], br1);
    uint32_t cc2 = scaleCol(yellow[0], yellow[1], yellow[2], br2);
    
    for(int i=0; i<NUM_PIXELS; i+=2) {
      strip.setPixelColor(i, cc1);
      strip.setPixelColor(i+1, cc2);
    }
    
    strip.show();
    delay(rdm() & 0x7F);  
  }
};

class RandomFade: public Effect
{
  byte idx;
  uint8_t br;
  uint8_t dbr;
  uint8_t r, g, b;
  uint32_t c;
  public:
  RandomFade() {}
  
  void init() {
    idx = rdm() & 0x3;
    br = 10;
    dbr = 4;
  }
  
  void update() {
      br += dbr;
      if(br > 200 || br < 10) {
        dbr = -dbr;
      }
      
      if (br < 10) {
        r = rdm(); g = rdm(); b = rdm();
        idx = r & 0x3;
      }
      uint32_t cc = scaleCol(r, g, b, br);
      setOne(idx, cc);   
  }
};

#define MAX_EFFECTS 5

Effect * effects[MAX_EFFECTS];
Effect * cur_effect;

void setup() {
  pinMode(BUTTON, INPUT_PULLUP);
  
  setupRNG();
  
  strip.begin();
  strip.show();
  
  effects[0] = new Rainbow();
  effects[1] = new Cycle(0,100,250);
  effects[2] = new Candle();
  effects[3] = new Larson(0,200, 0);
  effects[4] = new RandomFade();
  
  cur_effect = effects[0];
  cur_effect->init();    
}

byte btn=0;
unsigned long checkTimer;
boolean okCheckBtns = true;
uint8_t cur_eff_idx = 0;

byte last_btn;
boolean btn_pressed=false;

void loop() {
  
  // Check inputs also, debounce by waiting 100ms before next
  if(okCheckBtns) {
    last_btn = btn;

    btn_pressed = false;
    btn = digitalRead(BUTTON);
    
    okCheckBtns = false;
    checkTimer = millis();

    if(last_btn == 1 && btn == 0) {
      btn_pressed = true;    
    }
    
    if(btn_pressed) {
       cur_eff_idx = (cur_eff_idx+1) % MAX_EFFECTS;

       cur_effect = effects[cur_eff_idx];
       cur_effect->init();
    }               
  }
  
  cur_effect->update();        

  if(!okCheckBtns && (millis()-checkTimer) > 100) 
    okCheckBtns = true;
   
  delay(10); // 100Hz should be good
}

void rainbow(uint8_t wait, uint8_t j) {
  uint16_t i;
  for(i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel((i+j) & 255));
  }
  strip.show();
  delay(wait);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}



