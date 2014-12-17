 /**************************************************************************
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// fs453settings.c 

#include <string.h>
#include <vdr/remote.h>
#include <vdr/tools.h>
#include "BspCommChannel.h"
#include "HdCommChannel.h"
#include "fs453settings.h"
#include "reelbox.h"
#include "ReelBoxMenu.h"
#include <math.h>

#define MAXVAL 40

using Reel::RBSetup;

//--const-------------------------------------------------------------------

const char* fs453_settings_tab[FS453_SETTINGS];
int fs453_maxval_tab[FS453_SETTINGS];
int fs453_scaling_tab[FS453_SETTINGS];
int fs453_defaultval_tab[FS453_SETTINGS];

//---cFs453Settings----------------------------------------------------------

const char *cFs453Settings::fs453_set_tab[FS453_SETTINGS];
int cFs453Settings::currentset = 0; 

cFs453Settings::cFs453Settings(cPlugin *pl)
:cOsdObject(true),osd(NULL), fs453(NULL), keepsettings(false)
{ 

    if (!RBSetup.usehdext) {
      //fs453_settings_tab = { "brightness", "contrast", "colour", "sharpness", "gamma cor.", "flicker" };
      fs453_settings_tab[0] = trNOOP("brightness");
      fs453_settings_tab[1] = trNOOP("contrast");
      fs453_settings_tab[2] = trNOOP("colour");
      fs453_settings_tab[3] = trNOOP("sharpness");
      fs453_settings_tab[4] = trNOOP("gamma cor.");
      fs453_settings_tab[5] = trNOOP("flicker");
       //fs453_maxval_tab = { 1000, 1000, 1000, 256, 256, 16 };
      fs453_maxval_tab[0] = 1000;
      fs453_maxval_tab[1] = 1000;
      fs453_maxval_tab[2] = 1000;
      fs453_maxval_tab[3] = 256;
      fs453_maxval_tab[4] = 256;
      fs453_maxval_tab[5] = 16;
       //fs453_scaling_tab = { 1, 1, 1, 1, 128, 1 };
      fs453_scaling_tab[0] = 1;
      fs453_scaling_tab[1] = 1;
      fs453_scaling_tab[2] = 1;
      fs453_scaling_tab[3] = 128;
      fs453_scaling_tab[4] = 1;
      fs453_scaling_tab[5] = 1;
       //fs453_defaultval_tab = { 400, 500, 450, 128, 128, 0 };
      fs453_defaultval_tab[0] = fs453_defaultval_tab0_BSP;
      fs453_defaultval_tab[1] = fs453_defaultval_tab1_BSP;
      fs453_defaultval_tab[2] = fs453_defaultval_tab2_BSP;
      fs453_defaultval_tab[3] = fs453_defaultval_tab3_BSP;
      fs453_defaultval_tab[4] = fs453_defaultval_tab4_BSP;
      fs453_defaultval_tab[5] = fs453_defaultval_tab5_BSP;
    } else {
      //fs453_settings_tabs = { "brightness", "contrast", "gamma cor.", NULL, NULL, NULL };
      fs453_settings_tab[0] = trNOOP("brightness");
      fs453_settings_tab[1] = trNOOP("contrast");
      fs453_settings_tab[2] = trNOOP("gamma cor.");
      fs453_settings_tab[3] = NULL;
      fs453_settings_tab[4] = NULL;
      fs453_settings_tab[5] = NULL;
      //fs453_maxval_tab = { 127, 255, 195, 0, 0, 0 };
      fs453_maxval_tab[0] = 127;
      fs453_maxval_tab[1] = 255;
      fs453_maxval_tab[2] = 195;
      fs453_maxval_tab[3] = 0;
      fs453_maxval_tab[4] = 0;
      fs453_maxval_tab[5] = 0;
      //fs453_scaling_tab = { 1, 1, 1, 0, 0, 0 };
      fs453_scaling_tab[0] = 1;
      fs453_scaling_tab[1] = 1;
      fs453_scaling_tab[2] = 1;
      fs453_scaling_tab[3] = 0;
      fs453_scaling_tab[4] = 0;
      fs453_scaling_tab[5] = 0;
      //fs453_defaultval_tab = { 64, 120, 88, 0, 0, 0 };
      fs453_defaultval_tab[0] = fs453_defaultval_tab0_HD;
      fs453_defaultval_tab[1] = fs453_defaultval_tab1_HD;
      fs453_defaultval_tab[2] = fs453_defaultval_tab2_HD;
      fs453_defaultval_tab[3] = fs453_defaultval_tab3_HD;
      fs453_defaultval_tab[4] = fs453_defaultval_tab4_HD;
      fs453_defaultval_tab[5] = fs453_defaultval_tab5_HD;
     }

    LoadSettings();
    for (int i=0; fs453_settings_tab[i] && i < FS453_SETTINGS; i++)
    {
        fs453_set_tab[i] = tr(fs453_settings_tab[i]);
        displayBars[i] = NULL; 
        lastval[i] = currentval[i];
    }
    lastset = currentset;
    parent=pl;
}

cFs453Settings::~cFs453Settings()
{
    delete osd; 
    for (int i=0; fs453_settings_tab[i] && i < FS453_SETTINGS; i++)
        if (displayBars[i])
            delete displayBars[i]; 
    Store();
}

void cFs453Settings::LoadSettings()
{
    if (!RBSetup.usehdext) {
      currentval[2] = RBSetup.colour;
      //currentval[3] = (RBSetup.sharpness  * MAXVAL+(MAXVAL-1)) / fs453_maxval_tab[3]; //??????
      //currentval[4] = (RBSetup.gamma      * MAXVAL+(MAXVAL-1)) / fs453_maxval_tab[4]; //??????
      currentval[3] = RBSetup.sharpness;
      currentval[4] = RBSetup.gamma;
      currentval[5] = RBSetup.flicker;
    } else {
      currentval[2] = RBSetup.gamma;
    }
    currentval[0] = RBSetup.brightness;
    currentval[1] = RBSetup.contrast;
#ifdef NOT_THEME_LIKE
    backgroundColor = clrGray50;
    fgColor = clrBlue;
    frameColor =  clrBlue;
#else
    frameColor = Skins.Current()->Theme()->Color("clrWhiteText");
    backgroundColor = Skins.Current()->Theme()->Color("clrBackground");
    fgColor = Skins.Current()->Theme()->Color("clrWhiteText");
#endif

}

void cFs453Settings::SaveSettings()
{
    parent->SetupStore("Brightness",   RBSetup.brightness);
    parent->SetupStore("Contrast",     RBSetup.contrast);
    parent->SetupStore("Gamma",        RBSetup.gamma);
    if(!RBSetup.usehdext){
      parent->SetupStore("Sharpness",    RBSetup.sharpness);
      parent->SetupStore("Colour",       RBSetup.colour);
      parent->SetupStore("Flicker",      RBSetup.flicker);
    }
    /* write config file */
    ::Setup.Save();
}

