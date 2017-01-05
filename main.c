 /***********************************************************
  *   Program:    io manager
  *   Created:    oct 2006
  *   Author:     Ruediger Nahc Mohr
  *   Comments:   yeeeee ha!
  *   Update:     Nov 25 2006  checkalarms now snapshots to fix
  *                              missed state changes
  *               Aug 4  2010  added pwm stuffs
  *               Jan 13 2013  fixed pwm stuffs to it works.
  *
  ************************************************************/
#include "main.h"
#include "version.h"

void printHelp( void );
void inline printDigit(unsigned char n);
void printNumber8(unsigned char n) ;
void printNumber16(unsigned int n) ;
int  Analog (int n);
int  ExtractPortNum (char * args);
int  GetBit(unsigned char port) ;
void checkAlarms( void );
int  ExtractDir (char arg);
int  SetDir(unsigned char port, unsigned char value) ;
int  GetInput(unsigned char port) ;
char charValue(char arg);
unsigned int  ExtractValue (char * arg);
int  SetPort(unsigned char port, unsigned int value) ;
int  GetPort(unsigned char port) ;
int  SetAlarm(unsigned char port, unsigned char value) ;
void PWM_Start(unsigned char channel);
void PWM_Stop(unsigned char channel);
void PWM_Init( void ) ;

#define SetBit(BIT, PORT)     PORT |= (1<<BIT)
#define ClearBit(BIT, PORT)   PORT &= ~(1<<BIT)
#define WriteBit(BIT, PORT, VALUE)  PORT = (PORT & ~(1<<BIT))|((VALUE&1)<<BIT)

#define IsHigh(BIT, PORT)    (PORT & (1<<BIT)) != 0
#define IsLow(BIT, PORT)     (PORT & (1<<BIT)) == 0

#define IsDigit(C)  (((C)>='0') && ((C)<='9'))

 unsigned char oPINA, oPINB, oPINC, oPIND;     // old values of port bits for change detection
 unsigned char aPORTA, aPORTB, aPORTC, aPORTD; // alarm masks for port bits
 unsigned char tPORTA;                         // data type for port a, set = analog
 
 unsigned char portbits[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 
    0, 1, 2, 3, 4, 5, 6, 7, 
    7, 6, 5, 4, 3, 2, 1, 0, 
    7, 2, 3, 4, 5, 6
 };
 

int main(void) {

  char data;       // raw character
  
  char command[9]; // command buffer
  int port, value;
  unsigned char bcount;     // command byte counter

  // Activate ADC with Prescaler 
  ADCSRA =  1 << ADEN  |
            0 << ADSC  |
            0 << ADATE |
            0 << ADIF  |
            0 << ADIE  |
            1 << ADPS2 |
            0 << ADPS1 |
            0 << ADPS0 ;
  
   DDRA   = 0x00;   DDRB   = 0x00;  DDRC   = 0x00;  DDRD   = 0x00;  // all ports input 
   PORTA  = 0x00;   PORTB  = 0x00;  PORTC  = 0x00;  PORTD  = 0x00;  // no pullups 
   aPORTA = 0x00;   aPORTB = 0x00;  aPORTC = 0x00;  aPORTD = 0x00;  // no alarms      
   tPORTA = 0x00;  // all digital        
                                    
  //USART_Init( 12 ); //4800 @ 1Mhz
  //USART_Init( 207); // 4800 @ 16Mhz
  USART_Init( 103 ); // 9600 @ 16Mhz

  PWM_Init();        // pre-setup pwm. 8 bit, clk/512, io pin left in normal mode
  USART_printstring("READY.\n\r");
  bcount = 0;
	
    while(1) {
      while ( !(UCSRA & (1<<RXC)) ) {   checkAlarms();    } 	
      
      data = USART_Receive();
     // command[bcount++] = data;
      
      if (data == 0x0D) {  // \r       
       if (command[0] == '?') {
           printHelp();       
       } else  if ((port = ExtractPortNum(&command[1])) != (-1)) {
          command[bcount++] = data; // terminate string.
          switch (command[0]) {
          // set the direction of a port
          case 'D':
          case 'd':
            if ((value = ExtractDir(command[3])) != (-1)) {
              if (value == 2) { // request for analog
                if ((port > 7) & (port < 16)) {
                  SetDir(port, 0);   // make port an input
                  SetBit((port-8), tPORTA);
                } else {
                  USART_printstring("ERR PORT NOT ANA.\n\r");
                }
              } else if (value == 3){ // request for pwm               
                  if (0) {
                  } else if (port == 3) {  SetDir(port, 1); PWM_Start(0);
                  } else if (port == 24){  SetDir(port, 1); PWM_Start(1);
                  } else if (port == 27){  SetDir(port, 1); PWM_Start(2);
                  } else if (port == 28){  SetDir(port, 1); PWM_Start(3);      
                  } else {                 USART_printstring("ERR PORT NOT PWM.\n\r");
                  }
              } else { // std in or out request
                  if (0) {
                  } else if (port == 3) {  PWM_Stop(0);
                  } else if (port == 24){  PWM_Stop(1);
                  } else if (port == 27){  PWM_Stop(2);
                  } else if (port == 28){  PWM_Stop(3);      
                  }
                SetDir(port, value);
              }
            } else {
              USART_printstring("ERR MAULED DIR.\n\r");
            }
            break;
            
          // Get an input state
          case 'I':
          case 'i':  GetInput(port);   break;
          
          // Set an output state
          case 'O':
          case 'o':            
            if ((value = ExtractValue (&command[3])) != (-1)) {
              SetPort(port, value );
            } else {
              USART_printstring("ERR MAULED VAL.\n\r");
            }
            break;
          // Configure an input alarm
          case 'A':
          case 'a':
            if ((value = ExtractValue (command[3])) != (-1)) {
              SetAlarm(port, value & 1 );
            } else {
              USART_printstring("ERR MAULED VAL.\n\r");
            }            
            break;
        }
              
        USART_printstring("\n\r");		
        } else {      
           USART_printstring("ERR MAULED PORTNUM. ");
           printNumber8(port);
           USART_printstring("\n\r");
        }
	
	bcount = 0;
	
      } else {
        command[bcount++] = data;
        if (bcount > 8) bcount = 8;
      } // end of if /r
      
      
    } // end of while

}

