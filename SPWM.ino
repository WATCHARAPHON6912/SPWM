#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

#define PWM_OUT_H 11  // CCR2A H output (PB3)
#define PWM_OUT_L 3   // CCR2B L output (PD3)
#define EnablePin 6   // Power Driver (PD6)
#define Sink2 10      //1H  (PB2)
#define Sink1 9       //1L  (PB1)

#define MAX_AMPLITUDE 252
#define MIN_AMPLITUDE 50

volatile float freq;
//refclk=15686;  // =8Mhz 8000000Hz / 510 = 15686
const float refclk = 31372.54;
volatile unsigned long sigma;  // phase accumulator
volatile unsigned long delta;  // phase increment
volatile unsigned int c4ms;    // counter incremented every 4ms
byte Table_Buffer[128];
byte phase1, temp_ocr, ocr_outA, ocr_outB, tlavel, AC_lavel, temp, Volts, Volts_Show, Old_NVolts;
unsigned int V_Read, NVolts;
bool T_status;
bool EN_status = true;
//*****************************************************************
void (*resetFunc)(void) = 0;  //declare reset function at address 0
void setup() {
  Serial.begin(115200);
  pinMode(EnablePin, OUTPUT);
  pinMode(Sink1, OUTPUT);
  pinMode(Sink2, OUTPUT);
  pinMode(PWM_OUT_L, OUTPUT);
  pinMode(PWM_OUT_H, OUTPUT);

  OCR1A = 0;  // PWM_OUT_H
  OCR1B = 0;  // PWM_OUT_L
  OCR2A = 0;  // PWM_OUT_H
  OCR2B = 0;  // PWM_OUT_L
  T_status = false;
  c4ms = 0;
  digitalWrite(PWM_OUT_L, LOW);
  digitalWrite(PWM_OUT_H, LOW);
  digitalWrite(EnablePin, LOW);
  cbi(TIMSK2, TOIE2);
  setup_timer2();
  delay(2000);
  Old_NVolts = 0;
  freq = 25.5;
  LoadModulateBuffer(MIN_AMPLITUDE);
  delta = pow(2, 32) * freq / refclk;
  sbi(TIMSK2, TOIE2);
}
int i = 0;
void loop() {
  i++;
  if(i>=252)i=252;
  LoadModulateBuffer(i);  //100-300
  Serial.println(i);
  delay(50);
}
void setup_timer2(void) {
  sbi(TCCR2B, CS20);
  cbi(TCCR2B, CS21);
  cbi(TCCR2B, CS22);

  cbi(TCCR2A, COM2B0);
  sbi(TCCR2A, COM2B1);
  sbi(TCCR2A, COM2A0);
  sbi(TCCR2A, COM2A1);
  sbi(TCCR2A, WGM20);
  cbi(TCCR2A, WGM21);
  cbi(TCCR2B, WGM22);
}
ISR(TIMER2_OVF_vect) {
  sigma = sigma + delta;
  phase1 = sigma >> 25;

  temp_ocr = Table_Buffer[phase1];
  ocr_outA = temp_ocr;
  ocr_outB = temp_ocr;
  if (temp_ocr >> 0) ocr_outA = temp_ocr + 3;
  OCR2A = ocr_outA;  // pin D6
  OCR2B = ocr_outB;  // pin D5
  switch (phase1) {
    case 0:
      cbi(PORTB, 1);  //digitalWrite(Sink1, LOW);
      break;
    case 1:
      sbi(PORTB, 2);  //digitalWrite(Sink2, HIGH);
      break;
    case 65:
      if (EN_status) {
        digitalWrite(EnablePin, HIGH);  // ON Power Drive
        EN_status = false;
      }
      cbi(PORTB, 2);  //digitalWrite(Sink2, LOW);
      break;
    case 66:
      sbi(PORTB, 1);  //digitalWrite(Sink1, HIGH);
      break;
  }
}
//--------------------------------------------------------------------------------------------------------------------
void LoadModulateBuffer(byte Amplitude) {
  byte tmp, cout;
  double angle;
  if (Amplitude <= MIN_AMPLITUDE) Amplitude = MIN_AMPLITUDE;
  if (Amplitude >= MAX_AMPLITUDE) Amplitude = MAX_AMPLITUDE;
  for (cout = 0; cout < 128; cout++) {
    angle = cout * M_PI / 64;
    Table_Buffer[cout] = Amplitude * sin(angle);
  }
}