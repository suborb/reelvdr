/*****************************************************************************
*DESCRIPTION:
*$Id: menus.cpp,v 1.11 2005/10/03 14:05:20 ralf Exp $
 *
 *Contact:    ranga@teddycats.de
 *
 *Copyright (C) 2004 by Ralf Dotzert
 ****************************************************************************/

#include <string>
#include <iostream>

#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>

#include <vdr/debug.h>
#include <vdr/plugin.h>
#include <vdr/submenu.h>

#include "config.h"
#include "menus.h"
#include "menusysteminfo.h" // HasDevice()
#include <vdr/sysconfig_vdr.h>
#include "util.h"

#define DBG ""

using std::string;
using std::cout;
using std::endl;

std::vector<MenuEntry*> vEntrys;
std::vector<IfBlock*> vIfBlocks;

namespace setup
{
    const char *GetPluginMenuEntry(const char *Plugin)
    {
        cPlugin *p = cPluginManager::GetPlugin(Plugin);
        return p?p->MainMenuEntry():NULL;
    }
}

//TODO: put this to reel lib
bool FileExists(cString file)
{
    return (*file && access(file, F_OK) == 0);
}

bool IsNetworkDevicePresent(const char* dev)
{
    // check if '/sys/class/net/'dev exists
    cString dev_in_sys = cString::sprintf("/sys/class/net/%s", dev);
    return FileExists(dev_in_sys);
}

// ###################################################################################
//  MenuNode
// ###################################################################################

/**
 *Constructor of Object
 *
 */

MenuNode::MenuNode()
{
    _menu = NULL;
    _menuEntry = NULL;
    _menuType = mtUndefined;
}

/**
 *Destructor of Object
 *
 */

MenuNode::~MenuNode()
{
    destroy();
}

/**
 *Destroy Object
 */
void MenuNode::destroy()
{
    delete _menu;
    delete _menuEntry;
    _menuType = mtUndefined;
    _menu = NULL;
    _menuEntry = NULL;
}

/**
 *
 *@param menu
 */
void MenuNode::SetNode(Menu *menu)
{
    _menu = menu;
    if (menu->GetSystem())
    {
        _menuType = mtMenusystem;
    }
    else if (menu->GetPluginName())
    {
        _menuType = mtMenuplugin;
    }
    else if (menu->GetExecute())
    {
        _menuType = mtExecute;
    }
    else
    {
        _menuType = mtMenu;
    }
}

/**
 *
 *@param menuEntry
 */
void
MenuNode::SetNode(MenuEntry *menuEntry)
{
    _menuEntry = menuEntry;
    _menuType = mtEntry;
}

/**
 *
 *@param ifBlock
 */
void MenuNode::SetNode(IfBlock *ifBlock)
{
    _ifBlock = ifBlock;
    _menuType = mtIfblock;
}

/**
 *
 *@return
 */
eMenuType
MenuNode::GetType()
{
    return _menuType;
}

/**
 *
 *@return
 */
Menu *
MenuNode::GetMenu()
{
    if (_menuType >= mtMenu)
        return _menu;
    else
        return NULL;
}

/**
 *
 *@return
 */
IfBlock* MenuNode::GetIfBlock()
{
    if (_menuType == mtIfblock)
        return _ifBlock;
    else
        return NULL;
}

/**
 *
 *@return
 */
MenuEntry *
MenuNode::GetMenuEntry()
{
    // XXX Abfrage notwendig ?
    if (_menuType == mtEntry)
        return _menuEntry;
    else
        return NULL;
}

int
MenuNode::GetNr()
{
    if (_menuType == mtMenu)
        return _menu->GetNr();
    else if(_menuType == mtIfblock)
        return _ifBlock->isOpen() ? _ifBlock->GetNr():0 ;
    else
        return _menuEntry->GetNr();

}

MenuNode *
MenuNode::GetNode_openIfs(int index)
{
    if (_menuType == mtMenu)
        return _menu->GetNode_openIfs(index);
    else if(_menuType == mtIfblock)
        return _ifBlock->isOpen() ? _ifBlock->GetNode_openIfs(index):NULL ;
    else
        return _menuEntry->GetNode_openIfs(index);
}

MenuNode *
MenuNode::GetNode(int index)
{
    if (_menuType == mtMenu)
        return _menu->GetNode(index);
    else if(_menuType == mtIfblock)
        return _ifBlock->GetNode(index);
    else
        return _menuEntry->GetNode(index);
}

MenuNode *
MenuNode::GetNode(const char *NodeName)
{
    int index = 5;
    if (_menuType == mtMenu)
        return (_menu->GetNode(index));
    else if(_menuType == mtIfblock)
        return (_ifBlock->GetNode(index));
    else
        return (_menuEntry->GetNode(index));
}

/**
 *
 *@return
 */
const char *
MenuNode::GetName()
{
    if (_menu)
        return _menu->GetName();

    if (_menuEntry)
        return _menuEntry->GetName();

    return NULL;
}


/**
 *
 */
void
MenuNode::Print(int offset)
{
    if (_menuType == mtMenu)
    {
        _menu->Print(offset);
    }
    else if (_menuType == mtEntry)
    {
        _menuEntry->Print(offset);
    }
}

/**
 *
 */
void
MenuNode::Print()
{
    Print(0);
}

// ###################################################################################
//  Menu
// ###################################################################################

/**
 *
 *@return
 */
Menu::Menu()
{
    _name = NULL;
    _command = NULL;
    _execute = NULL;
    _plugin = NULL;
    _system = NULL;
    _info = NULL;
    _help = NULL;
}

