// Some useful resources on BLE and ESP32:
//      https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/examples/BLE_notify/BLE_notify.ino
//      https://microcontrollerslab.com/esp32-bluetooth-low-energy-ble-using-arduino-ide/
//      https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/
//      https://www.electronicshub.org/esp32-ble-tutorial/
#include <BLEDevice.h>
#include <BLE2902.h>
#include <M5Core2.h>
///////////////////////////////////////////////////////////////
// Variables
///////////////////////////////////////////////////////////////
static BLERemoteCharacteristic *bleRemoteCharacteristic;
static BLEAdvertisedDevice *bleRemoteServer;
static boolean doConnect = false;
static boolean doScan = false;
bool deviceConnected = false;
bool previouslyConnected = false;

// device
int sWidth;
int sHeight;

// buttons
Button whackerButton(0, 0, 0, 0, "Whacker");
Button popperButton(0, 0, 0, 0, "Popper");

// game
bool isMyTurn;
int turnsRemaining;
int othersTurnsRemaining;
const int TURNS = 10;
const int DICE_DIGITS = 6;

int whackerDiceRolls;
int popperDieRoll;

bool moles[DICE_DIGITS]; // true if mole is up at INDEX

enum Player
{
    WHACKER,
    POPPER
};

enum GameState
{
    CONNECTING,
    SETUP,
    INGAME,
    ENDGAME,
    ERROR
};

Player thisDevicePlayer;
GameState thisDeviceGameState;

// See the following for generating UUIDs: https://www.uuidgenerator.net/
static BLEUUID SERVICE_UUID("ceb12aed-022d-48f8-8141-f839037559b0");        // raztaz Service
static BLEUUID CHARACTERISTIC_UUID("0832a505-ddbd-4469-9082-e8e0337a88f0"); // raztaz Characteristic

///////////////////////////////////////////////////////////////
// Forward Declarations
///////////////////////////////////////////////////////////////
void broadcastBleServer();
void drawScreenTextWithBackground(String text, int backgroundColor);

// game methods
void gameSetUp();
void playerRolled();

// helper methods
int dieRoll();
void moleWhack(int rolls);
void molePop(int roll);
bool allMolesDown();
void newGame();
bool gameOver();

int clientReadFromBLE();
void clientWriteToBLE(int rollToWrite);

// drawing methods
void drawDie(int die);
void drawDice(int die1, int die2);
void drawGameDisplay();
void drawWaitingText();
void drawRollDie();
void drawIntroScreen();

///////////////////////////////////////////////////////////////
// BLE Client Callback Methods
// This method is called when the server that this client is
// connected to NOTIFIES this client (or any client listening)
// that it has changed the remote characteristic
///////////////////////////////////////////////////////////////
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    Serial.printf("Notify callback for characteristic %s of data length %d\n", pBLERemoteCharacteristic->getUUID().toString().c_str(), length);
    Serial.printf("\tData: %s", (char *)pData);
    std::string value = pBLERemoteCharacteristic->readValue();
    Serial.printf("\tValue was: %s", value.c_str());
}

///////////////////////////////////////////////////////////////
// BLE Server Callback Method
// These methods are called upon connection and disconnection
// to BLE service.
///////////////////////////////////////////////////////////////
class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
        deviceConnected = true;
        Serial.println("Client Device connected...");

        thisDeviceGameState = SETUP;
    }

    void onDisconnect(BLEClient *pclient)
    {
        deviceConnected = false;
        Serial.println("Device disconnected...");
        // drawScreenTextWithBackground("LOST connection to device.\n\nAttempting re-connection...", TFT_RED);
    }
};

