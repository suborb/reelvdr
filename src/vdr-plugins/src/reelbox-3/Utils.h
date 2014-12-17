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

// Utils.h

#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include "Reel.h"

#include <deque>

namespace Reel
{
    //--------------------------------------------------------------------------------------------------------------

    // Compile time type funcion: The result is S if cond is true otherwise T.
    template<bool cond, typename S, typename T>
    struct CondType
    {
        // The general template is never needed as there's a (partial) specialization for every case.
    };

    // Specialization for cond == true.
    template<typename S, typename T>
    struct CondType<true, S, T>
    {
        typedef S Type;
    };

    // Specialization for cond == false.
    template<typename S, typename T>
    struct CondType<false, S, T>
    {
        typedef T Type;
    };

    //--------------------------------------------------------------------------------------------------------------

    template<typename T>
    struct EfficientArgument
    {
        // We pass small types by value, others by reference to const.
        typedef typename CondType< (sizeof(T) <= 8), T, T const & >::Type Type;
    };

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, UInt S>
    class FixSizedQueue
    {
    public:
        typedef typename EfficientArgument<T>::Type ArgType;

        FixSizedQueue() NO_THROW;

        bool Empty() const NO_THROW;
            ///< Return true iff queue is empty.

        T Get();
            ///< Return next queued element. Queue must be nonempty.
            ///< Notes: This operation is not safe if the copy constructor of T throws (the element that should have
            ///<        been returned will be lost). Get() will only throw if the copy constructor of T throws.

        void Put(ArgType t); /* atomic */
            ///< Queue an element. Queue must not be full.
            ///< This operation will only throw if the copy constructor or the copy assignment operator of T throws.

    private:
        enum
        {
            QueueArraySize = S + 1 // So the queue is empty exactly when begin_ == end_ and is overfilled after a Put()
                                   // iff begin_ == end_.
        };

        T array_[QueueArraySize];

        T *const upperBound_;

        T *begin_;
        T *end_;
    };

    //--------------------------------------------------------------------------------------------------------------

    ///< A sequence container of object(pointer)s to T (polymorphic). ObjectSequence doesn't own the contained Ts.
    ///< S = one of {std::deque, std::vector, std::list}.
    ///<
    ///< Note: The interface of ObjectSequence is not compatible with the containers of the STL.
    template<typename T, class S = std::deque<T *> >
    class ObjectSequence /* final */
    {
    public:
        class Iterator
        {
        public:
            T &operator*();
            T *operator->();

            Iterator &operator++();
            Iterator operator++(int);

            Iterator &operator--();
            Iterator operator--(int);

            bool operator==(Iterator other);

            bool operator!=(Iterator other);

        private:
            typename S::iterator it_;

            friend class ObjectSequence<T, S>;

            Iterator(typename S::iterator it);
        };

        ObjectSequence();
        ~ObjectSequence();

        void PushBack(T &t);
            ///< Put an element at the end of the container.

        Iterator Begin();
        Iterator End();

        void Erase(Iterator it);

    private:
        S sequence_;
    };

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, UInt S>
    FixSizedQueue<T, S>::FixSizedQueue()
    :   upperBound_(array_ + QueueArraySize),
        begin_(array_), end_(array_)
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, UInt S>
    inline bool FixSizedQueue<T, S>::Empty() const NO_THROW
    {
        return begin_ == end_;
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, UInt S>
    inline T FixSizedQueue<T, S>::Get()
    {
        REEL_ASSERT(!Empty());

        T *const tmp = begin_;
        if (++ begin_ == upperBound_)
        {
            begin_ = array_;
        }

        return *tmp;
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, UInt S>
    inline void FixSizedQueue<T, S>::Put(ArgType t)
    {
        *end_++ = t;

        if (end_ == upperBound_)
        {
            end_ = array_;
        }

        REEL_ASSERT(begin_ != end_);
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline ObjectSequence<T, S>::ObjectSequence()
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline ObjectSequence<T, S>::~ObjectSequence()
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline void ObjectSequence<T, S>::PushBack(T &t)
    {
        sequence_.push_back(&t);
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline typename ObjectSequence<T, S>::Iterator ObjectSequence<T, S>::Begin()
    {
        return sequence_.begin();
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline typename ObjectSequence<T, S>::Iterator ObjectSequence<T, S>::End()
    {
        return sequence_.end();
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline void ObjectSequence<T, S>::Erase(typename ObjectSequence<T, S>::Iterator it)
    {
        sequence_.erase(it.it_);
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline T &ObjectSequence<T, S>::Iterator::operator*()
    {
        return **it_;
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline T *ObjectSequence<T, S>::Iterator::operator->()
    {
        return *it_;
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline bool ObjectSequence<T, S>::Iterator::operator==(typename ObjectSequence<T, S>::Iterator other)
    {
        return it_ == other.it_;
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline bool ObjectSequence<T, S>::Iterator::operator!=(typename ObjectSequence<T, S>::Iterator other)
    {
        return it_ != other.it_;
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline ObjectSequence<T, S>::Iterator::Iterator(typename S::iterator it)
    :   it_(it)
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline typename ObjectSequence<T, S>::Iterator &ObjectSequence<T, S>::Iterator::operator++()
    {
        ++it_;
        return *this;
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline typename ObjectSequence<T, S>::Iterator ObjectSequence<T, S>::Iterator::operator++(int)
    {
        return it_++;
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline typename ObjectSequence<T, S>::Iterator &ObjectSequence<T, S>::Iterator::operator--()
    {
        --it_;
        return *this;
    }

    //--------------------------------------------------------------------------------------------------------------

    template<typename T, class S>
    inline typename ObjectSequence<T, S>::Iterator ObjectSequence<T, S>::Iterator::operator--(int)
    {
        return it_--;
    }

    //--------------------------------------------------------------------------------------------------------------

//Start by Klaus
    inline uint HighWord(ULLong val)
    {
        return val >> 32;
    }

    inline uint LowWord(ULLong val)
    {
        return val & 0x00000000FFFFFFFF;
    }

    inline ULLong LongVal(uint high, uint low)
    {
        ULLong val = high;
        val <<= 32;
        val |= low;
        return val;  
    }
//End by Klaus

}

#endif // UTILS_H_INCLUDED