/**
 *
 *@param name
 *@return
 */
Menu::Menu(const char *Name)
{
    _name = Util::Strdupnew(Name);
    _command = NULL;
    _execute = NULL;
    _plugin = NULL;
    _system = NULL;
    _info = NULL;
    _help = NULL;
}

/**
 *
 *@return
 */
Menu::~Menu()
{
    destroy();
}

/**
 *Destroy Object
 */
void
Menu::destroy()
{
    delete[]_command;
    delete[]_execute;
    delete[]_plugin;
    delete[]_system;
    delete[]_info;
    delete[]_help;
    _name = NULL;
    _command = NULL;
    _execute = NULL;
    _system = NULL;
    _plugin = NULL;
    _info = NULL;
    _help = NULL;
}


/**
 *Load XML subtree
 *@param xmlNode current XML Node to Parse
 *@return true on success
 */
bool Menu::LoadXml(TiXmlNode *xmlNode)
{
    bool ok = true;
    TiXmlElement *elem = NULL;

    if (xmlNode != NULL)
    {
        do
        {
            if (xmlNode->Type() == TiXmlNode::ELEMENT)
            {
                elem = xmlNode->ToElement();
                if (strcmp(xmlNode->Value(), "menu") == 0)
                {
                    const char *name = elem->Attribute("name");
                    const char *command = elem->Attribute("command");
                    const char *system = elem->Attribute("system");
                    const char *plugin = elem->Attribute("plugin");
                    const char *execute = elem->Attribute("execute");
                    const char *info = elem->Attribute("info");
                    const char *help = elem->Attribute("help");

                    if (!name)  // first we have to set Translated Name
                    {
                        if (plugin)
                        {
                            name = setup::GetPluginMenuEntry(plugin);
                            //dsyslog(DBG " GetPlugin Entry %s",name);
                        }
                        if (system)
                            name = system;      //dup ??

                    }
                    if (name)   // all names should be set now
                    {
                        Menu *menu = new Menu(name);

                        if (menu && (ok = menu->LoadXml(xmlNode->FirstChild())) == true)
                        {
                            if (system)
                                menu->SetSystem(system);
                            if (plugin)
                                menu->SetPluginName(plugin);
                            if (command)
                                menu->SetCommand(command);
                            if (execute)
                                menu->SetExecute(execute);
                            if (info)
                                menu->SetInfo(info);
                            if (help)
                            {
                                if (!info)
                                    menu->SetInfo("(?)");
                                menu->SetHelp(help);
                            }

                            AddNode(menu);
                        }
                        else
                        {
                            delete menu;
                        }
                    }
                    else        // ok = false;
                    {
                        isyslog(DBG " menu entry for  %s found, but plugin not found", name);
                    }

                }
                else if (strcmp(xmlNode->Value(), "if_equal") == 0)
                {
                    const char *IDstring = elem->Attribute("id");
                    int ID = 0;
                    bool validID = Util::isNumber(IDstring, ID);
                    const char *value = elem->Attribute("value");
                    const char *name = elem->Attribute("name");
                    IfBlock *ifblock = new IfBlock();

                    if (validID && ifblock && (ok = ifblock->LoadXml(xmlNode->FirstChild())) == true)
                    {
                        ifblock->SetValue(value);
                        ifblock->SetID(ID);

                        vIfBlocks.push_back(ifblock);

                        AddNode(ifblock);
                    }
                    else if (ifblock && (ok = ifblock->LoadXml(xmlNode->FirstChild())) == true)
                    {
                        if (name)
                        {
                            ifblock->SetName(name);
                            ifblock->SetType(Util::UNDEFINED);
                        }

                        vIfBlocks.push_back(ifblock);

                        AddNode(ifblock);
                    }
                    else
                    {
                        delete ifblock;
                    }
                }
                else if (strcmp(xmlNode->Value(), "entry") == 0)
                {
                    const char *IDstring = elem->Attribute("id");
                    const char *command = elem->Attribute("command");
                    const char *info = elem->Attribute("info");
                    const char *help = elem->Attribute("help");
                    MenuEntry *entry = new MenuEntry();

                    if ((ok = entry->AddEntry(xmlNode)) == true)
                    {
                        entry->SetCommand(command);
                        if (info)
                            entry->SetInfo(info);
                        if (help)
                        {
                            if (!info)
                                entry->SetInfo("(?)");
                            entry->SetHelp(help);
                        }
                        if(IDstring)
                        {
                            int ID = 0;
                            bool duplicateID = false;
                            Util::isNumber(IDstring, ID);
                            entry->SetID(ID);

                            int i=0;
                            while(!duplicateID && i < (int)vEntrys.size())
                            {
                                if(vEntrys.at(i)->GetID() == ID)
                                    duplicateID = true;
                                ++i;
                            }
                            if(duplicateID)
                                esyslog("Warning: Duplicate ID-tag found in vdr-setup.xml: name=\"%s\" ID=%i", elem->Attribute("name"), ID);
                            else
                            {
                                if((entry->GetType() != Util::SPACE) && (entry->GetType() != Util::UNDEFINED))
                                    vEntrys.push_back(entry);
                            }
                        }
                        AddNode(entry);
                    }
                    else
                    {
                        delete entry;
                    }
                }
            }
        }
        while ((xmlNode = xmlNode->NextSibling()) != NULL && ok == true);

    }
    //DLOG(DBG " -- MenuLoad ret %s ", ok ? "YES" : "NO");
    return ok;
}

