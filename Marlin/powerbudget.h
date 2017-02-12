#ifndef POWERBUDGET_H
#define POWERBUDGET_H

// default values for wattage distribution
#ifndef DEFAULT_POWER_BUDGET
  #define DEFAULT_POWER_BUDGET     175
#endif
#ifndef DEFAULT_POWER_EXTRUDER
  #define DEFAULT_POWER_EXTRUDER    35
#endif
#ifndef DEFAULT_POWER_BUILDPLATE
  #define DEFAULT_POWER_BUILDPLATE 150
#endif


// min/max allowed input value
#define POWER_MINVALUE             0
#define POWER_MAXVALUE           999

extern uint16_t power_budget;
extern uint16_t power_buildplate;
extern uint16_t power_extruder[EXTRUDERS];

void PowerBudget_RetrieveSettings();

#ifdef ENABLE_ULTILCD2
// menu function
void lcd_menu_powerbudget();
#endif

#endif //POWERBUDGET_H
