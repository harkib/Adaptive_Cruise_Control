//To Determine Velocity to volatage table and accleration contant (Tau)
#include <SPI.h>
#include <SD.h>
using namespace std;

//Pins used for forward 
const int trigPin1 = 7;
const int echoPin1 = 6;
//const int seconds

//read from forward sensor
double getFwdDis(){
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);
  return pulseIn(echoPin1, HIGH)*0.034/2;
}


void setup() {
  Serial.begin(9600);

  //Prox Sensor Set Up
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  
  //Setup Channel A
  pinMode(13, OUTPUT); //Initiates Motor Channel B pin, high -> forwad,
  pinMode(8, OUTPUT); //Initiates Brake Channel B pin, low -> disengage break
    //pin 11 is sets speed, min = 0 max = 255

  
}

void loop() {

  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  delayMicroseconds(50000000);
  
  Serial.print("speed:127");
  digitalWrite(13, LOW); //Establishes forward direction of Channel A
  digitalWrite(8, LOW);   //Disengage the Brake for Channel A
  analogWrite(11, 255);
  
  for(int i = 0; i < 12000; i ++){
    double x = getFwdDis();
    dataFile.println(x);
    Serial.print("i: ");
    Serial.print(i);
    Serial.print(", ");
    Serial.println(x);
    delayMicroseconds(1000);
  }

  //digitalWrite(13, LOW); //Establishes forward direction of Channel A
  analogWrite(11, 0);
  digitalWrite(8, HIGH);   //Disengage the Brake for Channel A
  Serial.print("speed:0");
  
  dataFile.close();

  delayMicroseconds(1000);
  while(1);

}
