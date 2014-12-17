#ifdef REELVDR
#include <string>
#include <vector>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "debug.h"
#include "i18n.h"
#include "submenu.h" // dynamicmenuEnt
#include "tinyxml/tinyxml.h"
#include "help.h"
#include "skins.h"


using std::string;

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

cHelpPages HelpMenus;

// --- class cHelpSection -----------------------------------------------------

cHelpPage *cHelpSection::GetHelpByTitle(const char *Title) const
{
  for (cHelpPage *hp = this->First();hp; hp=this->cList<cHelpPage>::Next(hp))
  {
    if (strcmp(Title,hp->Title()) == 0)
    {
       return hp;
    }
  } 
  // if nothing found we give first dummy page back
  return this->First();
}

// --- class cHelpPages -----------------------------------------------------

void cHelpPages::DumpHelp()
{
  /*
  DLOG (" Dump HelpMenus \n");
  for (cHelpSection *hs = HelpMenus.First();hs; hs=HelpMenus.Next(hs))
 {
    DLOG ("\t +++ Section %s +++\n", hs->Section());
    for (cHelpPage *hp = hs->First();hp; hp=hs->cList<cHelpPage>::Next(hp))
    {
       DLOG ("\t\t -- Title %s\n", hp->Title());
    } 
  }
  DLOG (" Dump HelpMenus End \n\n\n\n");
  */
}


bool cHelpPages::Load()
{
  bool ok = false;

  std::string fileName =  setup::FileNameFactory("help");
  int fd = open(fileName.c_str(), O_RDONLY);
  if(fd != -1) {
     struct stat stat_;
     if(fstat(fd, &stat_) == 0) {
        if((lastModified_ != 0) && (lastModified_ == stat_.st_mtime)) {
           return true;
        } else {
           lastModified_ = stat_.st_mtime;
        }
     }
  }
     
  HelpMenus.Clear();
  const char *notAvailable = tr("No help available");
  cHelpSection *s = new cHelpSection(notAvailable);
  cHelpPage *help = new cHelpPage(notAvailable, notAvailable); 
  s->Add(help);
  HelpMenus.Add(s);
  
  DLOG (" Parse file %s", fileName.c_str());
  TiXmlDocument doc(fileName.c_str());
  ok = doc.LoadFile(); // args encoding
  
  if (ok)
  {
    DLOG (" Load OK");

    TiXmlHandle docHandle( &doc);
    TiXmlHandle handleSection = docHandle.FirstChild("help").FirstChild("section");
    ParseSection(handleSection);
  }
  else 
  {
    ok = false;

    DERR (" Error parsing %s : %s", fileName.c_str(), doc.ErrorDesc());
    DLOG (" \t Col=%d Row=%d\n",doc.ErrorCol(), doc.ErrorRow());
    esyslog(" Error parsing %s : %s ", fileName.c_str(), doc.ErrorDesc());
    esyslog(" Col=%d Row=%d\n",doc.ErrorCol(), doc.ErrorRow());
  }

  //DumpHelp();

  return ok;
}
    