/**
 *Save Menu to XML Structure
 *@param xml xml structure
 *@return true on success
 */
TiXmlNode *
Menu::SaveXml()
{
    // This Funcion is never used in this version
    bool ok = true;
    TiXmlElement *m = new TiXmlElement("menu");
    TiXmlNode *tmpXmlNode = NULL;

    // we don`t want names in system or plugin menus
    if (!(GetSystem() || GetPluginName()))
        m->SetAttribute("name", GetName());

    if (GetSystem() != NULL)
        m->SetAttribute("system", GetSystem());

    if (GetPluginName())
        m->SetAttribute("plugin", GetPluginName());

    if (GetCommand())
        m->SetAttribute("command", GetCommand());

    for (MenuNode *subMenu = _subMenus.First(); subMenu;
         subMenu = _subMenus.Next(subMenu))
    {
        if (subMenu->GetType() == mtMenu ||
            subMenu->GetType() == mtMenusystem ||
            subMenu->GetType() == mtMenuplugin)
        {
            tmpXmlNode = subMenu->GetMenu()->SaveXml();
        }
        else
        {
            if (subMenu->GetType() == mtEntry)
                tmpXmlNode = subMenu->GetMenuEntry()->SaveXml();

        }

        if (tmpXmlNode)
            m->LinkEndChild(tmpXmlNode);
        else
            ok = false;
    }

    if (ok == false)
    {
        delete m;
        m = NULL;
    }

    return (m);
}


/**
 *
 */
void
Menu::Print(int offset)
{
    for (int i = 0; i < offset; i++)
        printf("-");

    printf("Menu: Name: %s \n", _name);

    for (MenuNode *subMenu = _subMenus.First(); subMenu;
         subMenu = _subMenus.Next(subMenu))
    {
        for (int j = 0; j < offset; j++)
            printf("-");
        subMenu->Print(offset + 4);
    }
}

/**
 *
 */
void
Menu::Print()
{
    Print(0);
}

///
MenuNode* Menu::GetNode_openIfs(int index)
{
    MenuNode *tmpNode    = NULL;
    MenuNode *returnNode = NULL;


    int i=0;
    while (i < GetNr() )
    {
        tmpNode = _subMenus.Get(i);
        ++i;

        if ( !tmpNode ) { printf("(%s:%i) Null MenuNode at %i!!\n", __FILE__,__LINE__, i-1); continue; }

        // node exists & is not an "if block"
        if ( tmpNode->GetType() != mtIfblock )
        {
            if ( index == 0 ) return tmpNode;

            // look at the next one
            --index;
            continue;
        }

        // if block is not open: contibutes nothing to the OSD
        if ( ! tmpNode->GetIfBlock()->isOpen() ) continue;

        // if block OPEN
        // So, look into the block
        returnNode = tmpNode->GetIfBlock()->GetNode_openIfs ( index );

        // found it in the ifblock
        if ( returnNode ) return returnNode;

        // Not found : "if block" was too small
        // reduce index by "if block"'s actual size(): ie. a closed if inside if block doesnot count towards size()
        index -= tmpNode->GetIfBlock()->Get_actualSize();
    }

    // not found
    return NULL;
}

// returns the actual size of the menu , taking into account if_block's state
// closed if_block count as zero sized entry
int Menu::Get_actualSize()
{
    int size=0;
    int i=0;
    for ( i=0; i<GetNr(); ++i)
    {
        if ( _subMenus.Get(i)->GetType() != mtIfblock ) ++size;
        else
        // if block
        if (_subMenus.Get(i)->GetIfBlock()->isOpen())
        size += _subMenus.Get(i)->GetIfBlock()->Get_actualSize();
    }

    return size;
}
// ###################################################################################
//  IfBlock
// ###################################################################################

IfBlock::IfBlock()
{
    _ID = -1;
    _value = NULL;
    _name = NULL;
    _type = Util::UNDEFINED;

    _valueTextRef = NULL;
    _valueIpRef = NULL;
//    _valueTextMaxLenRef = NULL;
//    _valueIpMaxLenRef = NULL;
    _valueNumberRef = NULL;
    _valueBoolRef = NULL;
}

IfBlock::~IfBlock()
{
    delete[]_value;
    _value = NULL;
    if (_name)
    {
        delete[]_name;
        _name = NULL;
    }
}

void IfBlock::SetValue(const char *Value)
{
    if(Value)
    {
        delete[]_value;
        _value = Util::Strdupnew(Value, VALUETEXTMAXLEN);
    }
}

void IfBlock::SetName(const char *Name)
{
    if(Name)
    {
        delete[]_name;
        _name = Util::Strdupnew(Name, VALUETEXTMAXLEN);
    }
}

