
#include <stdio.h>
#include <malloc.h>

#include "list.hpp"

//*************************************************************************
// Klasse List
//*************************************************************************
//*************************************************************************
// Object
//*************************************************************************

List::List(Mode aMode)
{
   sorted = no;
   mode = aMode;
   compareFunction = 0;
   hashes = 0;

   count = 0;
   first = last = current = 0;
}

List::~List()
{
   if (!count)
      return;

   removeAll();
}

//*************************************************************************
// Insert
//*************************************************************************

int List::insert(void *item)
{
   Node* tmp;
   int res;
   
   if (!item)
      return fail;

   if (mode & mdSorted)
   {      
      if (!compareFunction)
         return errNoCompareFunction;

      tmp = new Node;
      tmp->item = item;

      if (!count)
      {
         appendNode(tmp);
         current = tmp;

         return success;
      }

      search(item);
      res = compareFunction(item, current->item);
      forgetHashes();

      if (res > 0)
         insertNode(tmp, yes);
      else if (res < 0)
         insertNode(tmp, no);
      else 
         insertNode(tmp, yes);

      return success;
   }
   else
   {
      tmp = new Node;
      tmp->item = item;
      
      if (!count || !current)
         appendNode(tmp);
      else
         insertNode(tmp, no);
   }

   return success;
}

//***************************************************************************
// Insert Node
//***************************************************************************

int List::insertNode(Node* node, int after)
{
   if (after)
   {
      node->next = current->next;
      node->prev = current;

      node->next->prev = node;
      node->prev->next = node;

      if (current == last)
         last = node;
   }
   else
   {
      node->next = current;
      node->prev = current->prev;

      node->next->prev = node;
      node->prev->next = node;

      if (current == first)
         first = node;
   }

   count++;
   current = node;

   return done;
}
//*************************************************************************
// Append Node
//*************************************************************************

int List::appendNode(Node* node)
{
   if (!count)
      first = last = node;
   
   node->prev = last;
   node->next = first;

   last->next  = node;
   last        = node;
   first->prev = node;

   count++;

   return success;
}

//*************************************************************************
// Anhaengen eines Elements an die letzte Position
//*************************************************************************

int List::append(void *item)
{
   Node* tmp;
   
   if (!item)
      return fail;
   
   if (mode & mdSorted)
      return errSortedList;
   
   forgetHashes();

   tmp = new Node;
   tmp->item = item;

   appendNode(tmp);

   return success;
}

//***************************************************************************
// Remove Current Item
//***************************************************************************

int List::removeCurrent()
{
   Node* tmp = current;

   if (!current || !count)
      return fail;

   forgetHashes();

   if (current != first)
      current = tmp->prev;
   else
      current = 0;

   tmp->prev->next = tmp->next;
   tmp->next->prev = tmp->prev;
   
   count--;

   if (!count)
      current = first = last = 0;
   else if (tmp == first)
      first = tmp->next;
   else if (tmp == last)
      last = tmp->prev;
   
   delete tmp;

   return success;
}

//*************************************************************************
// Remove
//*************************************************************************

int List::remove(void* item)
{
   Node* tmp;

   if (!count)
      return errEmptyList;

   if (!item)
      return fail;

   forgetHashes();

   for (tmp = first; (tmp->item != item) && (tmp != last); tmp = tmp->next);

   if (tmp->item != item)
      return errItemNotFound;

   if (--count)
   {
      tmp->prev->next = tmp->next;
      tmp->next->prev = tmp->prev;
  
      if (tmp == first)
         first = tmp->next;
      else if (tmp == last)
         last  = tmp->prev;
   }
   else
   {
      first = last = 0;
   }

   current = 0;
   delete tmp;

   return success;
}

//*************************************************************************
// Remove All
//*************************************************************************