void cFs453Settings::SendFS453Cmd(int currentsetx, int currentvalx)
{
    RBSetup.brightness = currentval[0];
    RBSetup.contrast   = currentval[1];
    if (!RBSetup.usehdext) {
      RBSetup.colour     = currentval[2];
      RBSetup.sharpness  = currentval[3];
      RBSetup.gamma      = currentval[4];
      RBSetup.flicker    = currentval[5];
    } else {
      RBSetup.gamma      = currentval[2];
    }
    if(!RBSetup.usehdext)
      Reel::Bsp::BspCommChannel::SetPicture();
    else
      Reel::HdCommChannel::SetPicture(&RBSetup);
}

void cFs453Settings::Store()
{
        SaveSettings();  
}

void cFs453Settings::Show(void)
{
    if (!osd)
    { 
        int lineHeight = cFont::GetFont(fontSml)->Height();
        int helper = RBSetup.usehdext ? 3+1 : FS453_SETTINGS+1;
        int totalHeight = helper * lineHeight + (helper + 1) * (lineHeight / 2);
        osd = cOsdProvider::NewTrueColorOsd(Setup.OSDLeft, Setup.OSDTop + Setup.OSDHeight - totalHeight, 0, 0);  
        tArea Areas[] = { { 0, 0, Setup.OSDWidth - 1, totalHeight - 1 , 32 } };
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
        osd->DrawRectangle(0, 0,Setup.OSDWidth - 1, totalHeight - 1, backgroundColor); 
        int offset = 0;
        for (int j = 0; fs453_settings_tab[j] && j < FS453_SETTINGS; j++)
        {
           offset = max(cFont::GetFont(fontSml)->Width(fs453_set_tab[j]), offset);
        }
        offset += (lineHeight + lineHeight / 2) +cFont::GetFont(fontSml)->Width("100000") ;
        for (int i = 0; fs453_settings_tab[i] && i < FS453_SETTINGS; i++)
        {
            displayBars[i] = new cSkinDisplayProgbar(osd, offset, i * lineHeight + (i+1) * lineHeight / 2, fs453_set_tab[i]); 
            displayBars[i]->Update(currentval[i], currentset == i, i); 
            SendFS453Cmd(i, currentval[i]);
        } 
        displayBars[helper-1] = new cSkinDisplayButton(osd, offset, (helper-1) * lineHeight + (helper) * lineHeight/2, tr("Reset to default"));
    }
    else
    {
        if (currentset != lastset)
            displayBars[lastset]->Update(currentval[lastset], false, lastset); 
        displayBars[currentset]->Update(currentval[currentset], true, currentset);  
        SendFS453Cmd(currentset, currentval[currentset]);
    }
}

