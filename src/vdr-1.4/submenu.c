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
#include "debug.h"
#include "submenu.h"
#include "plugin.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "thread.h"

using std::string;
using std::vector;
//using std::ifstream;
using std::cout;
using std::cerr;
using std::endl;


static const char* TAG_SYSTEM      = "system";
static const char* TAG_PLUGIN      = "plugin";
static const char* TAG_COMMAND     = "command";
static const char* TAG_MENU        = "menu";
static const char* TAG_INCLUDE     = "include";
static const char* TAG_UNDEFINED   = "undefined";
static const char* TRUE_STR        = "yes";

// ---- Namespace setup ----------------------------------------------------------------------

namespace setup
{
  string FileNameFactory(string FileType)
  {
    string configDir = cPlugin::ConfigDirectory();
    string::size_type pos = configDir.find("plugin");
    configDir.erase(pos-1);

#ifdef EXTRA_VERBOSE_DEBUG
    cout << " Config Dir : " << configDir  <<  endl;
#endif

    if (FileType == "configDirecory") // vdr config base directory 
    {
#if EXTRA_VERBOSE_DEBUG
       cout << "DEBUG [setup]: ConfigDirectory   " << configDir <<   endl;
#endif
       return configDir;
    }
    if (FileType == "help") // returns symbolic link
    {
       string configFile;
       configFile = cPlugin::ConfigDirectory();
       configFile += "/setup/help/help."; 
       string tmp = I18nLanguageCode(I18nCurrentLanguage());
       // if two token given we take the first one.
       string::size_type pos = tmp.find(','); 
       if (pos != string::npos)
       {
         configFile += tmp.substr(0,pos);
       }
       else 
       {
         configFile += tmp;
       }
       configFile += ".xml";
#ifdef EXTRA_VERBOSE_DEBUG
       cout << " debug config file: " << configFile <<   endl;
#endif
       return configFile;
    }

    //else if (FileType == "channelsFile") // returns  channels.conf
    else if (FileType == "link") // returns symbolic link
    {
#ifdef EXTRA_VERBOSE_DEBUG
       cout << " Config Dir : " << configDir  << "/channels.conf" <<  endl;
#endif
       return configDir += "/channels.conf";
    }
    else if (FileType == "channels") // returns channels dir;
    {
#ifdef EXTRA_VERBOSE_DEBUG
       cout << " Config Dir : " << configDir  << "/channels" <<  endl;
#endif
       return configDir += "/channels";
    }
    else if (FileType == "setup") // returns plugins/setup dir; change to  "configDir"  
    {
#ifdef EXTRA_VERBOSE_DEBUG
       cout << " Config Dir : " << configDir  << "/plugins/setup" <<  endl;
#endif
       return configDir += "/plugins/setup";
    }

    configDir.append("/");
    configDir += "/channels/";
    configDir += FileType;
#ifdef EXTRA_VERBOSE_DEBUG
    cout << " Config Dir end  : " << configDir << ".conf"  <<  endl;
#endif

    return configDir += ".conf";
  }
}

bool getfiles(const char *string, std::vector<char*> *files)
{
    char command[128];
    FILE *file;

    sprintf(command, "ls %s 2>/dev/null", string);
    file = popen(command, "r");
    if(file)
    {
        char *strBuff;
        cReadLine readline;
        strBuff = readline.Read(file);
        while(strBuff)
        {
            files->push_back(strdup(strBuff));
            strBuff = readline.Read(file);
        }
        pclose(file);
    }
    else
        return false;

    return true;
}

//################################################################################
//# SubMenuNode
//################################################################################

