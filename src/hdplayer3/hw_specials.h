#ifndef HW_SPECIALS_H_INCLUDED
#define HW_SPECIALS_H_INCLUDED
int correct_focus_field(void);
void RealtimePrioOff();
void RealtimePrioOn();
int init_vcxo(void);
void set_vcxo(int pwm);
void set_sdtv_aspect(int palntsc, int aspect);
void do_focus_sync(vdec_t *vdec, int64_t pts, int *sdtv_sync_retries, int *skip_next);
void set_gpio(int n,int val);
void hw_set_stc(int id, u64 stc);
u64 hw_get_stc(int id);
void fade(int v1);
#endif