eOSState cFs453Settings::ProcessKey(eKeys Key)
{ 
    switch (Key & ~k_Repeat) {
        case kUp:
            if(currentset > 0) 
            {
                currentset--;
                Show();
                lastset = currentset;
            }
            break; 
        case kDown: 
            if(currentset < (RBSetup.usehdext ? 3 : FS453_SETTINGS)) 
            {
                currentset++;
                Show();
                lastset = currentset;
            }
            break;
       case kRight: 
            if(currentval[currentset] < fs453_maxval_tab[currentset])
            {
                currentval[currentset] = IncrementVal(currentval[currentset], fs453_maxval_tab[currentset]);
                Show();
            }
            break; 
        case kLeft:
            if(currentval[currentset] > 0)
            {
                currentval[currentset] = DecrementVal(currentval[currentset], fs453_maxval_tab[currentset]);
                Show();
            }
            break;
        case kOk:
            if((RBSetup.usehdext && currentset == 3) || (!RBSetup.usehdext && currentset == 6)){
                 if (!RBSetup.usehdext) {
                     RBSetup.brightness = fs453_defaultval_tab0_BSP;
                     RBSetup.contrast   = fs453_defaultval_tab1_BSP;
                     RBSetup.colour     = fs453_defaultval_tab2_BSP;
                     RBSetup.sharpness  = fs453_defaultval_tab3_BSP;
                     RBSetup.gamma      = fs453_defaultval_tab4_BSP;
                     RBSetup.flicker    = fs453_defaultval_tab5_BSP;
                 } else {
                     RBSetup.brightness = fs453_defaultval_tab0_HD;
                     RBSetup.contrast   = fs453_defaultval_tab1_HD;
                     RBSetup.gamma      = fs453_defaultval_tab2_HD;
                     RBSetup.sharpness  = fs453_defaultval_tab3_HD;
                     RBSetup.colour     = fs453_defaultval_tab4_HD;
                     RBSetup.flicker    = fs453_defaultval_tab5_HD;
                 }
		LoadSettings();
                for (int i = 0; fs453_settings_tab[i] && i < FS453_SETTINGS; i++) {
        	    displayBars[i]->Update(currentval[i], i==currentset, i);  
	            SendFS453Cmd(i, currentval[i]);
		}
	        break;
            }
            keepsettings = true;
            return osEnd;
        case kBack:
            return osEnd;
        default: 
            return osContinue;
        }
    return osContinue;
}

int cFs453Settings::GetNextBarNum(int currentval, int maxval)
{
    float step = static_cast<float>(maxval)/MAXVAL;
    int num = static_cast<int>(currentval/step);
    if((float)currentval - num*step > step/2)
    {
        return num + 1;
    }
    return num;
}

int cFs453Settings::IncrementVal(int currentval, int maxval)
{
    if(maxval > 128)
    {
        return currentval + 2;
    }
    return ++currentval;
}

int cFs453Settings::DecrementVal(int currentval, int maxval)
{
    if(maxval > 128)
    {
        return currentval - 2;
    }
    return --currentval;
}

/*int cFs453Settings::IncrementVal(int currentval, int maxval)
{
    float step = static_cast<float>(maxval)/MAXVAL;
    int newval =  static_cast<int>(step * (GetNextBarNum(currentval, maxval) + 1));
    //printf("-------IncrementVal: old = %d, new = %d-------\n", newval, currentval);
    return newval;
}*/

/*int cFs453Settings::DecrementVal(int currentval, int maxval)
{
    float step = static_cast<float>(maxval)/MAXVAL;
    int newval =  static_cast<int>(step * (GetNextBarNum(currentval, maxval) - 1)); 
    //printf("-------DecrementVal: old = %d, new = %d-------\n", newval, currentval); 
    return newval;
}*/

// --- cSkinDisplayProgbar ---------------------------------------------

cSkinDisplayProgbar::cSkinDisplayProgbar(cOsd* Osd, int Xpos, int Ypos, const char* Prompt) 
:cSkinDisplayElem(Osd, Xpos, Ypos, Prompt)
{  
    osd = Osd;
    xpos = Xpos;
    ypos = Ypos;
    prompt = Prompt;
    font = cFont::GetFont(fontSml);
    height = font->Height();
    barwidth = (osd->Width() - height - xpos) / MAXVAL;
    length =  MAXVAL * barwidth;
#ifdef NOT_THEME_LIKE
    backgroundColor = clrGray50;
    textColor = clrYellow;
    upperBarColor = 0xFF248024;
    lowerBarColor = 0xFFBC8024;
    fgColor = clrBlue;
    frameColor =  clrBlue;
#else
    frameColor = Skins.Current()->Theme()->Color("clrWhiteText");
    backgroundColor = Skins.Current()->Theme()->Color("clrBackground");
    textColor = Skins.Current()->Theme()->Color("clrWhiteText");
    fgColor = Skins.Current()->Theme()->Color("clrWhiteText");
    upperBarColor =  Skins.Current()->Theme()->Color("clrButtonGreenBg");
    if(!upperBarColor) upperBarColor = 0xFF248024;
    lowerBarColor = Skins.Current()->Theme()->Color("clrButtonRedBg");
    if(!lowerBarColor) lowerBarColor = 0xFFBC8024;
#endif
    Draw();
}

