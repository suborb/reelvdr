#ifndef STL_LIKE_ITEM_TREE_H
#define STL_LIKE_ITEM_TREE_H

#include "tree.hh"
#include <vector>
#include <string>
#include <iostream>

#include <vdr/debugmacros.h>

using namespace std;


////////////////////////////////////////////////////////////////////////////////
// class cItemTree
////////////////////////////////////////////////////////////////////////////////
// is an item of the initial playlist : so, could also be a playlist. As is the 
// case with .m3u/shoutcast feeds

class cItemTree
{
    private:
        tree<string> myTree;
        tree<string>::leaf_iterator curr_leaf;

        // set direction
        bool Up;
        //bool isOutOfBound(tree<string>::leaf_iterator it) {  return Up? it == myTree.end_leaf(): it == myTree.begin_leaf(); }


    public:
        cItemTree()     { Up = true; }
        ~cItemTree()    { myTree.clear(); }

        cItemTree( const cItemTree& rhs )
        {
          Up = true;
          myTree = rhs.myTree;
          curr_leaf = myTree.begin_leaf();
        }
        
        // set the head of the Tree
        // done at the beginning of building the tree
        void set_head(string head_str) 
        { 
            myTree.set_head( head_str );  
            curr_leaf = myTree.begin_leaf();
        }

        // delete / clear the Tree
        void clear() 
        {  
            myTree.clear(); 
            //Rewind(); 
            curr_leaf = myTree.end_leaf();
        }

        // Get to the initial/final leaf
        void Rewind() 
        { 
            curr_leaf = Up? myTree.begin_leaf(): myTree.end_leaf(); 
        }

        void GoToFirstLeaf() 
        {
            curr_leaf = myTree.begin_leaf();
        }

        void GoToLastLeaf()
        {
            curr_leaf = myTree.end_leaf();
        }

        int Size() const
        {
            return myTree.size();
        }

        // get the next leaf , if necessary builds the tree
        string GetNext(bool up);
};

#endif

