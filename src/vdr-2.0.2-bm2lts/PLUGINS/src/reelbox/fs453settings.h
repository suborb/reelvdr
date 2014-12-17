 /**************************************************************************
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// fs453settings.h
 
#ifndef FS453SETTINGS_H
#define FS453SETTINGS_H

#include <stdio.h>
#include <vdr/osdbase.h>
#include <vdr/plugin.h>

#define FS453_SETTINGS 6

#define fs453_defaultval_tab0_HD 56     // brightness
#define fs453_defaultval_tab1_HD 128    // contrast
#define fs453_defaultval_tab2_HD 70     // gamma 
#define fs453_defaultval_tab3_HD 0      // sharpness  - currently unused
#define fs453_defaultval_tab4_HD 0      // colour     - currently unused
#define fs453_defaultval_tab5_HD 0      // flicker    - currently unused

#define fs453_defaultval_tab0_BSP 400   // brightness
#define fs453_defaultval_tab1_BSP 500   // contrast
#define fs453_defaultval_tab2_BSP 450   // colour
#define fs453_defaultval_tab3_BSP 128   // sharpness
#define fs453_defaultval_tab4_BSP 128   // gamma cor.
#define fs453_defaultval_tab5_BSP 0     // flicker

class cSkinDisplayElem;
class cSkinDisplayProgbar;
class cSkinDisplayButton;

class cFs453Settings : public cOsdObject {
private:	
    static const char *fs453_set_tab[FS453_SETTINGS];  
    cPlugin *parent;
    static int currentset;
    int currentval[FS453_SETTINGS]; 
    int lastval[FS453_SETTINGS];
    int lastset;
    cOsd *osd;
    FILE *fs453;
    cSkinDisplayElem *displayBars[FS453_SETTINGS];
    void SaveSettings();
    void LoadSettings();
    void SendFS453Cmd(int currentset, int currentval);
    void CloseFs453();
    void Store();
    bool keepsettings;
    virtual void Show();
    int backgroundColor;
    int textColor;
    int upperBarColor;
    int lowerBarColor;
    int fgColor;
    int frameColor;
    
public:
    cFs453Settings(cPlugin *pl);
    virtual ~cFs453Settings();
    eOSState ProcessKey(eKeys Key);    
    static int GetNextBarNum(int currentval, int maxval);
    static int IncrementVal(int currentval, int maxval);
    static int DecrementVal(int currentval, int maxval);
};

class cSkinDisplayElem
{
public:
    virtual void Flush(){}; 
    virtual void Draw(){};
    cSkinDisplayElem(cOsd *Osd, int Xpos, int Ypos, const char *Prompt){};
    virtual ~cSkinDisplayElem(){};
    virtual void Update(int currentval, bool active, int){};
};  

class cSkinDisplayProgbar : public cSkinDisplayElem 
{
    cOsd *osd;
    const cFont* font;
    int xpos, ypos, height, length, barwidth;
    const char* prompt;
    int backgroundColor;
    int textColor;
    int upperBarColor;
    int lowerBarColor;
    int fgColor;
    int frameColor;
public:
    void Flush(); 
    void Draw();
    cSkinDisplayProgbar(cOsd *Osd, int Xpos, int Ypos, const char *Prompt);
    ~cSkinDisplayProgbar(){};
    void Update(int currentval, bool active, int);
};  

class cSkinDisplayButton : public cSkinDisplayElem
{
    cOsd *osd;
    int xpos, ypos, height, length, barwidth;
    const cFont* font;
    const char* prompt;
    int backgroundColor;
    int textColor;
public:
    void Flush(); 
    void Draw();
    cSkinDisplayButton(cOsd *Osd, int xpos, int ypos, const char *prompt);
    ~cSkinDisplayButton(){};
    void Update(int currentval, bool active, int);
};  

#endif // FS453SETTINGS_H 