cSubMenuNode::cSubMenuNode(TiXmlElement * xml, int level, cSubMenuNodes *currentMenu, cSubMenuNodes *parentMenu)
{
  init();
  _parentMenu  = parentMenu;
  _currentMenu = currentMenu;
  _level       = level;
   
  if (xml != NULL && xml->Type() == TiXmlNode::ELEMENT)
  {     
     const char *tag =xml->Value();
         
     if (cSubMenuNode::IsType(tag) != cSubMenuNode::UNDEFINED )
     {
        SetType(tag);
        const char *name = xml->Attribute("name");
        const char *info = xml->Attribute("info");
        const char *help = xml->Attribute("help");
        const char *iconNumber = xml->Attribute("icon_number");
        SetName(name);
        SetIconNumber(iconNumber);
        if (info) SetInfo(info);
        if (help)
        {
           if (!info) SetInfo("(?)");
           SetHelp(help); 
        }

        if (strcmp(name,"setup") == 0) 
            SetSetupLink(xml->Attribute("link"));
             
        if (_type == COMMAND)
        {
           SetCommand(xml->Attribute("execute"));
           const char * confirmStr = xml->Attribute("confirm");
           if (confirmStr != NULL && strcmp(confirmStr, TRUE_STR) == 0)
              _commandConfirm = true;
        }
        else if (_type == PLUGIN) // Add Plugin Index
           SetPlugin(GetSetupLink());
        else if(_type == INCLUDE)
        {
            char *includeXML;
            asprintf(&includeXML, "%s/setup/%s", cPlugin::ConfigDirectory(), xml->Attribute("name"));
            if(strchr(includeXML, '*'))
            {
                std::vector<char*> includeXMLs;
                getfiles(includeXML, &includeXMLs);
                for(unsigned int i=0; i<includeXMLs.size(); ++i)
                    IncludeXml(includeXMLs.at(i));
            }
            else
                IncludeXml(includeXML);
            free(includeXML);
        }
        else if (_type == MENU && xml->NoChildren() == false )
        {
          xml = xml->FirstChildElement();
          do
          { 
              if(cSubMenuNode::IsType(xml->Value()) == cSubMenuNode::INCLUDE)
              {
                  char *includeXML;
                  asprintf(&includeXML, "%s/setup/%s", cPlugin::ConfigDirectory(), xml->Attribute("name"));
                  if(strchr(includeXML, '*'))
                  {
                      std::vector<char*> includeXMLs;
                      getfiles(includeXML, &includeXMLs);
                      for(unsigned int i=0; i<includeXMLs.size(); ++i)
                          IncludeXml(includeXMLs.at(i));
                  }
                  else
                      IncludeXml(includeXML);
                  free(includeXML);
              }
              else
              {
                  cSubMenuNode *node = new cSubMenuNode(xml, level+1, &_subMenus, currentMenu);
                  _subMenus.Add(node);
              }
          } while((xml=xml->NextSiblingElement()) !=NULL);
        }
     }
  }
  else
    throw "Invalid XML Node";
}

/**
 * Construct new Node empty Node
 *
 */

cSubMenuNode::cSubMenuNode(cSubMenuNodes *currentMenu, cSubMenuNodes *parentMenu )
{
  init();
  _parentMenu  = parentMenu;
  _currentMenu = currentMenu;
}

/**
 * 
 */

void cSubMenuNode::init()
{
  _iconNumber = NULL;
  _name = NULL;
  _command = NULL;
  _help = NULL;
  _info = NULL;
  _pluginMainMenuEntry= NULL;
  _type = UNDEFINED;
  _level = 0;
  _parentMenu = NULL;
  _currentMenu = NULL;
  _pluginIndex = 0;
  _commandConfirm = false;
  _setupLink = NULL;
}



cSubMenuNode::~ cSubMenuNode()
{
  if (_name != NULL ) free((void*)_name);
  if (_command != NULL ) free((void*)_command);
  if (_help != NULL ) free((void*)_help);
  if (_info != NULL ) free((void*)_info);
  if (_pluginMainMenuEntry != NULL ) free((void*)_pluginMainMenuEntry);
  if (_setupLink != NULL ) free((void*)_setupLink);
}

