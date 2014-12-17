/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// reelbox.h

#ifndef REELBOX_H_INCLUDED
#define REELBOX_H_INCLUDED

#include <vdr/plugin.h>
#include <vdr/status.h>

#ifndef ONLY_REELSKIN
#include "BspCommChannel.h"
#endif

#include "Reel.h"

namespace Reel
{
    class Plugin /* final */ : public ::cPlugin
    {
    public:
        Plugin() NO_THROW;

        /* override */ ~Plugin() NO_THROW;

        /* override */ PCSTR Description() NO_THROW;

        /* override */ bool Initialize() NO_THROW;

        /* override */ bool ProcessArgs(int argc, char *argv[]) NO_THROW;

        /* override */ bool Start() NO_THROW;

        /* override */ PCSTR MainMenuEntry() NO_THROW;

        /* override */ cOsdObject *MainMenuAction() NO_THROW;

        /* override */ bool Service(char const *id, void *data) NO_THROW;

        /* override */ bool SetupParse(PCSTR name, PCSTR value) NO_THROW;

        /* override */ const char* CommandLineHelp(void) NO_THROW;

        /* override */ PCSTR Version() NO_THROW;

        /* override */ cString SVDRPCommand(const char *Cmd, const char *Option, int &ReplyCode) NO_THROW;
        /* override */ bool HasSetupOptions(void)
#ifndef REELVDR
                                { return true; }
#else
                                { return false; }
#endif
        /* override */ cMenuSetupPage* SetupMenu(void);

    private:
        Plugin(Plugin const &); // Forbid copy construction.
        Plugin &operator=(Plugin const &); // Forbid copy assignment.
    };
}


#endif // REELBOX_H_INCLUDED
