#include "UltiLCD2.h"
#include "UltiLCD2_lib.h"
#include "cardreader.h"
#include "temperature.h"
#include "pins.h"

#define FILAMENT_REVERSAL_LENGTH 750
#define FILAMENT_REVERSAL_SPEED  25
#define FILAMENT_INSERT_SPEED    5

//#ifdef ENABLE_ULTILCD2
typedef void (*menuFunc_t)();

static void lcd_menu_startup();
static void lcd_menu_main();
static void lcd_menu_print();
static void lcd_menu_material();
static void lcd_menu_change_material_preheat();
static void lcd_menu_change_material_remove();
static void lcd_menu_change_material_remove_wait_user();
static void lcd_menu_change_material_insert_wait_user();
static void lcd_menu_change_material_insert();
static void lcd_menu_maintenance();

menuFunc_t currentMenu = lcd_menu_startup;
uint8_t led_glow = 0;
uint8_t led_glow_dir;

void lcd_init()
{
    lcd_lib_init();
}

#define ENCODER_TICKS_PER_MAIN_MENU_ITEM 4
#define ENCODER_TICKS_PER_FILE_MENU_ITEM 4
#define ENCODER_NO_SELECTION (ENCODER_TICKS_PER_MAIN_MENU_ITEM * 10)
void lcd_update()
{
    if (!lcd_lib_update_ready()) return;
    lcd_lib_buttons_update();
    
    if (led_glow_dir)
    {
        led_glow-=2;
        if (led_glow == 0) led_glow_dir = 0;
    }else{
        led_glow+=2;
        if (led_glow == 128) led_glow_dir = 1;
    }
    
    if (IsStopped())
    {
        lcd_lib_clear();
        lcd_lib_draw_stringP(15, 10, PSTR("ERROR - STOPPED"));
        switch(StoppedReason())
        {
        case STOP_REASON_MAXTEMP:
        case STOP_REASON_MINTEMP:
            lcd_lib_draw_stringP(15, 20, PSTR("Temp sensor"));
            break;
        case STOP_REASON_MAXTEMP_BED:
            lcd_lib_draw_stringP(15, 20, PSTR("Temp sensor BED"));
            break;
        case STOP_REASON_SAFETY_TRIGGER:
            lcd_lib_draw_stringP(15, 20, PSTR("Safety circuit"));
            break;
        }
        lcd_lib_draw_stringP(1, 40, PSTR("Contact:"));
        lcd_lib_draw_stringP(1, 50, PSTR("support@ultimaker.com"));
        lcd_lib_led_color(led_glow,0,0);
        lcd_lib_update_screen();
    }else{
        currentMenu();
    }
}

void lcd_menu_startup()
{
    lcd_encoder_pos = ENCODER_NO_SELECTION;
    
    if (lcd_lib_button_pressed)
    {
        lcd_lib_beep();
        currentMenu = lcd_menu_main;
    }
    currentMenu = lcd_menu_main;
}

void lcd_change_to_menu(menuFunc_t nextMenu)
{
    lcd_lib_beep();
    currentMenu = nextMenu;
    lcd_encoder_pos = ENCODER_NO_SELECTION;
}

void lcd_tripple_menu(const char* left, const char* right, const char* bottom)
{
    if (lcd_encoder_pos != ENCODER_NO_SELECTION)
    {
        if (lcd_encoder_pos < 0)
            lcd_encoder_pos += 3*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
        if (lcd_encoder_pos >= 3*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
            lcd_encoder_pos -= 3*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
    }

    lcd_lib_clear();
    lcd_lib_draw_vline(64, 5, 45);
    lcd_lib_draw_hline(3, 124, 48);

    if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 0)
    {
        lcd_lib_draw_box(3+2, 5+2, 64-3-2, 45-2);
        lcd_lib_set(3+3, 5+3, 64-3-3, 45-3);
        lcd_lib_clear_stringP(33 - strlen_P(left) * 3, 22, left);
    }else{
        lcd_lib_draw_stringP(33 - strlen_P(left) * 3, 22, left);
    }
    
    if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 1)
    {
        lcd_lib_draw_box(64+3+2, 5+2, 124-2, 45-2);
        lcd_lib_set(64+3+3, 5+3, 124-3, 45-3);
        lcd_lib_clear_stringP(64 + 33 - strlen_P(right) * 3, 22, right);
    }else{
        lcd_lib_draw_stringP(64 + 33 - strlen_P(right) * 3, 22, right);
    }
    
    if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 2)
    {
        lcd_lib_draw_box(3+2, 49+2, 124-2, 63-2);
        lcd_lib_set(3+3, 49+3, 124-3, 63-3);
        lcd_lib_clear_stringP(65 - strlen_P(bottom) * 3, 53, bottom);
    }else{
        lcd_lib_draw_stringP(65 - strlen_P(bottom) * 3, 53, bottom);
    }
    
    lcd_lib_update_screen();
}