bool IfBlock::isOpen()
{
    switch(_type)
    {
        case Util::BOOL:
            {
                bool valueBoolean;
                if(Util::isBool(_value, valueBoolean))
                    if(valueBoolean == *_valueBoolRef)
                        return true;
            }
            break;
        case Util::SELECTION:
            if(strcmp(_value, _selectionValuesRef->GetSelectedValue()) == 0)
                return true;
            break;
        case Util::CONSTANT_TEXT: return true;
        case Util::TEXT:
        case Util::NUMBER_TEXT:
        case Util::HEX:
        case Util::PIN:
            if(strcmp(_value, *_valueTextRef) == 0)
                return true;
            break;
        case Util::IP:
            if(strcmp(_value, *_valueIpRef) == 0)
                return true;
            break;
        case Util::NUMBER:
            {
                int valueNumber;
                if(Util::isNumber(_value, valueNumber))
                    if(valueNumber == *_valueNumberRef)
                        return true;
            }
        case Util::UNDEFINED:
            {
                /** TB: special ifBlock to be able to query if it's AVGI or AVGII within the XML-file */
                if (_name && strcmp(_name, "AVGI") == 0) {
                    if(HasDevice("808627a0"))
                        return true;
                } else if (_name && strcmp(_name, "AVGII") == 0) {
                    if (HasDevice("10027910"))
                        return true;
                } else if (_name && strcmp(_name, "RBAVGIORII") == 0) {
                    if (HasDevice("808627a0") || HasDevice("10027910"))
                        return true;
                } else if (_name && strcmp(_name, "RBAVG") == 0) {
                    if (HasDevice("808627a0") || HasDevice("10027910"))
                        return true;
                } else if (_name && strcmp(_name, "NORBAVG") == 0) {
                    if (!HasDevice("808627a0") && !HasDevice("10027910"))
                        return true;
                } else if (_name && strcmp(_name, "RBNCL") == 0) {
                    if(HasDevice("14f12450"))
                        return true;
                } else if (_name && strcmp(_name, "NORBNCL") == 0) {
                    if(!HasDevice("14f12450"))
                        return true;
                } else if (_name && strcmp(_name, "MULTIROOM") == 0) {
                    if(::Setup.ReelboxModeTemp != eModeStandalone)
                        return true;
                } else if (_name && strcmp(_name, "NOMULTIROOM") == 0) {
                    if(::Setup.ReelboxModeTemp == eModeStandalone)
                        return true;
                } else if (_name && strcmp(_name, "HASFP") == 0) {  //check for frontpanel (FP)
                    if (access("/dev/.fpctl.avr", R_OK)==0)
                        return true;
                } else if (_name && strcmp(_name, "HASNOFP") == 0) {
                    if (access("/dev/.fpctl.avr", R_OK) != 0)
                        return true;
                } else if (_name && strcmp(_name, "HASETH1") == 0) {
                    return IsNetworkDevicePresent("eth1");
                } else if (_name && strcmp(_name, "NOETH1") == 0) {
                    return ! IsNetworkDevicePresent("eth1");
                }
                return false;
            }
        default:
            return false;
    }

    return false;
}

// ###################################################################################
//  MenuEntry
// ###################################################################################
/**
 *
 *@return
 */
MenuEntry::MenuEntry()
{
    _sysconfigName = NULL;
    _info = NULL;
    _help = NULL;
    _valueText = NULL;
    _valueIp = NULL;
    _valueTextMaxLen = 0;
    _valueIpMaxLen = 0;
    _valueNumber = 0;
    _valueBool = 0;
    _setupCommand = NULL;
    _type = Util::UNDEFINED;
    _command = NULL;
    _system = NULL;
    _ID = 0;
}

/**
 *
 *@return
 */
MenuEntry::~MenuEntry()
{
    destroy();
}


/**
 *Destroy Object
 */
void
MenuEntry::destroy()
{
    delete[]_sysconfigName;
    delete[]_info;
    delete[]_help;
    delete[]_valueText;
    delete[]_valueIp;
    delete[]_setupCommand;
    _sysconfigName = NULL;
    _valueText = NULL;
    _valueIp = NULL;
    _info = NULL;
    _help = NULL;
    _setupCommand = NULL;

}



/**
 *set the corrspondig key in the sysconfig file
 *@param nam the name used in the sysconfig file
 */
void
MenuEntry::SetSysconfigName(const char *key)
{
    _sysconfigName = Util::Strdupnew(key);
}

/**
 *
 *@return the name used in the sysconfig file
 */
const char *
MenuEntry::SysconfigKey()
{
    return _sysconfigName;
}


/**
 *
 *@param typ Set Type
 */
void
MenuEntry::SetType(Util::Type Typ)
{
    _type = Typ;
}

/**
 *
 *@return
 */
Util::Type* MenuEntry::GetTypeRef()
{
    return &_type;
}

/**
 *
 *@return
 */
Util::Type MenuEntry::GetType()
{
    return _type;
}


/**
 *Set the Value
 *@param val
 */
void MenuEntry::SetValue(Util::Type Type, const char *Value)
{

    if (Type != Util::UNDEFINED)
        _type = Type;

    switch (_type)
    {

    case Util::BOOL:
        Util::isBool(Value, _valueBool);
        break;

    case Util::CONSTANT_TEXT:
        _valueText = NULL;
        _valueTextMaxLen = 0;
        break; // no value for constant text
    case Util::TEXT:
        delete[]_valueText;
        _valueText = Util::Strdupnew(Value, VALUETEXTMAXLEN);
        _valueTextMaxLen = VALUETEXTMAXLEN;
        break;
    case Util::IP:
        delete[]_valueIp;
        _valueIp = Util::Strdupnew(Value, VALUEIPMAXLEN);
        _valueIpMaxLen = VALUEIPMAXLEN;
        break;
    case Util::PIN:
        delete[]_valueText;
        _valueText = Util::Strdupnew(Value, 4);
        _valueTextMaxLen = 4;
        break;
    case Util::NUMBER_TEXT:
        delete[]_valueText;
        _valueText = Util::Strdupnew(Value, VALUETEXTMAXLEN);
        _valueTextMaxLen = VALUETEXTMAXLEN;
        break;
    case Util::HEX:
        delete[]_valueText;
        _valueText = Util::Strdupnew(Value, VALUETEXTMAXLEN);
        _valueTextMaxLen = VALUETEXTMAXLEN;
        break;
    case Util::NUMBER:
        Util::isNumber(Value, _valueNumber);
        break;
    case Util::SELECTION:
        delete[]_valueText;
        _valueText = Util::Strdupnew(Value, VALUETEXTMAXLEN);
        _valueTextMaxLen = VALUETEXTMAXLEN;
        break;
    case Util::SPACE:
    case Util::UNDEFINED:
        break;
    }

}