///////////////////////////////////////////////////////////////
// Method is called to connect to server
///////////////////////////////////////////////////////////////
bool connectToServer()
{
    // Create the client
    Serial.printf("Forming a connection to %s\n", bleRemoteServer->getName().c_str());
    BLEClient *bleClient = BLEDevice::createClient();
    bleClient->setClientCallbacks(new MyClientCallback());
    Serial.println("\tClient connected");

    // Connect to the remote BLE Server.
    if (!bleClient->connect(bleRemoteServer))
        Serial.printf("FAILED to connect to server (%s)\n", bleRemoteServer->getName().c_str());
    Serial.printf("\tConnected to server (%s)\n", bleRemoteServer->getName().c_str());

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService *bleRemoteService = bleClient->getService(SERVICE_UUID);
    if (bleRemoteService == nullptr)
    {
        Serial.printf("Failed to find our service UUID: %s\n", SERVICE_UUID.toString().c_str());
        bleClient->disconnect();
        return false;
    }
    Serial.printf("\tFound our service UUID: %s\n", SERVICE_UUID.toString().c_str());

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    bleRemoteCharacteristic = bleRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
    if (bleRemoteCharacteristic == nullptr)
    {
        Serial.printf("Failed to find our characteristic UUID: %s\n", CHARACTERISTIC_UUID.toString().c_str());
        bleClient->disconnect();
        return false;
    }
    Serial.printf("\tFound our characteristic UUID: %s\n", CHARACTERISTIC_UUID.toString().c_str());

    // Read the value of the characteristic
    if (bleRemoteCharacteristic->canRead())
    {
        std::string value = bleRemoteCharacteristic->readValue();
        Serial.printf("The characteristic value was: %s", value.c_str());
        // drawScreenTextWithBackground("Initial characteristic value read from server:\n\n" + String(value.c_str()), TFT_GREEN);
        // delay(3000);
    }

    // Check if server's characteristic can notify client of changes and register to listen if so
    if (bleRemoteCharacteristic->canNotify())
        bleRemoteCharacteristic->registerForNotify(notifyCallback);

    // deviceConnected = true;
    return true;
}

///////////////////////////////////////////////////////////////
// Scan for BLE servers and find the first one that advertises
// the service we are looking for.
///////////////////////////////////////////////////////////////
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    /**
     * Called for each advertising BLE server.
     */
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        // We have found a device, let us now see if it contains the service we are looking for.
        if (advertisedDevice.haveServiceUUID() &&
            advertisedDevice.isAdvertisingService(SERVICE_UUID) &&
            advertisedDevice.getName() == "RazTaz M5Core2")
        {
            BLEDevice::getScan()->stop();
            bleRemoteServer = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = true;
        }
    }
};

///////////////////////////////////////////////////////////////
// Put your setup code here, to run once
///////////////////////////////////////////////////////////////
void setup()
{
    // Init device
    M5.begin();
    M5.Lcd.setTextSize(3);
    sWidth = M5.Lcd.width();
    sHeight = M5.Lcd.height();

    // game variable defaults
    thisDeviceGameState = CONNECTING; // trying to connect to another device
    isMyTurn = false;                 // true for popper, false for whacker
    for (int i = 0; i < DICE_DIGITS; i++)
    {
        moles[i] = false;
    }
    turnsRemaining = TURNS;

    BLEDevice::init("");
    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
    drawScreenTextWithBackground("Scanning for BLE server...", TFT_BLUE);
}

