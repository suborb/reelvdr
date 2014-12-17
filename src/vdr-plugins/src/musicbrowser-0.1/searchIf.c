#include "searchIf.h"
#include "browserItems.h"
#include "browsermenu.h"

cSearchIf cSearchIf::instance;

cSearchIf::cSearchIf()
{
    text[0] = 0;
    groupBy = 0;

    groupByStrs.Append(strdup(tr("All")));
    groupByStrs.Append(strdup(tr("Album")));
    groupByStrs.Append(strdup(tr("Artist")));
    groupByStrs.Append(strdup(tr("Genre")));
    groupByStrs.Append(strdup(tr("Title")));
}


void cSearchIf::EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf, eOSState defOSState)
{
    menu->SetColumns(30, 4); // "title \t\t duration"
    menu->SetTitleString(tr("Search"));
    cBrowserIf::EnterState(menu, search, lastIf, defOSState);
}


eOSState cSearchIf::AnyKey(cBrowserMenu *menu, eKeys key)
{
    int oldGroupBy = groupBy;
    eOSState state = cBrowserBaseIf::AnyKey(menu, key);

    if (oldGroupBy != groupBy)
    {
        int c = menu->Current();
        menu->SetItems(GetOsdItems(menu));
        menu->SetCurrentItem(menu->Get(c));
        menu->Display();
    }
    return state;
}


eOSState cSearchIf::OkKey(cBrowserMenu *menu)
{
    cBrowserItem* curr = menu->GetCurrentBrowserItem();
    if (curr)
        curr->Open(menu, false);
    else {
        cMenuEditStrItem* item = dynamic_cast<cMenuEditStrItem*> (menu->Get(menu->Current()));
        if (item) {
            searchText = std::string("*") + text + std::string("*");

            /* redraw menu */
            int c = menu->Current();
            menu->SetItems(GetOsdItems(menu));
            menu->SetCurrentItem(menu->Get(c));
            menu->Display();
        }
    }
    return osContinue;
}


#define UNSELECTABLE(x) new cOsdItem(x, osUnknown, false)
std::vector<cOsdItem*> cSearchIf::GetOsdItems(cBrowserMenu *menu)
{
	std::vector<cOsdItem*> osdItems;
	cOsdItem *item = new cMenuEditStrItem(tr("Search"), text, 255, NULL, tr("All"));
	osdItems.push_back(item);
	item = new cMenuEditStraItem(tr("Filter"), &groupBy, groupByStrs.Size(), &groupByStrs.At(0));
	osdItems.push_back(item);

	switch (groupBy)
	{
		default:
		case 0: // all
			AddAlbumOsdItems(menu, osdItems);
			AddArtistOsdItems(menu, osdItems);
			AddGenreOsdItems(menu, osdItems);
			AddTitleOsdItems(menu, osdItems);
			break;

		case 1:
			AddAlbumOsdItems(menu, osdItems);

			break;
		case 2:
			AddArtistOsdItems(menu, osdItems);
			break;
		case 3:
			AddGenreOsdItems(menu, osdItems);
			break;
		case 4:
			AddTitleOsdItems(menu, osdItems);
			break;
	}

	return osdItems;
}


bool cSearchIf::AddGenreOsdItems(cBrowserMenu *menu, std::vector<cOsdItem *> &osdItems)
{

	int count = 10;
	std::vector<std::string> genreList = database_->Genres(searchText);
	printf("\033[0;91mGot %u genre from database\033[0m\n", genreList.size());
	if (!genreList.empty())
	{
		osdItems.push_back(UNSELECTABLE(""));
		osdItems.push_back(UNSELECTABLE(tr("Genres")));
		count = 10;
	}

	std::vector<std::string>::iterator it = genreList.begin();
	while (it != genreList.end())
	{
		cGenreItem* item = new cGenreItem(it->c_str(), menu, filters_);
		osdItems.push_back(item);
		++it;

		if (strlen(text) == 0 && count -- < 0) break;
	}
	return !genreList.empty();
}


bool cSearchIf::AddArtistOsdItems(cBrowserMenu *menu, std::vector<cOsdItem *> &osdItems)
{

	int count = 10;
	std::string emptyStr = "";
	std::vector<std::string> artistList = database_->Artists(emptyStr,searchText);
	printf("\033[0;91mGot %u artist from database\033[0m\n", artistList.size());
	if (!artistList.empty())
	{
		osdItems.push_back(UNSELECTABLE(""));
		osdItems.push_back(UNSELECTABLE(tr("Artists")));
		count = 10;
	}

	std::vector<std::string>::iterator it = artistList.begin();
	while (it != artistList.end())
	{
		cArtistItem* item = new cArtistItem(it->c_str(), menu, filters_);
		osdItems.push_back(item);
		++it;

		if (strlen(text) == 0 && count -- < 0) break;
	}
	return !artistList.empty();
}


bool cSearchIf::AddAlbumOsdItems(cBrowserMenu *menu, std::vector<cOsdItem *> &osdItems)
{

	int count = 10;
	std::string emptyStr = "";
	std::vector<std::string> albumList = database_->Albums(emptyStr,
														   emptyStr,
														   searchText
														   );
	printf("\033[0;91mGot %u album from database\033[0m\n", albumList.size());
	if (!albumList.empty())
	{
		osdItems.push_back(UNSELECTABLE(""));
		osdItems.push_back(UNSELECTABLE(tr("Albums")));
		count = 10;
	}

	std::vector<std::string>::iterator it = albumList.begin();
	while (it != albumList.end())
	{
		cAlbumItem* item = new cAlbumItem(it->c_str(), menu, filters_);
		osdItems.push_back(item);
		++it;

		if (strlen(text) == 0 && count -- < 0) break;
	}
	return !albumList.empty();
}


bool cSearchIf::AddTitleOsdItems(cBrowserMenu *menu, std::vector<cOsdItem *>& osdItems)
{
	int count = 10;
	std::string emptyStr = "";
	std::vector<cFileInfo> files = database_->Files(emptyStr, emptyStr, emptyStr, searchText);

	if (!files.empty())
	{
		osdItems.push_back(UNSELECTABLE(""));
		osdItems.push_back(UNSELECTABLE(tr("Songs")));
		count = 10;
	}
	printf("\033[0;91mGot %u titles from database\033[0m\n", files.size());
	std::vector<cFileInfo>::iterator it_f = files.begin();

	while (it_f != files.end())
	{
		cFileItem* item = new cFileItem(*it_f, menu);
		osdItems.push_back(item);
		++it_f;

		if (strlen(text) == 0 && count -- < 0) break;
	}

	return !files.empty();
}
