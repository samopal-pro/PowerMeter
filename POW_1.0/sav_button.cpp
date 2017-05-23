#include "sav_button.h"

/**
 * Конструктор класса кнопки
 * Кнопка это цифровой пин подтянутый к питанию и замыкаемый на землю
 * Событие срабатывания происходит по нажатию кнопки (возвращается 1)
 * и отпусканию кнопки (возвращается время нажатия кнопки, мсек)
 * tm1 - таймаут дребезга контактов. По умолчанию 50мс
 * tm2 - время длинного нажатия клавиши. По умолчанию 2000мс
 * tm3 - врямы перевода кнопки в генерацию серии нажатий. По умолсанию отключено
 * tm4 - время между кликами в серии. По умолчанию 500 мс. Если tm3 = 0 то не работает
 */
SButton::SButton(uint8_t pin,uint16_t tm1, uint16_t tm2,uint16_t tm3, uint16_t tm4, uint16_t tm5){
   Pin               = pin;
   State             = false;
   Long_press_state  = false;
   Auto_press_state  = false;
   Ms1               = 0;
   Ms2               = 0;
   Ms_auto_click     = 0;
   TM_bounce         = tm1;
   TM_long_click     = tm2;
   TM_auto_click     = tm3;
   Period_auto_click = tm4;
   TM_seq_click      = tm5;
   Count             = 0;
   Time              = 0;
}

/**
 * Инициализация кнопки
 */
void SButton::begin() {
   pinMode(Pin, INPUT_PULLUP);
#ifdef DEBUG_SERIAL      
   DebugDebugSerial.print("Init button pin ");
   DebugSerial.println(Pin);
#endif      
}

/**
 * Действие производимое в цикле или по таймеру
 * возвращает SB_NONE если кнопка не нажата и событие нажатие или динного нажатия кнопки
*/
SBUTTON_CLICK SButton::Loop() {
   uint32_t ms = millis();
   bool pin_state = digitalRead(Pin);
// Фиксируем нажатие кнопки 
   if( pin_state == LOW && State == false && (ms-Ms1)>TM_bounce ){
       uint16_t dt = ms - Ms1;
       Long_press_state = false;
       Auto_press_state = false;
#ifdef DEBUG_SERIAL      
       DebugSerial.print(">>>Event button, pin=");
       DebugSerial.print(Pin);
       DebugSerial.print(",press ON, tm=");
       DebugSerial.print(dt);
       DebugSerial.println(" ms");
#endif      
       State = true;
       if(TM_seq_click > 0 && dt < TM_seq_click )Count++;
       else Count = 0;
       Ms2    = ms;
       if( TM_long_click == 0 && TM_auto_click == 0 && Count == 0 )return SB_CLICK;
   }

// Фиксируем длинное нажатие кнопки   
   if( pin_state == LOW && !Long_press_state && TM_long_click>0 && ( ms - Ms2 )>TM_long_click ){
      uint16_t dt      = ms - Ms2;
      Long_press_state = true;
#ifdef DEBUG_SERIAL      
      DebugSerial.print(">>>Event button, pin=");
      DebugSerial.print(Pin);
      DebugSerial.print(",long press, tm=");
      DebugSerial.print(dt);
      DebugSerial.println(" ms");
#endif 
      return SB_LONG_CLICK;
   }

// Фиксируем авто нажатие кнопки   
   if( pin_state == LOW && TM_auto_click > 0 
       && ( ms - Ms2 ) > TM_auto_click 
       && ( ms - Ms_auto_click ) > Period_auto_click ){
      uint16_t dt      = ms - Ms2;
      Auto_press_state = true;
      Ms_auto_click    = ms;
#ifdef DEBUG_SERIAL      
      DebugSerial.print(">>>Event button, pin=");
      DebugSerial.print(Pin);
      DebugSerial.print(",auto press, tm=");
      DebugSerial.print(dt);
      DebugSerial.println(" ms");
#endif 
      return SB_AUTO_CLICK;
   }

   
// Фиксируем отпускание кнопки 
   if( pin_state == HIGH && State == true  && (ms-Ms2)>TM_bounce ){
       uint16_t dt      = ms - Ms2;
//       if( (ms - Ms1) < TM_bounce )return SB_NONE;
// Сбрасываем все флаги       
       State            = false;
#ifdef DEBUG_SERIAL      
       DebugSerial.print(">>>Event button, pin=");
       DebugSerial.print(Pin);
       DebugSerial.print(",press OFF, tm=");
       DebugSerial.print(dt);
       DebugSerial.println(" ms");
#endif 
      Ms1    = ms;
      Time   = ms - Ms2;
 // Возвращаем короткий клик  
      if( (TM_long_click != 0 || TM_auto_click != 0) && !Long_press_state && !Auto_press_state )return SB_CLICK;
       
   }

   if( Time <20000 )Time = ms - Ms2;

   return SB_NONE;
}     