///////////////////////////////////////////////////////////////
// Put your main code here, to run repeatedly
///////////////////////////////////////////////////////////////
void loop()
{
    if (doConnect == true)
     {
        if (connectToServer())
        {
            Serial.println("We are now connected to the BLE Server.");
            doConnect = false;
        }
        else
        {
            Serial.println("We have failed to connect to the server; there is nothin more we will do.");
        }
    }

    // not connected to a device yet
    if (thisDeviceGameState == CONNECTING && !deviceConnected)
    {
        drawScreenTextWithBackground("CLIENT waiting to connect", BLUE);

        /////////////////////////////////////////////////////////////////////////////////////////////// TODO remove: because no tabby :(
        //  deviceConnected = true;
        //  thisDeviceGameState = SETUP;
        /////////////////////////////////////
    }

    // connected to another device, we can start playing!
    if (deviceConnected)
    {
        M5.update();

        // choose your role and join the game!
        if (thisDeviceGameState == SETUP)
        {
            gameSetUp();

        } else if (thisDeviceGameState == ENDGAME) {
            Serial.println("ENDGAMEEEEEEEEEE");

            if (M5.BtnC.wasPressed()) {
                newGame();
            }

        } else if (thisDeviceGameState == INGAME)
        {
            switch (thisDevicePlayer)
            {

            case WHACKER:
                if (isMyTurn)
                {
                    if (M5.BtnC.wasPressed()) // roll the die
                    {
                        // roll the dice for new numbers
                        whackerDiceRolls = (dieRoll() * 10) + (dieRoll());

                        // update the game state
                        playerRolled();
                        moleWhack(whackerDiceRolls);

                        // update the UI
                        drawGameDisplay();
                        drawDice(whackerDiceRolls / 10, whackerDiceRolls % 10);
                        delay(1000);
                        drawGameDisplay();
                        drawWaitingText();

                        // Serial.println("you as the WHACKER rolled a " + String(whackerDiceRolls));
                    }
                }
                else
                {
                    // Serial.println("waiting for popper move");
                    int result = clientReadFromBLE();

                    othersTurnsRemaining = result / 10;
                    popperDieRoll = result % 10;

                    if (othersTurnsRemaining == turnsRemaining) {
                        // popper made a move!
                        molePop(popperDieRoll);
                        drawGameDisplay();
                        isMyTurn = true;
                        drawRollDie();

                    }
                }
                break;

            case POPPER:
                if (isMyTurn)
                {
                    if (M5.BtnC.wasPressed()) // roll the dice
                    {
                        // roll the dice for new numbers
                        popperDieRoll = dieRoll();

                        // update the game state
                        playerRolled();
                        molePop(popperDieRoll);

                        // update the UI
                        drawGameDisplay();
                        drawDie(popperDieRoll);
                        delay(1000);
                        drawGameDisplay();
                        drawWaitingText();

                        // Serial.println("you as the POPPER rolled a " + String(popperDieRoll));
                    }
                }
                else
                {
                    // Serial.println("waiting for whacker move");
                    int result = clientReadFromBLE();
                    Serial.println(result);

                    othersTurnsRemaining = result / 100;
                    whackerDiceRolls = result % 100;

                    if (othersTurnsRemaining == turnsRemaining + 1) {
                        // whacker made a move!
                        moleWhack(whackerDiceRolls);
                        drawGameDisplay();
                        isMyTurn = true;
                        drawRollDie();
                    }
                }
                break;

            default:
                drawScreenTextWithBackground("uh oh, something went wrong. dig again!", RED);
                if (M5.BtnB.wasPressed())
                {
                    thisDeviceGameState == SETUP;
                }
                break;
            }
            
            if (gameOver()) {
                thisDeviceGameState = ENDGAME;
                drawEndGameScreen();

            }
        }
    }

    // other device disconnected
    if (doScan && previouslyConnected && !deviceConnected && thisDeviceGameState != ERROR)
    {
        thisDeviceGameState = ERROR;
        drawScreenTextWithBackground("uh oh, somebody went too far into the hole. dig again by resetting the device!", RED);
    }
}

void gameSetUp()
{
    drawIntroScreen();    
    if (whackerButton.wasPressed()) {
        thisDevicePlayer = WHACKER;
        isMyTurn = false; // whacker goes second

        thisDeviceGameState = INGAME;
        drawGameDisplay();
        drawDie(1); // default num
        if (thisDevicePlayer == POPPER) {
            drawRollDie();
        }
    } else if (popperButton.wasPressed()) {

        thisDevicePlayer = POPPER;
        isMyTurn = true; // popper goes first

        thisDeviceGameState = INGAME;
        drawGameDisplay();
        drawDie(1); // default num
        if (thisDevicePlayer == POPPER) {
            drawRollDie();
        }
    }
}

