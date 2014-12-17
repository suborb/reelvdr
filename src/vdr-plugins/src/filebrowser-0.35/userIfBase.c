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


//userIfBase.c

#include "userIfBase.h"

std::vector<eIftype> *cUserIfBase::ifTypes = NULL;

eIftype cUserIfBase::GetCaller()
{
    if(ifTypes && ifTypes->size() > 1)
    {
        return (*ifTypes)[ifTypes->size()-2];
    }
    return if_none;
}

void cUserIfBase::EraseIfTypes()
{
    if(ifTypes)
    {
        ifTypes->clear();
    }
}

void cUserIfBase::SetHistory(std::vector<eIftype> *ifTps)
{
    ifTypes = ifTps;
}

void cUserIfBase::ChangeIfType(eIftype nextType)
{
    //printf("ChangeIfType,nextType = %d\n", nextType); 
    //PrintIfTypes();
    if(!ifTypes)
    {
        return;
    }
    if(ifTypes->size())
    {
	eIftype currentType = ifTypes->back();
        if(currentType == nextType)
        {
            return;
        }
        ifTypes->pop_back();
        if(ifTypes->size())
        {
	    if(ifTypes->back() == nextType)
            {
                PrintIfTypes();
	        return;
            }
        }
        ifTypes->push_back(currentType);
    }
    ifTypes->push_back(nextType);    
    //printf("------------ChangeIfType,nextType = %d\n", nextType); 
    //PrintIfTypes();
} 

void cUserIfBase::RemoveLastIf()
{
    if(ifTypes && ifTypes->size())
    {
        ifTypes->pop_back();
    }
}

eIftype cUserIfBase::GetPreviousIf()
{
    if(ifTypes && ifTypes->size() >= 2)
    {
        return (*ifTypes)[ifTypes->size()-2];
    }
    return if_none;
}

void cUserIfBase::PrintIfTypes()
{   
    if(!ifTypes)
    {
        return;
    }
    printf("\n");
    for(unsigned int i = 0; i < ifTypes->size(); ++i)
    {
        printf("(*ifTypes)[%d] = %d\n", i, (*ifTypes)[i]);
    }
    printf("\n");
}


