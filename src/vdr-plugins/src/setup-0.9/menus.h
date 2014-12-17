/*********************************************************
 * DESCRIPTION:
 *             Header File
 *
 * $Id: menus.h,v 1.8 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 *********************************************************/

#ifndef MENUS_H
#define MENUS_H
#include <vdr/tools.h>
#include <tinyxml/tinystr.h>
#include "util.h"
#include <vdr/sysconfig_vdr.h>

#include <vdr/debugmacros.h>

using std::vector;

/**
@author Ralf Dotzert
*/

#define VALUETEXTMAXLEN 65
#define VALUEIPMAXLEN   16

namespace setup
{
    const char *GetPluginMenuEntry(const char *Plugin);
    // warum auch in class Menu ???
}

enum eMenuType
{ mtUndefined, mtEntry, mtMenu, mtMenusystem, mtMenuplugin, mtExecute, mtIfblock };

class Menu;
class IfBlock;
class MenuEntry;
class MenuEntryValueList;
class MenuNode;

extern std::vector<IfBlock*> vIfBlocks;
extern std::vector<MenuEntry*> vEntrys;

// --- Class SubMenus   ----------------------------------------------------------
class SubMenus:public cList < MenuNode >
{
};

// --- Class MenuNode   ----------------------------------------------------------
//class XmlNode : public cListObject
class MenuNode:public cListObject
{
  public:
    MenuNode();
    virtual ~ MenuNode();
    void destroy();
    void SetNode(Menu * menu);
    void SetNode(IfBlock *ifBlock);
    void SetNode(MenuEntry * menuEntry);
    Menu *GetMenu();
    IfBlock *GetIfBlock();
    MenuEntry *GetMenuEntry();
    eMenuType GetType();
    const char *GetName();
    int GetNr();
    MenuNode *GetNode(int index);
    MenuNode *GetNode_openIfs(int index);
    MenuNode *GetNode(const char *nodeName);
    void Print();
    void Print(int offset);
  private:
      eMenuType _menuType;
    Menu *_menu;
    MenuEntry *_menuEntry;
    IfBlock *_ifBlock;
};

// --- Class Menu ----------------------------------------------------------

class Menu
{
  protected:
    const char *_name;
    const char *_command;
    const char *_system;
    const char *_plugin;
    const char *_execute;
    ///< similar excute as in  vdr/submenu.*
    const char *_info;          // short help
    const char *_help;          // long help
    SubMenus _subMenus;
  public:
    Menu();
    Menu(const char *name);
    virtual ~ Menu();

    void destroy();
    void SetName(const char *nam);
    const char *GetName();

    void SetCommand(const char *command);
    const char *GetCommand();

    void SetSystem(const char *system);
    const char *GetSystem();

    void SetPluginName(const char *plugin);
    const char *GetPluginName();

    void SetExecute(const char *execute);
    const char *GetExecute();

    const char *GetPluginMenuEntry(const char *Plugin);

    void SetInfo(const char *Info);
    const char *GetInfo();

    void SetHelp(const char *Help);
    const char *GetHelp();

    MenuNode *GetNode(int index);
    MenuNode *GetNode(const char *nodeName);

    void AddNode(Menu * menu);
    void AddNode(IfBlock *ifBlock);
    void AddNode(MenuEntry * menuEntry);
    
    bool LoadXml(TiXmlNode * node);
    TiXmlNode *SaveXml();
    
    int GetNr();
    MenuNode* GetNode_openIfs(int index);
    int Get_actualSize();

    virtual void Print();
    virtual void Print(int offset);
};

// --- Class MenuEntryValueList  ----------------------------------------------------------
class MenuEntryValueList
{
  private:
    const char **_values;
    int _nr;
    int _selection;
  public:
      MenuEntryValueList();
     ~MenuEntryValueList();
    //MenuEntryValueList const &MenuEntryValueList::operator=(MenuEntryValueList const &right);
    MenuEntryValueList const &operator=(MenuEntryValueList const &right);
    void copy(MenuEntryValueList const &other);
    void destroy();
    void Add(const char *value);
    int GetNr();
    const char *GetValue(int index);
    const char **GetValues();
    int *GetReferenceSelection();       // will be changed in Menu
    const char *GetSelectedValue();
    void SetSelection(const char *value);
    void Print();
    void Print(int offset);
};

// --- Class IfBlock --------------------------------------------------------------
class IfBlock : public Menu
{
    protected:
        int _ID;
        char *_value;
        Util::Type _type;
        char *_name;

        //References to values of object MenuEntry
        char **_valueTextRef;
        char **_valueIpRef;
//        int *_valueTextMaxLenRef;
//        int *_valueIpMaxLenRef;
        int *_valueNumberRef;
        int *_valueBoolRef;
        MenuEntryValueList *_selectionValuesRef;
    public:
        IfBlock();
        ~IfBlock();

        void SetID(int ID) { _ID = ID; };
        int GetID() const { return _ID; };
        void SetValue(const char *Value);
        char* GetValue() const { return _value; };
        void SetName(const char *Name);
        char* GetName() const { return _name; };
        void SetType(Util::Type Type) { _type = Type; };
        Util::Type GetType() const { return _type; };

        void SetValueTextRef(char **Reference) { _valueTextRef = Reference; };
        char **GetValueTextRef() const { return _valueTextRef; };
        void SetValueIpRef(char **Reference) { _valueIpRef = Reference; };
        char **GetValueIpRef() const { return _valueIpRef; };
        void SetValueNumberRef(int *Reference) { _valueNumberRef = Reference; };
        int *GetValueNumberRef() const { return _valueNumberRef; };
        void SetValueBoolRef(int *Reference) { _valueBoolRef = Reference; };
        int *GetValueBoolRef() const { return _valueBoolRef; };
        void SetSelectionValueRef(MenuEntryValueList *Reference) { _selectionValuesRef = Reference; };