// writes the player's roll(s) to the BLE characteristic and adjusts their turns
void playerRolled()
{
    int numToWrite;
    numToWrite += turnsRemaining * (thisDevicePlayer == WHACKER ? 100 : 10);
    numToWrite += (thisDevicePlayer == WHACKER ? whackerDiceRolls : popperDieRoll);
    clientWriteToBLE(numToWrite);
    turnsRemaining -= 1;
    isMyTurn = false;
}

// randomly generated roll
// reduces mod bias for 6 (source https://stackoverflow.com/questions/10984974/why-do-people-say-there-is-modulo-bias-when-using-a-random-number-generator)
int dieRoll()
{

    int roll = esp_random();
    int sides;

    // int randMax = INT32_MAX;
    // while (roll >= (randMax - randMax % sides)) {
    //     roll = esp_random();
    // };

    Serial.printf("random %d\t\t", roll);
    roll = (abs(roll) % DICE_DIGITS) + 1;
    Serial.printf("roll %d \n", roll);

    return roll;
}

void moleWhack(int rolls)
{
    int firstRoll = (rolls / 10) - 1;  // -1 for zero indexing
    int secondRoll = (rolls % 10) - 1; // -1 for zero indexing

    // check the first die
    if (moles[firstRoll])
    { // true, that means the mole is up
        moles[firstRoll] = false;
    }

    // check the second die
    if (moles[secondRoll])
    { // true, that means the mole is up
        moles[secondRoll] = false;
    }

}

void molePop(int roll)
{
    int firstRoll = roll - 1; /// -1 for zero indexing

    // put up the mole!
    moles[firstRoll] = true;
}

bool allMolesDown() {
    for (int i = 0; i < DICE_DIGITS; i++) {
        if (moles[i]) {
            return true; // one mole that was up was found
        }
    }
    return false;
}

void newGame() {
    thisDeviceGameState = SETUP; // go to the intro screen so that the players can pick roles
    isMyTurn = false; // default false

    // put all the moles down
    for (int i = 0; i < DICE_DIGITS; i++)
    {
        moles[i] = false;
    }

    // set turns remaining to the constantly defined TURNS
    turnsRemaining = TURNS;

     if (thisDevicePlayer == POPPER) {
        drawRollDie();
    }
}

bool gameOver() {
    if (thisDevicePlayer == WHACKER) {
        return turnsRemaining == 0;
        Serial.println("\t\tendgame - WHACKER");
    } else {
        return (whackerDiceRolls / 100 ) == 1; // the whacker sends 1 before updating their turnsRemaining to 0
        Serial.println("\t\tendgame - POPPER");
    }
        Serial.println("keep playing!");
}

int clientReadFromBLE()
{
    std::string readValue = bleRemoteCharacteristic->readValue();
    String valStr = readValue.c_str();
    int val = valStr.toInt();
    Serial.println(val);
    return val;
}

void clientWriteToBLE(int rollToWrite)
{
    String newValue = String(rollToWrite);
    bleRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
    Serial.println(rollToWrite);
    return;
}

///////////////////////////////////////////////////////////////
// Colors the background and then writes the text on top
///////////////////////////////////////////////////////////////
void drawScreenTextWithBackground(String text, int backgroundColor)
{
    M5.Lcd.fillScreen(backgroundColor);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println(text);
}

