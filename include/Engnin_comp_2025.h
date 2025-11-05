#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Fonts/FreeMono9pt7b.h>   //ï¿½rï¿½ï¿½9pt
#include <Fonts/FreeMono12pt7b.h>  //ï¿½rï¿½ï¿½12pt
#include <Fonts/FreeMono18pt7b.h>  //ï¿½rï¿½ï¿½18pt
#include <Fonts/FreeMono24pt7b.h>  //ï¿½rï¿½ï¿½24pt
#include <SPI.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
#define keyB(byte) (PINB & (1 << (byte)))
#define keyC(byte) (PINC & (1 << (byte)))
#define keyD(byte) (PIND & (1 << (byte)))

uint16_t  timer1_counter;
void timer_ini(uint16_t tm){
  noInterrupts();  //¸T¤î©Ò¦³¤¤?
  TCCR1A = 0;
  TCCR1B = 0;
  timer1_counter = tm;  //?¥[?timer1?65536-16MHz/256/2Hz
  TCNT1 = timer1_counter;  //?¥[?timer
  TCCR1B |= (1 << CS12);   //256 ¤À?¾¹(256 prescaler?)
  TIMSK1 |= (1 << TOIE1);  //?¥Î©w?¾¹·¸¥X¤¤?
  interrupts();  
}

uint16_t convert24to16(uint32_t rgb) {
    uint32_t r = (rgb >> 16) & 0xFF;
    uint32_t g = (rgb >> 8) & 0xFF;
    uint32_t b = rgb & 0xFF;
    r = map(r, 0x00, 0xFF, 0x00, 0x1F) << 11;
    g = map(g, 0x00, 0xFF, 0x00, 0x3F) << 5;
    b = map(b, 0x00, 0xFF, 0x00, 0x1F);
    return r | g | b;
}

