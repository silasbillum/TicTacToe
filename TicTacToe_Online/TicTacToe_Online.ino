#include <Arduino_MKRIoTCarrier.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
const char ssid[] = WIFI_SSID;  // Wi-Fi SSID
const char password[] = WIFI_PASSWORD  // Wi-Fi Password
const char mqtt_server[] = MQTT_SERVER;  // MQTT Broker
const int mqtt_port = 8883;  // Secure MQTT Port
const char mqtt_username[] = MQTT_USERNAME;  // MQTT Username
const char mqtt_password[] = MQTT_PASSWORD;  // MQTT Password
const char mqttTopic[] = "game/status";

WiFiSSLClient wifiClient;
PubSubClient client(wifiClient);

MKRIoTCarrier carrier;

char board[3][3] = {
  {'j ', ' ', ' '}, 
  {' ', ' ', ' '},
  {' ', ' ', ' '}
};

int currentPlayer = 1;
int cursorRow = 0;
int cursorCol = 0;
bool gameOver = false;


void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ArduinoMKR", mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker");
      client.subscribe(mqttTopic);  // Subscribe to the topic
    } else {
      Serial.print("Failed, status code: ");
      Serial.println(client.state());
      delay(5000);  // Retry after 5 seconds
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Example: Respond to incoming messages
  // If you wanted to listen for game reset commands, you could check the payload and reset the game accordingly
}


void setup() {
  Serial.begin(9600);

Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);  // Use your Wi-Fi credentials
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  
  // Set up the MQTT client
  client.setServer(mqtt_server, mqtt_port);  // Set the MQTT server and port
  client.setCallback(mqttCallback);  // (Optional) Set a callback function if needed for message handling

  while (!Serial);

   carrier.begin();

  clearBoard();
  drawGrid();
  drawBoard();
  drawCursor();
}

void clearBoard() {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      board[i][j] = ' ';
  gameOver = false;
  carrier.display.fillScreen(ST77XX_BLACK);
}

void drawGrid() {
  // Vertical lines
  carrier.display.drawLine(80, 0, 80, 240, ST77XX_WHITE);
  carrier.display.drawLine(160, 0, 160, 240, ST77XX_WHITE);

  // Horizontal lines
  carrier.display.drawLine(0, 80, 240, 80, ST77XX_WHITE);
  carrier.display.drawLine(0, 160, 240, 160, ST77XX_WHITE);
}

void drawBoard() {
  carrier.display.setTextSize(4); // Big text for X and O
  carrier.display.setTextColor(ST77XX_CYAN); // You can change colors

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      int x = j * 80 + 25; // Center X
      int y = i * 80 + 25; // Center Y
      carrier.display.setCursor(x, y);
      carrier.display.print(board[i][j]);
    }
  }
}

void drawCursor(){
  int x = cursorCol * 80;
  int y = cursorRow * 80;
  carrier.display.drawRect(x, y, 80, 80, ST77XX_RED);
}

void eraseCursor(){
   int x = cursorCol * 80;
  int y = cursorRow * 80;
  carrier.display.drawRect(x, y, 80, 80, ST77XX_BLACK);
  carrier.display.drawLine(80, 0, 80, 240, ST77XX_WHITE);
  carrier.display.drawLine(160, 0, 160, 240, ST77XX_WHITE);
  carrier.display.drawLine(0, 80, 240, 80, ST77XX_WHITE);
  carrier.display.drawLine(0, 160, 240, 160, ST77XX_WHITE);
}

bool checkWin(char symbol) {
    for (int i = 0; i < 3; i++) {
      if (board[i][0] == symbol && board[i][1] == symbol && board[i][2] == symbol)
      return true;
      if (board[0][i] == symbol && board[1][i] == symbol && board[2][i] == symbol)
      return true;
    }
     if (board[0][0] == symbol && board[1][1] == symbol && board[2][2] == symbol)
    return true;
  if (board[0][2] == symbol && board[1][1] == symbol && board[2][0] == symbol)
    return true;
  return false;
}

bool isBoardFull() {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      if (board[i][j] == ' ')
        return false;
  return true;
}


void loop() {
  if (!client.connected()) {
  reconnect();
}
client.loop();

  carrier.Buttons.update();

  if (gameOver) {
    delay(3000);
    clearBoard();
    drawGrid();
    drawBoard();
    drawCursor();
    return;
  }

  if (carrier.Buttons.onTouchDown(TOUCH0)) { // A: Move left
    if (cursorCol > 0) {
      eraseCursor();
      cursorCol--;
      drawCursor();
    }
  }

  if (carrier.Buttons.onTouchDown(TOUCH1)) { // B: Move right
    if (cursorCol < 2) {
      eraseCursor();
      cursorCol++;
      drawCursor();
    }
  }

  if (carrier.Buttons.onTouchDown(TOUCH4)) { // C: Move up
    if (cursorRow > 0) {
      eraseCursor();
      cursorRow--;
      drawCursor();
    }
  }

  if (carrier.Buttons.onTouchDown(TOUCH3)) { // C: Move up
    if (cursorRow < 2) {
      eraseCursor();
      cursorRow++;
      drawCursor();
    }
  }

   if (carrier.Buttons.onTouchDown(TOUCH2)) { // E: Place mark
    if (board[cursorRow][cursorCol] == ' ') {
      char symbol = (currentPlayer == 1) ? 'X' : 'O';
      board[cursorRow][cursorCol] = symbol;

   String movePayload = "{\"player\":\"Player" + String(currentPlayer) + "\",\"action\":\"move\",\"symbol\":\"" + symbol + "\",\"row\":" + String(cursorRow) + ",\"col\":" + String(cursorCol) + "}";
client.publish("game/status", movePayload.c_str());


      drawBoard();
      eraseCursor();
      drawCursor();

      if(checkWin(symbol)){
        clearBoard();
        gameOver = true; 

        String winPayload = "{\"player\":\"Player" + String(currentPlayer) + "\",\"action\":\"win\"}";
client.publish("game/status", winPayload.c_str());

        carrier.display.setTextSize(2);
        carrier.display.setCursor(30, 90);
        carrier.display.setTextColor(ST77XX_YELLOW);
        carrier.display.print("Player ");
        carrier.display.print(currentPlayer);
        carrier.display.print(" Wins!");
        return;
      }else if (isBoardFull()){
        clearBoard();
        gameOver = true;

        
String drawPayload = "{\"action\":\"draw\"}";
client.publish("game/status", drawPayload.c_str());

        carrier.display.setTextSize(3);
        carrier.display.setCursor(65, 90);
        carrier.display.setTextColor(ST77XX_YELLOW);
        carrier.display.print("Draw!");
        return;
      }

       currentPlayer = (currentPlayer == 1) ? 2 : 1;
    }
   }
}