int List::removeAll()
{
   Node *next, *tmp = first;

   forgetHashes();

   // clear all items

   for (int i = 0; i < count; i++)
   {
      // got next item

      next = tmp->next;

      delete tmp;
   
      // gehe zum nachsten

      tmp = next;
   }

   // reset counter

   count = 0;

   return success;
}

//*************************************************************************
// Get First
//*************************************************************************

void* List::getFirst()
{
   if (!count)
      return 0;

   // init cuurent

   current = first;

   return current->item;
}

//*************************************************************************
// Get Last
//*************************************************************************

void* List::getLast()
{
   if (!count)
      return 0;

   // init current

   current = last;

   return current->item;
} 

//*************************************************************************
// Hole naechstes Element
//*************************************************************************

void* List::getNext()
{
   if (!count)
      return 0;

   if (!current)
      return getFirst();

   if (current == last)
      return current = 0;

   // goto next item

   current = current->next;

   return current->item;
}

//*************************************************************************
// Hole vorheriges Element
//*************************************************************************

void* List::getPrevious()
{
   if (!count)
      return 0;

   if (!current)
      return getLast();

   if (current == first)
      return current = 0;

   // goto previous item

   current = current->prev;

   return current->item;
}

//*************************************************************************
// Hole aktuelles Element
//*************************************************************************

void* List::getCurrent()
{
   return current ? current->item : 0;
}

//***************************************************************************
// Sort
//***************************************************************************

int List::sort(cmpFct fct)
{
   cmpFct theFct = fct ? fct : compareFunction;
   
   if (!theFct)
      return 0; // errNoCompareFunction;

   if (count < 2)
      return success;

   if (!hashes) 
      createHashes();

   qSort(0, count-1, theFct, hashes);
   sorted = yes;

   return success;
}

//***************************************************************************
// Quick Sort
//***************************************************************************

void List::qSort(int start, int stop, cmpFct fct, Node** hash)
{
   int begin = start;
   int end = stop;
   void* test;
   void* swap;

   test = hash[(start+stop)/2]->item;

   do
   {
      while (fct(test, hash[end]->item) < 0)
         end--;

      while (fct(hash[begin]->item, test) < 0)
         begin++;

      if (begin <= end)   
      {                   
         swap = hash[begin]->item;
         hash[begin]->item = hash[end]->item;
         hash[end]->item = swap;
         begin++;
         end--;
      }

   } while (begin <= end);

   if (start < end)   qSort(start, end, fct, hash);
   if (begin < stop)  qSort(begin, stop, fct, hash);
}

//***************************************************************************
// Search
//***************************************************************************

void* List::search(void* what, cmpFct fct)
{
   void* res;
   cmpFct theFct = fct ? fct : compareFunction;
   
   if (!count || !theFct)
      return 0; // errNoCompareFunction;

   if (!hashes) 
      createHashes();

   res = bSearch(what, theFct, hashes);

   return res; 
}
//***************************************************************************
// Create Hash
//***************************************************************************

int List::createHashes()
{
   int i;
   Node* p;

   hashes = new Node*[count];

   for (p = first, i = 0; p != last; p = p->next, i++)
      hashes[i] = p;

   hashes[i] = last;

   return done;
}

//***************************************************************************
// Forget Hashes
//***************************************************************************

int List::forgetHashes()
{
   if (hashes)
   {
      delete hashes;
      hashes = 0;
   }

   return done;
}

//***************************************************************************
// Bin Search
//***************************************************************************

void* List::bSearch(void* what, cmpFct fct, Node** hash)
{
   int res, index;
   int start = 0;
   int stop = count-1;
   Node* test;

   if (!what) return 0;
   
   do 
   {
      index = (start+stop)/2;
      test = hash[index];
      
      res = fct(what, test->item);
      
      if (res > 0)
         start = index+1;

      else if (res < 0)
         stop = index-1;

      else
      {
         current = test;
         return current->item;
      }

   } while (index != start && index != stop);

   current = test;

   return 0;
}