void cHelpPages::ParseSection(TiXmlHandle HandleSection, int Level) 
{
  
  TiXmlElement *elemSection = HandleSection.ToElement();
  if (!elemSection) 
  { 
      //DLOG (" \t get Out ");
      return; 
  }

  int sec=1;
  int t=1; 
  
  for(; elemSection; elemSection=elemSection->NextSiblingElement("section"))
  {
       //DLOG (" ping 1\n");
       try 
       {
          const char *text =  NULL;
          const char *section = NULL;
          TiXmlAttribute *attr = elemSection->FirstAttribute();
          if (attr) 
          {
             section = attr->Value();
             text = elemSection->GetText();
             //DLOG ("  %2d.) Get section: <%s> \n",Level, section);
          }

          // we have to parse each line for <br> resp. <br />
          std::vector<string>vText;
          if (text)
          { 
             string tmp = text;
             tmp.append("\n");
             vText.push_back(tmp);
             text = NULL;
             //TiXmlNode* paragraph = elemSection->FirstChild("p");
             TiXmlNode* nextLine = elemSection->FirstChild("br");
             //DLOG ("Text: [%s] \n", nextLine?"has <br />":"");
             for (; nextLine; nextLine = nextLine->NextSibling()) 
			 {
                if (nextLine->Type() == TiXmlNode::TINYXML_TEXT)
                {
                   text = NULL;
                   TiXmlText *pText = nextLine->ToText();
                   text = pText->Value(); 
                   //DLOG( "Text: [%s] \n", text);
                   //vText.push_back(string(text) +"\n");
                   vText.push_back(text);
                }
                else if (nextLine->Type() ==TiXmlNode::TINYXML_ELEMENT)
                {
                    //DLOG (" is Element Node Val: %s \n", nextLine->Value());
                    if (strstr(nextLine->Value(), "br") ==  nextLine->Value())
                        vText.push_back("\n");
                    else if (strstr(nextLine->Value(), "p") ==  nextLine->Value())
                        vText.push_back("\n\n  ");

                    else if (strstr(nextLine->Value(), "li") ==  nextLine->Value())
                    {
                      if (nextLine->Type() == TiXmlNode::TINYXML_ELEMENT)
                      {
                        TiXmlElement *e = nextLine->ToElement();
                        TiXmlAttribute *attr = e->FirstAttribute();

                        string tmp("\n");
                        const char *symbol = attr->Value();
                        if (!symbol)
                            string tmp("\n ");
                        else 
                        tmp += symbol;
                        vText.push_back(tmp);
                      }
                   }
                }
			 }
             /*
             TiXmlNode* paragraph = elemSection->FirstChild("p");
             //DLOG ("Text: [%s] \n", paragraph?"has <br />":"");
             for (; paragraph; paragraph = paragraph->NextSibling()) 
			 {
                if (paragraph->Type() == TiXmlNode::TINYXML_TEXT)
                {
                   text = NULL;
                   TiXmlText *pText = paragraph->ToText();
                   text = pText->Value(); 
                   //DLOG( "Text: [%s] \n", text);
                   vText.push_back(string(text) +"\n");
                }
			 } */
          }

          cHelpSection *s = new cHelpSection(section);
          if (!vText.empty())
          {
             cHelpPage *h = new cHelpPage(section, vStringToString(vText)); 
             s->Add(h);
             vText.clear();
          }

          TiXmlNode* nodePage= elemSection->FirstChild("page");

          for (; nodePage; nodePage = nodePage->NextSibling("page"))
          {
            try 
            {
              vText.clear();
              text = NULL;
              const char *title = nodePage->ToElement()->Attribute("title");
              const char *text = nodePage->ToElement()->GetText();
              //DLOG ("\t  %2d.) page title: <%s>  \n",t, title);
              if (text) vText.push_back(string(text) +"\n");
              TiXmlNode* nextLine = nodePage->FirstChild("br");
              for (; nextLine; nextLine = nextLine->NextSibling()) 
              {
                if (nextLine->Type() == TiXmlNode::TINYXML_TEXT)
                {
                   text = NULL;
                   TiXmlText *pText = nextLine->ToText();
                   text = pText->Value(); 
                   //DLOG( "Text: [%s] \n", text);
                   vText.push_back(string(text) +"\n");
                }
              }
              cHelpPage *help = new cHelpPage(title, vStringToString(vText)); 
              s->Add(help);
              t++; 
            }
            catch (char *message)
            {
              esyslog("ERROR: while decoding XML Node. msg: \"%s\"",message);
              //ok=false;
            }
         }
         HelpMenus.Add(s);

       }
       catch (char *message)
       {
          esyslog("ERROR: while decoding XML Node. msg: \"%s\"",message);
          //ok=false;
       }
       sec++;
       //ok = true;
   }

   ParseSection(HandleSection.FirstChild("section"), Level+1);
}
   