bool cSubMenuNode::IncludeXml(const char *includeXML)
{
    TiXmlDocument  xmlDoc = TiXmlDocument(includeXML);
    TiXmlElement *xml;
    bool  ok =true;

    if ((ok=xmlDoc.LoadFile()))
    { 
        if ((xml = xmlDoc.FirstChildElement()) != NULL)
        {
            do
            {
                if (xml != NULL && xml->Type() == TiXmlNode::ELEMENT)
                {     
                    int type = IsType(xml->Value());

                    if ((type == PLUGIN) || (type == COMMAND) || (type == MENU && xml->NoChildren() == false))
                    {
                        cSubMenuNode *node = new cSubMenuNode(xml, _level+1, &_subMenus, _currentMenu);
                        _subMenus.Add(node);
                    }
                    else if(type == INCLUDE)
                    {
                        char *includeXML;
                        asprintf(&includeXML, "%s/setup/%s", cPlugin::ConfigDirectory(), xml->Attribute("name"));
                        if(strchr(includeXML, '*'))
                        {
                            std::vector<char*> includeXMLs;
                            getfiles(includeXML, &includeXMLs);
                            for(unsigned int i=0; i<includeXMLs.size(); ++i)
                                IncludeXml(includeXMLs.at(i));
                        }
                        else
                            IncludeXml(includeXML);
                        free(includeXML);
                    }
                }
                else
                    throw "Invalid XML Node";
            }
            while(ok==true && (xml=xml->NextSiblingElement()) !=NULL);
        }
    }
    else
    {
        esyslog("ERROR: in %s : %s  Col=%d Row=%d\n", includeXML,
                xmlDoc.ErrorDesc(),
                xmlDoc.ErrorCol(),
                xmlDoc.ErrorRow());

        ok=false;
    }

    return ok;
}

void cSubMenuNode::SetSetupLink(const char *link)
{
  if (_setupLink) free (_setupLink);

  if (link && strlen(link) > 0 )  
  {
    _setupLink = strdup(link);
  }
  else 
  {
    _setupLink = NULL;
  }
}

/**
 * 
 */
void cSubMenuNode::SetPlugin(const char *link)
{
  bool found = false;
   
  for (unsigned int pIndex = 0; pIndex < cPluginManager::PluginCount() ; pIndex++)
  {
     cPlugin *p = cPluginManager::GetPlugin(pIndex);
     if (p)
     {
        if (strcmp(_name, p->Name()) == 0 && p->MainMenuEntry() != NULL)
        {
           if (strcmp("setup",p->Name()) == 0 && p->MainMenuEntry() != NULL)
           {
              if (link) 
              {
                SetPluginMainMenuEntry(link);
                _pluginIndex = pIndex;
                found=true;
                break;
              }
           }
                 
           SetPluginMainMenuEntry(p->MainMenuEntry());
           _pluginIndex = pIndex;
           found=true;
           break;
        }
    }
    else
    {
      //break;
      esyslog("cSubMenuNode::SetPlugin (%s:%i) plugin #%i == NULL", __FILE__, __LINE__, pIndex);
    } 
  }

  if (!found)
  {
     _type=UNDEFINED;
  }
}

bool cSubMenuNode::SaveXml(TiXmlElement * root )
{ 
    
  bool ok = true;

  if (root!=NULL)
  {
     TiXmlElement  *e   = NULL; 
     switch(_type) 
     { 
         case SYSTEM:
              e = new TiXmlElement(TAG_SYSTEM);
              e->SetAttribute("name", GetName());
              break;
         case COMMAND:
              e = new TiXmlElement(TAG_COMMAND);
              e->SetAttribute("name", GetName());
              e->SetAttribute("execute", GetCommand());
              if (_commandConfirm)
                 e->SetAttribute("confirm", TRUE_STR);
              break;
         case PLUGIN:
              e = new TiXmlElement(TAG_PLUGIN);
              e->SetAttribute("name", GetName());
              break;
         case MENU:
              e = new TiXmlElement(TAG_MENU);
              e->SetAttribute("name", GetName());
              break;
         case UNDEFINED:
         default: ok=false;
                   break;
     }
     if (ok)
     {
       root->LinkEndChild(e);
       if (HasSubMenus())
       {
          for (cSubMenuNode *node = _subMenus.First(); node; node = _subMenus.Next(node))
          {
             node->SaveXml(e);
          }
       }
     }
  }
  else 
  {
    return false;
  }
  return ok;
}


cSubMenuNode::Type cSubMenuNode::IsType(const char *name)
{
  Type type = UNDEFINED;
         
  if (strcmp(name ,TAG_SYSTEM) == 0)
    type =  cSubMenuNode::SYSTEM;
  else if (strcmp(name ,TAG_PLUGIN) == 0)
    type =  cSubMenuNode::PLUGIN;
  else if (strcmp(name ,TAG_COMMAND) == 0)
    type =  cSubMenuNode::COMMAND;
  else if (strcmp(name ,TAG_MENU) == 0)
    type =  cSubMenuNode::MENU;
  else if (strcmp(name ,TAG_INCLUDE) == 0)
    type =  cSubMenuNode::INCLUDE;

  return type;

}

