#ifndef vlcclient_common_h
#define vlcclient_common_h

#include <vdr/tools.h>
#include <vdr/plugin.h>

#define PORT 8080

#define VPID 68
#define APID 69

#define INFO(s) Skins.Message(mtInfo, s)
#define STATUS(s) Skins.Message(mtInfo, s)
#define ERROR(s) Skins.Message(mtStatus, s)
#define FLUSH() Skins.Flush()


class cMenuEditIpItem: public cMenuEditItem {
private:
        char *value;
        int curNum;
        int pos;
        bool step;

protected:
        virtual void Set(void);

public:
        cMenuEditIpItem(const char *Name, char *Value); // Value must be 16 bytes
        ~cMenuEditIpItem();

        virtual eOSState ProcessKey(eKeys Key);
};

#endif
