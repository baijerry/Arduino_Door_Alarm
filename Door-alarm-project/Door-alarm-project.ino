#include <Servo.h> 

Servo myservo; // Servo object
const int iPin = 4; // Pin to control which change is made in interrupt1 subprocess
const int alarmPin = 5; // Pin of LED to show alarm state
const int alarmSoundPin = 6; // Pin of LED that blinks when the alarm goes off
const int digit1 = 10; // Button of alarm code digit 1
const int digit2 = 11; // Button of alarm code digit 2
const int digit3 = 12; // Button of alarm code digit 3
const int digit4 = 13; // Button of alarm code digit 4

int sPos = 0; // Servo position
volatile int doorState = HIGH; // doorState is HIGH when door is closed
volatile int lockState = HIGH; // lockState is HIGH when door is locked
volatile int alarmState = LOW; // Alarm is armed
volatile float time_count = 0; // Variable for time passed in custom millis
volatile long lastInterrupt1 = 0;
volatile long lastDoorInterrupt = 0;

void setup() {
  // Setting up the custom millis
  noInterrupts();           // disable all interrupts
  TCCR2B = 0;

  TCNT2 = 6;            // preload timer 256-16MHz/8/8000Hz
  TCCR2B |= (1 << CS21);    // 8 prescaler 
  TIMSK2 |= (1 << TOIE2);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts

  // Setting up the servo, input and output pins, interrupts, and serial monitor
  pinMode(iPin, INPUT);
  pinMode(alarmPin,OUTPUT);
  pinMode(alarmSoundPin,OUTPUT);
  pinMode(digit1, INPUT);
  pinMode(digit2, INPUT);
  pinMode(digit3, INPUT);
  pinMode(digit4, INPUT);

  // Custom interrupts
  attachInterrupt(0,interrupt1,RISING);
  attachInterrupt(1,interrupt2,FALLING);

  // Preparing serial monitor
  Serial.begin(9600);
}

ISR(TIMER2_OVF_vect)        // interrupt service routine for timer2 overflow
{
  TCNT2 = 6;
  time_count += 0.125;
}

void loop() {
  // Lock/Unlock logic
  if (lockState == HIGH && sPos == 0) {
    servoChange(lockState);
  } else if (lockState == LOW && sPos == 90) {
    servoChange(lockState);
  }

  // Trigger alarm if the alarm is armed and the door is open
  if (alarmState == HIGH && doorState == LOW) {
    alarmActivate();
  }
}


// Custom functions

// Function called when alarm is triggered (door opened while system is armed)
void alarmActivate() {
  Serial.println("Alarm sequence triggered");
  digitalWrite(alarmSoundPin, HIGH);
  long alarmStart = timeM();
  long timePassed = 0;
  char enteredDigits[11];
  unsigned long enteredTimes[11];
  int index = 0;
  
  while (timePassed < 10000) {
    timePassed = timeM() - alarmStart;
    if (digitalRead(digit1) == HIGH && enteredDigits[index - 1] != '1') {
      enteredDigits[index] = '1';
      enteredTimes[index] = timeM() - alarmStart;
      index++;
      Serial.println("Digit 1 entered");
    } else if (digitalRead(digit2) == HIGH && enteredDigits[index - 1] != '2') {
      enteredDigits[index] = '2';
      enteredTimes[index] = timeM() - alarmStart;
      index++;
      Serial.println("Digit 2 entered");
    } else if (digitalRead(digit3) == HIGH && enteredDigits[index - 1] != '3') {
      enteredDigits[index] = '3';
      enteredTimes[index] = timeM() - alarmStart;
      index++;
      Serial.println("Digit 3 entered");
    } else if (digitalRead(digit4) == HIGH && enteredDigits[index - 1] != '4') {
      enteredDigits[index] = '4';
      enteredTimes[index] = timeM() - alarmStart;
      index++;
      Serial.println("Digit 4 entered");
    }

    if (index == 10) {
      Serial.println("Too many digits entered.");
      break;
    }

    if ((index >= 3) && (enteredDigits[index-4] == '1') && (enteredDigits[index-3] == '2') && (enteredDigits[index-2] == '3') && (enteredDigits[index-1] == '4')) {
      if (alarmState == LOW) {
        double sTime = timePassed / 1000.0;
        Serial.print("Alarm disactivated after ");
        Serial.print(sTime);
        Serial.println(" seconds.");
        
        Serial.print("First correct digit entered after ");
        Serial.print(enteredTimes[index-4] / 1000.0);
        Serial.println(" seconds.");

        Serial.print("Second correct digit entered after ");
        Serial.print(enteredTimes[index-3] / 1000.0);
        Serial.println(" seconds.");

        Serial.print("Third correct digit entered after ");
        Serial.print(enteredTimes[index-2] / 1000.0);
        Serial.println(" seconds.");

        Serial.print("Fourth correct digit entered after ");
        Serial.print(enteredTimes[index-1] / 1000.0);
        Serial.println(" seconds.");

        digitalWrite(alarmSoundPin, LOW);
        return;
      }
    }   
  };
  
  Serial.println("Alarm has gone off");
  digitalWrite(alarmSoundPin, LOW);
  soundAlarm();
  return;
}

// Function to make noise if the alarm has gone off 
void soundAlarm() {
  while (alarmState == HIGH) {
    digitalWrite(alarmSoundPin, HIGH);
    delay(500);
    digitalWrite(alarmSoundPin, LOW);
    delay(500);
  }
  return;
}

// Function for moving servo 90 degrees
void servoChange(int lDirection) { 
  myservo.attach(9);
  if (lDirection == HIGH) {
    Serial.println("Locking");
    for(sPos = 0; sPos < 90; sPos += 5) { // goes from 0 degrees to 90 degrees in steps of 1 degree 
      myservo.write(sPos);              // tell servo to go to position in variable sPos 
      delay(15);                       // waits 15ms for the servo to reach the position 
    }
  } else if (lDirection == LOW) {
    Serial.println("Unlocking");
    for(sPos = 90; sPos>=1; sPos -= 5) { // goes from 90 degrees to 0 degrees                              
      myservo.write(sPos);              // tell servo to go to position in variable 'pos' 
      delay(15);                       // waits 15ms for the servo to reach the position 
    }  
  }
  myservo.detach();
}

// Get the current time count
unsigned long timeM() {
  return time_count;
}


// Interrupts

void interrupt1() {
  if (timeM() - lastInterrupt1 > 100) {
    lastInterrupt1 = timeM();
    int path = digitalRead(iPin);
    if (path == HIGH) {
      lockState = !lockState;
    } else {
      alarmState = !alarmState;
      if (alarmState) {
        Serial.println("Alarm armed.");
        digitalWrite(alarmPin, HIGH);   // Adjusting LED to display alarm state
      } else {
        Serial.println("Alarm disarmed.");
        digitalWrite(alarmPin, LOW);   // Adjusting LED to display alarm state
      }
    }
  }
}

void interrupt2() {
  if (timeM() - lastDoorInterrupt > 100) {
    doorState = !doorState;
    Serial.println("Door opened");
    lastDoorInterrupt = timeM();
  }
}