void cSubMenuNode::SetType(const char *name)
{
   _type=IsType(name);
}

void cSubMenuNode::SetType(enum Type type)
{
  _type=type;
}

cSubMenuNode::Type cSubMenuNode::GetType()
{
  return _type;
}
const char * cSubMenuNode::GetTypeAsString()
{
  const char *str=NULL;
  switch(_type)
  {
     case SYSTEM: str=TAG_SYSTEM;
          break;
     case COMMAND: str=TAG_COMMAND;
          break;
     case PLUGIN:str=TAG_PLUGIN;
          break;
     case MENU: str=TAG_MENU;
          break;
     case UNDEFINED: str = TAG_UNDEFINED;
     default:  
          break;
  }
  
  return str;
}

void cSubMenuNode::SetCommand(const char *command)
{
  if (_command != NULL ) free((void*)_command);
  if (command != NULL) _command= strdup(command);
  else _command=NULL;
}

const char * cSubMenuNode::GetCommand()
{
  return(_command);
}
bool cSubMenuNode::CommandConfirm()
{
 return(_commandConfirm);
}

void cSubMenuNode::SetName(const char *name)
{
  if (_name) free ((void*)_name);
  if (name != NULL)
    _name = strdup(name);
  else
    _name = NULL;
}
const char * cSubMenuNode::GetName()
{
 return(_name);
}


void cSubMenuNode::SetHelp(const char *help)
{
  if (_help) free ((void*)_help);

  help? _help = strdup(help):help = NULL;
}

const char * cSubMenuNode::GetHelp()
{
 return _help;
}


void cSubMenuNode::SetInfo(const char *info)
{
  if (_info) free((void*)_info);

  //info? _info = strdup(info):info = NULL;
  if (info)
      _info = strdup(tr(info));
  else _info = NULL;
}

const char * cSubMenuNode::GetInfo()
{
 return _info;
}

void cSubMenuNode::SetIconNumber(const char*icon)
{
    if (_iconNumber) 
    {
        free((void*)_iconNumber); 
        _iconNumber = NULL;
    }
    if (icon) _iconNumber = strdup(icon);
}

const char * cSubMenuNode::GetIconNumber()
{
 return _iconNumber;
}

int cSubMenuNode::GetLevel()
{
 return _level;
}

void cSubMenuNode::SetLevel(int level )
{
 _level =level;
 if (HasSubMenus()) //Adjust Levels of Subnodes
 {
    for (cSubMenuNode *node = _subMenus.First(); node; node = _subMenus.Next(node))
    {
       node->SetLevel(level+1);
    }
  }
}

int cSubMenuNode::GetPluginIndex()
{
 return _pluginIndex;
}

void cSubMenuNode::SetPluginIndex(int index )
{
 _pluginIndex = index;
}

void cSubMenuNode::SetPluginMainMenuEntry(const char * mainMenuEntry )
{
  if (_pluginMainMenuEntry != NULL) free((void*)_pluginMainMenuEntry);

  if (mainMenuEntry!=NULL)
    _pluginMainMenuEntry=strdup(mainMenuEntry);
  else
    _pluginMainMenuEntry=NULL;
}

const char * cSubMenuNode::GetPluginMainMenuEntry()
{
 return _pluginMainMenuEntry;
}

cSubMenuNodes * cSubMenuNode::GetParentMenu()
{
 return _parentMenu;
}

void cSubMenuNode::SetParentMenu(cSubMenuNodes * parent )
{
  _parentMenu = parent;
}

cSubMenuNodes * cSubMenuNode::GetCurrentMenu()
{
  return _currentMenu;
}

void cSubMenuNode::SetCurrentMenu(cSubMenuNodes * current )
{
  _currentMenu = current;
}


cSubMenuNodes * cSubMenuNode::GetSubMenus()
{
  return(&_subMenus);
}

bool cSubMenuNode::HasSubMenus()
{
  if (_subMenus.Count() >0)
    return(true);
  else
    return(false);
}


