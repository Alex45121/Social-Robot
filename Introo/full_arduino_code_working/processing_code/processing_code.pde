import processing.serial.*;  // Import the Processing Serial library

Serial port;

String[] emotions = {"NEUTRAL", "SUPRISED", "HAPPY", "ANGRY", "SAD"};  // Define the emotions
String selected_emotion = "NEUTRAL";

PVector servoPos = new PVector(500,500);

void setup(){
  size(1000, 1000);
  print(Serial.list());
  port = new Serial (this, Serial.list()[0], 115200); // Connect to the Arduino. Make sure to connect to the right port, for me it was [1]
}

void draw(){
  // Draw a nice GUI with buttons for emotions, and a circle we can move around to control the servos
  background(0);
  for (int i = 0; i < emotions.length; i++){
    noFill();
    stroke(255);
    strokeWeight(5);
    float button_x = map(i, 0, emotions.length, 0, width);
    float button_w = width / emotions.length;
    
    if (mouseX > button_x && mouseX < button_x + button_w && mouseY < 100) {
      fill(255, 100);
      if (mousePressed) selected_emotion = emotions[i];
    }
    if (selected_emotion == emotions[i]) fill(100, 255, 100, 100);
    rect(button_x, 0, button_w, 100);
    
    noStroke();
    textSize(24);
    fill(255);
    textAlign(CENTER, CENTER);
    text(emotions[i], map(i + 0.5, 0, emotions.length, 0, width), 50);
  }
  
  
  fill(255,0,0);
  
  ellipse(servoPos.x, servoPos.y, 20, 20);
  if (mousePressed && mouseY > 100){
    servoPos.set(constrain(mouseX, 0, width), constrain(mouseY, 100, height));
  }
  sendData();
  receiveData();
}

void sendData() {
  // Send the Data to the arduino in the format "123,abc,xyz,123,". We can send however many we like.
  String data = "" + int(map(servoPos.x, 0, width, 0, 180)) + "," + int(map(servoPos.y, 100, height, 0, 180)) + "," + selected_emotion + ",";
  port.write(data);
}

void receiveData() {
  // Receive data from the Arduino, currently the Arduino just sends back what it received
  int incomingData = 0;
  String read = "";
  while (port.available() > 0) {
    incomingData = port.read();
    if (char(incomingData) != '\n') {
      read += char(incomingData);
    }
  }
  
  if (read.length()>0) {
    println(read);
    // You can add your own stuff here if you need to obtain values from the Arduino
  }
}
