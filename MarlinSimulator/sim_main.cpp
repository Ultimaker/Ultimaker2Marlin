
#include <SDL/SDL.h>
#include <Arduino.h>

#include <avr/io.h>

#include "component/sdcard.h"
#include "component/adc.h"
#include "component/heater.h"
#include "component/serial.h"
#include "component/display_SSD1309.h"
#include "component/display_HD44780.h"
#include "component/led_PCA9632.h"
#include "component/arduinoIO.h"
#include "component/stepper.h"

#include "../Marlin/preferences.h"
#include "../Marlin/UltiLCD2.h"
#include "../Marlin/temperature.h"
#include "../Marlin/stepper.h"

SDL_Surface *screen;

extern int8_t lcd_lib_encoder_pos_interrupt;
extern int8_t encoderDiff;
extern uint8_t __eeprom__storage[4096];

bool cardInserted = true;
int stoppedValue;

void setupGui()
{
    if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
    {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);

    screen = SDL_SetVideoMode(1024, 600, 32, SDL_SWSURFACE);
}

unsigned long lastUpdate = SDL_GetTicks();
int key_delay;
#define KEY_REPEAT_DELAY 3
#define SCALE 3
void guiUpdate()
{
    for(unsigned int n=0; n<simComponentList.size(); n++)
        simComponentList[n]->tick();

    if (SDL_GetTicks() - lastUpdate < 25)
        return;
    lastUpdate = SDL_GetTicks();

    int clickX = -1, clickY = -1;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            key_delay = 0;
            break;
        case SDL_KEYUP:
            // If escape is pressed, quit
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                FILE* f = fopen("eeprom.save", "wb");
                fwrite(__eeprom__storage, sizeof(__eeprom__storage), 1, f);
                fclose(f);
                exit(0);
            }
            if (event.key.keysym.sym == SDLK_p)
            {
                SDL_Surface* tmpSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 128*SCALE,64*SCALE,32, 0, 0, 0, 0);
                SDL_Rect src = {SCALE, SCALE, 128*SCALE,64*SCALE};
                SDL_Rect dst = {0, 0, 128*SCALE,64*SCALE};
                SDL_BlitSurface(screen, &src, tmpSurface, &dst);
                char filename[128];
                static int screenshotNr = 0;
                sprintf(filename, "screen%i.bmp", screenshotNr);
                screenshotNr++;
                SDL_SaveBMP(tmpSurface, filename);
                SDL_FreeSurface(tmpSurface);
            }
            break;
        case SDL_MOUSEMOTION:
            if (event.motion.state)
            {
#ifdef ENABLE_ULTILCD2
                lcd_lib_encoder_pos_interrupt += event.motion.xrel / 2;
#endif
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            clickX = event.button.x;
            clickY = event.button.y;
            break;
        case SDL_QUIT:
            {
                FILE* f = fopen("eeprom.save", "wb");
                fwrite(__eeprom__storage, sizeof(__eeprom__storage), 1, f);
                fclose(f);
                exit(0);
            }
        }
    }
    Uint8* keys = SDL_GetKeyState(NULL);
    writeInput(BTN_ENC, !keys[SDLK_RETURN]);
    if (keys[SDLK_RIGHT])
    {
        if (key_delay == 0)
        {
#ifdef ENABLE_ULTILCD2
            lcd_lib_encoder_pos_interrupt+=2;
#endif
#ifdef ULTIPANEL
            encoderDiff+=2;
#endif
            key_delay = KEY_REPEAT_DELAY;
        }else
            key_delay--;
    }
    if (keys[SDLK_LEFT])
    {
        if (key_delay == 0)
        {
#ifdef ENABLE_ULTILCD2
            lcd_lib_encoder_pos_interrupt-=2;
#endif
#ifdef ULTIPANEL
            encoderDiff-=2;
#endif
            key_delay = KEY_REPEAT_DELAY;
        }else
            key_delay--;
    }

    SDL_FillRect(screen, NULL, 0x000000);
    for(unsigned int n=0; n<simComponentList.size(); n++)
        simComponentList[n]->doDraw();

    SDL_Rect rect;
    rect.w = 32;
    rect.h = 32;
    rect.x = 128 * SCALE + 32;
    rect.y = 32;

    if (clickX >= rect.x && clickX <= rect.x + rect.w && clickY >= rect.y && clickY <= rect.y + rect.h)
        cardInserted = !cardInserted;
    SDL_FillRect(screen, &rect, cardInserted ? 0x00FF00 : 0xFF0000);
    rect.y += 48;
    writeInput(SDCARDDETECT, !cardInserted);

    if (clickX >= rect.x && clickX <= rect.x + rect.w && clickY >= rect.y && clickY <= rect.y + rect.h)
        stoppedValue = !stoppedValue;
    writeInput(SAFETY_TRIGGERED_PIN, stoppedValue);

    SDL_FillRect(screen, &rect, stoppedValue ? 0x00FF00 : 0xFF0000);
    rect.y += 48;

    SDL_Flip(screen);
}

#define PRINTER_DOWN_SCALE 2
class printerSim : public simBaseComponent
{
private:
    stepperSim* x;
    stepperSim* y;
    stepperSim* z;
    stepperSim* e0;
    stepperSim* e1;
    int e0stepPos, e1stepPos;
    int map[X_MAX_LENGTH/PRINTER_DOWN_SCALE+1][Y_MAX_LENGTH/PRINTER_DOWN_SCALE+1];
public:
    printerSim(stepperSim* x, stepperSim* y, stepperSim* z, stepperSim* e0, stepperSim* e1)
    : x(x), y(y), z(z), e0(e0), e1(e1)
    {
        e0stepPos = e0->getPosition();
        e1stepPos = e1->getPosition();
        memset(map, 0, sizeof(map));
    }
    virtual ~printerSim()
    {
    }

