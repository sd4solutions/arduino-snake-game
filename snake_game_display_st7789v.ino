/*******************************************************************************
  Arduino snake game 
  https://github.com/sd4solutions/arduino-snake-game

  This code is provided by SD4Solutions di Domenico Spina
  Email me: info@sd4solutions.com
  https://github.com/sd4solutions
  Please leave my logo and my copyright then use as you want
  
  Thanks to:
  https://github.com/adafruit/Adafruit-GFX-Library
  https://github.com/adafruit/Adafruit-ST7735-Library/

   
  DISPLAY ST7789V 240X320

  Display PIN:
    VCC:  5V
    GND   GND
    DIN:  SPI DATA IN - arduino d11 (MOSI)
    CLK:  SPI CLOCK IN  - arduino d13 (SCK)
    CS :  ACTIVE LOW - arduino pin (9)
    DC :  DATA/COMMAND (HIGH DATA, LOW COMMAND) - arduino pin (8)
    RST:  RESET LOW ACTIVE - arduino pin (rst)
    BL :  5V
        
 ******************************************************************************/

#include <SPI.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_GFX.h>
#include <EEPROM.h>
#include "sprites.h"

//definizione PIN DISPLAY
#define display_CS        9
#define display_RST      -1 // -1 for arduino pin reset, or any other pin value to use as display reset
#define display_DC        8
Adafruit_ST7789 display = Adafruit_ST7789(display_CS, display_DC, display_RST);

//buzzer pin arduino
#define buzzer 10

//movement pin arduino
#define up 4
#define down 5
#define right 6
#define left 3

//init values
bool is_up = false, is_down = false, is_left = false, is_right = false;
short x_direction = 0; //-1 0 1
short y_direction = 0; //-1 0 1


//game area
int offsetX = 0;
int offsetY = 80; //offset for game area
int max_width = 240;
int max_height = 240;

//some vars
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
unsigned long speed = 350; //game speed
unsigned long delayTime = speed;

unsigned long max_score = 0;
unsigned long your_score = 0;
int up_score = 15;
bool over_max_score = false;

//snake
short snake_steps = 1;
short start_snake_size = 5;
short snake_size = start_snake_size;
uint16_t snake_width = 10;
uint16_t snake_height = 10;
int start_x = 120;
int start_y = 120;
uint16_t snake_color = ST77XX_MAGENTA;
uint16_t food_color = ST77XX_YELLOW;

//struct snake
struct snakeItem {
  int X; // x position
  int Y;  //y position
};

//max snake size
snakeItem snake[200];

//food is a snake struct
snakeItem food;

int current_x = 0;
int current_y = 0;

bool stop_game = false;
bool game_over = false;
bool in_game_over = false;


void setup(void) {

  Serial.begin(9600);

  //only first time to write some max score
  //writeEepromScore(225);
  
  pinMode(up, INPUT);pinMode(down, INPUT);pinMode(left, INPUT);pinMode(right, INPUT); pinMode(buzzer, OUTPUT);
  
  randomSeed(analogRead(0));
  
  display.init(240, 320);

  //sd4solutions
  display.fillScreen(ST77XX_BLACK);
  display.drawBitmap(0, 0, sd4solutions, 240, 320, ST77XX_WHITE);
  delay(3000);
  initGame();
}

void writeEepromScore(long score)
{ 
  int address = 0;
  EEPROM.write(address, (score >> 24) & 0xFF);
  EEPROM.write(address + 1, (score >> 16) & 0xFF);
  EEPROM.write(address + 2, (score >> 8) & 0xFF);
  EEPROM.write(address + 3, score & 0xFF);
}

long getEepromScore(){

  int address = 0;
  return ((long)EEPROM.read(address) << 24) +
   ((long)EEPROM.read(address + 1) << 16) +
   ((long)EEPROM.read(address + 2) << 8) +
   (long)EEPROM.read(address + 3);
}

void initGame(){
  
  display.fillScreen(ST77XX_BLACK);

  //we read maxscore from eeprom
  long score = getEepromScore();
  if(score>0)
    max_score = score;    

  //empty snake
  for(int i=0;i<snake_size;i++)
  {
      snake[i].X = -1;
      snake[i].Y = -1;
  }
  
  snake_size = start_snake_size;
  your_score = 0;
  delayTime = speed;
  over_max_score = false;
  
  int dir = random(1, 4);
  x_direction = (dir==1)?1:(dir==2)?-1:0;
  y_direction = (dir==3)?1:(dir==4)?-1:0;

  drawGameArea();
  drawSnake();
  drawFood();
}

void drawGameArea(){
  
  display.drawRect(0, 80, 240, 240, ST77XX_GREEN);
  display.setTextColor(ST77XX_GREEN);
  display.setTextSize(4);
  display.setCursor(60,0);
  display.print("SNAKE");

  display.setTextColor(ST77XX_ORANGE);
  display.setTextSize(2);
  display.setCursor(0, 35);
  display.print("Max Score");
  display.setCursor(0, 55);
  display.setTextColor(ST77XX_ORANGE);
  display.setTextSize(3);
  display.print(max_score);


  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(2);
  drawRightString("Your Score", 240,35);
  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(3);
  drawRightString(String(your_score), 240,55);
}



