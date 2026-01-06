#include "libopencm3/stm32/usart.h"
#include "libopencm3/stm32/rcc.h"   //Needed to enable clocks for particular GPIO ports
#include "libopencm3/stm32/gpio.h"  //Needed to define things on the GPIO
#include "libopencm3/stm32/adc.h" //Needed to convert analogue signals to digital
#include "stdbool.h" // bool data type
#include "stdlib.h" // rand function
#include "string.h" // memset 
#include "stdio.h"
#include "unistd.h"

struct apple {
    int appleX;
    int appleY;
    int appleZ;
};

float getJoystickInput(int);
void sendMessage(int[8][8][8]);
void displayWinAnimation(int[8][8][8]);
void mySleep(int time);
void game(void);
struct apple fastAppleSpawn(int [8][8][8]);
struct apple spawnApple(int [8][8][8]);


int main(void) {
    #define USART_PORT USART1
    #define LEDCUBE_PORT GPIOB
    #define TX_PIN GPIO6
    #define RX_PIN GPIO7
    rcc_periph_clock_enable(RCC_USART1); //Enable clock for RCC_USART1
    rcc_periph_clock_enable(RCC_GPIOB);

    //set B5 to high
    gpio_mode_setup(LEDCUBE_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
    gpio_set(LEDCUBE_PORT, GPIO5);

    // Setting TX pins
    gpio_mode_setup(LEDCUBE_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TX_PIN);
    gpio_set_af(LEDCUBE_PORT, GPIO_AF7, TX_PIN);

    // Setting RX pins
    gpio_mode_setup(LEDCUBE_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, RX_PIN);
    gpio_set_af(LEDCUBE_PORT, GPIO_AF7, RX_PIN);


    usart_set_baudrate(USART_PORT, 9600);
    usart_set_databits(USART_PORT, 8);
    usart_set_stopbits(USART_PORT, USART_STOPBITS_1);
    usart_set_mode(USART_PORT, USART_MODE_TX_RX);
    usart_set_parity(USART_PORT, USART_PARITY_NONE);
    usart_set_flow_control(USART_PORT, USART_FLOWCONTROL_NONE);

    usart_enable_rx_interrupt(USART_PORT);
    usart_enable_tx_interrupt(USART_PORT);

    usart_enable(USART_PORT);

    game();
}

void game(void){
    int snakeLength;
    bool alive = true;
    bool appleEaten = true;
    struct apple appleSpawned;
    int cubeGrid[8][8][8];
    int snakeBody[512][3]; // [3] stores x,y,z coordinates
    int frontPointer = 0;
    int rearPointer = -1;
    snakeBody[0][0] = 4; // snake starts at 4,4,4
    snakeBody[0][1] = 4; 
    snakeBody[0][2] = 4; 
    int direction = 0; // stores the direction that the snake is moving // 0,1,2,3 is +z, -x, -z, +x
    int planeDirection = 0; // 0 when its not moving up or down

    while (alive){ // loops forever
        memset(cubeGrid, 0, sizeof(cubeGrid)); // cubeGrid is resetted every time
        mySleep(500); // Waits for 500ms
        float joystick_A_x = getJoystickInput(1);
        float joystick_B_y = getJoystickInput(7);
        int newX = snakeBody[frontPointer][0];
        int newY = snakeBody[frontPointer][1];
        int newZ = snakeBody[frontPointer][2];
        frontPointer++;
        // adjust rearpointer depending on apple eaten
        if (joystick_A_x < 0.5){
            // for wrapping around use modulus
            newX = (snakeBody[frontPointer][0] + 1) % 8;
            direction = (direction - 1 + 4) % 4; // added by +4 to ensure that it doesn't go below 0
            planeDirection = 0; // resets plane direction
        } else if (joystick_A_x > 1.45){
            // x pos is added by 8 so it never goes below 0
            newX = (snakeBody[frontPointer][0] - 1) % 8;
            direction = (direction + 1 + 4) % 4; // added by +4 to ensure that it doesn't go below 0
            planeDirection = 0;
        } else if (joystick_B_y < 0.5){
            newY = (snakeBody[frontPointer][1] + 1) % 8;
            direction = 0;
            planeDirection = -1;
        } else if (joystick_B_y > 1.45){
            newY = (snakeBody[frontPointer][1] - 1) % 8;
            direction = 0;
            planeDirection = 1;
        } else{ // When no movement input is given. Go in direction or planeDirection
            if (planeDirection == 0){ // if planeDirecton = 0 then move in x or z 
                if (direction % 2 == 0){
                    newZ = (snakeBody[frontPointer][2] + direction + 8) % 8;
                } else{
                    newX = (snakeBody[frontPointer][0] + direction + 8) % 8;
                }
            } else{ // move in y
                newY = snakeBody[frontPointer][1] + planeDirection;
            }
        }
        
        snakeBody[frontPointer][0] = newX;
        snakeBody[frontPointer][1] = newY;
        snakeBody[frontPointer][2] = newZ;

        if (appleEaten){
            appleSpawned = spawnApple(cubeGrid); // Spawn an apple when it is eaten
        }
        else{
            rearPointer++;
        }
        cubeGrid[appleSpawned.appleX][appleSpawned.appleY][appleSpawned.appleZ] = 1; // places apple
        // Apple is eaten if the snake's head position is equal to the apple's position
        appleEaten = appleSpawned.appleX == newX && appleSpawned.appleY == newY && appleSpawned.appleZ == newZ;

        addSnakeToGrid(cubeGrid, snakeBody, frontPointer, rearPointer);
        // Check if the snake is dead after displaying LEDS so the player knows they lost
        sendMessage(cubeGrid);
        snakeLength =(frontPointer - rearPointer + 512) % 512;
        if (checkSnakeCollide(snakeBody, frontPointer, rearPointer)){
            alive = false;
            mySleep(1500); // waits for 1500ms to give time
        }
        if (snakeLength == 512){
            alive = false; 
            displayWinAnimation(cubeGrid);
        }
        
    }
}

void mySleep(int time){
    for (volatile int  i = 0; i < time * 100; i++){
    }
}

struct apple fastAppleSpawn(int cubeGrid[8][8][8]){
  
   // choose random index from cubeGrid
   int randX = rand() % 8;
   int randY = rand() % 8;
   int randZ = rand() % 8;
  
   for (int i = 0; i < 10; i++){ // attempted 10 times
       if (cubeGrid[randX][randY][randZ] == 1){
           // if apple spawns on snake, try again
           randX = rand() % 8;
           randY = rand() % 8;
           randZ = rand() % 8;
       } else {
           struct apple a = {randX, randY, randZ};
           return a; // successfully spawned apple
       }
   }
   struct apple failed = {-1, -1, -1};
   return failed; // failed, so we use the main spawn method now
}

struct apple spawnApple(int cubeGrid[8][8][8]){
    // to make it more efficient, first quick spawn which tries 10 times to spawn apple randomly
    struct apple isAppleSpawned = fastAppleSpawn(cubeGrid);
    if (isAppleSpawned.appleX != -1){
        return isAppleSpawned; // successfully spawned apple quickly
    }

    int candidates[512][3];
    int occ[512][3];
    int i = 0;
    int candCount = 0;
    for (int x = 0; x < 8; ++x){
        for (int y = 0; y < 8; ++y){
            for (int z = 0; z < 8; ++z){
                if (cubeGrid[x][y][z] == 0) {
                    occ[i][0] = x;
                    occ[i][1] = y;
                    occ[i][2] = z;
                    candCount++;
                }
                i++;
            }
        }
    }
    if (candCount == 0) {
        // Cube full -> no valid apple placement.
        struct apple failed = {-1, -1, -1};
        return failed;
    }
    int ri = rand() % candCount;
    int appleX = candidates[ri][0];
    int appleY = candidates[ri][1];
    int appleZ = candidates[ri][2];
    struct apple a = {appleX, appleY, appleZ};
    return a;
}

void displayWinAnimation(int cubeGrid[8][8][8]){
    // Basically just a flickering animation
    for (int i = 4; i > 0; i--){
        for (int x = 0; x < 8; x++){
             for (int y = 0; y < 8; y++){
                 for (int z = 0; z < 8; z++){
                    cubeGrid[x][y][z] = 1;
                }
            }
        }
        // Send message
        mySleep(i * 100); 
        memset(cubeGrid, 0, 512); // Reset cubeGrid
        // Send message
        mySleep(i * 100); 
        
}

void setupJoystick(void){
    rcc_periph_clock_enable(RCC_ADC12); //Enable clock for ADC registers 1 and 2

    adc_power_off(ADC1);  //Turn off ADC register whist we set it up

    adc_set_clk_prescale(ADC1, ADC_CCR_CKMODE_DIV1);  //Setup a scaling, none is fine for this
    adc_disable_external_trigger_regular(ADC1);   //We don't need to externally trigger the register...
    adc_set_right_aligned(ADC1);  //Make sure it is right aligned to get more usable values
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_61DOT5CYC);  //Set up sample time
    adc_set_resolution(ADC1, ADC_CFGR1_RES_12_BIT);  //Get a high resolution so you can detect small changes in the joystick

    adc_power_on(ADC1);  //Finished setup, turn on ADC register 1
}

float getJoystickInput(int channel){
    uint8_t channelArray[] = {channel};  //Define a channel that we want to look at
    adc_set_regular_sequence(ADC1, 1, channelArray);  //Set up the channel
    adc_start_conversion_regular(ADC1);  //Start converting the analogue signal

    while(!(adc_eoc(ADC1)));  //Wait until the register is ready to read data

    float adc_value = (float)adc_read_regular(ADC1);  //Read the value from the register and channel and coverts to float
    float adc_value_scaled = (adc_value / 4095.0f) * 2.0f; // Converts the value so its in range of 0 to 2
    return adc_value_scaled;
}

void sendMessage(int cubeGrid[8][8][8]){
    usart_send_blocking(USART1, 0xF2);
    for (int i = 0; i <8; i++){
        for (int j = 0; j < 8; j++){
            int hexValue = 0;
            for (int k = 0; k < 8; k++){
                hexValue += (cubeGrid[i][j][k] << k);
            }
            usart_send_blocking(USART1, hexValue);
        }
    }
}

void addSnakeToGrid(int cubeGrid[8][8][8], int snakeBody[512][3], int frontPointer, int rearPointer){
    while (frontPointer != rearPointer){
        cubeGrid[snakeBody[frontPointer][0]][snakeBody[frontPointer][1]][snakeBody[frontPointer][2]] = 1; // Changes the coordinate with the snake body to 1
        frontPointer = (frontPointer + 1) % 512; // Wraps around when the end of the array is reached. 
    }
}

bool checkSnakeCollide(int snakeBody[512][3], int frontPointer, int rearPointer){
    int counter = frontPointer;
    while (counter != rearPointer){
        counter = (counter + 1) % 512; // Wraps around when the end of the array is reached. 
        if (snakeBody[counter][0] == snakeBody[frontPointer][0] && snakeBody[counter][1] == snakeBody[frontPointer][1] && snakeBody[counter][2] == snakeBody[frontPointer][2])
            return true; // Returns true if the snake head collides with it's body
    }
    return false; // Returns false when the snake doesn't collide
    }
}