    void draw(int _x, int _y)
    {
        drawRect(_x, _y, X_MAX_LENGTH / PRINTER_DOWN_SCALE + 1, Y_MAX_LENGTH / PRINTER_DOWN_SCALE + 1, 0x202020);
        for(unsigned int py=0; py<Y_MAX_LENGTH/PRINTER_DOWN_SCALE; py++)
            for(unsigned int px=0; px<X_MAX_LENGTH/PRINTER_DOWN_SCALE; px++)
                drawRect(_x+px, _y+py, 1, 1, map[px][py] + 0x202020);

        drawRect(_x + X_MAX_LENGTH / PRINTER_DOWN_SCALE + 2, _y, 1, Z_MAX_LENGTH / PRINTER_DOWN_SCALE + 1, 0x202020);

        float pos[3];
        float stepsPerUnit[4] = DEFAULT_AXIS_STEPS_PER_UNIT;
        pos[0] = x->getPosition() / stepsPerUnit[X_AXIS];
        pos[1] = Y_MAX_POS - y->getPosition() / stepsPerUnit[Y_AXIS];
        pos[2] = z->getPosition() / stepsPerUnit[Z_AXIS];

        map[int(pos[0]/PRINTER_DOWN_SCALE)][int(pos[1]/PRINTER_DOWN_SCALE)] += e0->getPosition() - e0stepPos;
        map[int(pos[0]/PRINTER_DOWN_SCALE)][int(pos[1]/PRINTER_DOWN_SCALE)] += e1->getPosition() - e1stepPos;

        e0stepPos = e0->getPosition();
        e1stepPos = e1->getPosition();

        drawRect(_x + pos[0] / PRINTER_DOWN_SCALE, _y + pos[1] / PRINTER_DOWN_SCALE, 1, 1, 0xFFFFFF);
        drawRect(_x + X_MAX_LENGTH / PRINTER_DOWN_SCALE + 2, _y + pos[2] / PRINTER_DOWN_SCALE, 1, 1, 0xFFFFFF);
    }
};


void sim_setup_main()
{
    setupGui();
    sim_setup(guiUpdate);
    adcSim* adc = new adcSim();
    arduinoIOSim* arduinoIO = new arduinoIOSim();
    stepperSim* xStep = new stepperSim(arduinoIO, X_ENABLE_PIN, X_STEP_PIN, X_DIR_PIN, INVERT_X_DIR);
    stepperSim* yStep = new stepperSim(arduinoIO, Y_ENABLE_PIN, Y_STEP_PIN, Y_DIR_PIN, INVERT_Y_DIR);
    stepperSim* zStep = new stepperSim(arduinoIO, Z_ENABLE_PIN, Z_STEP_PIN, Z_DIR_PIN, INVERT_Z_DIR);
    stepperSim* e0Step = new stepperSim(arduinoIO, E0_ENABLE_PIN, E0_STEP_PIN, E0_DIR_PIN, INVERT_E0_DIR);
    stepperSim* e1Step = new stepperSim(arduinoIO, E1_ENABLE_PIN, E1_STEP_PIN, E1_DIR_PIN, INVERT_E1_DIR);
    float stepsPerUnit[4] = DEFAULT_AXIS_STEPS_PER_UNIT;
    xStep->setRange(0, X_MAX_POS * stepsPerUnit[X_AXIS]);
    yStep->setRange(0, Y_MAX_POS * stepsPerUnit[Y_AXIS]);
    zStep->setRange(0, Z_MAX_POS * stepsPerUnit[Z_AXIS]);
    xStep->setEndstops(X_MIN_PIN, X_MAX_PIN);
    yStep->setEndstops(Y_MIN_PIN, Y_MAX_PIN);
    zStep->setEndstops(Z_MIN_PIN, Z_MAX_PIN);
    (new printerSim(xStep, yStep, zStep, e0Step, e1Step))->setDrawPosition(5, 70);
    e0Step->setDrawPosition(130, 100);
    e1Step->setDrawPosition(130, 110);

    (new heaterSim(HEATER_0_PIN, adc, TEMP_0_PIN))->setDrawPosition(130, 70);
    (new heaterSim(HEATER_1_PIN, adc, TEMP_1_PIN))->setDrawPosition(130, 80);
    (new heaterSim(HEATER_BED_PIN, adc, TEMP_BED_PIN, 0.2))->setDrawPosition(130, 90);
    new sdcardSimulation("c:/models/", 5000);
    (new serialSim())->setDrawPosition(150, 0);
#if defined(ULTIBOARD_V2_CONTROLLER) || defined(ENABLE_ULTILCD2)
    i2cSim* i2c = new i2cSim();
    (new displaySDD1309Sim(i2c))->setDrawPosition(0, 0);
    (new ledPCA9632Sim(i2c))->setDrawPosition(1, 66);
#endif
#if defined(ULTIPANEL) && !defined(ULTIBOARD_V2_CONTROLLER)
    (new displayHD44780Sim(arduinoIO, LCD_PINS_RS, LCD_PINS_ENABLE, LCD_PINS_D4, LCD_PINS_D5,LCD_PINS_D6,LCD_PINS_D7))->setDrawPosition(0, 0);
#endif
}