void printHelp( void ) {

  USART_printstring("IOmon version ");
  USART_printstring(version);
  USART_printstring("\n\r");
  USART_printstring("pp port number, 00-29\n\r");
  USART_printstring("d  direction, I (in) O (out) A (analog in) P (pwm out)\n\r");
  USART_printstring("v  Value, 1 (on) 0 (off)\n\r");
  USART_printstring("e  Event, 1 (rising edge) 0 (falling edge) A (any edge) D (disable)\n\r");
  USART_printstring("\n\rDppd Set direction of port pp to d. D16O set port 16 to be an output \n\r");
  USART_printstring("Ipp  Get value of port pp. I04 get value for port 4\n\r");
  USART_printstring("Oppv Set value of output port pp to v. O161 set port 16 high \n\r");
  USART_printstring("Appe Set alarm on port pp for event type e. \n\r");
  USART_printstring("   A051 Enable rising edge alarm for port 5. \n\r");
  USART_printstring("\n\rDefaults are all ports digital input, output values to 0, no alarms. \n\r");    

}

void checkAlarms( void ){

  unsigned char tPINA, tPINB, tPINC, tPIND;
  
  tPINA = PINA;  // snapshot the ports
  if(aPORTA != 0) {
    if ((tPINA^oPINA) != 0) {
      if (aPORTA & 0x01) if ((tPINA^oPINA) & 0x01) { GetInput(8);  USART_printstring("\n\r"); } 
      if (aPORTA & 0x02) if ((tPINA^oPINA) & 0x02) { GetInput(9);  USART_printstring("\n\r"); }
      if (aPORTA & 0x04) if ((tPINA^oPINA) & 0x04) { GetInput(10); USART_printstring("\n\r"); }
      if (aPORTA & 0x08) if ((tPINA^oPINA) & 0x08) { GetInput(11); USART_printstring("\n\r"); }
      if (aPORTA & 0x10) if ((tPINA^oPINA) & 0x10) { GetInput(12); USART_printstring("\n\r"); }
      if (aPORTA & 0x20) if ((tPINA^oPINA) & 0x20) { GetInput(13); USART_printstring("\n\r"); }
      if (aPORTA & 0x40) if ((tPINA^oPINA) & 0x40) { GetInput(14); USART_printstring("\n\r"); }
      if (aPORTA & 0x80) if ((tPINA^oPINA) & 0x80) { GetInput(15); USART_printstring("\n\r"); }   
    }
  } 
  oPINA = tPINA;
  
  tPINB = PINB;
  if(aPORTB != 0) {
      if ((tPINB^oPINB) != 0) {
      if (aPORTB & 0x01) if ((tPINB^oPINB) & 0x01) { GetInput(0); USART_printstring("\n\r"); }
      if (aPORTB & 0x02) if ((tPINB^oPINB) & 0x02) { GetInput(1); USART_printstring("\n\r"); }
      if (aPORTB & 0x04) if ((tPINB^oPINB) & 0x04) { GetInput(2); USART_printstring("\n\r"); }
      if (aPORTB & 0x08) if ((tPINB^oPINB) & 0x08) { GetInput(3); USART_printstring("\n\r"); }
      if (aPORTB & 0x10) if ((tPINB^oPINB) & 0x10) { GetInput(4); USART_printstring("\n\r"); }
      if (aPORTB & 0x20) if ((tPINB^oPINB) & 0x20) { GetInput(5); USART_printstring("\n\r"); }
      if (aPORTB & 0x40) if ((tPINB^oPINB) & 0x40) { GetInput(6); USART_printstring("\n\r"); }
      if (aPORTB & 0x80) if ((tPINB^oPINB) & 0x80) { GetInput(7); USART_printstring("\n\r"); }
    }
  } 
  oPINB = tPINB;
  
  tPINC = PINC;
  if(aPORTC != 0) {
      if ((tPINC^oPINC) != 0) {
      if (aPORTC & 0x01) if ((tPINC^oPINC) & 0x01) { GetInput(23); USART_printstring("\n\r"); }
      if (aPORTC & 0x02) if ((tPINC^oPINC) & 0x02) { GetInput(22); USART_printstring("\n\r"); }
      if (aPORTC & 0x04) if ((tPINC^oPINC) & 0x04) { GetInput(21); USART_printstring("\n\r"); }
      if (aPORTC & 0x08) if ((tPINC^oPINC) & 0x08) { GetInput(20); USART_printstring("\n\r"); }
      if (aPORTC & 0x10) if ((tPINC^oPINC) & 0x10) { GetInput(19); USART_printstring("\n\r"); }
      if (aPORTC & 0x20) if ((tPINC^oPINC) & 0x20) { GetInput(18); USART_printstring("\n\r"); }
      if (aPORTC & 0x40) if ((tPINC^oPINC) & 0x40) { GetInput(17); USART_printstring("\n\r"); }
      if (aPORTC & 0x80) if ((tPINC^oPINC) & 0x80) { GetInput(16); USART_printstring("\n\r"); }
    }
  } 
  oPINC = tPINC;
  
  tPIND = PIND;
  if(aPORTD != 0) {
      if ((tPIND^oPIND) != 0) {
      if (aPORTD & 0x04) if ((tPIND^oPIND) & 0x04) { GetInput(25); USART_printstring("\n\r"); }
      if (aPORTD & 0x08) if ((tPIND^oPIND) & 0x08) { GetInput(26); USART_printstring("\n\r"); }
      if (aPORTD & 0x10) if ((tPIND^oPIND) & 0x10) { GetInput(27); USART_printstring("\n\r"); }
      if (aPORTD & 0x20) if ((tPIND^oPIND) & 0x20) { GetInput(28); USART_printstring("\n\r"); }
      if (aPORTD & 0x40) if ((tPIND^oPIND) & 0x40) { GetInput(29); USART_printstring("\n\r"); }
      if (aPORTD & 0x80) if ((tPIND^oPIND) & 0x80) { GetInput(24); USART_printstring("\n\r"); }
    }
  } 
  oPIND = tPIND;
  
}

