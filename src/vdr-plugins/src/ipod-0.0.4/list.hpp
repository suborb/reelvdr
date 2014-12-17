
#ifndef __LIST_H__
#define __LIST_H__

#include "common.hpp"

//*************************************************************************
// List
//*************************************************************************

class List 
{
   public:

      typedef int (*cmpFct)(void* p1, void* p2);

      enum Error
      {
         errEmptyList = -1000,
         errItemNotFound,

         errNotSorted,
         errNoCompareFunction,
         errSortedList
      };

      enum Mode
      {
         mdNone   = 0,
         mdSorted = 1
      };

      List(Mode aMode = mdNone);
      ~List(); 

      int insert(void *item);
      int append(void *item);
      int remove(void *item);
      int removeCurrent();

      int removeAll();

      int sort(cmpFct fct = 0);
      void* search(void* what, cmpFct fct = 0);

      void setCompareFunction(cmpFct fct) { compareFunction = fct; }

      void* getFirst();
      void* getNext();
      void* getPrevious();
      void* getLast();
      void* getCurrent();
      int getCount()        { return count; }
      int getMode()         { return mode; }

      int isSorted()        { return sorted; }

   protected:

      struct Node
      {
         void *item;

         Node *prev;
         Node *next;
      };

      int insertNode(Node* node, int after);
      int appendNode(Node* node);
      Node* nodeAt(int index); 
      void qSort(int start, int stop, cmpFct fct, Node** hash);
      void* bSearch(void* what, cmpFct fct, Node** hash);
      int createHashes();
      int forgetHashes();

      // register

      int mode;
      int sorted;
      cmpFct compareFunction;
      int count;
      Node* first;
      Node* last;
      Node* current;
      Node** hashes;
};

//*************************************************************************
#endif // __LIST_H__