void
MenuEntry::SetSysConfValue()
{
    cSysConfig_vdr::GetInstance().SetVariable(_sysconfigName, GetValueAsString());
}

const char *
MenuEntry::GetSelectedValue()
{
    return _selectionValues.GetSelectedValue();
}


const char *
MenuEntry::GetSelectionValue(int index)
{
    return _selectionValues.GetValue(index);
}

int
MenuEntry::GetNrOfSelectionValues()
{
    return _selectionValues.GetNr();
}

const char **
MenuEntry::GetSelectionValues()
{
    return _selectionValues.GetValues();
}

void
MenuEntry::SetSelection(const char *val)
{
    _selectionValues.SetSelection(val);
}

void
MenuEntry::SetSetupCommand(const char *command)
{
    if (command)
        _setupCommand = Util::Strdupnew(command);
}

const char *
MenuEntry::GetSetupCommand()
{
    return _setupCommand;
}


/**
 *Return the Value
 *@return
 */
char *
MenuEntry::GetValue()
{
    return _valueText;
}


int
MenuEntry::GetValueTextMaxLen()
{
    return _valueTextMaxLen;
}

/**
 *
 *@return
 */
const char *
MenuEntry::GetValueAsString()
{
    //const char *result=NULL;
    static char numberStr[20];

    switch (_type)
    {
    case Util::BOOL:
        return Util::boolToStr(_valueBool);
    case Util::NUMBER_TEXT:
    case Util::PIN:
    case Util::HEX:
    case Util::TEXT:
        return _valueText;
    case Util::IP:
        return _valueIp;
    case Util::NUMBER:
        sprintf(numberStr, "%d", _valueNumber);
        return (const char *)numberStr;
    case Util::SELECTION:
        return _selectionValues.GetSelectedValue();
    case Util::CONSTANT_TEXT: // no value for constant text
    default:
        return Util::typeToStr(_type);
    }
    return Util::typeToStr(_type);
}

/**
 *
 *@return
 */
int *
MenuEntry::GetValueBoolRef()
{
    return &_valueBool;
}

/**
 *
 *@return
 */
int *
MenuEntry::GetValueNumberRef()
{
    return &_valueNumber;
}


/**
 *
 *@param node
 *@param selection
 *@return
 */
//bool MenuEntry::AddSelectionValues(TiXmlNode *node, const char *selection)
bool
MenuEntry::AddSelectionValues(TiXmlNode *node)
{
    bool ok = true;

    if (_setupCommand)          // special cases
    {
        if (strcmp(_setupCommand, "networkdriver") == 0)
        {
            ok = ReadNetworkDrivers();
        }

        if (strcmp(_setupCommand, "CHANNELLIST") == 0)
        {
            ok = ChannelList();
        }
    }
    else if (node != NULL && node->Type() == TiXmlNode::ELEMENT)
    {
        do
        {
            _selectionValues.Add(node->FirstChild()->Value());

        }
        while ((node = node->NextSibling("value")) != NULL && ok);
    }
    return ok;
}


/**
 *
 *@return
 */
bool
MenuEntry::ChannelList()
{

    //printf ("  MenuEntry  ChannelList() \n");
    bool ok = false;
    DIR *dh = NULL;
    struct dirent *entry = NULL;

    string channelDir = setup::FileNameFactory("channels");
    //std:: cout << " channel dir returnd by FileNameFactory : " << channelDir << endl;

    if ((dh = opendir(channelDir.c_str())) != NULL)
    {
        while ((entry = readdir(dh)) != NULL)
        {
            char *tmp = NULL;
            if ((tmp = strstr(entry->d_name, ".conf")) != NULL)
            {
                tmp[0] = '\0';
                ////printf ("   Add  %s \n",  entry->d_name);
                _selectionValues.Add(entry->d_name);
            }
        }

        /*
           char buf[PATH_MAX];
           if (realpath(channelLink.c_str(), buf) != NULL)
           {
           printf (" Set Selection %s \n", buf);
           _selectionValues.SetSelection(buf);
           }
           else
           {
           DLOG("%s Can not resolve realpath of %s  errno=%d\n", DBG, channelLink.c_str(), errno);
           ok = false;
           }
         */
        ok = true;
    }
    else
    {
        DLOG("%s Can not read directory:%s  errno=%d -- %s:%d ", DBG,
             channelDir.c_str(), errno, __FILE__, __LINE__);
        ok = false;
    }

    if (dh != NULL)
        closedir(dh);

    return ok;
}

/**
 *
 *@return
 */
bool
MenuEntry::ReadNetworkDrivers()
{
    struct utsname uName;
    bool ok = true;

    if (uname(&uName) == 0)
    {
        char dir[255];

        snprintf(dir, 255, "/lib/modules/%s/kernel/drivers/net",
                 uName.release);
        readNetworkDriversDir(dir, (char*)"");
        //if (dir!= NULL) free(dir);
    }
    else
    {
        DLOG("%s UNAME failed, errno=%d\n", DBG, errno);
        ok = false;
    }

    return (ok);
}