void cSubMenuNode::Print(int index)
{
  for(int i=0; i<index;i++)
    PRINTF(" ");

  PRINTF("Name=%s Type=%s Level=%d", _name, GetTypeAsString(), _level);
  if (_type == COMMAND)
    PRINTF(" Command=%s", _command);
  PRINTF("\n");
  
  for (cSubMenuNode *node = _subMenus.First(); node; node = _subMenus.Next(node))
  {
     node->Print(index+4);
  }       
}



//################################################################################
//# 
//################################################################################
cSubMenu::cSubMenu()
{

 _menuSuffix = NULL;
 _fname = NULL;
 _commandResult = NULL;
 _currentMenuTree = &_menuTree;
 _currentParentMenuTree = NULL;
 _nodeArray = NULL;
 _nrNodes = 0;

}


cSubMenu::~cSubMenu()
{
  if (_menuSuffix) free(_menuSuffix);
  if (_fname) free(_fname);
  if (_commandResult) free(_commandResult);
  if (_nodeArray)  free(_nodeArray);
  _nrNodes = 0;
}

bool cSubMenu::LoadXml(const char *fname)
{
 TiXmlDocument  xmlDoc = TiXmlDocument(fname );
 TiXmlElement  *root = NULL;
 cSubMenuNode  *node = NULL;

  bool ok = true;
  //Clear previously loaded Menu
  if (_fname!= NULL ) free(_fname);
  _menuTree.Clear();
  _fname = strdup(fname);
  
  if ((ok=xmlDoc.LoadFile()))
  { 
     if ((root = xmlDoc.FirstChildElement("menus" ))!=NULL )
     {   
        char *tmp=NULL;
        if ((tmp =(char*)root->Attribute("suffix"))==NULL) {
              //asprintf(&_menuSuffix, " ..."); // set default menuSuffix
              //asprintf(&_menuSuffix, ""); // set default menuSuffix
              _menuSuffix = (char*)malloc(1);
              _menuSuffix[0] = '\0';
        } else
              asprintf(&_menuSuffix, tmp);
            
        if ((root = root->FirstChildElement()) != NULL)
        {
            do
            {
                try 
                {
                   node = new cSubMenuNode(root, 0,  &_menuTree, NULL);
                   const char* tmp_name = root->Attribute("name");
                   //printf("%s : %i \n",tmp_name, node->SubMenuSize());
                   if (tmp_name && strcmp("Additional Software",tmp_name) == 0 /*no translation of "Additional Software" here*/
                            && node->SubMenuSize() <= 0) // empty submenu;
                   // no additional plugins installed
                   {
                    delete node;
                    node = NULL;
                   }
                   else
                   _menuTree.Add(node);
                }
                catch (char *message)
                {
                   esyslog("ERROR: while decoding XML Node");
                   ok=false;
                }
    
              } while(ok==true && (root=root->NextSiblingElement()) !=NULL);
              //addMissingPlugins();
              removeUndefinedNodes();
            }
        }
        else
        {
          esyslog("ERROR: in %s, missing Tag <menus>\n", fname);
          ok=false;
        }
  }
  else
  {
     esyslog("ERROR: in %s : %s  Col=%d Row=%d\n", fname,
              xmlDoc.ErrorDesc(),
              xmlDoc.ErrorCol(),
              xmlDoc.ErrorRow());
    
    ok=false;
  }
  
  return ok;
}

bool cSubMenu::SaveXml()
{
  return(SaveXml(_fname));
}


bool cSubMenu::SaveXml(char *fname )
{
  
  if (_fname)
  {
     TiXmlDocument xml = TiXmlDocument(fname );
     TiXmlComment  comment;
     comment.SetValue(
     " Mini-VDR cSetupConfiguration File\n"
     " (c) Ralf Dotzert\n"
     "\n\n"
     " for Example see vdr-menu.xml.example\n\n"
     );
                    
     TiXmlElement root("menus");
     root.SetAttribute("suffix",   _menuSuffix);
     for (cSubMenuNode *node = _menuTree.First(); node; node = _menuTree.Next(node))
           node->SaveXml(&root);

     if (xml.InsertEndChild(comment) != NULL &&
         xml.InsertEndChild(root) != NULL)
     {
        return xml.SaveFile(fname);       
     }
  }
  else
  {
     return false;
  }

  return true;
}



