/****************************************************************************
 * DESCRIPTION: 
 *             Submenü
 *
 * $Id: vdr-1.3.33-Setup-0.2.0.diff,v 1.1 2005/10/03 15:40:53 ralf Exp $
 *
 * Contact:    ranga@teddycats.de        
 *
 * Copyright (C) 2004, 2005 by Ralf Dotzert 
 ****************************************************************************/
#ifndef SUBMENU_H
#define SUBMENU_H

#include "tools.h"
//#include "tinystr.h"
#include "tinyxml/tinyxml.h"
#include <string>
#include <vector>



class cSubMenuNode;
class cSubMenuNodes;
class cSubMenu;



namespace setup
{
  std::string FileNameFactory(std::string Filename);
}

class cSubMenuNodes : public cList<cSubMenuNode> {};

//################################################################################
//# SubMenuNode
//################################################################################
class cSubMenuNode :  public cListObject
{
public:
    enum Type { UNDEFINED, SYSTEM, COMMAND, PLUGIN, MENU, INCLUDE };
    cSubMenuNode( TiXmlElement * xml, int level,  cSubMenuNodes *currentMenu, cSubMenuNodes *parentMenu );
    cSubMenuNode( cSubMenuNodes *currentMenu, cSubMenuNodes *parentMenu );
    ~cSubMenuNode();
    
    bool IncludeXml(const char *includeXML);
    bool SaveXml(TiXmlElement * root);
    static cSubMenuNode::Type IsType(const char *name);
    void  SetType(const char *name);
    void  SetType(enum Type type);
    void  SetPlugin(const char *argv = NULL); 
    cSubMenuNode::Type GetType();
    const char*GetTypeAsString();
    void SetCommand(const char*command);
    bool CommandConfirm();
    const char*GetCommand();
    void SetName(const char* Name);
    const char*GetName();
    void SetHelp(const char *Help);
    const char *GetHelp();
    void SetInfo(const char *Info);
    const char *GetInfo();
    void SetIconNumber(const char* iconNumber);
    const char *GetIconNumber();

    void SetSetupLink(const char*name);
    const char *GetSetupLink();
    ///< Get/Set SetupLink enable user to jump 
    ///< directly into menu of setup plugin 
    ///< atribute link="<mysetupmenu>"  has to have the same name like  
    ///< the generic setup menu.  added by moviemax

    int  GetLevel();
    void SetLevel(int level);
    
    int  GetPluginIndex();
    void SetPluginIndex(int index);
    void SetPluginMainMenuEntry(const char *mainMenuEntry);
    const char *GetPluginMainMenuEntry();
    
    cSubMenuNodes *GetParentMenu();
    void SetParentMenu(cSubMenuNodes *parent);
    cSubMenuNodes *GetCurrentMenu();
    void SetCurrentMenu(cSubMenuNodes *current);
    cSubMenuNodes *GetSubMenus();
    bool HasSubMenus();
    void Print(int index=0);
    int SubMenuSize(){return _subMenus.Count();}
private:
    Type          _type;
    int           _level;
    // Plugin Variables
    int           _pluginIndex;
    const char   *_pluginMainMenuEntry;
    //common
    const char *_name;
    const char *_help;
    const char *_info;
    const char *_iconNumber;
    const char *_command;
    bool _commandConfirm;
    cSubMenuNodes _subMenus;
    cSubMenuNodes *_parentMenu;
    cSubMenuNodes *_currentMenu;
    char *_setupLink;
    void init();
};

inline const char * cSubMenuNode::GetSetupLink()
{
  return _setupLink;
}



//################################################################################
//# SubMenu Class
//################################################################################
class cSubMenu
{
public:
  cSubMenu();
  ~cSubMenu();
  enum Where { BEFORE, BEHIND, INTO};
  bool LoadXml(const char *fname);
  bool SaveXml(char *fname);
  bool SaveXml();
  cSubMenuNodes *GetMenuTree();
  bool Up(int *ParentIndex);
  bool Down(int index);
  int  GetNrOfNodes();
  cSubMenuNode* GetAbsNode(int index);
  cSubMenuNode* GetNode(int index);
  void PrintMenuTree();
  bool IsPluginInMenu(const char *name);
  void AddPlugin(const char *name);
  const char *ExecuteCommand(const char*command);
  void MoveMenu(int index, int toindex, enum Where);
  void CreateMenu(int index, const char *menuTitle);
  void DeleteMenu(int index);
  char *GetMenuSuffix() { return _menuSuffix;}
  int  SetMenuSuffix(char *suffix);
  bool isTopMenu();
  const char *GetParentMenuTitel();

private:
  cSubMenuNodes _menuTree;
  cSubMenuNodes *_currentMenuTree;
  cSubMenuNodes *_currentParentMenuTree;
  char *_fname;
  char *_commandResult;
  int  _nrNodes;
  cSubMenuNode  **_nodeArray;
  char *_menuSuffix;
  int countNodes(cSubMenuNodes *tree);
  void tree2Array(cSubMenuNodes *tree, int &index);
  void addMissingPlugins();
  void reloadNodeArray();
  void removeUndefinedNodes();
};

inline int cSubMenu::SetMenuSuffix(char *suffix) 
{ 
   if(_menuSuffix)
      free(_menuSuffix);

   return asprintf(&_menuSuffix, "%s", suffix);
}


inline bool cSubMenu::isTopMenu()
{ 
   return (_currentParentMenuTree==NULL); 
}
const char *GetParentMenuTitle();


//################################################################################
//#      Class cHelp  
//################################################################################


class cHelp 
{
public:
  cHelp();
  ~cHelp();
  void Load(std::string Filename);
  const char *Text();
  const char *Title();
private:
  int editableWidth_;
  std::vector<char> text_;
  std::vector<std::string> strText_;
  std::string title_;
};

inline const char *cHelp::Text()
{
  return &text_[0];
}

inline const char *cHelp::Title()
{
   return title_.c_str();
}



//################################################################################
//# 
//################################################################################

#endif
