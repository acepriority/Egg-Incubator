//TMP36 records temperatures from -40 to 125 degrees celcius
//Low value reads at 20 and high value reads at 358

#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
volatile uint8_t days = 1;
volatile uint8_t rotations = 0;
volatile bool interrupt_occured = false;
bool firstTime = true;
bool LCDfirstTime = true;
uint16_t temp = 0;
uint16_t humid = 0;
uint8_t theLow;
uint16_t theTenBitResult;
int degrees_celcuis;
short percentage_humidity;
volatile static uint8_t adc_convert_done = 1;

void Alarm(){
  int cycles = 400;
  int counter = 0;
  while(counter < cycles){
    int i;
    PORTB |= 0x01;
    for(i=0; i<512; i++);
    PORTB &= ~(0x01);
    for(i=0; i<512; i++);
    counter++;
  }
}
void FirstTime(){
  PORTB |= 0x20;
  CustomDelay(250);
  PORTB &= ~(0x20);
  firstTime = false;
  rotations++;
}
void SecondTime(){
  PORTB |= 0x10;
  CustomDelay(250);
  PORTB &= ~(0x10);
  firstTime = true;
  rotations++;
}

void Rotations(){
  if(rotations < 5){
     Rotator();
     return;
  }
  days++;
  rotations = 0;
}

void Rotator(){
  if(firstTime){
    FirstTime();
    return;
  }
  else {
    SecondTime();
    return;
  }
}

void LCD_Display(){
  if(LCDfirstTime){
    lcd.setCursor(1, 0);         
    lcd.print("Days ");
    lcd.print(days);
    CustomDelay(200);                 
    lcd.clear();
    LCDfirstTime = !LCDfirstTime;
    return;
  }else{
    lcd.setCursor(2, 0);
    lcd.print("Temp: ");
  	lcd.print(degrees_celcuis);  
  	lcd.print(" C");
    lcd.setCursor(2, 1);     
  	lcd.print("hum: "); 
    lcd.print(percentage_humidity); 
    lcd.print(" %");
  	CustomDelay(200); 
  	lcd.clear();
    LCDfirstTime = !LCDfirstTime;
    return;
  }
}

void CustomDelay(uint32_t mSecondsApx)
{
  volatile uint32_t long i;
  uint32_t endTime = 1000 * mSecondsApx;
  for (i = 0; i < endTime; i++);
}

ISR(TIMER1_COMPA_vect){
  interrupt_occured = true;
}

ISR(ADC_vect){
  adc_convert_done = 1;
}

ISR(PCINT2_vect){
  //Runs when pin change interrupt 2 occurs
  days = 1;
  rotations = 0;
}
void setup()
{
  //The setup code
  //inputs
  PORTD |= 0x10; //enable internal pull up resistor on pin 4
  ADMUX = 0x40; //select A0 and AVcc voltage reference selection
  ADCSRA = 0x8F; //enable ADC, ADC interrrupts and 128 prescaler
  DIDR0 = 0x03; //enable ADC 0 and 1
  //outputs
  DDRD |= 0x0F; //set only the RGB LED and Piezo as output
  DDRB |= 0x3D;
  //Pin change interrupt initialization
  PCICR = 0x04; //enable PCINT[23:16] pin interrupts
  PCMSK2 = 0x10; //enable pin change interrupt on pin 4
  //Timer 1 initialization 
  TCCR1A = 0;
  TCCR1B = 0x0D; // CTC mode with prescaler 1024 
  OCR1A = 31249; //interrupt period 2s
  TIMSK1 = 0x02; //enable interrupt on compare match 1
  sei(); //enable global interrupts
  //LCD initialization
  lcd.init(); 
  lcd.backlight();
}

void loop()
{
  //the loop
  LCD_Display();
  adc_convert_done = 0;
  ADCSRA |= 0x40; //To start the conversion
  while(adc_convert_done == 0);
  theLow = ADCL;
  theTenBitResult = ADCH<<8|theLow;

   switch(ADMUX){
     case 0x40:
         temp = theTenBitResult;
         ADMUX = 0x41;
         break;
     case 0x41:
         humid = theTenBitResult;
         ADMUX = 0x40;
         break;
     default:
         break;
    }

    /* To perform some magic on the temp in volts
    in order to display it in degrees Celcius */
    degrees_celcuis = ((temp*(5.0/1024.0))-0.5)/0.01;
    
    /* To perform some magic on the humidity in volts
    in order to display it as a percentage */
    percentage_humidity = (humid*100)/1024;
    
    //Temperature variation logic
    switch(degrees_celcuis){
      case -40 ... 35:
      	PORTD &= ~(0x05); //clear red and green
      	PORTD |= 0x0A; // set blue and turn on heater
      	PORTB &= ~(0x04); //turn off the Fan
      	break;
      case 36 ... 40:
      	PORTD &= ~(0x0E); //clear red and blue and turn off heater
      	PORTD |= 0x01; //set green
      	PORTB &= ~(0x04); //turn off the Fan
      	break;
      case 41 ... 125:
      	PORTD &= ~(0x0B); //clear blue and green and turn off heater
      	PORTD |= 0x04; //set red
      	PORTB |= 0x04; //turn on the Fan
      	break;
      default:
      	//pass
      	break;
    }
    if(interrupt_occured){
      Rotations();
      interrupt_occured = false;
    }
    if(days >= 21){
      Alarm();
    }
}