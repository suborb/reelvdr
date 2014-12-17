
#include "mediaplugin.h"

char* mediaplugin::_st_curDirectory = NULL;
char* mediaplugin::_st_mountCmd = NULL;
mediaType mediaplugin::_st_curMediaType = unknown;
char* mediaplugin::_st_tmpCdrom = NULL;
device* mediaplugin::_st_curDevice = NULL;
cMediadStatus* mediaplugin::_st_status = NULL;
int mediaplugin::_st_refCount = 0;

mediaplugin::mediaplugin(const char* pluginName) 
{
    _pluginName = strdup(pluginName);
    _pluginDescription = NULL;
    _needsUmount = false;
    _needsDirectory = false;
    _found = false;
    _doAutoStart = true; //false
    // default search mode should be a or method
    _sMode = Or;    
    _st_refCount++;
    _activated = true;
}

mediaplugin::mediaplugin(const mediaplugin& plugin)
{
    _patterns.clear();
    for(unsigned int i = 0; i < plugin._patterns.size(); i++)
        _patterns.push_back(plugin._patterns[i]);
    _pluginName = strdup(plugin._pluginName);
    if(plugin._pluginDescription)
        _pluginDescription = strdup(plugin._pluginDescription);
    else
        _pluginDescription = NULL;
    _needsUmount = plugin._needsUmount;
    _needsDirectory = plugin._needsDirectory;
    _found = plugin._found;
    _doAutoStart = plugin._doAutoStart;
    _sMode = plugin._sMode; 
    _st_refCount++;
    _activated = plugin._activated;
}

mediaplugin& mediaplugin::operator=(mediaplugin const& plugin)
{
    _patterns.clear();
    for(unsigned int i = 0; i < plugin._patterns.size(); i++)
        _patterns.push_back(plugin._patterns[i]);
    _pluginName = strdup(plugin._pluginName);
    if(plugin._pluginDescription)
        _pluginDescription = strdup(plugin._pluginDescription);
    else
        _pluginDescription = NULL;
    _needsUmount = plugin._needsUmount;
    _needsDirectory = plugin._needsDirectory;
    _found = plugin._found;
    _doAutoStart = plugin._doAutoStart;
    _sMode = plugin._sMode; 
    _activated = plugin._activated;
    return *this;
}

mediaplugin::~mediaplugin() 
{
    if(_pluginName)
        free(_pluginName);
    _patterns.clear();
    // free static stuff
    if(_st_refCount-- == 0) 
    {
        if(_st_curDirectory) free(_st_curDirectory);
        if(_st_mountCmd) free(_st_mountCmd);
        if(_st_tmpCdrom) { 
            free(_st_tmpCdrom);
            _st_tmpCdrom = NULL;
        }
    }
}

bool mediaplugin::searchPattern() 
{
    switch(_sMode) {
        case And:
        {
            _found = true;
            for(vector<pattern>::iterator i = _patterns.begin(); i != _patterns.end(); i++)
                switch ((*i).searchMethod) {
                    case State:
                        {
                            struct stat buf;
                            string path = string(_st_curDirectory) + "/" + (*i).searchPattern;
                            if (!stat(path.c_str(), &buf) == 0) 
                            {
                                // cerr << "cur path " << path << endl;
                                if(!(*i).caseSensitive)
                                {
                                    char *upperBuffer = strdup((*i).searchPattern);
                                    strToUpper(upperBuffer);
                                    path = string(_st_curDirectory) + "/" + upperBuffer;
                                    // cerr << "cur path " << path << endl;
                                    free(upperBuffer);
                                    if (!stat(path.c_str(), &buf) == 0) 
                                        _found = false;         
                                } else
                                        _found = false;         
                            } else
                                _pluginDescription = (*i).patternDescription;
                            break;
                        }
                    case Find:
                        if (!TestType(_st_curDirectory, (*i).searchPattern, (*i).caseSensitive))
                            _found = false;
                        else
                            _pluginDescription = (*i).patternDescription;
                        break;
                    case MediaType:
                        if((*i).mediaT != _st_curMediaType)
                            _found = false;
                        else
                            _pluginDescription = (*i).patternDescription;
                        break;
                }
            return _found;
            break;
        }
        case Or:
        {
            _found = false;
            for(vector<pattern>::iterator i = _patterns.begin(); i != _patterns.end(); i++)
                switch ((*i).searchMethod) {
                    case State:
                        { 
                        string path = string(_st_curDirectory) + "/" + (*i).searchPattern;
                        struct stat buf;
                        if (stat(path.c_str(), &buf) == 0) { 
                            _found = true;          
                            _pluginDescription = (*i).patternDescription;
                            return _found;
                        } else if(!(*i).caseSensitive) {
                            char *upperBuffer = strdup((*i).searchPattern);
                            strToUpper(upperBuffer);
                            path = string(_st_curDirectory) + "/" + upperBuffer;
                            free(upperBuffer);
                            if (stat(path.c_str(), &buf) == 0) 
                            {
                                _found = true;          
                                _pluginDescription = (*i).patternDescription;
                                return _found;
                            }
                        }
                        break;
                        }
                    case Find: 
                        if (TestType(_st_curDirectory, (*i).searchPattern, (*i).caseSensitive)) {
                            _found = true;
                            _pluginDescription = (*i).patternDescription;
                            return _found;
                        }
                        break;
                    case MediaType:
                        if((*i).mediaT == _st_curMediaType) {
                            _found = true;
                            _pluginDescription = (*i).patternDescription;
                            return _found;
                        }
                        break;
                }
        }
    }
    return false;
}