        // returns true if the if block condition is satisfied
        bool isOpen(); 
        TiXmlNode *SaveXml();
};


// --- Class MenuEntry   ----------------------------------------------------------
class MenuEntry:public Menu
{
  protected:
    int _ID;
    const char *_sysconfigName;
    char *_valueText;
    char *_valueIp;
    const char *_setupCommand;
    int _valueTextMaxLen;
    int _valueIpMaxLen;
    int _valueNumber;
    int _valueBool;
    MenuEntryValueList _selectionValues;
      Util::Type _type;
  public:
      MenuEntry();
     ~MenuEntry();
    void destroy();
    bool AddEntry(TiXmlNode * node);
    bool AddSubEntry(TiXmlNode * node);
    //bool AddSelectionValues(TiXmlNode *node, const char *selection); // old
    bool AddSelectionValues(TiXmlNode * node);
    TiXmlNode *SaveXml();       //weg
    void SetID(int ID) { _ID = ID; };
    int GetID() { return _ID; };
    void SetSysconfigName(const char *SysName); // rename SetSyscnf:Key
    const char *SysconfigKey();
    void SetValue(Util::Type type, const char *val);
    char *GetValue();           ///< will be changed in Menu
    char **GetValueRef() { return &_valueText; };
    char *GetValueIp() { return _valueIp; };
    char **GetValueIpRef() { return &_valueIp; };
    void SetSetupCommand(const char *command);
    const char *GetSetupCommand();
    int GetNrOfSelectionValues();
    const char *GetSelectionValue(int index);
    const char *GetSelectedValue();
    const char **GetSelectionValues();
    int *GetReferenceSelection();
    void SetSelection(const char *val);
    MenuEntryValueList *GetSelectionValueRef() { return &_selectionValues; };
    int GetValueTextMaxLen();
    int GetValueIpMaxLen()
    {
        return _valueIpMaxLen;
    }
    int *GetValueBoolRef();
    int *GetValueNumberRef();
    const char *GetValueAsString();
    void SetType(Util::Type typ);
    void SetSysConfig();
    Util::Type* GetTypeRef();
    Util::Type GetType();
    void Print();
    void Print(int offset);
    void SetSysConfValue();
  private:
    bool readNetworkDriversDir(char *dir, char *prefix);
    bool ReadNetworkDrivers();  //private
    bool ChannelList();         //private??
};

// --- Class Menus  ----------------------------------------------------------

class Menus:public cList < MenuNode >
{
  public:
    Menus();
    ~Menus();
    bool LoadXml(TiXmlNode * node);
    bool SaveXml(TiXmlNode * node);
    void AddNode(Menu * menu);
    MenuNode *GetMenuNode(int index);
    int GetNr();
    void Print();
};


// inline Defintions
// ----- Menus -----------------------------------------------

inline void
Menus::AddNode(Menu * menu)
{
    MenuNode *n = new MenuNode;
    Add(n);
    n->SetNode(menu);
}

inline MenuNode *
Menus::GetMenuNode(int index)
{
    return Get(index);
}

inline int
Menus::GetNr()
{
    return Count();
}

// inline Defintions
// ----- Menu -----------------------------------------------

inline int
Menu::GetNr()
{
    return _subMenus.Count();
}

inline void
Menu::SetName(const char *Name)
{
    if (_name)
    {
        delete[]_name;
    }
    _name = Util::Strdupnew(Name);
}

inline const char *
Menu::GetName()
{
    return _name;
}

inline const char *
Menu::GetCommand()
{
    return _command;
}

inline void
Menu::SetCommand(const char *Command)
{
    if (Command)
    {
        delete[]_command;
        _command = Util::Strdupnew(Command);
    }
}

inline const char *
Menu::GetSystem()
{
    return _system;
}

inline void
Menu::SetPluginName(const char *Plugin)
{
    if (Plugin)
    {
        delete[]_plugin;
        _plugin = Util::Strdupnew(Plugin);
    }
}

inline const char *
Menu::GetPluginName()
{
    return _plugin;
}
inline const char *
Menu::GetExecute()
{
    return _execute;
}

inline void
Menu::SetSystem(const char *system)
{
    delete[]_system;
    _system = Util::Strdupnew(system);
}

inline void
Menu::SetExecute(const char *Execute)
{
    //printf (" Menu::SetExecute  %s \n", Execute);
    delete[]_execute;
    _execute = Util::Strdupnew(Execute);
}

inline void
Menu::AddNode(Menu * menu)
{
    MenuNode *n = new MenuNode;
    n->SetNode(menu);
    _subMenus.Add(n);
}

inline void
Menu::AddNode(IfBlock *ifBlock)
{
    MenuNode *n = new MenuNode;
    n->SetNode(ifBlock);
    _subMenus.Add(n);
}

inline void
Menu::AddNode(MenuEntry * menuEntry)
{
    MenuNode *n = new MenuNode;
    n->SetNode(menuEntry);
    _subMenus.Add(n);
}

inline MenuNode *
Menu::GetNode(int index)
{
    return _subMenus.Get(index);
}



inline void
Menu::SetInfo(const char *Info)
{
    delete[]_info;
    _info = Util::Strdupnew(Info);
}

inline const char *
Menu::GetInfo()
{
    return _info;
}

inline void
Menu::SetHelp(const char *Help)
{
    delete[]_help;
    _help = Util::Strdupnew(Help);
}

inline const char *
Menu::GetHelp()
{
    return _help;
}

//TODO : move this to a REEL lib
bool IsNetworkDevicePresent(const char* dev);
#endif