void inline printDigit(unsigned char n) {
  USART_Transmit( (n & 0x0F) | 0x30 );
}

void printNumber8(unsigned char n) {
  unsigned char d;
  d = n/100;
  printDigit(d);
  n -= d * 100;
  
  d = n/10;
  printDigit(d);
  n -= d * 10;
  
  printDigit(n);
}

void printNumber16(unsigned int n) {
  unsigned int d;
  
  d = n/10000;
  printDigit(d);
  n -= d * 10000;
  
  d = n/1000;
  printDigit(d);
  n -= d * 1000;
  
  d = n/100;
  printDigit(d);
  n -= d * 100;
  
  d = n/10;
  printDigit(d);
  n -= d * 10;
  
  printDigit(n);
}

int Analog (int n) {

    // Select pin ADC0 using MUX
    ADMUX = n & 7;
    
    //Start conversion
    ADCSRA |= _BV(ADSC);
    
    // wait until converstion completed
    while (ADCSRA & _BV(ADSC) );
    
    // get converted value
    return ADC;  
}

int ExtractPortNum (char * args) {

//  if (((args[0] >= '0') & (args[0] <= '9')) & ((args[1] >= '0') & (args[1] <= '9'))) {
  if (IsDigit(args[0]) && IsDigit(args[1])) {
    return (((args[0] - '0') * 10) + (args[1] - '0')); 
  } else {
    return -1;
  }
  
}

