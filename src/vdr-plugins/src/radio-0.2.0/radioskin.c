/*
 * radioskin.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
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
    // enElchi
    "default~enElchi",
    0xC833AAEE, // clrTitleBack
    0xFF000000, // clrTitleText
    0xC8000066, // clrBack
    0xFFFFFFFF, // clrText
  },
  {
    // Moronimo, can't test it
    "Moronimo",
    0xDF3E5578, // clrTitleBack
    0xFF9BBAD7, // clrTitleText
    0xDF294A6B, // clrBack
    0xFF9A9A9A, // clrText
  },
  {
    // Duotone, can't test it
    "DuoTone",
    0xFFFCFCFC, // clrTitleBack
    0x7F000000, // clrTitleText
    0x7F000000, // clrBack
    0xFFFCFCFC, // clrText
  },
};

int theme_skin(void)
{
    //printf("vdr-radio: Theme~Skin = %s~%s\n", Setup.Theme, Setup.OSDSkin);
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