void updateScore(){

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(String(your_score), 240,55 , &x1, &y1, &w, &h);
  display.fillRect(240-w,55, w , h, ST77XX_BLACK);
  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(3);    
  your_score+=up_score;
  drawRightString(String(your_score), 240,55);

  //speedup snake after %10 size
  if(snake_size%10)
  {
    delayTime-=5;
    if(delayTime<100)
        delayTime = 100; //minimum value of speed
  }
   
}

void drawRightString(const String &text, int x, int y)
{
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
    display.setCursor(x - w, y);
    display.print(text);
}

void drawSnake(){

    int x = start_x+offsetX;
    int y = start_y+offsetY;

    current_x = x;
    current_y = y;
    
    for(int i=0;i<start_snake_size;i++)
    {
        snake[i].X = x;
        snake[i].Y = y;

        if(i==0)
        {
           display.drawRoundRect(snake[i].X, snake[i].Y, snake_width, snake_height, 3, snake_color);
        }
        else{
           display.fillRoundRect(snake[i].X, snake[i].Y, snake_width, snake_height, 3, snake_color);
        }
        
        x+= snake_width*x_direction*-1;    
        y+= snake_height*y_direction*-1;
    }    

}

void drawFood(){

  do {
    food.X = random(offsetX+snake_width, max_width-snake_width);
  } 
  while (food.X % snake_width != 0);
  do {
    food.Y = random(offsetY+snake_height, max_height-snake_height);
  }
  while (food.Y % snake_height != 0);

   display.fillRoundRect(food.X, food.Y, snake_width, snake_height, 5, food_color);
   tone(buzzer, 1500,100);
}

//we check only collition from head
void checkCollitions(){

    //food
    for(int i=0;i<snake_size;i++)
    {
        if(food.X==snake[i].X && food.Y==snake[i].Y)
        {
           tone(buzzer, 750,250);
           snake_size++;
           food.X = -1;
           food.Y = -1;        
           drawFood();
           updateScore();
           return;
        }
    }
   
    //playgroud
    if(snake[0].X>=(max_width-snake_width+offsetX) || (snake[0].X)<(0+offsetX+snake_width) || snake[0].Y>=(max_height-snake_height+offsetY) || snake[0].Y<(0+offsetY+snake_height))
    {
        game_over = true;
        stop_game = true;      
    }

    //with snake itself
    for(int i=1;i<snake_size;i++)
    {
        if(snake[0].X==snake[i].X && snake[0].Y==snake[i].Y)
        {
           game_over = true;
           stop_game = true; 
           break;
        }
    }
}

void moveSnake(){

    int head_x = 0;
    int head_y = 0;

    if(stop_game)
        return;
    
    if(millis()-previousMillis >delayTime)
    {
        tone(buzzer, 80,25);
        previousMillis = millis();
        
        //calculate new position
        current_x= snake[0].X+snake_width*x_direction;
        current_y= snake[0].Y+snake_height*y_direction;

        //remove last item
        display.fillRoundRect(snake[snake_size-1].X , snake[snake_size-1].Y, snake_width, snake_height, 3, ST77XX_BLACK);       
        
        //move all except head
        for(int i=(snake_size-1);i>0;i--)
        {
            snake[i].X = snake[i-1].X;
            snake[i].Y = snake[i-1].Y;
            display.fillRoundRect(snake[i].X , snake[i].Y, snake_width, snake_height, 3, snake_color);
        }

        //move the head
        snake[0].X = current_x;
        snake[0].Y = current_y;
        display.drawRoundRect(current_x, current_y, snake_width, snake_height, 3, snake_color);
                
    }
}

void readButtons(){
    is_up = digitalRead(up);
    is_down = digitalRead(down);
    is_left = digitalRead(left);
    is_right = digitalRead(right);

    if(in_game_over && game_over && (is_up || is_down || is_left || is_right))
    {
      in_game_over  = false;
      game_over = false;
      stop_game = false; 
      initGame();
      return;
    }
      
    if(is_up && !y_direction){
      y_direction = -1;
      x_direction = 0;
    }
    if(is_down && !y_direction){
      y_direction = 1;
      x_direction = 0;
    }
    if(is_right && !x_direction){
      y_direction = 0;
      x_direction = 1;
    }
    if(is_left && !x_direction){
      y_direction = 0;
      x_direction = -1;
    }
}

void displayGameOver()
{
  if(in_game_over || !game_over)
      return;

  over_max_score = your_score>max_score;

  if(over_max_score)
  {
    writeEepromScore(your_score);
  }
  
  if(over_max_score)
  {
    display.fillRect(0, 80, 240, 240, ST77XX_GREEN); 
  }
  else
  {
    display.fillRect(0, 80, 240, 240, ST77XX_RED);
  }
  
  display.setTextColor(ST77XX_BLACK);
  display.setTextSize(4);
  display.setCursor(18,170);
  if(over_max_score)
  {
    display.print("-YOU WIN-");
  }
  else
  {
    display.print("GAME OVER");
  }

  tone(buzzer, 3000);
  delay(250);
  tone(buzzer, 1500);
  delay(250);
  tone(buzzer, 750);
  delay(250);
  tone(buzzer, 250);
  delay(1000);
  noTone(buzzer);
  
  in_game_over = true;  
  
  display.setCursor(6,220);
  display.setTextSize(2);
  display.print("press a key to play");
  
}

void loop() {
  readButtons();
  checkCollitions();
  moveSnake();
  displayGameOver();
}