void drawGameDisplay()
{
    // background and text color
    uint16_t primaryTextColor;
    M5.Lcd.fillScreen(TFT_DARKGREEN);
    primaryTextColor = TFT_WHITE;

    // variables
    int pad = 10;
    int xOffset = sWidth / 11;
    int halfXOffset = xOffset / 2;
    int holeRad = 25;
    int moleWidth = 30;
    int moleHeight = 25;
    int triWidth = sWidth / 11;
    int vSpace = 10;

    // title
    M5.Lcd.setCursor(pad, pad);
    M5.Lcd.setTextColor(primaryTextColor);
    M5.Lcd.setTextSize(3);
    M5.Lcd.println(thisDevicePlayer == WHACKER ? "Whacker" : "Popper");
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(pad, sHeight - 20);
    M5.Lcd.println("Turns Left:  " + String(turnsRemaining));

    // draw buttons and numbers
    for (int i = 0; i < DICE_DIGITS / 2; i++)
    {
        xOffset = ((i * 2) + 1) * (sWidth / 9);
        int yOffset = 80; // offset for the text

        // if(die == i+1){
        if (moles[i])
        {
            M5.Lcd.fillEllipse(xOffset, yOffset, holeRad, holeRad / 2, TFT_MAROON);
            M5.Lcd.fillRect(xOffset - 15, yOffset - 20, moleWidth, moleHeight, TFT_DARKGREY);
            M5.Lcd.fillRoundRect(xOffset - 15, yOffset - 30, moleWidth, moleHeight, 15, TFT_DARKGREY);
            M5.Lcd.fillTriangle(xOffset - 6, yOffset - 15, xOffset + 6, yOffset - 15, xOffset, yOffset - 29, TFT_LIGHTGREY);
            M5.Lcd.fillEllipse(xOffset, yOffset - 15, 6, 3, TFT_LIGHTGREY);
            M5.Lcd.fillCircle(xOffset, yOffset - 27, 2, TFT_MAGENTA);
            M5.Lcd.fillCircle(xOffset + 10, yOffset - 20, 2, TFT_BLACK);
            M5.Lcd.fillCircle(xOffset - 10, yOffset - 20, 2, TFT_BLACK);
            M5.Lcd.drawLine(xOffset - 2, yOffset - 16, xOffset + 2, yOffset - 16, TFT_BLACK);
        }
        else
        {
            // draw the up arrow
            M5.Lcd.fillEllipse(xOffset, yOffset, holeRad, holeRad / 2, TFT_MAROON);
        }

        yOffset = (yOffset * 2); //+ vSpace;

        // if(die == (i+1)+3){
        if (moles[i + 3])
        {
            M5.Lcd.fillEllipse(xOffset, yOffset, holeRad, holeRad / 2, TFT_MAROON);
            M5.Lcd.fillRect(xOffset - 15, yOffset - 20, moleWidth, moleHeight, TFT_DARKGREY);
            M5.Lcd.fillRoundRect(xOffset - 15, yOffset - 30, moleWidth, moleHeight, 15, TFT_DARKGREY);
            M5.Lcd.fillTriangle(xOffset - 6, yOffset - 15, xOffset + 6, yOffset - 15, xOffset, yOffset - 29, TFT_LIGHTGREY);
            M5.Lcd.fillEllipse(xOffset, yOffset - 15, 6, 3, TFT_LIGHTGREY);
            M5.Lcd.fillCircle(xOffset, yOffset - 27, 2, TFT_MAGENTA);
            M5.Lcd.fillCircle(xOffset + 10, yOffset - 20, 2, TFT_BLACK);
            M5.Lcd.fillCircle(xOffset - 10, yOffset - 20, 2, TFT_BLACK);
            M5.Lcd.drawLine(xOffset - 2, yOffset - 16, xOffset + 2, yOffset - 16, TFT_BLACK);
        }
        else
        {
            // draw the down arrow
            M5.Lcd.fillEllipse(xOffset, yOffset, holeRad, holeRad / 2, TFT_MAROON);
        }
    }
}

