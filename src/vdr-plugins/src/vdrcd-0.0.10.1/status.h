#ifndef VDR_VDRCD_STATUS_H
#define VDR_VDRCD_STATUS_H

#include <vdr/status.h>

class cVdrcdStatus: public cStatus {
private:
	const char *m_Title;
	const char *m_Current;
	int         m_NumClears;

protected:
	virtual void OsdTitle(const char *Title);
  virtual void OsdCurrentItem(const char *Text);
	virtual void OsdClear(void);

public:
	cVdrcdStatus(void);
	~cVdrcdStatus();

	const char *Title(void) const { return m_Title; }
	const char *Current(void) const { return m_Current; }
	int        &NumClears(void) { return m_NumClears; }
};

#endif // VDR_VDRCD_STATUS_H
