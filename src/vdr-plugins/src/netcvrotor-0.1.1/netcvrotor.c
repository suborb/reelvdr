/*
  netcvrotor - Rotor plugin control for NetCeiver
  (c) 2008 BayCom GmbH

  #include <gpl_v2.h>

  Just overwrites the satellite position in the NetCeiver-specific 
  Diseqc control message

  Todos: Allow other path than /etc/vdr/rotor.conf
*/

#include "netcvrotor.h"
#include "netcvrotorservice.h"
#include "netcvmenu.h"
#include <vdr/diseqc.h>
#include <vdr/sources.h>

#if VDRVERSNUM < 10330
#error "Needs VDR version >= 1.3.30"
#endif

cPluginNCRotor::cPluginNCRotor(void)
{
	// Initialize any member variables here.
	// DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
	// VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
	LoadConfig();
}

cPluginNCRotor::~cPluginNCRotor()
{
	// Clean up after yourself!
}

bool cPluginNCRotor::Service(const char *Id, void *Data)
{
	void **data=(void**)Data;
	
	if (Data == NULL)
	        return false;

	if (strcmp(Id,"netcvrotor.getDiseqc") == 0) {
		int tuner=(int)data[0];
		int position=(int)data[1];

		data[2]=(void*)-1;

		if (tuner>=1 && tuner<=MAX_ROTORS) {
			if (rotor_data[tuner-1].available) {

				// neg -> E, pos -> W
				if (cSource::st_Neg&position)
					position=(position&cSource::st_Pos);
				else
					position=-(position&cSource::st_Pos);
				position+=1800;	
				printf("netcvrotor: Move to %i\n",position);
				position^=1800;
				data[2]=(void*)position;				
				return true;
			}
		}
	} else if (strcmp(Id,"netcvrotor.loadConfig") == 0) { 
                //printf("--cPluginNCRotor::Service---------netcvrotor.loadConfig---\n");
                LoadConfig();
                return true;
        } else if (strcmp(Id,"netcvrotor.tunerIsRotor") == 0) {
                //printf("--cPluginNCRotor::Service-------netcvrotor.tunerIsRotor---------\n");
                int tuner=(int)data[0]; 
                //printf("--cPluginNCRotor::Service-------tuner = %d---------\n", tuner);
                if(tuner>=0 && tuner<=MAX_ROTORS && rotor_data[tuner].available)
                   data[1] = (void*)1;
                else
                   data[1] = (void*)0;
                return true;
        } else if (strcmp(Id,"netcvrotor.getRotorData") == 0) {
                //printf("--cPluginNCRotor::Service--netcvrotor.getRotorData--\n");
                int tuner = (int)data[3];
		RotorData *request = (RotorData*)data;
                if(tuner>=1 && tuner<=MAX_ROTORS) {
                   *request = rotor_data[tuner-1];
                }
                return true;
        } else if (strcmp(Id,"netcvrotor.OpenMenu") == 0) {
                //printf("--cPluginNCRotor::Service--netcvrotor.OpenMenu--\n");
                netcvrotor_service_data *datamenu = (netcvrotor_service_data*) Data;
                datamenu->menu = new cMenuNCRotor("fasellall");
                return true;
        }
	return false;
}

int cPluginNCRotor::LoadConfig(void)
{
    // Create a array of active tuners with correspondend number
    char buffer[256];
    int tunercount = 0;
    int activecount = 0;
    int tuneractive[8];
    for(int i=0; i<8; ++i)
        tuneractive[i] = -1;
    const char *cmd = "netcvdiag -t | grep Preference";
    FILE *file = popen(cmd, "r");
    if(file)
    {
        while(fgets(buffer, 256, file))
        {
            char *start = strstr(buffer, "Preference");
            if(!strstr(start, "-1"))
            {
                //printf("---tuneractive[%d] = %d----\n", activecount, tunercount);
                tuneractive[activecount++] = tunercount;
            }
            ++tunercount;
        }
        pclose(file);
    }
    else
        return -1;

    // Read data from rotor.conf
	file = NULL;
	int n;
	for(n=0;n<MAX_ROTORS;n++)
		rotor_data[n].available=0;

	file=fopen(PATH_ROTOR_CONF,"r");
	if (!file)
		return -1;

	while(!feof(file)) {
		int dev, minr,maxr, af, lat, lng;
		char first,n;
		dev=0;
		first=0;
		n=fscanf(file,"%c%i %i %i %i %i %i\n",&first,&dev,&minr,&maxr,&af,&lat,&lng);
		if (n==7 && first=='A' && dev>=1 && dev<=8) {
			isyslog("ROTOR %i: %.1f%c-%.1f%c\n",dev,abs(minr-1800)/10.0,(minr<1800?'W':'E'),abs(maxr-1800)/10.0,(maxr<1800?'W':'E'));
            int tuner = tuneractive[dev-1];
            //printf("---rotor_data[%d] = 1-----\n", tuner);
			rotor_data[tuner].available=1;
			rotor_data[tuner].minpos=minr;
			rotor_data[tuner].maxpos=maxr;
		}
	}
	fclose(file);
	return 0;
}

const char *cPluginNCRotor::CommandLineHelp(void)
{
	// Return a string that describes all known command line options.
	return NULL;
}

bool cPluginNCRotor::ProcessArgs(int argc, char *argv[])
{
	// Implement command line argument processing here if applicable.
	return true;
}

bool cPluginNCRotor::Initialize(void)
{
	// Initialize any background activities the plugin shall perform.
	return true;
}

bool cPluginNCRotor::Start(void)
{
	// Start any background activities the plugin shall perform.
//	RegisterI18n(Phrases);
	return true;
}

void cPluginNCRotor::Housekeeping(void)
{
	// Perform any cleanup or other regular tasks.
}

bool cPluginNCRotor::ProvidesSource(int Source, int Tuner)
{
	int position,minr,maxr;
	printf ("netcvrotor: Tuner %i Provides source %x\n",Tuner,Source);
	if (Tuner>=1 && Tuner<=MAX_ROTORS) {
		if (!rotor_data[Tuner-1].available)
			return false;
		minr=rotor_data[Tuner-1].minpos;
		maxr=rotor_data[Tuner-1].maxpos;
		if (cSource::st_Neg&Source)
			position=(Source&cSource::st_Pos);
		else
			position=-(Source&cSource::st_Pos);
		position+=1800;	
		if (position>=minr && position<=maxr)
			return true;
	}
	
	return false;
}

cOsdObject *cPluginNCRotor::MainMenuAction(void)
{
	// Perform the action when selected from the main VDR menu.
	return new cMenuNCRotor("fasellall");
}

cMenuSetupPage *cPluginNCRotor::SetupMenu(void)
{
	// Return a setup menu in case the plugin supports one.
	return NULL;
}

bool cPluginNCRotor::SetupParse(const char *Name, const char *Value)
{
	// Parse your own setup parameters and store their values.
	return true;
}

VDRPLUGINCREATOR(cPluginNCRotor); // Don't touch this!