void drawDie(int die)
{

    switch (die)
    {
    case 1:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 75, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 35, 110, 4, TFT_BLACK); // y = 75 + 35
        break;

    case 2:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 75, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 92, 4, TFT_BLACK);  // left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 127, 4, TFT_BLACK); // right
        break;
    case 3:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 75, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 92, 4, TFT_BLACK);  // left
        M5.Lcd.fillCircle((sWidth * 0.75) + 35, 110, 4, TFT_BLACK); // middle
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 127, 4, TFT_BLACK); // right
        break;
    case 4:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 75, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 92, 4, TFT_BLACK);  // top left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 127, 4, TFT_BLACK); // bottom right
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 92, 4, TFT_BLACK);  // top right
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 127, 4, TFT_BLACK); // bottom left
        break;
    case 5:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 75, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 92, 4, TFT_BLACK);  // top left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 127, 4, TFT_BLACK); // bottom right
        M5.Lcd.fillCircle((sWidth * 0.75) + 35, 110, 4, TFT_BLACK); // middle
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 92, 4, TFT_BLACK);  // top right
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 127, 4, TFT_BLACK); // bottom left
        break;
    case 6:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 75, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 92, 4, TFT_BLACK);  // top left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 127, 4, TFT_BLACK); // bottom right
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 109, 4, TFT_BLACK); // middle left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 109, 4, TFT_BLACK); // middle right
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 92, 4, TFT_BLACK);  // top right
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 127, 4, TFT_BLACK); // bottom left
        break;
    default:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 100, 75, 75, 10, TFT_WHITE);
        break;
    }
}

void drawDice(int die1, int die2)
{

    switch (die1)
    {
    case 1:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 25, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 35, 59, 4, TFT_BLACK); // y = 75 + 35
        break;

    case 2:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 25, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 42, 4, TFT_BLACK); // left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 76, 4, TFT_BLACK); // right
        break;
    case 3:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 25, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 42, 4, TFT_BLACK); // left
        M5.Lcd.fillCircle((sWidth * 0.75) + 35, 59, 4, TFT_BLACK); // middle
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 76, 4, TFT_BLACK); // right
        break;
    case 4:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 25, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 42, 4, TFT_BLACK); // top left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 76, 4, TFT_BLACK); // bottom right
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 42, 4, TFT_BLACK); // top right
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 76, 4, TFT_BLACK); // bottom left
        break;
    case 5:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 25, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 42, 4, TFT_BLACK); // top left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 76, 4, TFT_BLACK); // bottom right
        M5.Lcd.fillCircle((sWidth * 0.75) + 35, 59, 4, TFT_BLACK); // middle
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 42, 4, TFT_BLACK); // top right
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 76, 4, TFT_BLACK); // bottom left
        break;
    case 6:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 25, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 42, 4, TFT_BLACK); // top left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 72, 4, TFT_BLACK); // bottom right
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 59, 4, TFT_BLACK); // middle left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 59, 4, TFT_BLACK); // middle right
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 42, 4, TFT_BLACK); // top right
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 72, 4, TFT_BLACK); // bottom left
        break;
    default:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 25, 70, 70, 10, TFT_WHITE);
        break;
    }
    switch (die2)
    {
    case 1:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 125, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 35, 159, 4, TFT_BLACK); // y = 75 + 35
        break;

    case 2:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 125, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 142, 4, TFT_BLACK); // left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 176, 4, TFT_BLACK); // right
        break;
    case 3:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 125, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 142, 4, TFT_BLACK); // left
        M5.Lcd.fillCircle((sWidth * 0.75) + 35, 159, 4, TFT_BLACK); // middle
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 176, 4, TFT_BLACK); // right
        break;
    case 4:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 125, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 142, 4, TFT_BLACK); // top left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 176, 4, TFT_BLACK); // bottom right
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 142, 4, TFT_BLACK); // top right
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 176, 4, TFT_BLACK); // bottom left
        break;
    case 5:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 125, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 142, 4, TFT_BLACK); // top left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 176, 4, TFT_BLACK); // bottom right
        M5.Lcd.fillCircle((sWidth * 0.75) + 35, 159, 4, TFT_BLACK); // middle
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 142, 4, TFT_BLACK); // top right
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 176, 4, TFT_BLACK); // bottom left
        break;
    case 6:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 125, 70, 70, 10, TFT_WHITE);
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 142, 4, TFT_BLACK); // top left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 172, 4, TFT_BLACK); // bottom right
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 159, 4, TFT_BLACK); // middle left
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 159, 4, TFT_BLACK); // middle right
        M5.Lcd.fillCircle((sWidth * 0.75) + 52, 142, 4, TFT_BLACK); // top right
        M5.Lcd.fillCircle((sWidth * 0.75) + 17, 172, 4, TFT_BLACK); // bottom left
        break;
    default:
        M5.Lcd.fillRoundRect(sWidth * 0.75, 125, 70, 70, 10, TFT_WHITE);
        break;
    }
}