void mediaplugin::setCurrentDevice(device* dev)
{
    if(_st_curDirectory)
        free(_st_curDirectory);
    _st_curDirectory = strdup(dev->getMountPoint());
    _st_curDevice = dev;
    _st_curMediaType = dev->getCurMediaType();
}

void mediaplugin::setMountCmd(const char* mount)
{
    if(_st_mountCmd)
        free(_st_mountCmd);
    _st_mountCmd = strdup(mount);
}

int mediaplugin::TestType(const char *dir, const char *file, bool caseSensitive) {
    int cnt=0;
    string cmd;
    if(caseSensitive)
       cmd = string("find ") + dir + " -follow -type f -name '" + file + "' 2> /dev/null";
    else
       cmd = string("find ") + dir + " -follow -type f -iname '" + file + "' 2> /dev/null";
    FILE *p = popen(cmd.c_str(), "r");
    if (p) {
            char *s;
            cReadLine r;
            while ((s = r.Read(p)) != NULL) {
                if(strcmp(s,"")!=0) cnt++;
        }
    }
    DDD("TYPE %s %d", cmd.c_str(), cnt);
    fclose(p);
    return cnt;
}


cOsdObject * mediaplugin::startPlugin() 
{
    cPlugin *p = cPluginManager::GetPlugin(getName());
    cOsdObject *osd = NULL;
    if (checkPluginAvailable(p)) {
        osd = p->MainMenuAction();
    }
    return osd;
}

void mediaplugin::preStartUmount() 
{
    if(_needsUmount)
    { 
            _st_curDevice->umount();
    }
}

bool mediaplugin::checkPluginAvailable(bool ignoreActivation) 
{
    cPlugin *p = cPluginManager::GetPlugin(getName());
    if(p && (_activated || ignoreActivation))
        return true;
    else
        return false;
}

bool mediaplugin::checkPluginAvailable(const cPlugin* plugin) 
{
    if(!plugin)
    {
        Skins.Message(mtError, tr("Missing appropriate PlugIn!"));

        if (_needsUmount) { // Unmount again if nothing found
            _st_curDevice->umount();
        }
        return false;
    }
    return true;
}

void mediaplugin::setStatus(cMediadStatus* status)
{
    _st_status = status;
}

int* mediaplugin::isAutoStartSetup() 
{
    return &_doAutoStart;
}

int* mediaplugin::isActiveSetup() 
{
    return &_activated;
}

void mediaplugin::strToUpper(char *lower)
{
    for (char *iter = lower; *iter != '\0'; ++iter)
    {
       *iter = std::toupper(*iter);
    }
}

// remove the mounted media from the DataBase
void mediaplugin::RemoveFromDataBase()
{

    // compute SHA1 of directory
    std::string tmpSHA = SHA_directory( _st_curDevice->getMountPoint() );

    cMediaDataBase db( DVD_DB_FILENAME );
    db.Remove( tmpSHA );
    db.Save();
}

// add the mounted media to the DataBase
void mediaplugin::Write2DataBase()
{
    if ( !_st_curDevice || _st_curDevice->isMounted() == false ) return;

    std::string buffer;
    char cmd[512];
    FILE* p;

    // compute SHA1 of directory
    std::string tmpSHA = SHA_directory( _st_curDevice->getMountPoint() );
    if( tmpSHA.length() != 40 )  // SHA1 = 40 bytes
    { 
        cerr << "(" << __FILE__ << ":" << __LINE__ << ") SHA1 error of " << _st_curDevice->getMountPoint() << ". Doing nothing." << endl;
        esyslog("(%s:%d) SHA1 error of %s. Doing nothing", __FILE__, __LINE__, _st_curDevice->getMountPoint() );
    }
    buffer += tmpSHA;
    buffer += "\t";

    /// plugin Name
    buffer += _pluginName;
    buffer += "\t";

    /// UDI
    std::string udi_str;
    const char* udi = _st_curDevice->getUDI();
    if (udi) 
        udi_str = udi;
    
    size_t idx;

    idx = udi_str.find("volume_label_");
    if (idx != string::npos) {
        udi_str.replace(0,idx + 13 , "");    // 13 = strlen(volume_label_)
    } 
    else 
        udi_str = "Unknown-DVD\t";
    buffer += udi_str; // volume_name
    buffer += "\t";

    /// get size of media
    char* size_kbytes=NULL;
    sprintf( cmd,"df -k | grep %s| awk '{print $3}'", _st_curDevice->getDeviceName() );
    p = popen(cmd, "r"); // code from devicesetup.c -  device::mount() 

    if (p) {
        char *s;
        cReadLine r;

        if ((s = r.Read(p)) != NULL)
        { 
            size_kbytes = strdup(s);
        }
        pclose(p);
    }

    //pclose(p);
    //if(size_kbytes)
    //{
        buffer += size_kbytes;
    //}

    buffer += "\t";

    /// time
    char time_array[32];
    sprintf(time_array,"%lld", (long long) time(0) );

    buffer += time_array;
    //cout << "BUFFER: \"" << buffer.c_str() << "\"" << endl;

    ///write to DB 
    FILE* fp = fopen("/tmp/DVD.db","a+"); // append to DB "a+"
    fprintf(fp,"%s\n",buffer.c_str());
    fclose(fp);

    ///write to DB
    cMediaDataBase db( DVD_DB_FILENAME );
    long long msize = atoll(size_kbytes);
    //cout << "BUFFER2: \"" << tmpSHA.c_str() << " " << _pluginName << " " << udi_str.c_str() << " " << msize << "\"" << endl;
    db.Add( tmpSHA , _pluginName, udi_str.c_str(), msize );
    db.Save();

    /// free memory
    free(size_kbytes);  
    //free(sha1_str);
}

