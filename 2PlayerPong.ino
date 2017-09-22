#include "Timer.h" //Download and install the Timer library from here: https://github.com/JChristensen/Timer
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define PIN 6
#define SAVE_SCORES //Comment out to not save or read high scores to/from EEPROM

//Don't change these define statements
#define X 0
#define Y 1

/* 
 *2 player neopixel matrix pong
 *Code made by Arduino "having11" Guy
 *Use addressable neopixel strips
*/

#ifdef SAVE_SCORES

	#include <EEPROM.h>
	const int scoreAddr = 1;

#endif

const int player1Analog = A0;
const int player2Analog = A1;

const int matrixX = 10; //Matrix size goes here
const int matrixY = 12;

const int paddleWidth = 4; //Paddle is 4 pixels wide
const int ballSpeed = 40; //Move ball every 40 ms
const int refreshSpeed = 10; //Update the screen every 10 ms

Timer timer; //Timer object

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(matrixX, matrixY, PIN,
  NEO_MATRIX_BOTTOM     + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);

const uint16_t textColor = matrix.Color(170, 0, 0); //RED
const uint16_t ballColor = matrix.Color(170, 170, 0); //YELLOW
const uint16_t P1PaddleColor = matrix.Color(0, 170, 0); //GREEN
const uint16_t P2PaddleColor = matrix.Color(0, 0, 170); //BLUE
  
const uint16_t colors[5] = {textColor, ballColor, P1PaddleColor, P2PaddleColor, matrix.Color(0,0,0)};
  
int player1[2];
int player2[2];

int oldPlayer1[2];
int oldPlayer2[2];

uint8_t ballDirection[2]; //Can be either 1,1 or -1,-1 (X, Y)
int ballPosition[2];
int oldBallPosition[2];

int scores[2]; //Player 1 score and player 2 score

int timerIDs[2];

void setup()
{
	matrix.begin();
	
	#ifdef SAVE_SCORES
	beginGame(true);
  #endif
  
	#ifndef SAVE_SCORES
	beginGame(false);
	#endif

}

void loop()
{
	timer.update();
}

void printInfo(String toPrint)
{
	int x = matrixX;
	for(int i=0;i<30;i++){
		matrix.fillScreen(0);
		matrix.setCursor(x,2);
		matrix.print(toPrint);
		x --;
		matrix.show();
		delay(150);
	}
	delay(1000);
	matrix.fillScreen(0);
	matrix.show();
}

void beginGame(bool printHS)
{
	randomSeed(analogRead(player1Analog));
	matrix.fillScreen(0); //Clear screen
	matrix.setTextColor(colors[0]);
	
	if(printHS){
		int scoreEEP = EEPROM.read(scoreAddr);
		if(scores[0] > scoreEEP){
			EEPROM.write(scoreAddr, scores[0]);
		}
		else if(scores[1] > scoreEEP){
			EEPROM.write(scoreAddr, scores[1]);
		  }
     scoreEEP = EEPROM.read(scoreAddr);
   
      printInfo("HS: "+String(scoreEEP)); //It may be wrong for the first game
		}
		
	player1[X] = (matrixX / 2) - (paddleWidth / 2); //Put in the middle
	player1[Y] = 0; //Bottom of screen
	
	player2[X] = (matrixX / 2) - (paddleWidth / 2);
	player2[Y] = matrixY - 1; //Put at the top of the screen
	
	ballPosition[X] = random(0, matrixX - 1);
	ballPosition[Y] = random(1, matrixY - 1);
	int ballRand = random(1,3);
	if(ballRand == 1){	
		ballDirection[0] = 1;
		ballDirection[1] = 1;
	}
	else {	
		ballDirection[0] = -1;
		ballDirection[1] = -1;
	}
	
	updateScreen();
	
	delay(1000);
	
	timerIDs[0] = timer.every(ballSpeed, updateBall);
	timerIDs[1] = timer.every(refreshSpeed, updateScreen);
	
}

void updateScreen(){
	
	oldPlayer1[X] = player1[X];
	oldPlayer2[X] = player2[X];
	
	player1[X] = map(analogRead(player1Analog), 0, 1023, 0, matrixX-(paddleWidth - 1));
	player2[X] = map(analogRead(player2Analog), 0, 1023, 0, matrixX-(paddleWidth - 1));
	
	matrix.fillRect(oldPlayer1[X], oldPlayer1[Y], paddleWidth, 1, colors[4]);
	matrix.fillRect(oldPlayer2[X], oldPlayer2[Y], paddleWidth, 1, colors[4]);
	
	matrix.fillRect(player1[X], player1[Y], paddleWidth, 1, colors[2]);
	matrix.fillRect(player2[X], player2[Y], paddleWidth, 1, colors[3]);
	
	matrix.show();
	
	int has_won = checkWon();
	
	switch (has_won){
		case 2: //Player 2 has won
			timer.stop(timerIDs[0]);
      timer.stop(timerIDs[1]);
			matrix.fillScreen(0);
			scores[1] += 1;
			printInfo("Score: "+String(scores[0])+" - "+String(scores[1]));
     #ifdef SAVE_SCORES
        beginGame(true);
     #else
        beginGame(false);
     #endif
			break;
		
		case 1: //Player 1 has won
			timer.stop(timerIDs[0]);
      timer.stop(timerIDs[1]);
			matrix.fillScreen(0);
			scores[0] += 1;
			printInfo("Score: "+String(scores[0])+" - "+String(scores[1]));
			#ifdef SAVE_SCORES
        beginGame(true);
      #else
        beginGame(false);
      #endif
			break;
			
		case 0: //No one has won
			break;
			
  }
}

void updateBall(){
	
	oldBallPosition[X] = ballPosition[X];
	oldBallPosition[Y] = ballPosition[Y];
	
	ballPosition[X] += ballDirection[X];
	ballPosition[Y] += ballDirection[Y];
	
	matrix.drawPixel(oldBallPosition[X], oldBallPosition[Y], colors[4]);
	matrix.drawPixel(ballPosition[X], ballPosition[Y], colors[1]);
	
	bool is_collided = checkCollisions();
	
}

bool checkCollisions(){
	
	if(ballPosition[X] >= player1[X] && ballPosition[X] <= player1[X] + 3){ //Hit paddle
		if(ballPosition[Y] == player1[Y]+1){
			ballDirection[0] = 1;
			ballDirection[1] = 1;
			return true;
		}
	}
	else if(ballPosition[X] >= player2[X] && ballPosition[X] <= player2[X] + 3){
		if(ballPosition[Y] == player2[Y]+1){
			ballDirection[0] = -1;
			ballDirection[1] = -1;
			return true;
		}
	}
	
	else if(ballPosition[X] <= 0 || ballPosition[X] >= matrixX-1){ //Bounce off of wall
		ballDirection[X] = -ballDirection[X];
		return true;
	}
	
	else{ return false;}
	
}

int checkWon(){
	if(ballPosition[Y] < 0){
		return 2;
	}
	else if(ballPosition[Y] > matrixY-1){
		return 1;
	}
	else {return 0;}
}