void drawWaitingText() {
    int pad = sWidth * 0.75;
    int extraMargin = thisDevicePlayer == WHACKER ? 0 : 10;
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(2);

    M5.Lcd.setCursor(pad - 10, (sHeight / 2) - 30);
    M5.Lcd.println("Waiting");

    M5.Lcd.setCursor(pad + 10, M5.Lcd.getCursorY() + 5);
    M5.Lcd.println("for");

    M5.Lcd.setCursor(pad - extraMargin, M5.Lcd.getCursorY() + 5);
    M5.Lcd.println(thisDevicePlayer == WHACKER ? "Popper" : "Whacker");
}

void drawRollDie(){
    M5.Lcd.setCursor(sWidth-80, sHeight-55);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Roll");
    M5.Lcd.drawLine(sWidth - 50, sHeight-35, sWidth- 50, sHeight - 20, TFT_WHITE);
    M5.Lcd.fillTriangle(sWidth - 60, sHeight-20, sWidth- 40, sHeight- 20, sWidth- 50, sHeight -10, TFT_WHITE);

}

void drawIntroScreen(){
    while(thisDeviceGameState == SETUP){
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.setCursor(sWidth/6, sHeight/3);
        M5.Lcd.setTextColor(TFT_WHITE);
        M5.Lcd.setTextSize(3);
        M5.Lcd.println("Whack-A-Mole");
        M5.Lcd.setCursor(sWidth/5, sHeight/2);
        
        M5.Lcd.setTextSize(2);
        M5.Lcd.println("Select a Player:");

        M5.Lcd.setCursor(sWidth-120, sHeight-55);
        popperButton = Button(sWidth- 140, sHeight- 75, 120, 55, "Popper");  
        M5.Lcd.println("Popper");

        M5.Lcd.setCursor(40, sHeight-55);
        whackerButton = Button(20, sHeight- 75, 120, 55, "Whacker");
        M5.Lcd.println("Whacker");

        delay(600);
        M5.Lcd.fillRect(sWidth/5, sHeight/2, 200, 20, TFT_BLACK);
        delay(600);
    }
}