/**
 *
 *@param dir
 *@return
 */
bool
MenuEntry::readNetworkDriversDir(char *dir, char *prefix)
{
    DIR *modDir = NULL;
    struct dirent *entry = NULL;
    struct stat buf;
    bool ok = true;
    char *path = NULL;

    asprintf(&path, "%s/%s", dir, prefix);

    if ((modDir = opendir(path)) != NULL)
    {
        while ((entry = readdir(modDir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") != 0
                && strcmp(entry->d_name, "..") != 0)
            {
                char *tmp = NULL;
                asprintf(&tmp, "%s/%s", path, entry->d_name);
                stat(tmp, &buf);
                free(tmp);

                if (S_ISDIR(buf.st_mode))
                {
                    char *newPrefix = NULL;
                    if (strlen(prefix) > 0)
                        asprintf(&newPrefix, "%s/%s", prefix, entry->d_name);
                    else
                        asprintf(&newPrefix, "%s", entry->d_name);

                    readNetworkDriversDir(dir, newPrefix);
                    free(newPrefix);
                }
                else
                {
                    if (S_ISREG(buf.st_mode))
                    {
                        if ((tmp = strstr(entry->d_name, ".o")) != NULL ||
                            (tmp = strstr(entry->d_name, ".ko")) != NULL)
                        {
                            tmp[0] = '\0';
                            char *myEntry = NULL;
                            if (strlen(prefix) != 0)
                                asprintf(&myEntry, "%s/%s", prefix,
                                         entry->d_name);
                            else
                                asprintf(&myEntry, "%s", entry->d_name);

                            _selectionValues.Add(myEntry);
                            free(myEntry);
                        }
                    }
                }
            }
        }
    }
    else
    {
        ok = false;
        DLOG("%s Can not read directory:%s  errno=%d -- %s:%d ", DBG, path,
             errno, __FILE__, __LINE__);
    }
    if (modDir != NULL)
        closedir(modDir);
    if (path != NULL)
        free(path);

    return ok;
}

int *MenuEntry::GetReferenceSelection()
{
    return _selectionValues.GetReferenceSelection();
}


/**
 *Add SubEntry to
 *@param node XML poiter to subtree
 *@return ok on success
 */
bool MenuEntry::AddEntry(TiXmlNode *node)
{
    //printf (" MenuEntry::AddEntry(TiXmlNode *node) start \n");
    bool ok = true;
    TiXmlElement *elem = NULL;
    cSysConfig_vdr & sysConfig = cSysConfig_vdr::GetInstance();

    if (node && (node->Type() == TiXmlNode::ELEMENT))
    {
        //printf ("  (node->Type() == TiXmlNode::ELEMENT)) \n");

        elem = node->ToElement();
        const char *nam = elem->Attribute("name");
        const char *sysconf = elem->Attribute("sysconfig");
        const char *typStr = elem->Attribute("type");
        const char *val = elem->Attribute("value");     // default Value
        const char *setup = elem->Attribute("setup");
        const char *info = elem->Attribute("info");
        const char *help = elem->Attribute("help");

        Util::Type typ = Util::UNDEFINED;
        bool validType = Util::isType(typStr, typ);

        if (validType &&  nam != NULL && sysconf != NULL && typStr != NULL && val != NULL)
        {
            //printf (" default elem   \n");
            const char *tmp;
            if ((tmp = sysConfig.GetVariable(sysconf)) != NULL)
            {
                if (strcmp(sysconf, "CHANNELLIST") == 0)
                {
                    setup = "CHANNELLIST";
                }

                SetValue(typ, tmp);
            }
            else
            {
                SetValue(typ, val);     // Set default value from xml file if no Sysconfig Value found
            }
            SetName(nam);
            SetSysconfigName(sysconf);
            SetSetupCommand(setup);

            if (info)
                SetInfo(info);
            if (help)
            {
                if (!info)
                    SetInfo("(?)");
                SetHelp(help);
            }

            ok = AddSelectionValues(node->FirstChild("value"));

            if (ok && _type == Util::SELECTION)
            {
                SetSelection(tmp ? tmp : val);
            }
        }
        else if (typ == Util::SPACE)
        {
//           printf("\033[0;41m  GET SPACE Item ret true \033[0m\n");
           SetName("SPACE");
           SetSysconfigName("SPACE");
           SetSetupCommand("SPACE");
           SetValue(typ, "");
           ok = true;
        }
        else if (typ == Util::CONSTANT_TEXT && nam != NULL)
        {
            SetName(nam);
            SetValue(typ, "");
            ok = true;
        }
        else
        {
            DLOG("%s Error in XML File, Column=%d Row%d\n", DBG,
                 node->Column(), node->Row());
            ok = false;
        }
    }
    return ok;
}

TiXmlNode* IfBlock::SaveXml()
{
    TiXmlElement *if_xml = new TiXmlElement("if_equal");
    /***
     * <if_equal id="1" value="2">
     *
     *      <entry.../>
     *      <menu .../>
     *      <if_equal .../>
     *
     * </if_equal>
     ***/

     if ( if_xml==NULL ) return NULL;

     // set attributes : id and value
     if_xml->SetAttribute( "id",     GetID() );
     if_xml->SetAttribute( "value",  GetValue() );

    // get all the children
    TiXmlNode *child;

    for (MenuNode *subMenu = _subMenus.First(); subMenu;
         subMenu = _subMenus.Next(subMenu))
    {
        switch (subMenu->GetType() )
        {
            case mtEntry:
                child = subMenu->GetMenuEntry()->SaveXml();
                break;
            case mtIfblock:
                child = subMenu->GetIfBlock()->SaveXml();
                break;
            case mtMenu:
                child = subMenu->GetMenu()->SaveXml();
                break;
            default:
                printf("(%s:%i) Should not be here\n",__FILE__,__LINE__);
                break;
        }

        if_xml->LinkEndChild( child );
        child = NULL;
    }

    return if_xml;
}

/**
 *Convert Entry in XML Structure
 *@return xml Element or NULL if error
 */
TiXmlNode *
MenuEntry::SaveXml()
{
    printf(" ------------------- Save XML -------------------------- \n");
    bool ok = true;
    char *channelDir = NULL;
    char *channelFile = NULL;

    asprintf(&channelDir, "%s/../channels", cPlugin::ConfigDirectory());
    asprintf(&channelFile, "%s/../channels.conf", cPlugin::ConfigDirectory());

    TiXmlElement *e = new TiXmlElement("entry");
    if (e != NULL)
    {
        e->SetAttribute("name", GetName());
        e->SetAttribute("sysconfig", SysconfigKey());
        e->SetAttribute("type", Util::typeToStr(_type));

        if (_setupCommand != NULL)
            e->SetAttribute("setup", _setupCommand);

        if (_type == Util::SELECTION)
        {
            if (_setupCommand != NULL)
            {
                if (strcmp(_setupCommand, "CHANNELLIST") == 0)
                {
                    /// XXX Fixed this
                    if (unlink(channelFile) == 0)
                    {
                        char *tmp = NULL;
                        asprintf(&tmp, "%s/%s.conf", channelDir,
                                 GetSelectedValue());
                        if (symlink(tmp, channelFile) != 0)
                        {
                            ok = false;
                            DLOG("%s Can not link File %s  to %s errno=%d\n", DBG, tmp, channelFile, errno);
                        }
                        free(tmp);
                    }
                    else
                    {
                        ok = false;
                        DLOG("%s Can not unlink File %s  errno=%d\n", DBG, channelFile, errno);
                    }
                }
            }
            else
            {
                for (int i = 0; i < GetNrOfSelectionValues(); i++)
                {
                    TiXmlElement *e1 = new TiXmlElement("value");
                    e1->LinkEndChild(new TiXmlText(GetSelectionValue(i)));
                    e->LinkEndChild(e1);
                }

            }
            e->SetAttribute("value", GetSelectedValue());
        }
        else
            e->SetAttribute("value", GetValueAsString());

        if (GetCommand() != NULL)
            e->SetAttribute("command", GetCommand());

    }

    for (MenuNode *subMenu = _subMenus.First(); subMenu;
         subMenu = _subMenus.Next(subMenu))
    {
        e->LinkEndChild(subMenu->GetMenuEntry()->SaveXml());
    }

    if (channelDir != NULL)
        free(channelDir);
    if (channelFile != NULL)
        free(channelFile);

    return (e);
}




void
MenuEntry::SetSysConfig()
{
    cSysConfig_vdr & sysConfig = cSysConfig_vdr::GetInstance();
    //XXX const_cast ??
    sysConfig.SetVariable(SysconfigKey(),
                          const_cast < char *>(GetValueAsString()));

}


/**
 *
 */
void
MenuEntry::Print(int offset)
{
    for (int i = 0; i < offset; i++)
        printf("-");

    printf("Entry: Name: %s SysconfigName=%s Type=%s Value=%s",
           _name, _sysconfigName, Util::typeToStr(_type), GetValueAsString());
    if (_command != NULL)
        printf(" command=%s", _command);
    _selectionValues.Print(offset);


    for (MenuNode *subMenu = _subMenus.First(); subMenu;
         subMenu = _subMenus.Next(subMenu))
    {
        for (int j = 0; j < offset; j++)
            printf("-");
        subMenu->Print(offset + 4);
    }

}

/**
 *
 */
void
MenuEntry::Print()
{
    Print(0);
}


// ###################################################################################
//  Menus
// ###################################################################################


/**
 *Constructor of Object
 *
 */
Menus::Menus()
{

}


/**
 *Destructor of Object
 *
 */
Menus::~Menus()
{
}


/**
 *Load XML representation
 *@param node current node in XML Tree
 *@return true on success
 */
bool
Menus::LoadXml(TiXmlNode *node)
{
    vEntrys.clear();
    vIfBlocks.clear();

    //printf(" ---- Load XML MENUES  \n");
    bool ok = true;
    TiXmlElement *elem = NULL;
    if (node && node->Type() == TiXmlNode::ELEMENT
        && (node = node->FirstChild()) != NULL)
    {
        do
        {
            if (node->Type() == TiXmlNode::ELEMENT
                && strcmp(node->Value(), "menu") == 0)
            {
                elem = node->ToElement();
                const char *name = elem->Attribute("name");
                const char *command = elem->Attribute("command");
                const char *execute = elem->Attribute("execute");
                const char *system = elem->Attribute("system");
                const char *plugin = elem->Attribute("plugin");
                const char *info = elem->Attribute("info");
                const char *help = elem->Attribute("help");

                if (!name)      // if no name means MenuEntry!
                {
                    if (plugin)
                        name = setup::GetPluginMenuEntry(plugin);
                    if (system)
                        name = system;
                }
                if (name)
                {
                    ok = true;
                    Menu *menu = new Menu(name);
                    if (command)
                        menu->SetCommand(command);
                    if (execute)
                        menu->SetExecute(execute);
                    if (system)
                        menu->SetSystem(system);
                    if (plugin)
                        menu->SetPluginName(plugin);
                    if (info)
                        menu->SetInfo(info);
                    if (help)
                    {
                        if (!info)
                            menu->SetInfo("(?)");
                        menu->SetHelp(help);
                    }

                    if (menu && (ok = menu->LoadXml(node->FirstChild())) == true)
                    {
                        AddNode(menu);
                    }

                    else if (menu && (system || plugin))
                    {
                        AddNode(menu);
                    }
                    else
                    {
                        delete menu;
                    }
                }
                else
                {
                    isyslog(DBG
                            " menu entry for  %s found, but plugin not found",
                            name);
                }
            }
        }
        while ((node = node->NextSibling()) != NULL && ok == true);

        // Link the IfBlock-variables to correspondending Entry-variables
        for(int i=0; i < (int)vIfBlocks.size(); ++i)
        {
            bool found = false;
            int j=0;
            while(!found && j < (int)vEntrys.size())
            {
                if(vIfBlocks.at(i)->GetID() == vEntrys.at(j)->GetID())
                {
                    found = true;
                    vIfBlocks.at(i)->SetType(vEntrys.at(j)->GetType());
                    vIfBlocks.at(i)->SetValueTextRef(vEntrys.at(j)->GetValueRef());
                    vIfBlocks.at(i)->SetValueIpRef(vEntrys.at(j)->GetValueIpRef());
                    vIfBlocks.at(i)->SetValueNumberRef(vEntrys.at(j)->GetValueNumberRef());
                    vIfBlocks.at(i)->SetValueBoolRef(vEntrys.at(j)->GetValueBoolRef());
                    vIfBlocks.at(i)->SetSelectionValueRef(vEntrys.at(j)->GetSelectionValueRef());
                }
                ++j;
            }
        }
    }
    else
    {
        DLOG(DBG " ---- Load XML MENUES -- No XML::ELEMENT  \n");
        ok = false;
    }

    //DLOG(DBG " ---- Load XML MENUES -- erfolg? %s \n", ok ? "YES" : "NO");
    return ok;
}


/**
 *Save Menues to XML Structure
 *@param xml XML structure
 *@return true on success
 */
bool
Menus::SaveXml(TiXmlNode *root)
{
    bool ok = true;
    TiXmlNode *tmp = NULL;
    TiXmlElement *m = new TiXmlElement("menus");        // Root element of menus
    for (int i = 0; i < Count() && ok == true; i++)
    {
        if ((tmp = Get(i)->GetMenu()->SaveXml()) != NULL)
        {
            m->LinkEndChild(tmp);
        }
        else
            ok = false;
    }

    root->LinkEndChild(m);
    return ok;
}

/**
 *Print Nodes
 */
void
Menus::Print()
{
    for (int i = 0; i < Count(); i++)
        Get(i)->Print();
}


// ###################################################################################
//  MenuEntryValueList
// ###################################################################################

MenuEntryValueList::MenuEntryValueList()
{
    _values = NULL;
    _nr = 0;
    _selection = 0;
}

MenuEntryValueList::~MenuEntryValueList()
{
    destroy();
}

void
MenuEntryValueList::copy(MenuEntryValueList const &other)
{

    for (int i = 0; i < other._nr; i++)
    {
        Add(other._values[i]);
    }
    _nr = other._nr;
    _selection = other._selection;

}

void
MenuEntryValueList::destroy()
{
    if (_values != NULL)
    {
        for (int i = 0; i < _nr; i++)
            delete[]_values[i];
        delete[]_values;
        _nr = 0;
    }
    _values = NULL;
}



MenuEntryValueList const &
MenuEntryValueList::operator =(MenuEntryValueList const &right)
{
    if (this != &right)
    {
        destroy();
        copy(right);
    }
    return *this;
}


void
MenuEntryValueList::Add(const char *value)
{
    const char **valueList = new const char *[_nr + 1];

    for (int i = 0; i < _nr; i++)
    {
        valueList[i] = Util::Strdupnew(_values[i]);
    }

    valueList[_nr] = Util::Strdupnew(value);
    int nr = _nr;
    destroy();
    _nr = ++nr;
    _values = valueList;
}

void
MenuEntryValueList::Print()
{
    Print(0);
}

void
MenuEntryValueList::Print(int offset)
{
    if (_nr > 0)
    {
        for (int i = 0; i < _nr; i++)
        {
            for (int j = 0; j < offset + 2; j++)
                printf("-");
            printf("Value[%d]=%s\n", i, _values[i]);
        }
    }
}


int
MenuEntryValueList::GetNr()
{
    return _nr;
}

const char *
MenuEntryValueList::GetValue(int index)
{
    const char *result = NULL;
    if (_nr > 0 && index < _nr && index >= 0)
    {
        result = _values[index];
    }
    return (result);
}

const char **
MenuEntryValueList::GetValues()
{
    return _values;
}

int *
MenuEntryValueList::GetReferenceSelection()
{
    return &_selection;
}


void
MenuEntryValueList::SetSelection(const char *value)
{
    bool found = false;
    _selection = 0;

    for (int i = 0; i < _nr && found == false; i++)
    {
        if (strcmp(_values[i], value) == 0)
        {
            _selection = i;
            found = true;
        }
    }
}

const char *
MenuEntryValueList::GetSelectedValue()
{
    if (_nr == 0)
        return (NULL);
    else
    {
        return (_values[_selection]);
    }
}
