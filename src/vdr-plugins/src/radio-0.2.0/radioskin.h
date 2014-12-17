#ifndef __RADIO_SKIN_H
#define __RADIO_SKIN_H


enum eRadioSkin
{
  eRadioSkinDummy,
  eRadioSkinClassicDefault,
  eRadioSkinSttngDefault,
  eRadioSkinEgalsTryDefault,
  eRadioSkinEnigmaDefault,
  eRadioSkinEnigmaDarkBlue,
  eRadioSkinDeepBlueDefault,
  eRadioSkinSilverGreenDefault,
  eRadioSkinLightBlueDefault,
  eRadioSkinSoppalusikkaDefault,
  eRadioSkinSoppalusikkaMint,
  eRadioSkinSoppalusikkaOrange,
  eRadioSkinSoppalusikkaVanilla,
  eRadioSkinElchiDefault,
  eRadioSkinMoronimoDefault,
  eRadioSkinDuotoneDefault,
  eRadioSkinMaxNumber
};

struct cRadioSkin
{
  const char *name;		// Theme~Skin
  int clrTitleBack;	// Title-Background
  int clrTitleText;	// Title-Text
  int clrBack;		// Background
  int clrText;		// Text
};
extern const cRadioSkin radioSkin[eRadioSkinMaxNumber];

int theme_skin(void);


#endif //__RADIO_SKIN_H