int ExtractDir (char arg) {
  switch(arg) {
    case 'I':
    case 'i':
      return 0;
    case 'O':
    case 'o':
      return 1;
    case 'A':
    case 'a':
      return 2;
    case 'P':
    case 'p':
      return 3;
    default:
      return -1;
  }

}
char charValue(char arg){

  if ((arg >= '0') & (arg <= '9')) {
    return (arg - '0'); 
  } else {
    return -1;
  }

}


unsigned int ExtractValue (char * arg) {
  
  char d;
  unsigned int rv;
  char  * p;
  
  p = arg;
  rv = 0;

  while((d = charValue(*p)) != (-1)) {
    rv = rv * 10;
    rv = rv + d;
    p++;
  }
  
  return rv;
  
}

int GetInput(unsigned char port) {
  
  printNumber8(port);
  USART_Transmit( ',' );
  printNumber16(GetPort(port));
  
  //printNumber16(Analog(0));  
  return 0;
}


int GetPort(unsigned char port) {

  switch(port) {
  
   case 0:
      return IsHigh(0, PINB);
   case 1:
      return IsHigh(1, PINB);
   case 2:
      return IsHigh(2, PINB);
   case 3:
      return IsHigh(3, PINB);
   case 4:
      return IsHigh(4, PINB);
   case 5:
      return IsHigh(5, PINB);
   case 6:
      return IsHigh(6, PINB);
   case 7:
      return IsHigh(7, PINB);
   case 8:
       if (IsHigh(0, tPORTA)) {
         return Analog(0);
       } else {
         return IsHigh(0, PINA); // *
       }
   case 9:
      if (IsHigh(1, tPORTA)) {
         return Analog(1);
      } else {
         return IsHigh(1, PINA); // *
      }
   case 10:
      if (IsHigh(2, tPORTA)) {
        return Analog(2);
      } else {
        return IsHigh(2, PINA); // *
      }
   case 11:
      if (IsHigh(3, tPORTA)) {
         return Analog(3);
      } else {
         return IsHigh(3, PINA); // *
      }
   case 12:
      if (IsHigh(4, tPORTA)) {
         return Analog(4);
      } else {
         return IsHigh(4, PINA); // *
      }
   case 13:
     if (IsHigh(5, tPORTA)) {
        return Analog(5);
     } else {
        return IsHigh(5, PINA); // *
     }
   case 14:
     if (IsHigh(6, tPORTA)) {
       return Analog(6);
     } else {
       return IsHigh(6, PINA); // *
     }
   case 15:
     if (IsHigh(7, tPORTA)) {
       return Analog(7);
     } else {
       return IsHigh(7, PINA); // *
     }
   case 16:
      return IsHigh(7, PINC);
   case 17:
      return IsHigh(6, PINC);
   case 18:
      return IsHigh(5, PINC);
   case 19:
      return IsHigh(4, PINC);
   case 20:
      return IsHigh(3, PINC);
   case 21:
      return IsHigh(2, PINC);
   case 22:
      return IsHigh(1, PINC);
   case 23:
      return IsHigh(0, PINC);
   case 24:
      return IsHigh(7, PIND);
   case 25:
      return IsHigh(2, PIND);
   case 26:
      return IsHigh(3, PIND);
   case 27:
      return IsHigh(4, PIND);
   case 28:
      return IsHigh(5, PIND);
   case 29:
      return IsHigh(6, PIND);
   
  }
  return 0;
}