void cSkinDisplayProgbar::Draw()
{  
    osd->DrawText(height / 2, ypos, prompt, textColor , backgroundColor, font);  
    osd->DrawRectangle(xpos, ypos, xpos + length - 1, ypos +  height - 1, frameColor);
    osd->DrawEllipse(xpos + length + height / 2, ypos, xpos + length + height, ypos + height - 1, frameColor, 5);

    Flush();
}

void cSkinDisplayProgbar::Flush()
{  
    osd->Flush();
}


void cSkinDisplayProgbar::Update(int currentval, bool active, int set)
{
#ifdef NOT_THEME_LIKE
    if(active)
        lowerBarColor = clrYellow;
    else
        lowerBarColor = 0xFFBC8024; 
#else
    if(active) {
        lowerBarColor = Skins.Current()->Theme()->Color("clrButtonYellowBg");
        if(!lowerBarColor) lowerBarColor = clrYellow;
    } else {
        lowerBarColor = Skins.Current()->Theme()->Color("clrButtonRedBg");
        if(!lowerBarColor) lowerBarColor = 0xFFBC8024;
    }
#endif

    int s = barwidth;
    int w = barwidth - 4; 
    int h = height;
    int o = xpos + 2;
    int color;

    char buf[32];
 
    if (fs453_scaling_tab[set]==1)
        sprintf(buf,"%-4i ", currentval-fs453_defaultval_tab[set]);
    else
        sprintf(buf,"%-1.2f ",0.005+((float)currentval)/fs453_scaling_tab[set]); //??? div by zero when !RBSetup.usehdext?

    osd->DrawRectangle(xpos-cFont::GetFont(fontSml)->Width("100000"), ypos, xpos, ypos+cFont::GetFont(fontSml)->Height(), backgroundColor);
    osd->DrawText(xpos-cFont::GetFont(fontSml)->Width("100000"), ypos, buf, textColor , backgroundColor, font);  

    if (active) // mark at right side
        osd->DrawEllipse(xpos + length + height / 2, ypos, xpos + length + height, ypos + height - 1, lowerBarColor , 5);
    else
        osd->DrawEllipse(xpos + length + height / 2, ypos, xpos + length + height, ypos + height - 1, fgColor , 5);
    for (int i = 0; i < MAXVAL; i++)
    {
        if (i < cFs453Settings::GetNextBarNum(currentval, fs453_maxval_tab[set]))
            color =  lowerBarColor;
        else 
            color =  upperBarColor;
        osd->DrawRectangle(o + i * s, ypos + 2, o + i * s + w - 1, ypos + h - 3, color);
    }

    Flush();
}

// --- cSkinDisplayButton ----------------------------------------------

cSkinDisplayButton::cSkinDisplayButton(cOsd* Osd, int Xpos, int Ypos, const char* Prompt) 
:cSkinDisplayElem(Osd, Xpos, Ypos, Prompt)
{
    osd = Osd;
    xpos = Xpos;
    ypos = Ypos;
    prompt = Prompt;
    font = cFont::GetFont(fontSml);
    height = font->Height();
#ifdef NOT_THEME_LIKE
    backgroundColor = clrGray50;
    textColor = clrYellow;
#else
    backgroundColor = Skins.Current()->Theme()->Color("clrBackground");
    textColor = Skins.Current()->Theme()->Color("clrWhiteText");
#endif
    Draw();
}

void cSkinDisplayButton::Draw()
{
    osd->DrawText(height / 2 + osd->Width()/2 - cFont::GetFont(fontSml)->Width(prompt)/2, ypos, prompt, textColor , backgroundColor, font);  
    Flush();
}

void cSkinDisplayButton::Update(int currentval, bool active, int set)
{
    int clrThemeYellow = clrYellow;
#ifndef NOT_THEME_LIKE
    clrThemeYellow = Skins.Current()->Theme()->Color("clrButtonYellowBg");
#endif
 
    osd->DrawText(height / 2 + osd->Width()/2 - cFont::GetFont(fontSml)->Width(prompt)/2, ypos, prompt, (active ? clrThemeYellow : textColor) , backgroundColor, font);  
    Flush();
}

void cSkinDisplayButton::Flush(void)
{ 
    osd->Flush();
}

