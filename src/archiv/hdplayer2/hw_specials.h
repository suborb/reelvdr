#ifndef HW_SPECIALS_H_INCLUDED
#define HW_SPECIALS_H_INCLUDED
int correct_focus_field(void);
void RealtimePrioOff();
void RealtimePrioOn();
int init_vcxo(void);
void set_vcxo(int pwm);

#endif
