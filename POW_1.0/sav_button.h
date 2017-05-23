#ifndef SavButton_h
#define SavButton_h
#include "Arduino.h"

//#define DEBUG_SERIAL 1
#define DebugSerial Serial1

typedef enum {
   SB_NONE = 0,
   SB_CLICK,
   SB_AUTO_CLICK,
   SB_LONG_CLICK,
   SB_SEQ_CLICK,
}SBUTTON_CLICK;


class SButton {
  private :
     uint8_t  Pin;
     bool     State;
     bool     Long_press_state;
     bool     Auto_press_state;
     uint32_t Ms1;
     uint32_t Ms2;
     uint32_t Ms_auto_click;
     uint16_t TM_bounce;
     uint16_t TM_long_click;
     uint16_t TM_auto_click;
     uint16_t TM_seq_click;
     uint16_t Period_auto_click;
  public :
     SButton(uint8_t pin,uint16_t tm1 = 50, uint16_t tm2 = 0, uint16_t tm3 = 0, uint16_t tm4 = 500, uint16_t tm5 = 0);
     void begin();
     SBUTTON_CLICK Loop();
     uint16_t Count;
     uint16_t Time;
};     


#endif
