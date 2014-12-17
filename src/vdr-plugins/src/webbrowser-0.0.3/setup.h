/*
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#ifndef ___SETUP_H
#define ___SETUP_H

#include <vdr/menu.h>
// ----------------------------------------------------------------

class cWebbrowserSetup
{
  public:
    cWebbrowserSetup(void);
    char startURL[256];
    char FFstartURL[256];
    int fontSize;
    int optimize;
    int scalepics;
    int menuFontSize;
    int transparency;
    int margin_x;
    int margin_y;
    int X11alpha;
    int mode;
    int center_full;
};

extern cWebbrowserSetup WebbrowserSetup;

class cMenuWebbrowserSetup:public cMenuSetupPage
{
  private:
    char startURL[256];
    char FFstartURL[256];
    int fontSize;
    int menuFontSize;
    int optimize;
    int scalepics;
    int transparency;
    int margin_x;
    int margin_y;
    int X11alpha;
    int mode;
    int center_full;
    virtual void Setup(void);
  protected:
      virtual void Store(void);
    virtual eOSState ProcessKey(eKeys Key);
  public:
      cMenuWebbrowserSetup(void);
};

#endif //___SETUP_H