int SetPort(unsigned char port, unsigned int value) {

  switch(port) {
  
   case 0:  return WriteBit(0, PORTB, value);
   case 1:  return WriteBit(1, PORTB, value);
   case 2:  return WriteBit(2, PORTB, value);
   case 3:  OCR0 = value; 
            return WriteBit(3, PORTB, value);  // pwm
   case 4:  return WriteBit(4, PORTB, value);
   case 5:  return WriteBit(5, PORTB, value);
   case 6:  return WriteBit(6, PORTB, value);
   case 7:  return WriteBit(7, PORTB, value);
   case 8:  return WriteBit(0, PORTA, value);
   case 9:  return WriteBit(1, PORTA, value);
   case 10: return WriteBit(2, PORTA, value);
   case 11: return WriteBit(3, PORTA, value);
   case 12: return WriteBit(4, PORTA, value);
   case 13: return WriteBit(5, PORTA, value);
   case 14: return WriteBit(6, PORTA, value);
   case 15: return WriteBit(7, PORTA, value);
   case 16: return WriteBit(7, PORTC, value);
   case 17: return WriteBit(6, PORTC, value);
   case 18: return WriteBit(5, PORTC, value);
   case 19: return WriteBit(4, PORTC, value);
   case 20: return WriteBit(3, PORTC, value);
   case 21: return WriteBit(2, PORTC, value);
   case 22: return WriteBit(1, PORTC, value);
   case 23: return WriteBit(0, PORTC, value);
   case 24: OCR2 = value;
            return WriteBit(7, PORTD, value); // pwm
   case 25: return WriteBit(2, PORTD, value);
   case 26: return WriteBit(3, PORTD, value);
   case 27: OCR1B = value;
            return WriteBit(4, PORTD, value); // pwm
   case 28: OCR1A  = value;
            return WriteBit(5, PORTD, value); // pwm
   case 29: return WriteBit(6, PORTD, value);   
  }
  return 0;
}


int SetDir(unsigned char port, unsigned char value) {

  switch(port) {
  
   case 0:
      return WriteBit(0, DDRB, value);
   case 1:
      return WriteBit(1, DDRB, value);
   case 2:
      return WriteBit(2, DDRB, value);
   case 3:
      return WriteBit(3, DDRB, value);
   case 4:
      return WriteBit(4, DDRB, value);
   case 5:
      return WriteBit(5, DDRB, value);
   case 6:
      return WriteBit(6, DDRB, value);
   case 7:
      return WriteBit(7, DDRB, value);
   case 8:
      ClearBit(0, tPORTA);
      return WriteBit(0, DDRA, value);
   case 9:
      ClearBit(1, tPORTA);
      return WriteBit(1, DDRA, value);
   case 10:
      ClearBit(2, tPORTA);
      return WriteBit(2, DDRA, value);
   case 11:
      ClearBit(3, tPORTA);
      return WriteBit(3, DDRA, value);
   case 12:
      ClearBit(4, tPORTA);
      return WriteBit(4, DDRA, value);
   case 13:
      ClearBit(5, tPORTA);
      return WriteBit(5, DDRA, value);
   case 14:
      ClearBit(6, tPORTA);
      return WriteBit(6, DDRA, value);
   case 15:
      ClearBit(7, tPORTA);
      return WriteBit(7, DDRA, value);
   case 16:
      return WriteBit(7, DDRC, value);
   case 17:
      return WriteBit(6, DDRC, value);
   case 18:
      return WriteBit(5, DDRC, value);
   case 19:
      return WriteBit(4, DDRC, value);
   case 20:
      return WriteBit(3, DDRC, value);
   case 21:
      return WriteBit(2, DDRC, value);
   case 22:
      return WriteBit(1, DDRC, value);
   case 23:
      return WriteBit(0, DDRC, value);
   case 24:
      return WriteBit(7, DDRD, value);
   case 25:
      return WriteBit(2, DDRD, value);
   case 26:
      return WriteBit(3, DDRD, value);
   case 27:
      return WriteBit(4, DDRD, value);
   case 28:
      return WriteBit(5, DDRD, value);
   case 29:
      return WriteBit(6, DDRD, value);
   
  }
  return 0;
}

