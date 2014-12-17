/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: setup.h,v 1.21 2006/09/16 18:33:37 lordjaxom Exp $
 */

#ifndef VDR_BURN_SETUP_H
#define VDR_BURN_SETUP_H

#include "common.h"
#include "forward.h"
#include "proctools/property.h"
#include <string>
#include <vdr/menuitems.h>

namespace vdr_burn
{

// --- cBurnParameters --------------------------------------------------------

struct cBurnParameters
{
	cBurnParameters();

	bool ProcessArgs(int argc, char **argv);

	std::string DataPath;
	std::string TempPath;
	std::string DvdDevice;
	std::string IsoPath;
	bool fixedStoreMode;
};

extern cBurnParameters BurnParameters;

	//!--- plugin_setup -----------------------------------------------------------

	struct plugin_setup : public proctools::property_bag
	{
		proctools::property< bool > RemovePath;
		proctools::property< bool > EjectAfterBurn;
//		proctools::property< int > CustomDiskSize;
//		proctools::property< int > BurnSpeed;
		proctools::property< int > DemuxType;
		proctools::property< int > RequantType;
		proctools::property< bool > PreserveLogFiles;
		proctools::property< int > DefaultLanguage;
		proctools::property< bool > MainMenuStatus;

		proctools::property< bool > OfferDiskType;
		proctools::property< bool > OfferChapters;
//		proctools::property< bool > OfferDiskSize;
		proctools::property< bool > OfferBurnSpeed;
		proctools::property< bool > OfferStoreMode;
		proctools::property< bool > OfferDmhArchiveMode;
		proctools::property< bool > OfferCutOnDemux;
		proctools::property< bool > OfferSkipTitlemenu;
		proctools::property< bool > OfferSkipMainmenu;

		plugin_setup();
	};

	plugin_setup& global_setup();

	//!--- job_options --------------------------------------------------------

	struct job_options : public proctools::property_bag
	{
		proctools::property< int >  DiskType;
		proctools::property< int >  SkinIndex;
		proctools::property< int >  ChaptersMode;
		proctools::property< int >  StoreMode;
		proctools::property< bool > DmhArchiveMode;
		proctools::property< int >  DiskSize;
		proctools::property< int > BurnSpeed;
		proctools::property< bool > CutOnDemux;
		proctools::property< bool > SkipTitlemenu;
		proctools::property< bool > SkipMainmenu;

		job_options();

		bool set( const std::string& name_, const std::string& value_ );
	};

	job_options& job_defaults();

	//!--- job_options_base -------------------------------------------------------

	class job_options_base : public cMenuSetupPage
	{
	protected:
		job_options_base( job_options& options_, bool showAll_ );

		void add_job_options();

		menu::list_edit_item* m_storeModeItem;
		bool m_storeModeItem_selectable;
		menu::list_edit_item* m_diskTypeItem;
		bool m_diskTypeItem_selectable;
		menu::list_edit_item* m_skinItem;
		bool m_skinItem_selectable;
		menu::list_edit_item* m_chaptersItem;
		bool m_chaptersItem_selectable;
		menu::list_edit_item* m_diskSizeItem;
		bool m_diskSizeItem_selectable;
		menu::list_edit_item* m_burnSpeedItem;
		bool m_burnSpeedItem_selectable;
		menu::bool_edit_item* m_cutItem;
		bool m_cutItem_selectable;
		menu::bool_edit_item* m_archiveItem;
		bool m_archiveItem_selectable;
		menu::bool_edit_item* m_skipTitleItem;
		bool m_skipTitleItem_selectable;
		menu::bool_edit_item* m_skipMainItem;
		bool m_skipMainItem_selectable;
                std::string s;
		bool m_showAll;

	private:
		job_options& m_options;

	};

	//!--- plugin_setup_editor ----------------------------------------------------

	class plugin_setup_editor : public job_options_base
	{
	public:
		plugin_setup_editor();

	protected:
		virtual void Store() { store_setup(); }

	private:
		plugin_setup m_setup;
		job_options m_defaults;

		void store_setup();
		void store_value( const std::pair< std::string, proctools::property_base* >& value );
	};

	//!--- job_options_editor -----------------------------------------------------

	class job;

	class job_options_editor : public job_options_base
	{
	public:
		job_options_editor( job& job_ );

	protected:
		virtual void Store() {}
		virtual eOSState ProcessKey( eKeys key_ );

	private:
		job& m_job;
		job_options m_options;
		menu::size_text_item* m_infoTextItem;
		menu::size_bar_item* m_infoBarItem;

		void draw();
		void check_item_states();
		eOSState store_options();
		eOSState start_job();

	};

}

#endif // VDR_BURN_SETUP_H