void drawEndGameScreen() {
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setCursor(sWidth/5, sHeight/3);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.println("Game Over!");
    M5.Lcd.setCursor(sWidth-125, sHeight-55);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Dig Again?");
    M5.Lcd.drawLine(sWidth - 50, sHeight-30, sWidth- 50, sHeight - 20, TFT_WHITE);
    M5.Lcd.fillTriangle(sWidth - 60, sHeight-20, sWidth- 40, sHeight- 20, sWidth- 50, sHeight -10, TFT_WHITE);

    //MOLE TIME 
    int xOffset = (sWidth / 11) + 20 ;
    int yOffset = sHeight - 30;
    int holeRad = 25;
    int moleWidth = 30;
    int moleHeight = 25;    

    M5.Lcd.fillEllipse(xOffset, yOffset, holeRad, holeRad/2, TFT_MAROON);
    M5.Lcd.fillCircle(xOffset+40, yOffset-5, 1, TFT_MAROON);
    M5.Lcd.fillCircle(xOffset+50, yOffset-1, 1, TFT_MAROON);
    M5.Lcd.fillCircle(xOffset+35, yOffset+3, 2, TFT_MAROON);
    M5.Lcd.fillRect(xOffset-15, yOffset-20, moleWidth, moleHeight, TFT_DARKGREY);
    M5.Lcd.fillRoundRect(xOffset-15, yOffset-30, moleWidth, moleHeight, 15, TFT_DARKGREY);
    M5.Lcd.fillTriangle(xOffset-6, yOffset - 15, xOffset+6, yOffset - 15, xOffset, yOffset-29,TFT_LIGHTGREY);
    M5.Lcd.fillEllipse(xOffset, yOffset- 15, 6, 3, TFT_LIGHTGREY);
    M5.Lcd.fillCircle(xOffset,yOffset-27, 2, TFT_MAGENTA );
    M5.Lcd.fillCircle(xOffset+10,yOffset-20, 2, TFT_BLACK );
    M5.Lcd.fillCircle(xOffset-10,yOffset-20, 2, TFT_BLACK );
    M5.Lcd.drawLine(xOffset-2, yOffset-16, xOffset+2, yOffset-16, TFT_BLACK);

    if((thisDevicePlayer == WHACKER && allMolesDown()) || (thisDevicePlayer == POPPER && !allMolesDown())){
        //You won
        M5.Lcd.setCursor(sWidth/4, sHeight/2);
        M5.Lcd.setTextSize(3);
        M5.Lcd.setTextColor(TFT_GREENYELLOW);
        M5.Lcd.println("You Won!");

    }
    else{
        //You lost
        M5.Lcd.setCursor((sWidth/5)-10, sHeight/2);
        M5.Lcd.setTextSize(3);
        M5.Lcd.setTextColor(TFT_MAGENTA);
        M5.Lcd.println("You Lost :(");
    }

    while(!M5.BtnC.wasPressed()){
        M5.Lcd.fillEllipse(xOffset, yOffset, holeRad, holeRad/2, TFT_MAROON);
        M5.Lcd.fillCircle(xOffset+40, yOffset-5, 1, TFT_MAROON);
        M5.Lcd.fillCircle(xOffset+50, yOffset-1, 1, TFT_MAROON);
        M5.Lcd.fillCircle(xOffset+35, yOffset+3, 2, TFT_MAROON);
        M5.Lcd.fillRect(xOffset-15, yOffset-20, moleWidth, moleHeight, TFT_DARKGREY);
        M5.Lcd.fillRoundRect(xOffset-15, yOffset-30, moleWidth, moleHeight, 15, TFT_DARKGREY);
        M5.Lcd.fillTriangle(xOffset-6, yOffset - 15, xOffset+6, yOffset - 15, xOffset, yOffset-29,TFT_LIGHTGREY);
        M5.Lcd.fillEllipse(xOffset, yOffset- 15, 6, 3, TFT_LIGHTGREY);
        M5.Lcd.fillCircle(xOffset,yOffset-27, 2, TFT_MAGENTA );
        M5.Lcd.fillCircle(xOffset+10,yOffset-20, 2, TFT_BLACK );
        M5.Lcd.fillCircle(xOffset-10,yOffset-20, 2, TFT_BLACK );
        M5.Lcd.drawLine(xOffset-2, yOffset-16, xOffset+2, yOffset-16, TFT_BLACK);

        delay(1000);
       //M5.Lcd.fillRect(sWidth/4, sHeight/2, 150, 40, TFT_BLACK);
        M5.Lcd.fillRect(xOffset - (holeRad+5), yOffset-30, holeRad*4, moleHeight * 2 , TFT_BLACK);
        M5.Lcd.fillEllipse(xOffset, yOffset, holeRad, holeRad/2, TFT_MAROON);
        delay(700);
    }
}