static void lcd_info_screen(menuFunc_t cancelMenu, menuFunc_t callbackOnCancel = NULL)
{
    if (lcd_encoder_pos != ENCODER_NO_SELECTION)
    {
        if (lcd_encoder_pos < 0)
            lcd_encoder_pos += 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
        if (lcd_encoder_pos >= 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
            lcd_encoder_pos -= 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
    }
    if (lcd_lib_button_pressed && lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 0)
    {
        if (callbackOnCancel) callbackOnCancel();
        lcd_change_to_menu(cancelMenu);
    }

    lcd_lib_clear();
    lcd_lib_draw_hline(3, 124, 48);

    if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 0)
    {
        lcd_lib_draw_box(3+2, 49+2, 124-2, 63-2);
        lcd_lib_set(3+3, 49+3, 124-3, 63-3);
        lcd_lib_clear_stringP(65 - strlen_P(PSTR("CANCEL")) * 3, 53, PSTR("CANCEL"));
    }else{
        lcd_lib_draw_stringP(65 - strlen_P(PSTR("CANCEL")) * 3, 53, PSTR("CANCEL"));
    }
}

static void lcd_progressbar(uint8_t progress)
{
    lcd_lib_draw_box(3, 38, 124, 46);
    
    for(uint8_t n=0; n<progress;n++)
    {
        if (n>120) break;
        uint8_t m = (progress-n-1) % 12;
        if (m < 5)
            lcd_lib_draw_vline(4 + n, 40, 40+m);
        else if (m < 10)
            lcd_lib_draw_vline(4 + n, 40+m-5, 44);
    }
}

static void doCooldown()
{
    for(uint8_t n=0; n<EXTRUDERS; n++)
        setTargetHotend(0, n);
    setTargetBed(0);
    quickStop();//Abort all moves.
}

static void lcd_menu_main()
{
    lcd_lib_led_color(32,32,40);
    lcd_tripple_menu(PSTR("PRINT"), PSTR("MATERIAL"), PSTR("MAINTENANCE"));

    if (lcd_lib_button_pressed)
    {
        if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 0)
        {
            lcd_change_to_menu(lcd_menu_print);
            lcd_encoder_pos = ENCODER_TICKS_PER_FILE_MENU_ITEM / 2;
        }
        else if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 1)
            lcd_change_to_menu(lcd_menu_material);
        else if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 2)
            lcd_change_to_menu(lcd_menu_maintenance);
    }
}

static void lcd_menu_material()
{
    lcd_tripple_menu(PSTR("CHANGE"), PSTR("SETTINGS"), PSTR("RETURN"));

    if (lcd_lib_button_pressed)
    {
        if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 0)
            lcd_change_to_menu(lcd_menu_change_material_preheat);//TODO: If multiple extruders, select extruder.
        else if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 2)
            lcd_change_to_menu(lcd_menu_main);
    }
}
static void lcd_menu_maintenance()
{
    lcd_tripple_menu(PSTR("FIRST RUN"), PSTR("EXPERT"), PSTR("RETURN"));

    if (lcd_lib_button_pressed)
    {
        if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 2)
            lcd_change_to_menu(lcd_menu_main);
    }
}

