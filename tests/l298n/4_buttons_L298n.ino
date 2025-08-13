int motor1pin1 = 2;
int motor1pin2 = 3;

int motor2pin1 = 4;
int motor2pin2 = 5;

const int buttonPin1 = 7;  // the number of the pushbutton pin
const int buttonPin2 = 8;  // the number of the pushbutton pin
const int buttonPin3 = 9;  // the number of the pushbutton pin
const int buttonPin4 = 10;  // the number of the pushbutton pin


// variables will change:
int buttonState1 = 0;  // variable for 
int buttonState2 = 0;
int buttonState3 = 0;  // variable for 
int buttonState4 = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(motor1pin1, OUTPUT);
  pinMode(motor1pin2, OUTPUT);
  pinMode(motor2pin1, OUTPUT);
  pinMode(motor2pin2, OUTPUT);

  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPin3, INPUT);
  pinMode(buttonPin4, INPUT);

}

void loop() {
   // put your main code here, to run repeatedly:

    buttonState1 = digitalRead(buttonPin1);
    buttonState2 = digitalRead(buttonPin2);
    buttonState3 = digitalRead(buttonPin3);
    buttonState4 = digitalRead(buttonPin4);


  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState1 == HIGH && buttonState2 != HIGH) { 
    // Run motors forward
    digitalWrite(motor1pin1, HIGH);
    digitalWrite(motor1pin2, LOW); 
  } 
  else if (buttonState1 != HIGH && buttonState2 == HIGH) { 
    // Run motors backward
    digitalWrite(motor1pin1, LOW);
    digitalWrite(motor1pin2, HIGH); 
  } 
  else {
    // Stop motors
    digitalWrite(motor1pin1, LOW);
    digitalWrite(motor1pin2, LOW); 
  }

  if (buttonState3 == HIGH && buttonState4 != HIGH) { 
    // Run motors forward
    digitalWrite(motor2pin1, HIGH);
    digitalWrite(motor2pin2, LOW); 
  } 
  else if (buttonState3 != HIGH && buttonState4 == HIGH) { 
    // Run motors backward
    digitalWrite(motor2pin1, LOW);
    digitalWrite(motor2pin2, HIGH); 
  } 
  else {
    // Stop motors
    digitalWrite(motor2pin1, LOW);
    digitalWrite(motor2pin2, LOW); 
  }

  
}
