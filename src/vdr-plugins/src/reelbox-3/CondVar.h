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

// CondVar.h
 
#ifndef COND_VAR_H_INCLUDED
#define COND_VAR_H_INCLUDED

#include "Mutex.h"

#include <pthread.h>
#include <sys/time.h>
#include <time.h>

namespace Reel
{
    /**
      * Condition variable class.
      */
    class CondVar /* final */
    {
    public:
        CondVar();
        ~CondVar() NO_THROW;

        void Broadcast(Mutex const &mutex);
        void Wait(Mutex &mutex);
        void Wait(Mutex &mutex, LLong timeoutNs);
        
    private:
        CondVar(CondVar const &); // forbid copy construction.
        CondVar &operator=(CondVar const &); // forbid copy assignment.

        pthread_cond_t cond_;
    };
    
    inline CondVar::CondVar()
    {
        REEL_VERIFY(pthread_cond_init(&cond_, 0) == 0);
    }
    
    inline CondVar::~CondVar() NO_THROW
    {
        REEL_VERIFY(pthread_cond_destroy(&cond_) == 0);
    }
    
    inline void CondVar::Broadcast(Mutex const &mutex)
    {
        REEL_ASSERT(mutex.locked_ == 1);
        REEL_ASSERT(mutex.owner_ == pthread_self());
        REEL_VERIFY(pthread_cond_broadcast(&cond_) == 0);
    }
    
    inline void CondVar::Wait(Mutex &mutex)
    {
        REEL_ASSERT(mutex.locked_ == 1);
        REEL_ASSERT(mutex.owner_ == pthread_self());
        REEL_IF_DEBUG(mutex.locked_ = 0);
        REEL_VERIFY(pthread_cond_wait(&cond_, &mutex.mutex_) == 0);
        REEL_ASSERT(!mutex.locked_);
        REEL_IF_DEBUG(++mutex.locked_);
        REEL_IF_DEBUG(mutex.owner_ = pthread_self());
    }

    inline void CondVar::Wait(Mutex &mutex, LLong timeoutNs)
    {
        REEL_ASSERT(mutex.locked_ == 1);
        REEL_ASSERT(mutex.owner_ == pthread_self());
        REEL_IF_DEBUG(mutex.locked_ = 0);
        
        struct ::timeval now;
        ::gettimeofday(&now, NULL);
        LLong t = now.tv_sec * 1000000000LL + now.tv_usec * 1000LL + timeoutNs;
        struct ::timespec ts;
        ts.tv_sec = t / 1000000000LL;
        ts.tv_nsec = t % 1000000000LL;

#ifdef REEL_DEBUG
        int r = pthread_cond_timedwait(&cond_, &mutex.mutex_, &ts);
#else
        pthread_cond_timedwait(&cond_, &mutex.mutex_, &ts);
#endif
        REEL_ASSERT(r == 0 || r == ETIMEDOUT);
        REEL_ASSERT(!mutex.locked_);
        REEL_IF_DEBUG(++mutex.locked_);
        REEL_IF_DEBUG(mutex.owner_ = pthread_self());
    }
}

#endif // COND_VAR_H_INCLUDED
