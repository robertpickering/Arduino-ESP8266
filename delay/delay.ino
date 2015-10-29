void setup() {
  // put your setup code here, to run once:
 pinMode(BUILTIN_LED, OUTPUT);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  blinking( 10, 250 );
  delay(5000);

}

void blinking( int times, int blinktime ) {
  int i;
  int led= 0; // start off
  
  for (i=0;i<times*2;i++) // loop twice to blink 
  {
   digitalWrite(BUILTIN_LED,led);
   led=led^1; // swap state
   delay(blinktime);
  }
}

