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

// Thread.h
 
#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include "Reel.h"

#include <pthread.h>

namespace Reel
{
    template<typename T>
    class Thread /* final */
    {
    public:
        Thread();
        ~Thread() NO_THROW;

        typedef void (T::*MemFunc)();

        void Join();
        void SetSchedParam(int policy, int priority);
        void Start(T &obj, MemFunc memFunc);

    private:
        Thread(Thread const &); // forbid copy construction.
        Thread &operator=(Thread const &); // forbid copy assignment.

        static void *StartProc(void *arg);
    
        T           *obj_;
        MemFunc      memFunc_;
        ::pthread_t  threadId_;
    };

    template<typename T>    
    inline Thread<T>::Thread()
    :   obj_(0)
    {
    }

    template<typename T>
    inline Thread<T>::~Thread()
    {
        ::printf (" ~Thread  ID: %d \n",(int) threadId_);
        REEL_ASSERT(!obj_);
    }

    template<typename T>
    inline void Thread<T>::Join()
    {
        REEL_ASSERT(obj_);
        ::printf (" Join ID: %d \n",(int) threadId_);
        REEL_VERIFY(::pthread_join(threadId_, 0) == 0);
        obj_ = 0;
    }

    template<typename T>
    void Thread<T>::SetSchedParam(int policy, int priority)
    {
#ifdef REEL_DEBUG
        int p;
#endif
        struct ::sched_param param;
        REEL_ASSERT(::pthread_getschedparam(threadId_, &p, &param) == 0);
        param.sched_priority = priority;
        REEL_ASSERT(::pthread_setschedparam(threadId_, policy, &param) == 0);
    }

    template<typename T>
    void Thread<T>::Start(T &obj, MemFunc memFunc)
    {
        obj_ = &obj;
        memFunc_ = memFunc;
        REEL_VERIFY(::pthread_create(&threadId_, 0, &StartProc, this) == 0);
        ::printf (" Start Thread ID: %d \n",(int) threadId_);
    }

    template<typename T>
    void *Thread<T>::StartProc(void *arg)
    {
        Thread const *thread = static_cast<Thread const *>(arg);
        (thread->obj_->*thread->memFunc_)();
        return 0;
    }
}

#endif // THREAD_H_INCLUDED