cHelpPage *cHelpPages::GetByTitle(const char *Title) const
{
  //DLOG (" GetByTitle:  Title %s +++\n", Title);
  for (cHelpSection *hs = HelpMenus.First();hs; hs=HelpMenus.Next(hs))
  {
    //DLOG ("\t parse HelpMenus:  Section %s +++\n", hs->Section());
    for (cHelpPage *hp = hs->First();hp; hp=hs->cList<cHelpPage>::Next(hp))
    {
       if (strcmp(Title,hp->Title()) == 0)
       {
           //DLOG("\t\t -- Title %s\n", hp->Title());
           return hp;
       }
    } 
  }
  return HelpMenus.First()->First();
}

cHelpSection *cHelpPages::GetSectionByTitle(const char *Title) const
{

  //DLOG (" GetSectioniByTitle:  Title %s +++\n", Title);
  for (cHelpSection *hs = HelpMenus.First();hs; hs=HelpMenus.Next(hs))
  {
     // DLOG ("\t parse HelpMenus:  Section %s +++\n", hs->Section());

    for (cHelpPage *hp = hs->First();hp; hp=hs->cList<cHelpPage>::Next(hp))
    {
       if (strcmp(Title,hp->Title()) == 0)
       {
           //DLOG("\t\t -- Title %s\n", hp->Title());
           return hs;
       }
    } 
  }
  return HelpMenus.First();
}

// --- cMenuHelp --------------------------------------------------------

cMenuHelp::cMenuHelp(cHelpSection *Section, const char *Title)
:cOsdMenu(Title)
{

  text = NULL;
  helpPage = NULL;
  section = Section;
  char buffer[128];
  snprintf(buffer,128, "%s - %s",tr("Help"), Title);
  SetTitle(buffer);

  if (Section)
     helpPage = Section->GetHelpByTitle(Title);

  if (helpPage)
    SetText(helpPage->Text());

  Display();
}

cMenuHelp::~cMenuHelp()
{
  if (text) free(text);
}

void cMenuHelp::SetText(const char *Text)
{
  if (text) free(text);
  text = Text ? strdup(Text) : NULL;
}
void cMenuHelp::SetNextHelp()
{

  SetStatus(NULL);
  cHelpPage *h =  static_cast<cHelpPage *>(helpPage->cListObject::Next());
  if (h) // aviod malloc/free!
  {
    helpPage = h;
    const char *myTitle = helpPage->Title();
    SetText(helpPage->Text());
    char buffer[128];
    snprintf(buffer,128,"%s - %s",tr("Help"), myTitle);
    SetTitle(buffer);
    Display();
  }
  else
  {
     SetStatus(tr("Already first help item"));
  }
}

void cMenuHelp::SetPrevHelp()
{
  SetStatus(NULL);
  cHelpPage *h = static_cast<cHelpPage *>(helpPage->cListObject::Prev());
  if (h)
  {
    helpPage = h;
    const char *myTitle = helpPage->Title();
    SetText(helpPage->Text());

    char buffer[1024];
    snprintf(buffer,1024, "%s - %s",tr("Help"), myTitle);
    SetTitle(buffer);
    Display();
  }
  else
  {
     SetStatus(tr("Already last help item"));
  }
}

void cMenuHelp::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetText(text, font == fontFix); //XXX define control character in text to choose the font???
  if (text)
     cStatus::MsgOsdTextItem(text);
}

eOSState cMenuHelp::ProcessKey(eKeys Key)
{
  switch (Key) {
    case kUp|k_Repeat:
    case kUp:

                  SetNextHelp();
                  break;
    case kDown|k_Repeat:
    case kDown:
                  SetPrevHelp();
                  break;
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
                  DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
                  cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp);
                  return osContinue;
    case kInfo: return osBack; // XXX TOTEST
    default: break;
    }

  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kInfo: return osBack;
       case kOk: return osBack;
       default:  state = osContinue;
       }
     }
  return state;
}
#endif /*REELVDR*/