static void lcd_menu_change_material_preheat()
{
    setTargetHotend(230, 0);
    int16_t temp = degHotend(0) - 20;
    int16_t target = degTargetHotend(0) - 10 - 20;
    if (temp < 0) temp = 0;
    if (temp > target)
    {
        current_position[E_AXIS] = 0;
        plan_set_e_position(current_position[E_AXIS]);
        for(uint8_t n=0;n<4;n++)
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], (n+1)*-FILAMENT_REVERSAL_LENGTH/10/4, (n+1)*FILAMENT_REVERSAL_SPEED/5, 0);//Do the first bit slower
        for(uint8_t n=1;n<10;n++)
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], (n+1)*-FILAMENT_REVERSAL_LENGTH/10, FILAMENT_REVERSAL_SPEED, 0);
        currentMenu = lcd_menu_change_material_remove;
        temp = target;
    }

    uint8_t progress = uint8_t(temp * 125 / target);
    char buffer[10];
    sprintf(buffer, "%i %i", temp, target);
    
    lcd_info_screen(lcd_menu_main, doCooldown);
    lcd_lib_draw_stringP(3, 10, PSTR("Heating printhead"));
    lcd_lib_draw_stringP(3, 20, PSTR("for material removal"));
    lcd_lib_draw_string(3, 30, buffer);

    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_remove()
{
    lcd_info_screen(lcd_menu_main, doCooldown);
    lcd_lib_draw_stringP(3, 20, PSTR("Reversing material"));
    
    if (!blocks_queued())
    {
        lcd_lib_beep();
        led_glow_dir = led_glow = 0;
        currentMenu = lcd_menu_change_material_remove_wait_user;
        lcd_encoder_pos = ENCODER_TICKS_PER_MAIN_MENU_ITEM + ENCODER_TICKS_PER_MAIN_MENU_ITEM/2;
        //Disable the extruder motor so you can pull out the remaining filament.
        disable_e0();
        disable_e1();
        disable_e2();
    }

    long pos = -st_get_position(E_AXIS);
    long targetPos = lround(FILAMENT_REVERSAL_LENGTH*axis_steps_per_unit[E_AXIS]);
    uint8_t progress = (pos * 125 / targetPos);
    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_remove_wait_user()
{
    lcd_lib_led_color(8 + led_glow / 2, 8 + led_glow / 2, 16 + led_glow / 4);

    if (lcd_lib_button_pressed)
    {
        if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 1)
        {
            current_position[E_AXIS] = 0;
            plan_set_e_position(current_position[E_AXIS]);
            lcd_change_to_menu(lcd_menu_change_material_insert_wait_user);
        }
    }
    
    lcd_info_screen(lcd_menu_main, doCooldown);
    lcd_lib_draw_stringP(65 - strlen_P(PSTR("Remove material roll")) * 3, 20, PSTR("Remove material roll"));
    if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 1)
    {
        lcd_lib_draw_box(3+2, 29+2, 124-2, 43-2);
        lcd_lib_set(3+3, 29+3, 124-3, 43-3);
        lcd_lib_clear_stringP(65 - strlen_P(PSTR("READY")) * 3, 33, PSTR("READY"));
    }else{
        lcd_lib_draw_stringP(65 - strlen_P(PSTR("READY")) * 3, 33, PSTR("READY"));
    }
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_insert_wait_user()
{
    lcd_lib_led_color(8 + led_glow / 2, 8 + led_glow / 2, 16 + led_glow / 4);

    if (lcd_lib_button_pressed)
    {
        if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 1)
            lcd_change_to_menu(lcd_menu_change_material_insert);
    }
    if (!blocks_queued())
    {
        current_position[E_AXIS] += 0.5;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], FILAMENT_INSERT_SPEED, 0);
    }
    
    lcd_info_screen(lcd_menu_main, doCooldown);
    lcd_lib_draw_stringP(65 - strlen_P(PSTR("Insert new material")) * 3, 20, PSTR("Insert new material"));
    if (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM == 1)
    {
        lcd_lib_draw_box(3+2, 29+2, 124-2, 43-2);
        lcd_lib_set(3+3, 29+3, 124-3, 43-3);
        lcd_lib_clear_stringP(65 - strlen_P(PSTR("READY")) * 3, 33, PSTR("READY"));
    }else{
        lcd_lib_draw_stringP(65 - strlen_P(PSTR("READY")) * 3, 33, PSTR("READY"));
    }
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_insert()
{
    //TODO
}

