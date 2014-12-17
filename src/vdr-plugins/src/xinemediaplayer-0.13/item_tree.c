#include "item_tree.h"
#include "Player.h"

#include "pls_parser.h"
#include "m3u_parser.h"
#include "curl_download.h"
#include <iostream>

#include <vdr/tools.h> // esyslog
#include <vdr/debugmacros.h> 

using namespace std;

#define MAX_INVALID_PL_COUNT 3


void print_tree(const tree<std::string>& tr)
   {
   tree<std::string>::iterator it=tr.begin(), end = tr.end();
   if(!tr.is_valid(it)) return;
   int rootdepth=tr.depth(it);
   std::cout << "-----" << std::endl;
   while(it!=end) {
      for(int i=0; i<tr.depth(it)-rootdepth; ++i)
         std::cout << "  ";
      std::cout << (*it) << std::endl << std::flush;
      ++it;
      }
   std::cout << "-----" << std::endl;
   }



/// returns true if mrl is handled by xineLib
/// pls, m3u playlist files are handled externally (in Player class)

bool isPlayable(string mrl)
{
  size_t found;
//  DEBUG_VAR(mrl);

  if (mrl.size() < 4 ) return true;

  found = mrl.rfind(".m3u");
  //DEBUG_VAR(found);
  if (found != string::npos) return false;
  found = mrl.rfind(".pls");
  //DEBUG_VAR(found);
  if (found != string::npos) return false;

  return true;
}

// go to & return next-playable leaf 
// if current leaf is a playlist (m3u/pls) then build the tree with its contents
/// return null/empty string when no more playable mrl/playlist is found in the tree
string cItemTree::GetNext(bool up) 
{
    //Up = up; // set direction
    tree<string>::leaf_iterator it = curr_leaf;
    string item_str; // to be returned
    
    int download_failure = 0; // count number of download failures
    //int ii =0 ;
    //while ( !isOutOfBound(it) )
    int invalid_playlist_count = 0;

    while ( it != myTree.end_leaf() ) 
    {

        if (it->size()== 0) // go to next
        {
            ++it;
            continue;
        }

        if ( !isPlayable(*it) ) // add leafs
        {
            // if url to a playlist, download and save it in 'filename'
            std::string filename="/tmp/xine_playlist";
            

            // get playlist
            if ( isUrl(it->c_str() ) )
            {
                LOC; cout<<"calling curl...";
        int flag = curl_download( it->c_str(), filename.c_str() );  
        if (!flag) 
        {
            download_failure++;
            
            Reel::XineMediaplayer::Player::NetworkErrorCount++;
            Skins.QueueMessage(mtError, tr("Network read error"), 2, -1);

            if (download_failure>3) return ""; // no more tries
        }
        else Reel::XineMediaplayer::Player::NetworkErrorCount = 0;// network was accesible, so reset

                cout<<flag << " done: failure count"<<download_failure<<endl;
            }
            else
                // local playlist
                filename = *it;


            std::vector<std::string> pls_list, dummy; // we donot need the name_list : hence dummy

            if ( pls_parser(filename, pls_list, dummy) || m3u_parser(filename, pls_list, dummy) ) // parse
            {
                std::vector<std::string>::iterator it_n = pls_list.begin();
                

                while( it_n != pls_list.end() ) // add children
                {
                    myTree.append_child(it, *it_n);
                    ++it_n;
                }
                
                it = myTree.begin_leaf(it); // first child

            }
            else
            {
                ++invalid_playlist_count; 
                if (invalid_playlist_count > MAX_INVALID_PL_COUNT)
                {
                    invalid_playlist_count = 0;

                    // neither a pls/m3u/playable file
                    esyslog("xinemediaplayer (%s:%d): could not handle %s",  __FILE__, __LINE__, it->c_str() );
                    Skins.QueueMessage(mtError, tr("Got Invalid playlist from server. Playing next."), 2,-1);
                    // jump to next
                    ++it;
                }
                else 
                    dsyslog("downloading playlist '%s' once again : try #%i", it->c_str(), invalid_playlist_count);
            }

        }
        else // playable
        {
            item_str = *it;
            ++it;
            curr_leaf = it; // donot check for end() here

            return item_str;
        }
    }

    // no playable item found
    // rewind to initial leaf
    curr_leaf = myTree.begin_leaf();//Rewind();

    // empty string signifying end_of_tree
    return ""; 
}