cSubMenuNodes * cSubMenu::GetMenuTree()
{
  return _currentMenuTree;
}

void cSubMenu::PrintMenuTree()
{
   for (cSubMenuNode *node = _menuTree.First(); node; node = _menuTree.Next(node))
   {
      node->Print();
   }
}

int cSubMenu::GetNrOfNodes()
{
  if (_nrNodes == 0 )
  {
    if ((_nrNodes = countNodes(&_menuTree))>0)
    {
       _nodeArray = (cSubMenuNode**) malloc(sizeof(cSubMenuNode*)*_nrNodes);
       int index = 0;
       tree2Array(&_menuTree, index);
    }       
  }
    
  return _nrNodes;
}

/**
 * returns the specified node within the current menu
 * @param index position in the current menu
 * @return node or null if not found
 */
cSubMenuNode *cSubMenu::GetNode(int index)
{
 cSubMenuNode *node = NULL;

 if (_currentMenuTree == NULL || (node=_currentMenuTree->Get(index)) == NULL)
   esyslog("ERROR: illegal call of cSubMenu::GetNode(%d)", index);

 return node;

}




/**
 * Get the specified Node 
 * @param index specfies the absolut indes in the list of all nodes
 * @return node or NULL if not found
 */
cSubMenuNode * cSubMenu::GetAbsNode(int index)
{
  cSubMenuNode *node=NULL;
  
  GetNrOfNodes();
  if (_nrNodes>0 && index >=0 && index <_nrNodes)
  {
    node = _nodeArray[index];
  }
  return node;
}


bool cSubMenu::Down(int index)
{

 cSubMenuNode *node = NULL; 

 if (_currentMenuTree != NULL &&  (node=_currentMenuTree->Get(index)) !=NULL && node->GetType()==cSubMenuNode::MENU)
 {
   _currentParentMenuTree = _currentMenuTree;
   _currentMenuTree=node->GetSubMenus();
 }
 else
 {
   esyslog("ERROR: illegal call of cSubMenu::Down(%d)", index);
   return false;
 }
 return true;
}

bool cSubMenu::Up(int *parentIndex)
{

  if (_currentMenuTree != NULL && parentIndex != NULL)
  {
     cSubMenuNode *node = NULL; 
     *parentIndex =0;
     if (_currentParentMenuTree != NULL)
     {
       for(int i=0; (node=_currentParentMenuTree->Get(i))!= NULL ; i++)
       {
          if (_currentMenuTree == node->GetSubMenus())
          {
            *parentIndex =i;
            break;
          }
       }
     }

     _currentMenuTree = _currentParentMenuTree;
     if (_currentMenuTree != NULL)
        _currentParentMenuTree = _currentMenuTree->Get(0)->GetParentMenu();
     else
       return false;
  }
  else
  {
    esyslog("ERROR: illegal call of cSubMenu::Up()");
    return false;
  }
  return true;
}

const char *cSubMenu::ExecuteCommand(const char *cmd)
{
  free(_commandResult);
  _commandResult = NULL;

  dsyslog("executing command '%s'", cmd);
  cPipe p;
  if (p.Open(cmd, "r")) {
     int l = 0;
     int c;
     while ((c = fgetc(p)) != EOF) {
        if (l % 20 == 0)
           _commandResult = (char *)realloc(_commandResult, l + 21);
         _commandResult[l++] = c;
           }
        if (_commandResult)
          _commandResult[l] = 0;
	  p.Close();
  }
  else
     esyslog("ERROR: can't open pipe for command '%s'", cmd);

  return _commandResult;
}

/**
 * Move Menu Entry to new Position
 * @param index  index of menu entry to move 
 * @param toIndex index of destination
 * @param where After ore before the destination index
 */
