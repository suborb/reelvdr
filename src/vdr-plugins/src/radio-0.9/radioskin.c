/*
 * radioskin.c - part of radio.c, a plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "radioskin.h"
#include <vdr/config.h>


const cRadioSkin radioSkin[eRadioSkinMaxNumber] =
{
  {
    // dummy
    "dummy",
    0xFFFCFCFC, // clrTitleBack
    0xFF000000, // clrTitleText
    0x7F000000, // clrBack
    0xFFFCFCFC, // clrText
  },
  {
    // Classic
    "default~classic",
    0xFF00FCFC, // clrTitleBack
    0xFF000000, // clrTitleText
    0x7F000000, // clrBack
    0xFFFCFCFC, // clrText
  },
  {
    // ST:TNG
    "default~sttng",
    0xFFFCC024, // clrTitleBack
    0xFF000000, // clrTitleText
    0x7F000000, // clrBack
    0xFFFCC024, // clrText
  },
  {
    // EgalsTry
    "default~EgalsTry",
    0xDFBEBAC3, // clrTitleBack
    0xFF280249, // clrTitleText
    0xCA280249, // clrBack
    0xDFD4D7DB, // clrText
  },
  {
    // EgalsTry
    "blue~EgalsTry",
    0xDFBEBAC3, // clrTitleBack
    0xFF280249, // clrTitleText
    0xCA2B1B9E, // clrBack
    0xDFCFCFCF, // clrText
  },
  {
    // Enigma
    "default~Enigma",
    0xB84158BC, // clrTitleBack
    0xFFFFFFFF, // clrTitleText
    0xB8DEE5FA, // clrBack
    0xFF000000, // clrText
  },
  {
    // Enigma-DarkBlue
    "DarkBlue~Enigma",
    0xB84158BC, // clrTitleBack
    0xFFFFFFFF, // clrTitleText
    0xB82B2B3C, // clrBack
    0xFFFFFFFF, // clrText
  },
  {
    // Enigma-WineRed
    "WineRed~Enigma",
    0xB8BC5241, // clrTitleBack
    0xFFFFFFFF, // clrTitleText
    0xB8FAE2DE, // clrBack
    0xFF000000, // clrText
  },
  {
    // Enigma-AppleGreen
    "AppleGreen~Enigma",
    0xB847BC41, // clrTitleBack
    0xFFFFFFFF, // clrTitleText
    0xB8E2FADE, // clrBack
    0xFF000000, // clrText
  },
  {
    // Enigma-WomensLike
    "WomensLike~Enigma",
    0xB8BC41B2, // clrTitleBack
    0xFFFFFFFF, // clrTitleText
    0xB8FADEFA, // clrBack
    0xFF000000, // clrText
  },
  {
    // Enigma-YellowSun
    "YellowSun~Enigma",
    0xE5ffd927, // clrTitleBack
    0xFF000000, // clrTitleText
    0xE5fae9bc, // clrBack
    0xFF000000, // clrText
  },
  {
    // Enigma-Black
    "Black~Enigma",
    0xEE37383a, // clrTitleBack
    0xFFDDDDDD, // clrTitleText
    0xEE3e4044, // clrBack
    0xFFDDDDDD, // clrText
  },
  {
    // Enigma-Blue
    "Blue~Enigma",
    0xEE4158BC, // clrTitleBack
    0xFFDDDDDD, // clrTitleText
    0xEE496DCC, // clrBack
    0xFFDDDDDD, // clrText
  },
  {
    // Enigma-Blue2
    "Blue2~Enigma",
    0xEE4158BC, // clrTitleBack
    0xFFDDDDDD, // clrTitleText
    0xEE393D8C, // clrBack
    0xFFDDDDDD, // clrText
  },
  {
    // Enigma-Blue3
    "Blue3~Enigma",
    0xEE4158BC, // clrTitleBack
    0xFFDDDDDD, // clrTitleText
    0xEE333388, // clrBack
    0xFFDDDDDD, // clrText
  },
  {
    // Enigma-Coolblue
    "Coolblue~Enigma",
    0xEEC40000, // clrTitleBack
    0xFFDDDDDD, // clrTitleText
    0xEE000066, // clrBack
    0xFFDDDDDD, // clrText
  },
  {
    // Enigma-Grey
    "Grey~Enigma",
    0xEE5f6064, // clrTitleBack
    0xFFDDDDDD, // clrTitleText
    0xEE3e4044, // clrBack
    0xFFDDDDDD, // clrText
  },
  {
    // Enigma-MoBuntu
    "MoBuntu~Enigma",
    0xDF793809, // clrTitleBack
    0xFFFFFFFF, // clrTitleText
    0xDF200F02, // clrBack
    0xFFFFFFFF, // clrText
  },
  {
    // Enigma-bgw
    "bgw~Enigma",
    0xE5333333, // clrTitleBack
    0xFFDCDCDC, // clrTitleText
    0xE5DCDCDC, // clrBack
    0xFF000000, // clrText
  },
  {
    // DeepBlue
    "default~DeepBlue",
    0xC832557A, // clrTitleBack
    0xFF000000, // clrTitleText
    0xC80C0C0C, // clrBack
    0xFF9A9A9A, // clrText
  },
  {
    // SilverGreen
    "default~SilverGreen",
    0xD9293841, // clrTitleBack
    0xFFB3BDCA, // clrTitleText
    0xD9526470, // clrBack
    0xFFB3BDCA, // clrText
  },
  {
    // LightBlue 16/256
    "default~lightblue",
    0xC88488AA, // clrTitleBack
    0xFFFFFFFF, // clrTitleText
    0xC853567B, // clrBack
    0xFF8488AA, // clrText
  },
  {
    // Soppalusikka
    "default~soppalusikka",
    0xC833AAEE, // clrTitleBack
    0xFF000000, // clrTitleText
    0xC8000066, // clrBack
    0xFFFFFFFF, // clrText
  },
  {
    // Soppalusikka-Mint
    "mint~soppalusikka",
    0xCCBBFFFF, // clrTitleBack
    0xFF000000, // clrTitleText
    0xBB005555, // clrBack
    0xFFFFFFFF, // clrText
  },
  {
    // Soppalusikka-Orange
    "orange~soppalusikka",
    0xDDFF5500, // clrTitleBack
    0xFF000000, // clrTitleText
    0x88111100, // clrBack
    0xFFFFFFFF, // clrText
  },
  {
    // Soppalusikka-Vanilla
    "vanilla~soppalusikka",
    0xFF00FCFC, // clrTitleBack
    0xFF000000, // clrTitleText
    0x7F000000, // clrBack
    0xFFFFFFFF, // clrText
  },
  {
    // Soppalusikka-Blackberry
    "blackberry~soppalusikka",
    0xDD0000C0, // clrTitleBack
    0xFFEEEEEE, // clrTitleText
    0x88111100, // clrBack
    0xFFFFFFFF, // clrText
  },
  {
    // Soppalusikka-Citron
    "citron~soppalusikka",
    0xDDFCC024, // clrTitleBack
    0xFF000000, // clrTitleText
    0xAF101000, // clrBack
    0xFFFFFFFF, // clrText
  },
  {
    // Elchi-default
    "default~Elchi",
    0xCC2BA7F1, // clrTitleBack
    0xFF000000, // clrTitleText
    0x77000066, // clrBack
    0xFFFCFCFC, // clrText
  },
  {
    // Elchi-MVBlack
    "MVBlack~Elchi",
    0xCC0000C0, // clrTitleBack
    0xFFEEEEEE, // clrTitleText
    0x88111100, // clrBack
    0xFFEEBB22, // clrText
  },
  {
    // Elchi-MVWhite
    "MVWhite~Elchi",
    0xCC0000C0, // clrTitleBack
    0xFFEEEEEE, // clrTitleText
    0x88CCCCCC, // clrBack
    0xFF444488, // clrText
  },
  {
    // Elchi-Moose
    "Moose~Elchi",
    0xCC2BA7F1, // clrTitleBack
    0xFF000000, // clrTitleText
    0x77000066, // clrBack
    0xFFFCFCFC, // clrText
  },
  {
    // Elchi_Plugin
    "change~Elchi_Plugin",
    0xC833AAEE, // clrTitleBack
    0xFF000000, // clrTitleText
    0xC8000066, // clrBack
    0xFFFFFFFF, // clrText
  },
  {
    // EgalOrange
    "default~EgalOrange",
    0xDFCC8037, // clrTitleBack
    0xFF202020, // clrTitleText
    0xBF2D4245, // clrBack
    0xDFCFCFCF, // clrText
  },
  {
    // Moronimo, can't test it
    "default~Moronimo",
    0xDF3E5578, // clrTitleBack
    0xFF9BBAD7, // clrTitleText
    0xDF294A6B, // clrBack
    0xFF9A9A9A, // clrText
  },
  {
    // Duotone, can't test it
    "default~DuoTone",
    0xFFFCFCFC, // clrTitleBack
    0x7F000000, // clrTitleText
    0x7F000000, // clrBack
    0xFFFCFCFC, // clrText
  },
  {
    // EgalT2-default
    "default~EgalT2",
    0x9F000000, // clrTitleBack
    0xFFFFFFFF, // clrTitleText
    0x9F000000, // clrBack
    0xFFABABAB, // clrText
  },
  {
    // EgalT2-BluYell
    "bluyell~EgalT2",
    0x9F00005F, // clrTitleBack
    0xFFFCF680, // clrTitleText
    0x9F00005F, // clrBack
    0xAFC4BF64, // clrText
  },
  {
    // EgalT2-Purple
    "purple~EgalT2",
    0xAF3e074c, // clrTitleBack
    0xFFFFFFFF, // clrTitleText
    0xAF3e074c, // clrBack
    0xFFABABAB, // clrText
  },
};

int theme_skin(void)
{
    //printf("vdr-radio: Theme~Skin = %s~%s\n", Setup.OSDTheme, Setup.OSDSkin);
    char *temp;
    int i = 0;
    
    asprintf(&temp, "%s~%s", Setup.OSDTheme, Setup.OSDSkin);
    for (i = eRadioSkinMaxNumber-1; i > 0; i--) {
        if (strstr(temp, radioSkin[i].name) != NULL)
            break;
        }
    free(temp);

    return i;
}


//--------------- End -----------------------------------------------------------------
