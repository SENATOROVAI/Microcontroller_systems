const int BTN_SEND = 2;
const int LED_CTRL = 4;

const char CMD_OFF = '4';
const char CMD_ON  = '5';

bool btnPrev = false;

void setup() {
  pinMode(BTN_SEND, INPUT_PULLUP);
  pinMode(LED_CTRL, OUTPUT);
  digitalWrite(LED_CTRL, LOW);

  Serial.begin(9600);
  delay(500);
  Serial.println("Lab3 start");
  Serial.println("Press button to send packet");
  Serial.println("Send '5' to turn LED ON, '4' to turn LED OFF");
}

void sendPacketIRQStyle(const char *s) {
  while (*s) {
    Serial.write(*s++);
    delay(1);
  }
}

void loop() {
  bool btnNow = (digitalRead(BTN_SEND) == LOW);

  if (btnNow && !btnPrev) {
    sendPacketIRQStyle("USART IRQ packet\\r\\n");
  }
  btnPrev = btnNow;

  while (Serial.available() > 0) {
    char rx = (char)Serial.read();

    if (rx == CMD_ON) {
      digitalWrite(LED_CTRL, HIGH);
      Serial.println("LED ON");
    } else if (rx == CMD_OFF) {
      digitalWrite(LED_CTRL, LOW);
      Serial.println("LED OFF");
    }
  }
}