void cSubMenu::MoveMenu(int index, int toIndex, enum Where where)
{
  if (index<0   || index > _nrNodes ||  // invalid index is ignored
     toIndex<0 || toIndex >_nrNodes || index == toIndex)
      return;

 cSubMenuNode *srcNode  = GetAbsNode(index);
 cSubMenuNode *destNode = GetAbsNode(toIndex);

 if (where == cSubMenu::INTO && destNode->GetType() != cSubMenuNode::MENU)
   return;
 

 if (where == cSubMenu::INTO)
 {
  if (destNode->GetType() == cSubMenuNode::MENU)
  {
     srcNode->GetCurrentMenu()->Del(srcNode, false);
         srcNode->SetLevel(destNode->GetLevel()+1);
     srcNode->SetParentMenu(destNode->GetCurrentMenu());
     srcNode->SetCurrentMenu(destNode->GetSubMenus());

     destNode->GetSubMenus()->Add(srcNode);
         reloadNodeArray();
  }
 }
 else
 {
   srcNode->GetCurrentMenu()->Del(srcNode, false);
   srcNode->SetLevel(destNode->GetLevel());
   srcNode->SetParentMenu(destNode->GetParentMenu());
   srcNode->SetCurrentMenu(destNode->GetCurrentMenu());

   if (where == cSubMenu::BEHIND)
   {
     destNode->GetCurrentMenu()->Add(srcNode, GetAbsNode(toIndex));
     reloadNodeArray();
   }
   else
   {
     destNode->GetCurrentMenu()->Ins(srcNode, GetAbsNode(toIndex));
         reloadNodeArray();
   }
 } 
}

/**
 * Create a new Menu Entry
 * @param index index of destination
 * @param menuTitle  Titel of new Menu entry
 */
void cSubMenu::CreateMenu(int index,const char * menuTitle )
{
  if (index >= 0  && index < _nrNodes)
  {
    cSubMenuNode *srcNode  = GetAbsNode(index);
    if (srcNode != NULL)
    {
         cSubMenuNode *newNode = new cSubMenuNode(srcNode->GetParentMenu(), srcNode->GetCurrentMenu());
         newNode->SetLevel(srcNode->GetLevel());
         newNode->SetName(menuTitle);
         newNode->SetType(cSubMenuNode::MENU);
         // Markus add SetInfo Help SetInfo ?? 
         newNode->SetParentMenu(srcNode->GetParentMenu());
         newNode->SetCurrentMenu(srcNode->GetCurrentMenu());

                  
         srcNode->GetCurrentMenu()->Add(newNode, GetAbsNode(index));
         reloadNodeArray();
    }
  }
}

/**
 * delete the specified entry, or subtree if the specified entry is a menu
 * @param index destion index 
 */
void cSubMenu::DeleteMenu(int index )
{
  if (index >= 0  && index < _nrNodes)
  {
    cSubMenuNode *srcNode  = GetAbsNode(index);
    srcNode->GetCurrentMenu()->Del(srcNode, true);
    reloadNodeArray();
  }
}


// Private Methods

int cSubMenu::countNodes(cSubMenuNodes * tree )
{
  int count =0;
  if (tree != NULL)
  {   
     for (cSubMenuNode *node = tree->First(); node; node = tree->Next(node))
     {
        count++;
        if (node->HasSubMenus())
        {
          count += countNodes(node->GetSubMenus());
        }         
     }
  }
  return count;
}


void cSubMenu::tree2Array(cSubMenuNodes * tree, int &index )
{
  if (tree != NULL)
  {
    for (cSubMenuNode *node = tree->First(); node; node = tree->Next(node))
    {
      _nodeArray[index++]=node;
      if (node->HasSubMenus())
      {
         tree2Array(node->GetSubMenus(), index);
      }         
    }
  }
}

bool cSubMenu::IsPluginInMenu(const char * name )
{
  bool found = false;
  for(int i=0; i<_nrNodes && found==false; i++)
  {
     cSubMenuNode *node = GetAbsNode(i);
     if (node != NULL && node->GetType()== cSubMenuNode::PLUGIN && strcmp(name, node->GetName())==0 )
          found = true;
  }
  return(found);
}

/**
 * Adds the given plugin to the Menu-Tree if not allready in List
 * @param name specifies the name of the plugin
 */
void cSubMenu::AddPlugin(const char * name )
{
  if (! IsPluginInMenu(name))
  {
     cSubMenuNode *node = new  cSubMenuNode(&_menuTree, NULL);
     node->SetName(name);
     node->SetType("plugin");
     node->SetPlugin();
     _menuTree.Add(node);
  }
}

