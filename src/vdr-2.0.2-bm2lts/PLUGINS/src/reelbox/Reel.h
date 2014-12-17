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

// Reel.h
 
#ifndef REEL_H_INCLUDED
#define REEL_H_INCLUDED

#include <exception>

//#define REEL_DEBUG

#define REEL_STRINGIFY(arg) #arg

#define NO_THROW /*throw()*/

#define FB_DEFAULT_DEVICE "/dev/fb0"

namespace Reel
{
    //const bool useHd = true;

    typedef unsigned char       Byte;
    typedef int                 Int;
    typedef long long           LLong;
    typedef char const         *PCSTR;
    typedef short               Short;
    typedef unsigned int        UInt;
    typedef unsigned long long  ULLong;
    typedef unsigned int        uint;

    const int nullptr = 0;

#define MemoryBarrier() asm volatile("" : : : "memory")

#ifdef REEL_DEBUG

    Int DebugAssertionFailed(PCSTR file, Int line);

#define REEL_ASSERT(exp) (void)((exp)?0:DebugAssertionFailed(__FILE__, __LINE__))
#define REEL_VERIFY(exp) REEL_ASSERT(exp)
#define REEL_IF_DEBUG(exp) (exp)

    bool const ReelDebug = true;

#else

#define REEL_ASSERT(exp) ((void)0)
#define REEL_VERIFY(exp) (exp)
#define REEL_IF_DEBUG(exp) ((void)0)

    bool const ReelDebug = false;

#endif

    template<typename T1, typename T2>
    inline T1 implicit_cast(T2 t2) NO_THROW
    {
        return t2;
    }

    class ReelException : public std::exception
    {
    public:
        ReelException(PCSTR what, int line)
        :what_(what), line_(line)
        {
        }

        /* override */ PCSTR what() const throw()
        {
            return what_;
        }

        int GetLine() const NO_THROW
        {
            return line_;
        }

    private:
        PCSTR what_;
        int   line_;
    };

#define REEL_LOG_EXCEPTION LogException
#define REEL_THROW() throw ReelException(__FILE__, __LINE__)

    void LogException(std::exception const &e) NO_THROW;

    void Restart() NO_THROW;
}

#endif // REEL_H_INCLUDED
