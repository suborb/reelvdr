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

// Mutex.h
 
#ifndef MUTEX_H_INCLUDED
#define MUTEX_H_INCLUDED

#include "Reel.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h> //FIXME

namespace Reel
{
    /**
      * Non-reentrant mutex class.
      */
    class Mutex /* final */
    {
    public:
        Mutex();
        ~Mutex() NO_THROW;

        void Lock();
        void Unlock() NO_THROW;
        bool TryLock();
            ///< Return true iff lock succeeded.
        
        friend class CondVar;

    private:

        Mutex(Mutex const &); // forbid copy construction.
        Mutex &operator=(Mutex const &); // forbid copy assignment.

        pthread_mutex_t mutex_;
#ifdef REEL_DEBUG
        Int locked_;
        pthread_t owner_;
#endif // REEL_DEBUG
    };

    /* RAII pattern for Mutex::Lock()/Unlock() */
    class MutexLocker /* final */
    {
    public:
        MutexLocker(Mutex &mutex);
        ~MutexLocker();

    private:
        MutexLocker(MutexLocker const &); // forbid copy construction.
        MutexLocker &operator=(MutexLocker const &); // forbid copy assignment.

        Mutex &mutex_;
    };

    /* RAII pattern for Mutex::TryLock()/Unlock() */
    class MutexTryLocker /* final */
    {
    public:
        MutexTryLocker(Mutex &mutex);
        ~MutexTryLocker();

        bool Acquired() const;

    private:
        MutexTryLocker(MutexTryLocker const &); // forbid copy construction.
        MutexTryLocker &operator=(MutexTryLocker const &); // forbid copy assignment.

        Mutex *mutex_;
    };

    inline Mutex::Mutex()
    {
#ifdef REEL_DEBUG
        pthread_mutexattr_t attr = {{ ReelDebug ? PTHREAD_MUTEX_RECURSIVE_NP : PTHREAD_MUTEX_FAST_NP }};
#else	
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
#endif
                // Note: Two braces necessary for type correctness (otherwise Compiler WARNING). There's
                //       something seriously wrong in pthread.h. 
//        REEL_ASSERT(pthread_mutex_init(&mutex_, &attr) == 0);
	pthread_mutex_init(&mutex_, &attr);
        REEL_IF_DEBUG(locked_ = 0);
    }

    inline Mutex::~Mutex() NO_THROW
    {
        REEL_ASSERT(!locked_);
#ifdef REEL_DEBUG
        REEL_VERIFY(pthread_mutex_destroy(&mutex_) == 0);
#endif
    }

    inline void Mutex::Lock()
    {
        REEL_VERIFY(pthread_mutex_lock(&mutex_) == 0);

        REEL_ASSERT(!locked_);
        REEL_IF_DEBUG(++locked_);
        REEL_IF_DEBUG(owner_ = pthread_self());
    }
    
    inline bool Mutex::TryLock()
    {
        int err = pthread_mutex_trylock(&mutex_);

        if (err == EBUSY)
        {
            return false;
        }

        REEL_ASSERT(err == 0);
        REEL_ASSERT(!locked_);
        REEL_IF_DEBUG(++locked_);
        REEL_IF_DEBUG(owner_ = pthread_self());
        return true;
    }
    
    inline void Mutex::Unlock() NO_THROW
    {
        REEL_ASSERT(locked_ == 1);
        REEL_ASSERT(owner_ == pthread_self());
        REEL_IF_DEBUG(--locked_);

        REEL_VERIFY(pthread_mutex_unlock(&mutex_) == 0);
    }

    inline MutexLocker::MutexLocker(Mutex &mutex)
    :   mutex_(mutex)
    {
        mutex.Lock();
    }

    inline MutexLocker::~MutexLocker()
    {
        mutex_.Unlock();
    }

    inline MutexTryLocker::MutexTryLocker(Mutex &mutex)
    :   mutex_(0)
    {
        if (mutex.TryLock())
        {
            mutex_ = &mutex;
        }
    }

    inline MutexTryLocker::~MutexTryLocker()
    {
        if (mutex_)
        {
            mutex_->Unlock();
        }
    }

    inline bool MutexTryLocker::Acquired() const
    {
        return mutex_;
    }
}

#endif // MUTEX_H_INCLUDED
