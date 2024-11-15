#include <M5Core2.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//#include "EGR425_Phase1_weather_bitmap_images.h"
#include "WiFi.h"

////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////

// Time variables
unsigned long lastTime = 0;
unsigned long timerDelay = 10000; // 5000; 5 minutes (300,000ms) or 5 seconds (5,000ms)

// LCD variables
int sWidth;
int sHeight;


// mode variable
enum Screen
{
    S_WACKAMOLE,
    S_WINNER
};
static Screen screen = S_WACKAMOLE;


// zipcode variables
int zipcode[6] = {9, 2, 5, 0, 4};
const int DICE_DIGITS = 6;
//int die = 5;

//color variables
//uint16_t brown = color565(84, 41, 18);


////////////////////////////////////////////////////////////////////
// Method header declarations
////////////////////////////////////////////////////////////////////
void drawDie(int die);
void drawGameDisplay(int die);
void drawDice(int die1, int die2);
void drawIntroScreen();
void drawRollDie();
void drawEndGameScreen();
void drawConnectingScreen();

///////////////////////////////////////////////////////////////
// Put your setup code here, to run once
///////////////////////////////////////////////////////////////
void setup()
{
    // Initialize the device
    M5.begin();

    // Set screen orientation and get height/width
    sWidth = M5.Lcd.width();
    sHeight = M5.Lcd.height();

    //drawRollDie();
    // drawIntroScreen();
    // drawGameDisplay(5); 
    // drawDice(4, 1);
    //drawEndGameScreen();
    drawConnectingScreen();
}

///////////////////////////////////////////////////////////////
// Put your main code here, to run repeatedly
///////////////////////////////////////////////////////////////
void loop()
{
    M5.update();
    // update the screen with the new units
   // drawZipCodeDisplay(); 
}