static void lcd_menu_print()
{
    if (!IS_SD_INSERTED)
    {
        lcd_info_screen(lcd_menu_main);
        lcd_lib_draw_stringP(30,10, PSTR("No SD CARD!"));
        lcd_lib_draw_stringP(10,25, PSTR("Please insert card"));
        lcd_lib_update_screen();
        card.release();
        return;
    }
    lcd_lib_clear();
    
    if (!card.cardOK)
    {
        lcd_info_screen(lcd_menu_main);
        lcd_lib_draw_stringP(0,16, PSTR("Reading card..."));
        lcd_lib_update_screen();
        card.initsd();
        if (!lcd_lib_update_ready()) return;
        lcd_lib_clear();
    }
    
    uint8_t nrOfFiles = card.getnrfilenames() + 1;
    if (nrOfFiles == 1)
    {
        lcd_info_screen(lcd_menu_main);
        lcd_lib_draw_stringP(20, 25, PSTR("No files found!"));
        lcd_lib_update_screen();
        return;
    }
    
    static int16_t viewPos = 0;
    if (lcd_encoder_pos < 0) lcd_encoder_pos += nrOfFiles * ENCODER_TICKS_PER_FILE_MENU_ITEM;
    if (lcd_encoder_pos >= nrOfFiles * ENCODER_TICKS_PER_FILE_MENU_ITEM) lcd_encoder_pos -= nrOfFiles * ENCODER_TICKS_PER_FILE_MENU_ITEM;
    
    uint8_t selIndex = uint16_t(lcd_encoder_pos/ENCODER_TICKS_PER_FILE_MENU_ITEM);

    if (lcd_lib_button_pressed)
    {
        if (selIndex == 0)
        {
            if (card.atRoot())
            {
                lcd_change_to_menu(lcd_menu_main);
            }else{
                card.updir();
            }
        }else{
            card.getfilename(selIndex - 1);
            if (!card.filenameIsDir)
            {
                //Start print
                card.openFile(card.filename, true);
                //lcd_change_to_menu(lcd_menu_print_start);
            }else{
                lcd_lib_beep();
                card.chdir(card.filename);
                lcd_encoder_pos = 0;
            }
            return;//Return so we do not continue after changing the directory or selecting a file. The nrOfFiles is invalid at this point.
        }
    }

    int16_t targetViewPos = selIndex * 8 - 15;

    if (targetViewPos < 0) targetViewPos += nrOfFiles * 8;
    if (targetViewPos >= nrOfFiles * 8) targetViewPos -= nrOfFiles * 8;
    int16_t viewDiff = targetViewPos - viewPos;
    if (viewDiff > nrOfFiles * 4) viewDiff = (targetViewPos - nrOfFiles * 8) - viewPos;
    if (viewDiff < -nrOfFiles * 4) viewDiff = (targetViewPos + nrOfFiles * 8) - viewPos;
    viewPos += viewDiff / 4;
    if (viewDiff > 0) viewPos ++;
    if (viewDiff < 0) viewPos --;
    if (viewPos < 0) viewPos += nrOfFiles * 8;
    if (viewPos >= nrOfFiles * 8) viewPos -= nrOfFiles * 8;
    
    char selectedName[LONG_FILENAME_LENGTH];
    uint8_t selectedType = 2;
    uint8_t drawOffset = 10 - (uint16_t(viewPos) % 8);
    uint8_t itemOffset = uint16_t(viewPos) / 8;
    for(uint8_t n=0; n<6; n++)
    {
        uint8_t itemIdx = n + itemOffset;
        if (itemIdx >= nrOfFiles * 2)
            itemIdx += nrOfFiles;
        while(itemIdx >= nrOfFiles)
            itemIdx -= nrOfFiles;

        char* ptr;
        if (itemIdx == 0)
        {
            if (card.atRoot())
            {
                ptr = ("<RETURN>");
                if (itemIdx == selIndex)
                {
                    strcpy_P(selectedName, PSTR("Return to main menu"));
                    selectedType = 2;
                }
            }else{
                ptr = ("<- BACK");
                if (itemIdx == selIndex)
                {
                    strcpy_P(selectedName, PSTR("Previous directory"));
                    selectedType = 2;
                }
            }
        }else{
            card.getfilename(itemIdx - 1);
            ptr = card.longFilename[0] ? card.longFilename : card.filename;
            if (!card.filenameIsDir)
            {
                if (strchr(ptr, '.')) strchr(ptr, '.')[0] = '\0';
            }
            if (itemIdx == selIndex)
            {
                strcpy(selectedName, ptr);
                selectedType = card.filenameIsDir ? 1 : 0;
            }
            ptr[10] = '\0';
        }
        if (itemIdx == selIndex)
        {
            lcd_lib_set(3, drawOffset+8*n-1, 62, drawOffset+8*n+7);
            lcd_lib_clear_string(4, drawOffset+8*n, ptr);
        }else{
            lcd_lib_draw_string(4, drawOffset+8*n, ptr);
        }
    }
    selectedName[20] = '\0';

    lcd_lib_set(3, 0, 62, 8);
    lcd_lib_clear(3, 47, 62, 63);
    lcd_lib_clear(3, 9, 62, 9);

    lcd_lib_draw_vline(64, 5, 45);
    lcd_lib_draw_hline(3, 124, 48);
    
    lcd_lib_clear_stringP(10, 1, PSTR("SD CARD"));

    lcd_lib_draw_string(5, 53, selectedName);
    if (selectedType == 0)
    {
        lcd_lib_draw_stringP(70, 5, PSTR("FILE"));
        lcd_lib_draw_stringP(70,13, PSTR("INFO"));
    }
    else if (selectedType == 1)
    {
        lcd_lib_draw_stringP(70, 5, PSTR("FOLDER"));
        lcd_lib_draw_stringP(70,13, PSTR("ICON"));
    }
    else if (selectedType == 2)
    {
        lcd_lib_draw_stringP(70, 5, PSTR("RETURN"));
        lcd_lib_draw_stringP(70,13, PSTR("ICON"));
    }
    lcd_lib_update_screen();
}

/* Warning: This function is called from interrupt context */
void lcd_buttons_update()
{
    lcd_lib_buttons_update_interrupt();
}

//#endif//ENABLE_ULTILCD2