void cSubMenu::addMissingPlugins()
{
   _nrNodes = GetNrOfNodes();
   for (unsigned int i = 0; i<cPluginManager::PluginCount() ; i++)
   {
     cPlugin *p = cPluginManager::GetPlugin(i);
     if (p)
     {
       AddPlugin(p->Name());
     }
     else
     {
//	break;
     }
   }
   reloadNodeArray();
}

/**
 * reloads the internal Array of Nodes
 */
void cSubMenu::reloadNodeArray()
{
  if (_nrNodes >0 ) free(_nodeArray);
  _nodeArray = NULL;
  _nrNodes   = 0;
  _nrNodes  = GetNrOfNodes();
}

/**
 * remove Undefined Nodes
 */
void cSubMenu::removeUndefinedNodes()
{
  bool remove=false;

  reloadNodeArray();
  for(int i=0; i<_nrNodes; i++)
  {
     cSubMenuNode *node = GetAbsNode(i);
     if (node != NULL && node->GetType()==cSubMenuNode::UNDEFINED)
     {
        cSubMenuNodes *pMenu = node->GetCurrentMenu();
        pMenu->Del(node, true);
        remove=true;
     }
  }
  if (remove)
    reloadNodeArray();
}

/**
* Retrieves the menu titel of parent menu
*/
const char *cSubMenu::GetParentMenuTitel()
{
  const char * result="";
  
  if( _currentMenuTree != NULL && _currentParentMenuTree != NULL)
  {
    cSubMenuNode *node = NULL;
    for(int i=0; (node=_currentParentMenuTree->Get(i))!= NULL ; i++)
    {
      if( _currentMenuTree == node->GetSubMenus())
      {
        result=node->GetName();
        break;
      }
    }
  }
  return(result);
}

// ------ cHelp -------------------------------------------------------------

cHelp::cHelp()
{
  editableWidth_ = cSkinDisplay::Current()->EditableWidth();
  // dsyslog(DBG_PREFIX "   edw: %d", editableWidth_);
  std::string helpDocument = setup::FileNameFactory("help");
  //node_
}

cHelp::~cHelp()
{

}


void cHelp::Load(std::string HelpFile)
{
  //load with cMenuMain() && cMenuSetupOSD (if lang changed)
  dsyslog(" SUBMENU cHelp::Load (parse help.de.xml)  %s ",HelpFile.c_str());



//http://www.grinninglizard.com/tinyxmldocs/tutorial0.html


#if 0
  text_.clear();

  dsyslog (" TR test: tr(eng)  %s  \n", I18nLanguageCode(::Setup.OSDLanguage));   // const char *s = I18nLanguageCode(Values[i]); 
  string helpfileEnding  = I18nLanguageCode(::Setup.OSDLanguage);   // const char *s = I18nLanguageCode(Values[i]); 
  helpfileEnding = helpfileEnding.substr(0, 3);
  
  string baseName = setup::FileNameFactory("help");
  baseName += HelpFile;
  string trHelpFile = baseName;
  trHelpFile += '.';
  trHelpFile += helpfileEnding;

  std::ifstream inFile(trHelpFile.c_str());

  if (!inFile.is_open())
  {
    esyslog ("Setup Erorr: Can`t open  help file %s ", trHelpFile.c_str());
    inFile.close(); 
    inFile.clear(); 

    inFile.open(baseName.c_str());
    if (!inFile.is_open())
       esyslog ("Setup Erorr: Can`t open  default help file  %s. Please check xmlConfig file ",baseName.c_str());
    
  }

  char line[255];

  while (inFile)
  {
    inFile.getline(line,255);
     // skip comments
    const char *res = NULL;
    res = strchr(line, '#');
    title_ = tr("Help");  //XX TODO
    if (res != &line[0])
    {
       for (unsigned int i = 0; i < strlen(&line[0]); i++)
           text_.push_back(line[i]);

       text_.push_back('\n');
    }
  }

  text_.push_back('\0');
  inFile.close();

  /*
  std::cout << " Dump Help Text \n";
  for(vector<char>::const_iterator iter = text_.begin(); iter != text_.end(); ++iter)
  std::cout << *iter;
  */
#endif

}


//################################################################################
//# 
//################################################################################

#endif