/////////////////////////////////////////////////////////////////
// Update the display to set a zip code
/////////////////////////////////////////////////////////////////
void drawGameDisplay(int die)
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
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Turns Left:  ");



    // draw buttons and numbers
    for (int i = 0; i < DICE_DIGITS/2; i++)
    {
        xOffset = ((i * 2) + 1) * (sWidth / 9);
        int yOffset = 80; // offset for the text

        if(die == i+1){
            M5.Lcd.fillEllipse(xOffset, yOffset, holeRad, holeRad/2, TFT_MAROON);
            M5.Lcd.fillRect(xOffset-15, yOffset-20, moleWidth, moleHeight, TFT_DARKGREY);
            M5.Lcd.fillRoundRect(xOffset-15, yOffset-30, moleWidth, moleHeight, 15, TFT_DARKGREY);
            M5.Lcd.fillTriangle(xOffset-6, yOffset - 15, xOffset+6, yOffset - 15, xOffset, yOffset-29,TFT_LIGHTGREY);
            M5.Lcd.fillEllipse(xOffset, yOffset- 15, 6, 3, TFT_LIGHTGREY);
            M5.Lcd.fillCircle(xOffset,yOffset-27, 2, TFT_MAGENTA );
            M5.Lcd.fillCircle(xOffset+10,yOffset-20, 2, TFT_BLACK );
            M5.Lcd.fillCircle(xOffset-10,yOffset-20, 2, TFT_BLACK );
            M5.Lcd.drawLine(xOffset-2, yOffset-16, xOffset+2, yOffset-16, TFT_BLACK);
        }
        else{
            // draw the up arrow
            M5.Lcd.fillEllipse(xOffset, yOffset, holeRad, holeRad/2, TFT_MAROON);
            
        }

        yOffset = (yOffset * 2); //+ vSpace;

        if(die == (i+1)+3){
            M5.Lcd.fillEllipse(xOffset, yOffset, holeRad, holeRad/2, TFT_MAROON);
            M5.Lcd.fillRect(xOffset-15, yOffset-20, moleWidth, moleHeight, TFT_DARKGREY);
            M5.Lcd.fillRoundRect(xOffset-15, yOffset-30, moleWidth, moleHeight, 15, TFT_DARKGREY);
            M5.Lcd.fillTriangle(xOffset-6, yOffset - 15, xOffset+6, yOffset - 15, xOffset, yOffset-29,TFT_LIGHTGREY);
            M5.Lcd.fillEllipse(xOffset, yOffset- 15, 6, 3, TFT_LIGHTGREY);
            M5.Lcd.fillCircle(xOffset,yOffset-27, 2, TFT_MAGENTA );
            M5.Lcd.fillCircle(xOffset+10,yOffset-20, 2, TFT_BLACK );
            M5.Lcd.fillCircle(xOffset-10,yOffset-20, 2, TFT_BLACK );
            M5.Lcd.drawLine(xOffset-2, yOffset-16, xOffset+2, yOffset-16, TFT_BLACK);
        }
        else{
            // draw the down arrow
            M5.Lcd.fillEllipse(xOffset, yOffset, holeRad, holeRad/2, TFT_MAROON);
        }
    }

}
void drawDie(int die){

    switch(die){
        case 1:
            M5.Lcd.fillRoundRect(sWidth*0.75, 75, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+35, 110, 4, TFT_BLACK);         //y = 75 + 35   
            break;

        case 2:
            M5.Lcd.fillRoundRect(sWidth*0.75, 75, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 92, 4, TFT_BLACK);  //left
             M5.Lcd.fillCircle((sWidth*0.75)+52, 127, 4, TFT_BLACK);  //right
            break;
        case 3:
            M5.Lcd.fillRoundRect(sWidth*0.75, 75, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 92, 4, TFT_BLACK); //left
            M5.Lcd.fillCircle((sWidth*0.75)+35, 110, 4, TFT_BLACK);  //middle
             M5.Lcd.fillCircle((sWidth*0.75)+52, 127, 4, TFT_BLACK);  //right
            break;
        case 4:
            M5.Lcd.fillRoundRect(sWidth*0.75, 75, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 92, 4, TFT_BLACK);  //top left
            M5.Lcd.fillCircle((sWidth*0.75)+52, 127, 4, TFT_BLACK); //bottom right
            M5.Lcd.fillCircle((sWidth*0.75)+52, 92, 4, TFT_BLACK);   //top right
            M5.Lcd.fillCircle((sWidth*0.75)+17, 127, 4, TFT_BLACK);  //bottom left
            break;
        case 5:
            M5.Lcd.fillRoundRect(sWidth*0.75, 75, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 92, 4, TFT_BLACK);  //top left
            M5.Lcd.fillCircle((sWidth*0.75)+52, 127, 4, TFT_BLACK); //bottom right
            M5.Lcd.fillCircle((sWidth*0.75)+35, 110, 4, TFT_BLACK);  //middle
            M5.Lcd.fillCircle((sWidth*0.75)+52, 92, 4, TFT_BLACK); //top right
            M5.Lcd.fillCircle((sWidth*0.75)+17, 127, 4, TFT_BLACK); //bottom left
            break;
        case 6:
            M5.Lcd.fillRoundRect(sWidth*0.75, 75, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 92, 4, TFT_BLACK); //top left
             M5.Lcd.fillCircle((sWidth*0.75)+52, 127, 4, TFT_BLACK);  // bottom right 
            M5.Lcd.fillCircle((sWidth*0.75)+17, 109, 4, TFT_BLACK);  // middle left
            M5.Lcd.fillCircle((sWidth*0.75)+52, 109, 4, TFT_BLACK);  // middle right
            M5.Lcd.fillCircle((sWidth*0.75)+52, 92, 4, TFT_BLACK);   // top right
            M5.Lcd.fillCircle((sWidth*0.75)+17, 127, 4, TFT_BLACK);//bottom left
            break;
        default:
            M5.Lcd.fillRoundRect(sWidth*0.75, 100, 75, 75, 10, TFT_WHITE);
            break;
    }
}
void drawDice(int die1, int die2){

    switch(die1){
        case 1:
            M5.Lcd.fillRoundRect(sWidth*0.75, 25, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+35, 59, 4, TFT_BLACK);         //y = 75 + 35   
            break;

        case 2:
            M5.Lcd.fillRoundRect(sWidth*0.75, 25, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 42, 4, TFT_BLACK);  //left
             M5.Lcd.fillCircle((sWidth*0.75)+52, 76, 4, TFT_BLACK);  //right
            break;
        case 3:
            M5.Lcd.fillRoundRect(sWidth*0.75, 25, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 42, 4, TFT_BLACK); //left
            M5.Lcd.fillCircle((sWidth*0.75)+35, 59, 4, TFT_BLACK);  //middle
             M5.Lcd.fillCircle((sWidth*0.75)+52, 76, 4, TFT_BLACK);  //right
            break;
        case 4:
            M5.Lcd.fillRoundRect(sWidth*0.75, 25, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 42, 4, TFT_BLACK);  //top left
            M5.Lcd.fillCircle((sWidth*0.75)+52, 76, 4, TFT_BLACK); //bottom right
            M5.Lcd.fillCircle((sWidth*0.75)+52, 42, 4, TFT_BLACK);   //top right
            M5.Lcd.fillCircle((sWidth*0.75)+17, 76, 4, TFT_BLACK);  //bottom left
            break;
        case 5:
            M5.Lcd.fillRoundRect(sWidth*0.75, 25, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 42, 4, TFT_BLACK);  //top left
            M5.Lcd.fillCircle((sWidth*0.75)+52, 76, 4, TFT_BLACK); //bottom right
            M5.Lcd.fillCircle((sWidth*0.75)+35, 59, 4, TFT_BLACK);  //middle
            M5.Lcd.fillCircle((sWidth*0.75)+52, 42, 4, TFT_BLACK); //top right
            M5.Lcd.fillCircle((sWidth*0.75)+17, 76, 4, TFT_BLACK); //bottom left
            break;
        case 6:
            M5.Lcd.fillRoundRect(sWidth*0.75, 25, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 42, 4, TFT_BLACK); //top left
             M5.Lcd.fillCircle((sWidth*0.75)+52, 72, 4, TFT_BLACK);  // bottom right 
            M5.Lcd.fillCircle((sWidth*0.75)+17, 59, 4, TFT_BLACK);  // middle left
            M5.Lcd.fillCircle((sWidth*0.75)+52, 59, 4, TFT_BLACK);  // middle right
            M5.Lcd.fillCircle((sWidth*0.75)+52, 42, 4, TFT_BLACK);   // top right
            M5.Lcd.fillCircle((sWidth*0.75)+17, 72, 4, TFT_BLACK);//bottom left
            break;
        default:
            M5.Lcd.fillRoundRect(sWidth*0.75, 25, 70, 70, 10, TFT_WHITE);
            break;
    }
    switch(die2){
        case 1:
            M5.Lcd.fillRoundRect(sWidth*0.75, 125, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+35, 159, 4, TFT_BLACK);         //y = 75 + 35   
            break;

        case 2:
            M5.Lcd.fillRoundRect(sWidth*0.75, 125, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 142, 4, TFT_BLACK);  //left
             M5.Lcd.fillCircle((sWidth*0.75)+52, 176, 4, TFT_BLACK);  //right
            break;
        case 3:
            M5.Lcd.fillRoundRect(sWidth*0.75, 125, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 142, 4, TFT_BLACK); //left
            M5.Lcd.fillCircle((sWidth*0.75)+35, 159, 4, TFT_BLACK);  //middle
             M5.Lcd.fillCircle((sWidth*0.75)+52, 176, 4, TFT_BLACK);  //right
            break;
        case 4:
            M5.Lcd.fillRoundRect(sWidth*0.75, 125, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 142, 4, TFT_BLACK);  //top left
            M5.Lcd.fillCircle((sWidth*0.75)+52, 176, 4, TFT_BLACK); //bottom right
            M5.Lcd.fillCircle((sWidth*0.75)+52, 142, 4, TFT_BLACK);   //top right
            M5.Lcd.fillCircle((sWidth*0.75)+17, 176, 4, TFT_BLACK);  //bottom left
            break;
        case 5:
            M5.Lcd.fillRoundRect(sWidth*0.75, 125, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 142, 4, TFT_BLACK);  //top left
            M5.Lcd.fillCircle((sWidth*0.75)+52, 176, 4, TFT_BLACK); //bottom right
            M5.Lcd.fillCircle((sWidth*0.75)+35, 159, 4, TFT_BLACK);  //middle
            M5.Lcd.fillCircle((sWidth*0.75)+52, 142, 4, TFT_BLACK); //top right
            M5.Lcd.fillCircle((sWidth*0.75)+17, 176, 4, TFT_BLACK); //bottom left
            break;
        case 6:
            M5.Lcd.fillRoundRect(sWidth*0.75, 125, 70, 70, 10, TFT_WHITE);            
            M5.Lcd.fillCircle((sWidth*0.75)+17, 142, 4, TFT_BLACK); //top left
             M5.Lcd.fillCircle((sWidth*0.75)+52, 172, 4, TFT_BLACK);  // bottom right 
            M5.Lcd.fillCircle((sWidth*0.75)+17, 159, 4, TFT_BLACK);  // middle left
            M5.Lcd.fillCircle((sWidth*0.75)+52, 159, 4, TFT_BLACK);  // middle right
            M5.Lcd.fillCircle((sWidth*0.75)+52, 142, 4, TFT_BLACK);   // top right
            M5.Lcd.fillCircle((sWidth*0.75)+17, 172, 4, TFT_BLACK);//bottom left
            break;
        default:
            M5.Lcd.fillRoundRect(sWidth*0.75, 125, 70, 70, 10, TFT_WHITE);
            break;
    }
    
}
void drawIntroScreen(){
    while(!M5.BtnA.wasPressed() || !M5.BtnC.wasPressed()){
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.setCursor(sWidth/6, sHeight/3);
        M5.Lcd.setTextColor(TFT_WHITE);
        M5.Lcd.setTextSize(3);
        M5.Lcd.println("Whack-A-Mole");
        M5.Lcd.setCursor(sWidth/5, sHeight/2);
        M5.Lcd.setTextSize(2);
        M5.Lcd.println("Select a Player:");
        M5.Lcd.setCursor(sWidth-120, sHeight-55);
        M5.Lcd.drawRect(sWidth- 140, sHeight- 75, 120,55,TFT_RED );  
        M5.Lcd.setTextSize(2);
        M5.Lcd.println("Popper");
        M5.Lcd.setCursor(40, sHeight-55);
        M5.Lcd.drawRect(20, sHeight- 75, 120,55,TFT_GREEN );  
        M5.Lcd.println("Whacker");

        delay(600);

        M5.Lcd.fillRect(sWidth/5, sHeight/2, 200, 20, TFT_BLACK);

        delay(600);
    }
    
    
}
void drawRollDie(){

    M5.Lcd.setCursor(sWidth-80, sHeight-55);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Roll");
    M5.Lcd.drawLine(sWidth - 50, sHeight-35, sWidth- 50, sHeight - 20, TFT_WHITE);
    M5.Lcd.fillTriangle(sWidth - 60, sHeight-20, sWidth- 40, sHeight- 20, sWidth- 50, sHeight -10, TFT_WHITE);

}
void drawEndGameScreen() {

    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setCursor(sWidth/5, sHeight/3);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.println("Game Over!");
    // M5.Lcd.setCursor(sWidth/4, sHeight/2);
    // M5.Lcd.setTextSize(3);
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

        if(true){
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
void drawConnectingScreen(){
    M5.Lcd.fillScreen(TFT_DARKGREEN);
    M5.Lcd.setCursor(sWidth/10, sHeight/5);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.println("Looking for\n  a shovel and\n  a hammer...");


    int xOffset = sWidth - (sWidth / 11) - 40 ;
    int yOffset = sHeight - 50;
    int holeRad = 25;  

    M5.Lcd.fillEllipse(xOffset, yOffset, holeRad, holeRad/2, TFT_MAROON);
    M5.Lcd.fillCircle(xOffset-40, yOffset-5, 1, TFT_MAROON);
    M5.Lcd.fillCircle(xOffset-50, yOffset-1, 1, TFT_MAROON);
    M5.Lcd.fillCircle(xOffset-35, yOffset+3, 2, TFT_MAROON);
}