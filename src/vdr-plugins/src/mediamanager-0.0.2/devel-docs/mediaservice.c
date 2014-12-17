#include "mediaservice.h"

cPlugin *cMediaService::manager = NULL;
bool cMediaService::active = false;
bool cMediaService::replaying = false;
char *cMediaService::pluginname = NULL;

cMediaService::cMediaService(const char *PluginName)
{
  manager = NULL;
  pluginname = strdup(PluginName);
}

cMediaService::~cMediaService(void)
{
  manager = NULL;
  free(pluginname);
}

void cMediaService::SetManager(cPlugin *Manager)
{
  manager = Manager;
}

bool cMediaService::HaveManager(void)
{
  if(manager) return true;
  return false;
}

void cMediaService::SetReplaying(bool Replaying)
{
  if(manager && active) {
    MediaManager_Status_v1_0 st;
    st.set = true;
    st.flags = MEDIA_STATUS_REPLAY;
    st.isreplaying = Replaying;
    manager->Service(MEDIA_MANAGER_STATUS_ID, &st);
    replaying = Replaying;
    }
   else
     replaying = false;
}

void cMediaService::SetActive(bool Active)
{
  active = Active;
}

void cMediaService::SetCrashed(void)
{
  if(manager && active) {
    MediaManager_Status_v1_0 st;
    st.set = true;
    st.flags = MEDIA_STATUS_CRASHED;
    st.crashed = true;
    manager->Service(MEDIA_MANAGER_STATUS_ID, &st);
    }
}

bool cMediaService::IsReplaying(void)
{
  return replaying;
}

bool cMediaService::IsActive(void)
{
  return active;
}

void cMediaService::EjectDisc(void)
{
  if(manager && active) {
    MediaManager_Status_v1_0 st;
    st.set = true;
    st.flags = MEDIA_STATUS_EJECTDISC;
    st.ejectdisc = true;
    manager->Service(MEDIA_MANAGER_STATUS_ID, &st);
    }
}

void cMediaService::Suspend(bool Suspend)
{
  if(manager) {
    MediaManager_Suspend_v1_0 s;
	s.PluginName = pluginname;
    s.suspend = Suspend;
    manager->Service(MEDIA_MANAGER_SUSPEND_ID, &s);
    }
}
