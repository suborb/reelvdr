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

// Fair_Mutex.h
 
#ifndef MY_MUTEX_H_INCLUDED
#define MY_MUTEX_H_INCLUDED

#include "Reel.h"
#include "Mutex.h"
#include "CondVar.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h> //FIXME
#include <deque>

#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>

#define CONDVAR_POOL_SIZE 8

namespace Reel
{
    /**
      * fair mutex class.
      */
    class FairMutex /* final */
    {
    public:
        FairMutex();
        ~FairMutex() NO_THROW;

        void Lock();
        void Unlock() NO_THROW;
        
        friend class CondVar;

    private:

        std::deque<CondVar*> condVarQueue_;
        std::deque<CondVar*> condVarPool_;
        CondVar *condVar_;
        Mutex mutex_;
        Int locked_;
        bool condVarPresent_;

        FairMutex(FairMutex const &); // forbid copy construction.
        FairMutex &operator=(FairMutex const &); // forbid copy assignment.

        CondVar *GetFromPool();
        void PutInPool(CondVar *cond);
        void CreatePool();
        void DestroyPool();
    };

    FairMutex::FairMutex()
    : locked_(0), condVarPresent_ (false)
    {
        CreatePool();
    }

    FairMutex::~FairMutex()
    {
        DestroyPool();
    }

    inline void FairMutex::CreatePool()
    {
        CondVar *cond;
        for(int i = 0; i < CONDVAR_POOL_SIZE; i++)
        {
            cond = new CondVar;
            condVarPool_.push_front(cond);
        }
    }

    inline void FairMutex::DestroyPool() //FIXME
    {
        CondVar *cond;
        for(uint i = 0; i < condVarPool_.size(); i++)
        {
            cond = condVarPool_.back();
            condVarPool_. pop_back();
            delete cond;
        }
    }

    inline CondVar *FairMutex::GetFromPool()
    {
        CondVar *cond = NULL;
        if (!condVarPool_.empty())
        {
            cond = condVarPool_.back();
            condVarPool_. pop_back();
        }
        return cond;
    }

    void FairMutex::PutInPool (CondVar *cond)
    {
        condVarPool_.push_front(cond);
    }

    inline void FairMutex::Lock()
    {
        mutex_.Lock();
        ++locked_;

        if (locked_ == 1)
        {
            mutex_.Unlock();
            return;
        }
        else
        {
            CondVar *cond = GetFromPool(); 
            condVarQueue_.push_front(cond);

            while(condVar_ != cond)
            {
                cond->Wait(mutex_);
            }
            PutInPool(condVar_);
            condVar_ = NULL;
        }
        mutex_.Unlock();
    }
    
    inline void FairMutex::Unlock() NO_THROW
    {
        mutex_.Lock(); // RAII !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        --locked_;
        if(!condVarQueue_.empty())
        {
            condVar_ = condVarQueue_.back();
            condVarQueue_.pop_back();
            //printf("-------------FairMutex::Unlock, vor Broadcast, tid = %d--------\n", syscall(__NR_gettid));
            condVar_->Broadcast(mutex_);
        }
        mutex_.Unlock();
    }

}

#endif // MY_MUTEX_H_INCLUDED