int SetAlarm(unsigned char port, unsigned char value) {

  switch(port) {
   case 0:    return WriteBit(0, aPORTB, value);
   case 1:    return WriteBit(1, aPORTB, value);
   case 2:    return WriteBit(2, aPORTB, value);
   case 3:    return WriteBit(3, aPORTB, value); // pwm
   case 4:    return WriteBit(4, aPORTB, value);
   case 5:    return WriteBit(5, aPORTB, value);
   case 6:    return WriteBit(6, aPORTB, value);
   case 7:    return WriteBit(7, aPORTB, value);
   case 8:    return WriteBit(0, aPORTA, value);
   case 9:    return WriteBit(1, aPORTA, value);
   case 10:   return WriteBit(2, aPORTA, value);
   case 11:   return WriteBit(3, aPORTA, value);
   case 12:   return WriteBit(4, aPORTA, value);
   case 13:   return WriteBit(5, aPORTA, value);
   case 14:   return WriteBit(6, aPORTA, value);
   case 15:   return WriteBit(7, aPORTA, value);
   case 16:   return WriteBit(7, aPORTC, value);
   case 17:   return WriteBit(6, aPORTC, value);
   case 18:   return WriteBit(5, aPORTC, value);
   case 19:   return WriteBit(4, aPORTC, value);
   case 20:   return WriteBit(3, aPORTC, value);
   case 21:   return WriteBit(2, aPORTC, value);
   case 22:   return WriteBit(1, aPORTC, value);
   case 23:   return WriteBit(0, aPORTC, value);
   case 24:   return WriteBit(7, aPORTD, value); // pwm
   case 25:   return WriteBit(2, aPORTD, value);
   case 26:   return WriteBit(3, aPORTD, value);
   case 27:   return WriteBit(4, aPORTD, value); // pwm
   case 28:   return WriteBit(5, aPORTD, value); // pwm
   case 29:   return WriteBit(6, aPORTD, value);   
  }
  return 0;
}

void PWM_Init( void )  {  // pre-setup pwm. 8 bit, clk/512, io pin left in normal mode
  // clear pwm levels
  OCR0  = 0; 
  OCR1A = 0;
  OCR1B = 0;
  OCR2  = 0;
  
  // set up WGM, clock, and mode for timer 0
  TCCR0 = 0 << FOC0  | 
          1 << WGM00 | /* fast pwm */
          0 << COM01 | /* off */
          0 << COM00 |
          1 << WGM01 | /* fast pwm */
          1 << CS02  | /* clk/256 */
          0 << CS01  |
          0 << CS00  ;
  
  TCCR1A = 0 << COM1A1 | /* OFF */
           0 << COM1A0 |
           0 << COM1B1 | /* OFF */
           0 << COM1B0 |
           0 << FOC1A  | /* no */
           0 << FOC1B  | /* no */
           0 << WGM11  | /* 8 bit fast */
           1 << WGM10  ;
  
  TCCR1B = 0 << ICNC1  | /* no noise cancel */
           0 << ICES1  | /* none of whatever that is */
           0 << WGM13  | /* 8 bit fast pwm */
           1 << WGM12  |
           1 << CS12   | /* clk/256 */
           0 << CS11   |
           0 << CS10   ;
  
  // set up WGM, clock, and mode for timer 2
  TCCR2 = 0 << FOC2  | 
          1 << WGM20 |
          0 << COM21 | /* off */
          0 << COM20 |
          1 << WGM21 |
          1 << CS22  | /* clk/256 */
          1 << CS21  |
          0 << CS20  ;
}

void PWM_Start(unsigned char channel) { // channels  0: B3, 1: D4, 2: D5, 3: D7
  if (0) {
  } else if (channel == 0) {  // B3, OC0
    TCCR0  = TCCR0  | ( 1 << COM01  | 0 << COM00 ) ;
  } else if (channel == 2) {  // D4, OC1B
    TCCR1A = TCCR1A | ( 1 << COM1B1 | 0 << COM1B0 ) ;
  } else if (channel == 3) {  // D5, OC1A
    TCCR1A = TCCR1A | ( 1 << COM1A1 | 0 << COM1A0 ) ;
  } else if (channel == 1) {  // D7, OC2
    TCCR2 = TCCR2   | ( 1 << COM21  | 0 << COM20 ) ;
  }
}

void PWM_Stop(unsigned char channel) {
  if (0) {
  } else if (channel == 0) {  // B3, OC0
    TCCR0  = TCCR0  &  ~( 1 << COM01  | 1 << COM00 ) ;
  } else if (channel == 2) {  // D4, OC1B
    TCCR1A = TCCR1A &  ~( 1 << COM1B1 | 1 << COM1B0 ) ;
  } else if (channel == 3) {  // D5, OC1A
    TCCR1A = TCCR1A &  ~( 1 << COM1A1 | 1 << COM1A0 ) ;
  } else if (channel == 1) {  // D7, OC2
    TCCR2 = TCCR2   &  ~( 1 << COM21  | 1 << COM20 ) ;
  }
}
