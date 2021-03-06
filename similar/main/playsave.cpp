/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Functions to load & save player's settings (*.plr file)
 *
 */

#include <stdio.h>
#include <string.h>
#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif
#include <errno.h>
#include <ctype.h>

#include "dxxerror.h"
#include "strutil.h"
#include "game.h"
#include "gameseq.h"
#include "player.h"
#include "playsave.h"
#include "joy.h"
#include "digi.h"
#include "newmenu.h"
#include "palette.h"
#include "menu.h"
#include "config.h"
#include "text.h"
#include "state.h"
#include "gauges.h"
#include "screens.h"
#include "powerup.h"
#include "makesig.h"
#include "byteswap.h"
#include "u_mem.h"
#include "strio.h"
#include "physfsx.h"
#include "args.h"
#include "vers_id.h"
#include "newdemo.h"
#include "gauges.h"

#if defined(DXX_BUILD_DESCENT_I)
//version 5  ->  6: added new highest level information
//version 6  ->  7: stripped out the old saved_game array.
//version 7 -> 8: readded the old saved_game array since this is needed
//                for shareware saved games
//the shareware is level 4

#define SAVED_GAME_VERSION 8 //increment this every time saved_game struct changes
#define COMPATIBLE_SAVED_GAME_VERSION 4
#define COMPATIBLE_PLAYER_STRUCT_VERSION 16
#elif defined(DXX_BUILD_DESCENT_II)
//version 5  ->  6: added new highest level information
//version 6  ->  7: stripped out the old saved_game array.
//version 7  ->  8: added reticle flag, & window size
//version 8  ->  9: removed player_struct_version
//version 9  -> 10: added default display mode
//version 10 -> 11: added all toggles in toggle menu
//version 11 -> 12: added weapon ordering
//version 12 -> 13: added more keys
//version 13 -> 14: took out marker key
//version 14 -> 15: added guided in big window
//version 15 -> 16: added small windows in cockpit
//version 16 -> 17: ??
//version 17 -> 18: save guidebot name
//version 18 -> 19: added automap-highres flag
//version 19 -> 20: added kconfig data for windows joysticks
//version 20 -> 21: save seperate config types for DOS & Windows
//version 21 -> 22: save lifetime netstats 
//version 22 -> 23: ??
//version 23 -> 24: add name of joystick for windows version.

#define PLAYER_FILE_VERSION 24 //increment this every time the player file changes
#define COMPATIBLE_PLAYER_FILE_VERSION 17
#endif

#define SAVE_FILE_ID MAKE_SIG('D','P','L','R')

struct player_config PlayerCfg;
#if defined(DXX_BUILD_DESCENT_I)
static void plyr_read_stats();
saved_game_sw saved_games[N_SAVE_SLOTS];
#elif defined(DXX_BUILD_DESCENT_II)
static inline void plyr_read_stats() {}
static int get_lifetime_checksum (int a,int b);
#endif

int new_player_config()
{
#if defined(DXX_BUILD_DESCENT_I)
	for (unsigned i=0;i<N_SAVE_SLOTS;i++)
		saved_games[i].name[0] = 0;
#endif
	InitWeaponOrdering (); //setup default weapon priorities
	PlayerCfg.ControlType=0; // Assume keyboard
	memcpy(PlayerCfg.KeySettings, DefaultKeySettings, sizeof(DefaultKeySettings));
	memcpy(PlayerCfg.KeySettingsRebirth, DefaultKeySettingsRebirth, sizeof(DefaultKeySettingsRebirth));
	kc_set_controls();

	PlayerCfg.DefaultDifficulty = 1;
	PlayerCfg.AutoLeveling = 1;
	PlayerCfg.NHighestLevels = 1;
	PlayerCfg.HighestLevels[0].Shortname[0] = 0; //no name for mission 0
	PlayerCfg.HighestLevels[0].LevelNum = 1; //was highest level in old struct
	PlayerCfg.KeyboardSens[0] = PlayerCfg.KeyboardSens[1] = PlayerCfg.KeyboardSens[2] = PlayerCfg.KeyboardSens[3] = PlayerCfg.KeyboardSens[4] = 16;
	PlayerCfg.JoystickSens[0] = PlayerCfg.JoystickSens[1] = PlayerCfg.JoystickSens[2] = PlayerCfg.JoystickSens[3] = PlayerCfg.JoystickSens[4] = PlayerCfg.JoystickSens[5] = 8;
	PlayerCfg.JoystickDead[0] = PlayerCfg.JoystickDead[1] = PlayerCfg.JoystickDead[2] = PlayerCfg.JoystickDead[3] = PlayerCfg.JoystickDead[4] = PlayerCfg.JoystickDead[5] = 0;
	PlayerCfg.MouseFlightSim = 0;
	PlayerCfg.MouseSens[0] = PlayerCfg.MouseSens[1] = PlayerCfg.MouseSens[2] = PlayerCfg.MouseSens[3] = PlayerCfg.MouseSens[4] = PlayerCfg.MouseSens[5] = 8;
	PlayerCfg.MouseFSDead = 0;
	PlayerCfg.MouseFSIndicator = 1;
	PlayerCfg.CockpitMode[0] = PlayerCfg.CockpitMode[1] = CM_FULL_COCKPIT;
	PlayerCfg.ReticleType = RET_TYPE_CLASSIC;
	PlayerCfg.ReticleRGBA[0] = RET_COLOR_DEFAULT_R; PlayerCfg.ReticleRGBA[1] = RET_COLOR_DEFAULT_G; PlayerCfg.ReticleRGBA[2] = RET_COLOR_DEFAULT_B; PlayerCfg.ReticleRGBA[3] = RET_COLOR_DEFAULT_A;
	PlayerCfg.ReticleSize = 0;
	PlayerCfg.HudMode = 0;
#if defined(DXX_BUILD_DESCENT_I)
	PlayerCfg.BombGauge = 1;
#elif defined(DXX_BUILD_DESCENT_II)
	PlayerCfg.Cockpit3DView[0]=CV_NONE;
	PlayerCfg.Cockpit3DView[1]=CV_NONE;
	PlayerCfg.MissileViewEnabled = 1;
	PlayerCfg.HeadlightActiveDefault = 1;
	PlayerCfg.GuidedInBigWindow = 0;
	strcpy(PlayerCfg.GuidebotName,"GUIDE-BOT");
	strcpy(PlayerCfg.GuidebotNameReal,"GUIDE-BOT");
	PlayerCfg.EscortHotKeys = 1;
#endif
	PlayerCfg.PersistentDebris = 0;
	PlayerCfg.PRShot = 0;
	PlayerCfg.NoRedundancy = 0;
	PlayerCfg.MultiMessages = 0;
	PlayerCfg.NoRankings = 0;
	PlayerCfg.AutomapFreeFlight = 0;
	PlayerCfg.NoFireAutoselect = 0;
	PlayerCfg.CycleAutoselectOnly = 0;
	PlayerCfg.AlphaEffects = 0;
	PlayerCfg.DynLightColor = 0;

	// Default taunt macros
#if defined(DXX_BUILD_DESCENT_I)
	strcpy(PlayerCfg.NetworkMessageMacro[0], TXT_DEF_MACRO_1);
	strcpy(PlayerCfg.NetworkMessageMacro[1], TXT_DEF_MACRO_2);
	strcpy(PlayerCfg.NetworkMessageMacro[2], TXT_DEF_MACRO_3);
	strcpy(PlayerCfg.NetworkMessageMacro[3], TXT_DEF_MACRO_4);
#elif defined(DXX_BUILD_DESCENT_II)
	strcpy(PlayerCfg.NetworkMessageMacro[0], "Why can't we all just get along?");
	strcpy(PlayerCfg.NetworkMessageMacro[1], "Hey, I got a present for ya");
	strcpy(PlayerCfg.NetworkMessageMacro[2], "I got a hankerin' for a spankerin'");
	strcpy(PlayerCfg.NetworkMessageMacro[3], "This one's headed for Uranus");
#endif
	PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
	
	return 1;
}

static int read_player_dxx(const char *filename)
{
	PHYSFS_file *f;
	int rc = 0;
	char line[50],*word;
	int Stop=0;

	plyr_read_stats();

	f = PHYSFSX_openReadBuffered(filename);

	if(!f || PHYSFS_eof(f))
		return errno;

	while(!Stop && !PHYSFS_eof(f))
	{
		PHYSFSX_fgets(line,50,f);
		word=splitword(line,':');
		d_strupr(word);
#if defined(DXX_BUILD_DESCENT_I)
		if (strstr(word,"WEAPON REORDER"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			d_strupr(word);
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				unsigned int wo0=0,wo1=0,wo2=0,wo3=0,wo4=0,wo5=0;
				if(!strcmp(word,"PRIMARY"))
				{
					sscanf(line,"0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",&wo0, &wo1, &wo2, &wo3, &wo4, &wo5);
					PlayerCfg.PrimaryOrder[0]=wo0; PlayerCfg.PrimaryOrder[1]=wo1; PlayerCfg.PrimaryOrder[2]=wo2; PlayerCfg.PrimaryOrder[3]=wo3; PlayerCfg.PrimaryOrder[4]=wo4; PlayerCfg.PrimaryOrder[5]=wo5;
				}
				else if(!strcmp(word,"SECONDARY"))
				{
					sscanf(line,"0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",&wo0, &wo1, &wo2, &wo3, &wo4, &wo5);
					PlayerCfg.SecondaryOrder[0]=wo0; PlayerCfg.SecondaryOrder[1]=wo1; PlayerCfg.SecondaryOrder[2]=wo2; PlayerCfg.SecondaryOrder[3]=wo3; PlayerCfg.SecondaryOrder[4]=wo4; PlayerCfg.SecondaryOrder[5]=wo5;
				}
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				d_strupr(word);
			}
		}
		else
#endif
		if (strstr(word,"KEYBOARD"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			d_strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"SENSITIVITY0"))
					PlayerCfg.KeyboardSens[0] = atoi(line);
				if(!strcmp(word,"SENSITIVITY1"))
					PlayerCfg.KeyboardSens[1] = atoi(line);
				if(!strcmp(word,"SENSITIVITY2"))
					PlayerCfg.KeyboardSens[2] = atoi(line);
				if(!strcmp(word,"SENSITIVITY3"))
					PlayerCfg.KeyboardSens[3] = atoi(line);
				if(!strcmp(word,"SENSITIVITY4"))
					PlayerCfg.KeyboardSens[4] = atoi(line);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				d_strupr(word);
			}
		}
		else if (strstr(word,"JOYSTICK"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			d_strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"SENSITIVITY0"))
					PlayerCfg.JoystickSens[0] = atoi(line);
				if(!strcmp(word,"SENSITIVITY1"))
					PlayerCfg.JoystickSens[1] = atoi(line);
				if(!strcmp(word,"SENSITIVITY2"))
					PlayerCfg.JoystickSens[2] = atoi(line);
				if(!strcmp(word,"SENSITIVITY3"))
					PlayerCfg.JoystickSens[3] = atoi(line);
				if(!strcmp(word,"SENSITIVITY4"))
					PlayerCfg.JoystickSens[4] = atoi(line);
				if(!strcmp(word,"SENSITIVITY5"))
					PlayerCfg.JoystickSens[5] = atoi(line);
				if(!strcmp(word,"DEADZONE0"))
					PlayerCfg.JoystickDead[0] = atoi(line);
				if(!strcmp(word,"DEADZONE1"))
					PlayerCfg.JoystickDead[1] = atoi(line);
				if(!strcmp(word,"DEADZONE2"))
					PlayerCfg.JoystickDead[2] = atoi(line);
				if(!strcmp(word,"DEADZONE3"))
					PlayerCfg.JoystickDead[3] = atoi(line);
				if(!strcmp(word,"DEADZONE4"))
					PlayerCfg.JoystickDead[4] = atoi(line);
				if(!strcmp(word,"DEADZONE5"))
					PlayerCfg.JoystickDead[5] = atoi(line);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				d_strupr(word);
			}
		}
		else if (strstr(word,"MOUSE"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			d_strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"FLIGHTSIM"))
					PlayerCfg.MouseFlightSim = atoi(line);
				if(!strcmp(word,"SENSITIVITY0"))
					PlayerCfg.MouseSens[0] = atoi(line);
				if(!strcmp(word,"SENSITIVITY1"))
					PlayerCfg.MouseSens[1] = atoi(line);
				if(!strcmp(word,"SENSITIVITY2"))
					PlayerCfg.MouseSens[2] = atoi(line);
				if(!strcmp(word,"SENSITIVITY3"))
					PlayerCfg.MouseSens[3] = atoi(line);
				if(!strcmp(word,"SENSITIVITY4"))
					PlayerCfg.MouseSens[4] = atoi(line);
				if(!strcmp(word,"SENSITIVITY5"))
					PlayerCfg.MouseSens[5] = atoi(line);
				if(!strcmp(word,"FSDEAD"))
					PlayerCfg.MouseFSDead = atoi(line);
				if(!strcmp(word,"FSINDI"))
					PlayerCfg.MouseFSIndicator = atoi(line);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				d_strupr(word);
			}
		}
		else if (strstr(word,"WEAPON KEYS V2"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			d_strupr(word);
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				unsigned int kc1=0,kc2=0,kc3=0;
				int i=atoi(word);
				
				if(i==0) i=10;
					i=(i-1)*3;
		
				sscanf(line,"0x%x,0x%x,0x%x",&kc1,&kc2,&kc3);
				PlayerCfg.KeySettingsRebirth[i]   = kc1;
				PlayerCfg.KeySettingsRebirth[i+1] = kc2;
				PlayerCfg.KeySettingsRebirth[i+2] = kc3;
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				d_strupr(word);
			}
		}
		else if (strstr(word,"COCKPIT"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			d_strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
#if defined(DXX_BUILD_DESCENT_I)
				if(!strcmp(word,"MODE"))
					PlayerCfg.CockpitMode[0] = PlayerCfg.CockpitMode[1] = atoi(line);
				else
#endif
				if(!strcmp(word,"HUD"))
					PlayerCfg.HudMode = atoi(line);
				else if(!strcmp(word,"RETTYPE"))
					PlayerCfg.ReticleType = atoi(line);
				else if(!strcmp(word,"RETRGBA"))
					sscanf(line,"%i,%i,%i,%i",&PlayerCfg.ReticleRGBA[0],&PlayerCfg.ReticleRGBA[1],&PlayerCfg.ReticleRGBA[2],&PlayerCfg.ReticleRGBA[3]);
				else if(!strcmp(word,"RETSIZE"))
					PlayerCfg.ReticleSize = atoi(line);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				d_strupr(word);
			}
		}
		else if (strstr(word,"TOGGLES"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			d_strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
#if defined(DXX_BUILD_DESCENT_I)
				if(!strcmp(word,"BOMBGAUGE"))
					PlayerCfg.BombGauge = atoi(line);
#elif defined(DXX_BUILD_DESCENT_II)
				if(!strcmp(word,"ESCORTHOTKEYS"))
					PlayerCfg.EscortHotKeys = atoi(line);
#endif
				if(!strcmp(word,"PERSISTENTDEBRIS"))
					PlayerCfg.PersistentDebris = atoi(line);
				if(!strcmp(word,"PRSHOT"))
					PlayerCfg.PRShot = atoi(line);
				if(!strcmp(word,"NOREDUNDANCY"))
					PlayerCfg.NoRedundancy = atoi(line);
				if(!strcmp(word,"MULTIMESSAGES"))
					PlayerCfg.MultiMessages = atoi(line);
				if(!strcmp(word,"NORANKINGS"))
					PlayerCfg.NoRankings = atoi(line);
				if(!strcmp(word,"AUTOMAPFREEFLIGHT"))
					PlayerCfg.AutomapFreeFlight = atoi(line);
				if(!strcmp(word,"NOFIREAUTOSELECT"))
					PlayerCfg.NoFireAutoselect = atoi(line);
				if(!strcmp(word,"CYCLEAUTOSELECTONLY"))
					PlayerCfg.CycleAutoselectOnly = atoi(line);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				d_strupr(word);
			}
		}
		else if (strstr(word,"GRAPHICS"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			d_strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"ALPHAEFFECTS"))
					PlayerCfg.AlphaEffects = atoi(line);
				if(!strcmp(word,"DYNLIGHTCOLOR"))
					PlayerCfg.DynLightColor = atoi(line);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				d_strupr(word);
			}
		}
		else if (strstr(word,"PLX VERSION")) // know the version this pilot was used last with - allow modifications
		{
			int v1=0,v2=0,v3=0;
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			d_strupr(word);
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				sscanf(line,"%i.%i.%i",&v1,&v2,&v3);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				d_strupr(word);
			}
			if (v1 == 0 && v2 == 56 && v3 == 0) // was 0.56.0
				if (DXX_VERSION_MAJORi != v1 || DXX_VERSION_MINORi != v2 || DXX_VERSION_MICROi != v3) // newer (presumably)
				{
					// reset joystick/mouse cycling fields
#if defined(DXX_BUILD_DESCENT_I)
					PlayerCfg.KeySettings[1][44] = 255;
					PlayerCfg.KeySettings[1][45] = 255;
					PlayerCfg.KeySettings[1][46] = 255;
					PlayerCfg.KeySettings[1][47] = 255;
					PlayerCfg.KeySettings[2][27] = 255;
#endif
					PlayerCfg.KeySettings[2][28] = 255;
#if defined(DXX_BUILD_DESCENT_II)
					PlayerCfg.KeySettings[2][29] = 255;
#endif
				}
		}
		else if (strstr(word,"END") || PHYSFS_eof(f))
		{
			Stop=1;
		}
		else
		{
#if defined(DXX_BUILD_DESCENT_I)
			if(word[0]=='['&&!strstr(word,"D1X OPTIONS"))
#elif defined(DXX_BUILD_DESCENT_II)
			if(word[0]=='['&&!strstr(word,"D2X OPTIONS"))
#endif
			{
				while(!strstr(line,"END") && !PHYSFS_eof(f))
				{
					PHYSFSX_fgets(line,50,f);
					d_strupr(line);
				}
			}
		}

		if(word)
			d_free(word);
	}

	PHYSFS_close(f);

	return rc;
}

#if defined(DXX_BUILD_DESCENT_I)
static const char effcode1[]="d1xrocks_SKCORX!D";
static const char effcode2[]="AObe)7Rn1 -+/zZ'0";
static const char effcode3[]="aoeuidhtnAOEUIDH6";
static const char effcode4[]="'/.;]<{=,+?|}->[3";

static unsigned char * decode_stat(unsigned char *p,int *v,const char *effcode)
{
	unsigned char c;
	int neg,i;

	if (p[0]==0)
		return NULL;
	else if (p[0]>='a'){
		neg=1;/*I=p[0]-'a';*/
	}else{
		neg=0;/*I=p[0]-'A';*/
	}
	i=0;*v=0;
	p++;
	while (p[i*2] && p[i*2+1] && p[i*2]!=' '){
		c=(p[i*2]-33)+((p[i*2+1]-33)<<4);
		c^=effcode[i+neg];
		*v+=c << (i*8);
		i++;
	}
	if (neg)
	     *v *= -1;
	if (!p[i*2])
		return NULL;
	return p+(i*2);
}

static void plyr_read_stats_v(int *k, int *d)
{
	char filename[PATH_MAX];
	int k1=-1,k2=0,d1=-1,d2=0;
	PHYSFS_file *f;
	
	*k=0;*d=0;//in case the file doesn't exist.

	memset(filename, '\0', PATH_MAX);
	snprintf(filename,sizeof(filename),PLAYER_DIRECTORY_STRING("%s.eff"),Players[Player_num].callsign);
	f = PHYSFSX_openReadBuffered(filename);

	if(f)
	{
		char line[256],*word;
		if(!PHYSFS_eof(f))
		{
			 PHYSFSX_fgets(line,50,f);
			 word=splitword(line,':');
			 if(!strcmp(word,"kills"))
				*k=atoi(line);
			 d_free(word);
		}
		if(!PHYSFS_eof(f))
                {
			 PHYSFSX_fgets(line,50,f);
			 word=splitword(line,':');
			 if(!strcmp(word,"deaths"))
				*d=atoi(line);
			 d_free(word);
		 }
		if(!PHYSFS_eof(f))
		{
			 PHYSFSX_fgets(line,50,f);
			 word=splitword(line,':');
			 if(!strcmp(word,"key") && strlen(line)>10){
				 unsigned char *p;
				 if (line[0]=='0' && line[1]=='1'){
					 if ((p=decode_stat((unsigned char*)line+3,&k1,effcode1))&&
					     (p=decode_stat(p+1,&k2,effcode2))&&
					     (p=decode_stat(p+1,&d1,effcode3))){
						 decode_stat(p+1,&d2,effcode4);
					 }
				 }
			 }
			 d_free(word);
		}
		if (k1!=k2 || k1!=*k || d1!=d2 || d1!=*d)
		{
			*k=0;*d=0;
		}
	}

	if(f)
		PHYSFS_close(f);
}

static void plyr_read_stats()
{
	plyr_read_stats_v(&PlayerCfg.NetlifeKills,&PlayerCfg.NetlifeKilled);
}

void plyr_save_stats()
{
	int kills = PlayerCfg.NetlifeKills,deaths = PlayerCfg.NetlifeKilled, neg, i;
	char filename[PATH_MAX];
	unsigned char buf[16],buf2[16],a;
	PHYSFS_file *f;

	memset(filename, '\0', PATH_MAX);
	snprintf(filename,sizeof(filename),PLAYER_DIRECTORY_STRING("%s.eff"),Players[Player_num].callsign);
	f = PHYSFSX_openWriteBuffered(filename);

	if(!f)
		return; //broken!

	PHYSFSX_printf(f,"kills:%i\n",kills);
	PHYSFSX_printf(f,"deaths:%i\n",deaths);
	PHYSFSX_printf(f,"key:01 ");

	if (kills<0)
	{
		neg=1;
		kills*=-1;
	}
	else
		neg=0;

	for (i=0;kills;i++)
	{
		a=(kills & 0xFF) ^ effcode1[i+neg];
		buf[i*2]=(a&0xF)+33;
		buf[i*2+1]=(a>>4)+33;
		a=(kills & 0xFF) ^ effcode2[i+neg];
		buf2[i*2]=(a&0xF)+33;
		buf2[i*2+1]=(a>>4)+33;
		kills>>=8;
	}

	buf[i*2]=0;
	buf2[i*2]=0;

	if (neg)
		i+='a';
	else
		i+='A';

	PHYSFSX_printf(f,"%c%s %c%s ",i,buf,i,buf2);

	if (deaths<0)
	{
		neg=1;
		deaths*=-1;
	}else
		neg=0;

	for (i=0;deaths;i++)
	{
		a=(deaths & 0xFF) ^ effcode3[i+neg];
		buf[i*2]=(a&0xF)+33;
		buf[i*2+1]=(a>>4)+33;
		a=(deaths & 0xFF) ^ effcode4[i+neg];
		buf2[i*2]=(a&0xF)+33;
		buf2[i*2+1]=(a>>4)+33;
		deaths>>=8;
	}

	buf[i*2]=0;
	buf2[i*2]=0;

	if (neg)
		i+='a';
	else
		i+='A';

	PHYSFSX_printf(f,"%c%s %c%s\n",i,buf,i,buf2);
	
	PHYSFS_close(f);
}
#endif

static int write_player_dxx(const char *filename)
{
	PHYSFS_file *fout;
	int rc=0;
	char tempfile[PATH_MAX];

	strcpy(tempfile,filename);
	tempfile[strlen(tempfile)-4]=0;
	strcat(tempfile,".pl$");
	fout=PHYSFSX_openWriteBuffered(tempfile);
	
	if (!fout && GameArg.SysUsePlayersDir)
	{
		PHYSFS_mkdir(PLAYER_DIRECTORY_STRING(""));	//try making directory
		fout=PHYSFSX_openWriteBuffered(tempfile);
	}
	
	if(fout)
	{
#if defined(DXX_BUILD_DESCENT_I)
		PHYSFSX_printf(fout,"[D1X Options]\n");
		PHYSFSX_printf(fout,"[weapon reorder]\n");
		PHYSFSX_printf(fout,"primary=0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",PlayerCfg.PrimaryOrder[0], PlayerCfg.PrimaryOrder[1], PlayerCfg.PrimaryOrder[2],PlayerCfg.PrimaryOrder[3], PlayerCfg.PrimaryOrder[4], PlayerCfg.PrimaryOrder[5]);
		PHYSFSX_printf(fout,"secondary=0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",PlayerCfg.SecondaryOrder[0], PlayerCfg.SecondaryOrder[1], PlayerCfg.SecondaryOrder[2],PlayerCfg.SecondaryOrder[3], PlayerCfg.SecondaryOrder[4], PlayerCfg.SecondaryOrder[5]);
		PHYSFSX_printf(fout,"[end]\n");
#elif defined(DXX_BUILD_DESCENT_II)
		PHYSFSX_printf(fout,"[D2X OPTIONS]\n");
#endif
		PHYSFSX_printf(fout,"[keyboard]\n");
		PHYSFSX_printf(fout,"sensitivity0=%d\n",PlayerCfg.KeyboardSens[0]);
		PHYSFSX_printf(fout,"sensitivity1=%d\n",PlayerCfg.KeyboardSens[1]);
		PHYSFSX_printf(fout,"sensitivity2=%d\n",PlayerCfg.KeyboardSens[2]);
		PHYSFSX_printf(fout,"sensitivity3=%d\n",PlayerCfg.KeyboardSens[3]);
		PHYSFSX_printf(fout,"sensitivity4=%d\n",PlayerCfg.KeyboardSens[4]);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[joystick]\n");
		PHYSFSX_printf(fout,"sensitivity0=%d\n",PlayerCfg.JoystickSens[0]);
		PHYSFSX_printf(fout,"sensitivity1=%d\n",PlayerCfg.JoystickSens[1]);
		PHYSFSX_printf(fout,"sensitivity2=%d\n",PlayerCfg.JoystickSens[2]);
		PHYSFSX_printf(fout,"sensitivity3=%d\n",PlayerCfg.JoystickSens[3]);
		PHYSFSX_printf(fout,"sensitivity4=%d\n",PlayerCfg.JoystickSens[4]);
		PHYSFSX_printf(fout,"sensitivity5=%d\n",PlayerCfg.JoystickSens[5]);
		PHYSFSX_printf(fout,"deadzone0=%d\n",PlayerCfg.JoystickDead[0]);
		PHYSFSX_printf(fout,"deadzone1=%d\n",PlayerCfg.JoystickDead[1]);
		PHYSFSX_printf(fout,"deadzone2=%d\n",PlayerCfg.JoystickDead[2]);
		PHYSFSX_printf(fout,"deadzone3=%d\n",PlayerCfg.JoystickDead[3]);
		PHYSFSX_printf(fout,"deadzone4=%d\n",PlayerCfg.JoystickDead[4]);
		PHYSFSX_printf(fout,"deadzone5=%d\n",PlayerCfg.JoystickDead[5]);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[mouse]\n");
		PHYSFSX_printf(fout,"flightsim=%d\n",PlayerCfg.MouseFlightSim);
		PHYSFSX_printf(fout,"sensitivity0=%d\n",PlayerCfg.MouseSens[0]);
		PHYSFSX_printf(fout,"sensitivity1=%d\n",PlayerCfg.MouseSens[1]);
		PHYSFSX_printf(fout,"sensitivity2=%d\n",PlayerCfg.MouseSens[2]);
		PHYSFSX_printf(fout,"sensitivity3=%d\n",PlayerCfg.MouseSens[3]);
		PHYSFSX_printf(fout,"sensitivity4=%d\n",PlayerCfg.MouseSens[4]);
		PHYSFSX_printf(fout,"sensitivity5=%d\n",PlayerCfg.MouseSens[5]);
		PHYSFSX_printf(fout,"fsdead=%d\n",PlayerCfg.MouseFSDead);
		PHYSFSX_printf(fout,"fsindi=%d\n",PlayerCfg.MouseFSIndicator);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[weapon keys v2]\n");
		PHYSFSX_printf(fout,"1=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsRebirth[0],PlayerCfg.KeySettingsRebirth[1],PlayerCfg.KeySettingsRebirth[2]);
		PHYSFSX_printf(fout,"2=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsRebirth[3],PlayerCfg.KeySettingsRebirth[4],PlayerCfg.KeySettingsRebirth[5]);
		PHYSFSX_printf(fout,"3=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsRebirth[6],PlayerCfg.KeySettingsRebirth[7],PlayerCfg.KeySettingsRebirth[8]);
		PHYSFSX_printf(fout,"4=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsRebirth[9],PlayerCfg.KeySettingsRebirth[10],PlayerCfg.KeySettingsRebirth[11]);
		PHYSFSX_printf(fout,"5=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsRebirth[12],PlayerCfg.KeySettingsRebirth[13],PlayerCfg.KeySettingsRebirth[14]);
		PHYSFSX_printf(fout,"6=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsRebirth[15],PlayerCfg.KeySettingsRebirth[16],PlayerCfg.KeySettingsRebirth[17]);
		PHYSFSX_printf(fout,"7=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsRebirth[18],PlayerCfg.KeySettingsRebirth[19],PlayerCfg.KeySettingsRebirth[20]);
		PHYSFSX_printf(fout,"8=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsRebirth[21],PlayerCfg.KeySettingsRebirth[22],PlayerCfg.KeySettingsRebirth[23]);
		PHYSFSX_printf(fout,"9=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsRebirth[24],PlayerCfg.KeySettingsRebirth[25],PlayerCfg.KeySettingsRebirth[26]);
		PHYSFSX_printf(fout,"0=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsRebirth[27],PlayerCfg.KeySettingsRebirth[28],PlayerCfg.KeySettingsRebirth[29]);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[cockpit]\n");
#if defined(DXX_BUILD_DESCENT_I)
		PHYSFSX_printf(fout,"mode=%i\n",PlayerCfg.CockpitMode[0]);
#endif
		PHYSFSX_printf(fout,"hud=%i\n",PlayerCfg.HudMode);
		PHYSFSX_printf(fout,"rettype=%i\n",PlayerCfg.ReticleType);
		PHYSFSX_printf(fout,"retrgba=%i,%i,%i,%i\n",PlayerCfg.ReticleRGBA[0],PlayerCfg.ReticleRGBA[1],PlayerCfg.ReticleRGBA[2],PlayerCfg.ReticleRGBA[3]);
		PHYSFSX_printf(fout,"retsize=%i\n",PlayerCfg.ReticleSize);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[toggles]\n");
#if defined(DXX_BUILD_DESCENT_I)
		PHYSFSX_printf(fout,"bombgauge=%i\n",PlayerCfg.BombGauge);
#elif defined(DXX_BUILD_DESCENT_II)
		PHYSFSX_printf(fout,"escorthotkeys=%i\n",PlayerCfg.EscortHotKeys);
#endif
		PHYSFSX_printf(fout,"persistentdebris=%i\n",PlayerCfg.PersistentDebris);
		PHYSFSX_printf(fout,"prshot=%i\n",PlayerCfg.PRShot);
		PHYSFSX_printf(fout,"noredundancy=%i\n",PlayerCfg.NoRedundancy);
		PHYSFSX_printf(fout,"multimessages=%i\n",PlayerCfg.MultiMessages);
		PHYSFSX_printf(fout,"norankings=%i\n",PlayerCfg.NoRankings);
		PHYSFSX_printf(fout,"automapfreeflight=%i\n",PlayerCfg.AutomapFreeFlight);
		PHYSFSX_printf(fout,"nofireautoselect=%i\n",PlayerCfg.NoFireAutoselect);
		PHYSFSX_printf(fout,"cycleautoselectonly=%i\n",PlayerCfg.CycleAutoselectOnly);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[graphics]\n");
		PHYSFSX_printf(fout,"alphaeffects=%i\n",PlayerCfg.AlphaEffects);
		PHYSFSX_printf(fout,"dynlightcolor=%i\n",PlayerCfg.DynLightColor);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[plx version]\n");
		PHYSFSX_printf(fout,"plx version=%s\n", VERSION);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[end]\n");

		PHYSFS_close(fout);
		if(rc==0)
		{
			PHYSFS_delete(filename);
			rc = PHYSFSX_rename(tempfile,filename);
		}
		return rc;
	}
	else
		return errno;

}

//read in the player's saved games.  returns errno (0 == no error)
int read_player_file()
{
	char filename[PATH_MAX];
	PHYSFS_file *file;
#if defined(DXX_BUILD_DESCENT_I)
	int shareware_file = -1;
	int player_file_size;
#elif defined(DXX_BUILD_DESCENT_II)
	int rewrite_it=0;
	int swap = 0;
	short player_file_version;
#endif

	Assert(Player_num>=0 && Player_num<MAX_PLAYERS);

	memset(filename, '\0', PATH_MAX);
	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%.8s.plr"), Players[Player_num].callsign);
	if (!PHYSFSX_exists(filename,0))
		return ENOENT;

	file = PHYSFSX_openReadBuffered(filename);

	if (!file)
		goto read_player_file_failed;

	new_player_config(); // Set defaults!

#if defined(DXX_BUILD_DESCENT_I)
	// Unfortunatly d1x has been writing both shareware and registered
	// player files with a saved_game_version of 7 and 8, whereas the
	// original decent used 4 for shareware games and 7 for registered
	// games. Because of this the player files didn't get properly read
	// when reading d1x shareware player files in d1x registered or
	// vica versa. The problem is that the sizeof of the taunt macros
	// differ between the share and registered versions, causing the
	// reading of the player file to go wrong. Thus we now determine the
	// sizeof the player file to determine what kinda player file we are
	// dealing with so that we can do the right thing
	PHYSFS_seek(file, 0);
	player_file_size = PHYSFS_fileLength(file);
#endif
	int id;
	PHYSFS_readSLE32(file, &id);
#if defined(DXX_BUILD_DESCENT_I)
	short saved_game_version, player_struct_version;
	saved_game_version = PHYSFSX_readShort(file);
	player_struct_version = PHYSFSX_readShort(file);
	PlayerCfg.NHighestLevels = PHYSFSX_readInt(file);
	PlayerCfg.DefaultDifficulty = PHYSFSX_readInt(file);
	PlayerCfg.AutoLeveling = PHYSFSX_readInt(file);
#elif defined(DXX_BUILD_DESCENT_II)
	player_file_version = PHYSFSX_readShort(file);
#endif

	if (id!=SAVE_FILE_ID) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Invalid player file");
		PHYSFS_close(file);
		return -1;
	}

#if defined(DXX_BUILD_DESCENT_I)
	if (saved_game_version<COMPATIBLE_SAVED_GAME_VERSION || player_struct_version<COMPATIBLE_PLAYER_STRUCT_VERSION) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_ERROR_PLR_VERSION);
		PHYSFS_close(file);
		return -1;
	}

	/* determine if we're dealing with a shareware or registered playerfile */
	switch (saved_game_version)
	{
		case 4:
			shareware_file = 1;
			break;
		case 5:
		case 6:
			shareware_file = 0;
			break;
		case 7:
			/* version 7 doesn't have the saved games array */
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == (2212 - sizeof(saved_games)))
				shareware_file = 1;
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == (2252 - sizeof(saved_games)))
				shareware_file = 0;
			break;
		case 8:
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == 2212)
				shareware_file = 1;
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == 2252)
				shareware_file = 0;
			/* d1x-rebirth v0.31 to v0.42 broke things by adding stuff to the
			   player struct without thinking (sigh) */
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == (2212 + 2*sizeof(int)))
			{

				shareware_file = 1;
				/* skip the cruft added to the player_info struct */
				PHYSFS_seek(file, PHYSFS_tell(file)+2*sizeof(int));
			}
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == (2252 + 2*sizeof(int)))
			{
				shareware_file = 0;
				/* skip the cruft added to the player_info struct */
				PHYSFS_seek(file, PHYSFS_tell(file)+2*sizeof(int));
			}
	}

	if (shareware_file == -1) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Error invalid or unknown\nplayerfile-size");
		PHYSFS_close(file);
		return -1;
	}

	if (saved_game_version <= 5) {

		//deal with old-style highest level info

		PlayerCfg.HighestLevels[0].Shortname[0] = 0;							//no name for mission 0
		PlayerCfg.HighestLevels[0].LevelNum = PlayerCfg.NHighestLevels;	//was highest level in old struct

		//This hack allows the player to start on level 8 if he's made it to
		//level 7 on the shareware.  We do this because the shareware didn't
		//save the information that the player finished level 7, so the most
		//we know is that he made it to level 7.
		if (PlayerCfg.NHighestLevels==7)
			PlayerCfg.HighestLevels[0].LevelNum = 8;
		
	}
	else {	//read new highest level info
		if (PHYSFS_read(file,PlayerCfg.HighestLevels,sizeof(hli),PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels)
			goto read_player_file_failed;
	}

	if ( saved_game_version != 7 ) {	// Read old & SW saved games.
		if (PHYSFS_read(file,saved_games,sizeof(saved_games),1) != 1)
			goto read_player_file_failed;
	}

#elif defined(DXX_BUILD_DESCENT_II)
	if (player_file_version > 255) // bigendian file?
		swap = 1;

	if (swap)
		player_file_version = SWAPSHORT(player_file_version);

	if (player_file_version<COMPATIBLE_PLAYER_FILE_VERSION) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_ERROR_PLR_VERSION);
		PHYSFS_close(file);
		return -1;
	}

	PHYSFS_seek(file,PHYSFS_tell(file)+2*sizeof(short)); //skip Game_window_w,Game_window_h
	PlayerCfg.DefaultDifficulty = PHYSFSX_readByte(file);
	PlayerCfg.AutoLeveling       = PHYSFSX_readByte(file);
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(sbyte)); // skip ReticleOn
	PlayerCfg.CockpitMode[0] = PlayerCfg.CockpitMode[1] = PHYSFSX_readByte(file);
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(sbyte)); //skip Default_display_mode
	PlayerCfg.MissileViewEnabled      = PHYSFSX_readByte(file);
	PlayerCfg.HeadlightActiveDefault  = PHYSFSX_readByte(file);
	PlayerCfg.GuidedInBigWindow      = PHYSFSX_readByte(file);
	if (player_file_version >= 19)
		PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(sbyte)); //skip Automap_always_hires

	//read new highest level info

	PlayerCfg.NHighestLevels = PHYSFSX_readShort(file);
	if (swap)
		PlayerCfg.NHighestLevels = SWAPSHORT(PlayerCfg.NHighestLevels);
	Assert(PlayerCfg.NHighestLevels <= MAX_MISSIONS);

	if (PHYSFS_read(file, PlayerCfg.HighestLevels, sizeof(hli), PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels)
		goto read_player_file_failed;
#endif

	//read taunt macros
	{
		int len;

#if defined(DXX_BUILD_DESCENT_I)
		len = shareware_file? 25:35;
#elif defined(DXX_BUILD_DESCENT_II)
		len = MAX_MESSAGE_LEN;
#endif

		for (unsigned i = 0; i < sizeof(PlayerCfg.NetworkMessageMacro) / sizeof(PlayerCfg.NetworkMessageMacro[0]); i++)
			if (PHYSFS_read(file, PlayerCfg.NetworkMessageMacro[i], len, 1) != 1)
				goto read_player_file_failed;
	}

	//read kconfig data
	{
		ubyte dummy_joy_sens;

		if (PHYSFS_read(file, &PlayerCfg.KeySettings[0], sizeof(PlayerCfg.KeySettings[0]),1)!=1)
			goto read_player_file_failed;
		if (PHYSFS_read(file, &PlayerCfg.KeySettings[1], sizeof(PlayerCfg.KeySettings[1]),1)!=1)
			goto read_player_file_failed;
		PHYSFS_seek( file, PHYSFS_tell(file)+(sizeof(ubyte)*MAX_CONTROLS*3) ); // Skip obsolete Flightstick/Thrustmaster/Gravis map fields
		if (PHYSFS_read(file, &PlayerCfg.KeySettings[2], sizeof(PlayerCfg.KeySettings[2]),1)!=1)
			goto read_player_file_failed;
		PHYSFS_seek( file, PHYSFS_tell(file)+(sizeof(ubyte)*MAX_CONTROLS) ); // Skip obsolete Cyberman map field
#if defined(DXX_BUILD_DESCENT_I)
		if (PHYSFS_read(file, &PlayerCfg.ControlType, sizeof(ubyte), 1 )!=1)
#elif defined(DXX_BUILD_DESCENT_II)
		if (player_file_version>=20)
			PHYSFS_seek( file, PHYSFS_tell(file)+(sizeof(ubyte)*MAX_CONTROLS) ); // Skip obsolete Winjoy map field
		ubyte control_type_dos, control_type_win;
		if (PHYSFS_read(file, (ubyte *)&control_type_dos, sizeof(ubyte), 1) != 1)
			goto read_player_file_failed;
		else if (player_file_version >= 21 && PHYSFS_read(file, (ubyte *)&control_type_win, sizeof(ubyte), 1) != 1)
#endif
			goto read_player_file_failed;
		else if (PHYSFS_read(file, &dummy_joy_sens, sizeof(ubyte), 1) !=1 )
			goto read_player_file_failed;

#if defined(DXX_BUILD_DESCENT_II)
		PlayerCfg.ControlType = control_type_dos;
	
		for (unsigned i=0;i<11;i++)
		{
			PlayerCfg.PrimaryOrder[i] = PHYSFSX_readByte(file);
			PlayerCfg.SecondaryOrder[i] = PHYSFSX_readByte(file);
		}

		if (player_file_version>=16)
		{
			PHYSFS_readSLE32(file, &PlayerCfg.Cockpit3DView[0]);
			PHYSFS_readSLE32(file, &PlayerCfg.Cockpit3DView[1]);
			if (swap)
			{
				PlayerCfg.Cockpit3DView[0] = SWAPINT(PlayerCfg.Cockpit3DView[0]);
				PlayerCfg.Cockpit3DView[1] = SWAPINT(PlayerCfg.Cockpit3DView[1]);
			}
		}
#endif
	}

#if defined(DXX_BUILD_DESCENT_I)
	if ( saved_game_version != 7 ) 	{
		int i, found=0;
		
		Assert( N_SAVE_SLOTS == 10 );

		for (i=0; i<N_SAVE_SLOTS; i++ )	{
			if ( saved_games[i].name[0] )	{
				state_save_old_game(i, saved_games[i].name, &saved_games[i].sg_player, saved_games[i].difficulty_level, saved_games[i].primary_weapon, saved_games[i].secondary_weapon, saved_games[i].next_level_num );
				// make sure we do not do this again, which would possibly overwrite
				// a new newstyle savegame
				saved_games[i].name[0] = 0;
				found++;
			}
		}
		if (found)
			write_player_file();
	}
#elif defined(DXX_BUILD_DESCENT_II)
	if (player_file_version>=22)
	{
		PHYSFS_readSLE32(file, &PlayerCfg.NetlifeKills);
		PHYSFS_readSLE32(file, &PlayerCfg.NetlifeKilled);
		if (swap) {
			PlayerCfg.NetlifeKills = SWAPINT(PlayerCfg.NetlifeKills);
			PlayerCfg.NetlifeKilled = SWAPINT(PlayerCfg.NetlifeKilled);
		}
	}
	else
	{
		PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
	}

	if (player_file_version>=23)
	{
		int i;
		PHYSFS_readSLE32(file, &i);
		if (swap)
			i = SWAPINT(i);
		if (i!=get_lifetime_checksum (PlayerCfg.NetlifeKills,PlayerCfg.NetlifeKilled))
		{
			PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
			nm_messagebox(NULL, 1, "Shame on me", "Trying to cheat eh?");
			rewrite_it=1;
		}
	}

	//read guidebot name
	if (player_file_version >= 18)
		PHYSFSX_readString(file, PlayerCfg.GuidebotName);
	else
		strcpy(PlayerCfg.GuidebotName,"GUIDE-BOT");

	strcpy(PlayerCfg.GuidebotNameReal,PlayerCfg.GuidebotName);

	{
		char buf[128];

		if (player_file_version >= 24) 
			PHYSFSX_readString(file, buf);			// Just read it in fpr DPS.
	}
#endif

	if (!PHYSFS_close(file))
		goto read_player_file_failed;

#if defined(DXX_BUILD_DESCENT_II)
	if (rewrite_it)
		write_player_file();
#endif

	filename[strlen(filename) - 4] = 0;
	strcat(filename, ".plx");
	read_player_dxx(filename);
	kc_set_controls();

	return EZERO;

 read_player_file_failed:
	nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s\n\n%s", "Error reading PLR file", PHYSFS_getLastError());
	if (file)
		PHYSFS_close(file);

	return -1;
}


//finds entry for this level in table.  if not found, returns ptr to 
//empty entry.  If no empty entries, takes over last one 
static int find_hli_entry()
{
	int i;

	for (i=0;i<PlayerCfg.NHighestLevels;i++)
		if (!d_stricmp(PlayerCfg.HighestLevels[i].Shortname, Current_mission_filename))
			break;

	if (i==PlayerCfg.NHighestLevels) { //not found. create entry

		if (i==MAX_MISSIONS)
			i--; //take last entry
		else
			PlayerCfg.NHighestLevels++;

		strcpy(PlayerCfg.HighestLevels[i].Shortname, Current_mission_filename);
		PlayerCfg.HighestLevels[i].LevelNum = 0;
	}

	return i;
}

//set a new highest level for player for this mission
void set_highest_level(int levelnum)
{
	int ret,i;

	if ((ret=read_player_file()) != EZERO)
		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return;

	i = find_hli_entry();

	if (levelnum > PlayerCfg.HighestLevels[i].LevelNum)
		PlayerCfg.HighestLevels[i].LevelNum = levelnum;

	write_player_file();
}

//gets the player's highest level from the file for this mission
int get_highest_level(void)
{
	int i;
	int highest_saturn_level = 0;
	read_player_file();
#ifndef SATURN
	if (strlen(Current_mission_filename)==0 )	{
		for (i=0;i<PlayerCfg.NHighestLevels;i++)
			if (!d_stricmp(PlayerCfg.HighestLevels[i].Shortname, "DESTSAT")) // Destination Saturn.
				highest_saturn_level = PlayerCfg.HighestLevels[i].LevelNum;
	}
#endif
	i = PlayerCfg.HighestLevels[find_hli_entry()].LevelNum;
	if ( highest_saturn_level > i )
		i = highest_saturn_level;
	return i;
}

//write out player's saved games.  returns errno (0 == no error)
void write_player_file()
{
	char filename[PATH_MAX];
	PHYSFS_file *file;
	int errno_ret;

	if ( Newdemo_state == ND_STATE_PLAYBACK )
		return;

	errno_ret = WriteConfigFile();

	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%.8s.plx"), Players[Player_num].callsign);
	write_player_dxx(filename);
	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%.8s.plr"), Players[Player_num].callsign);
	file = PHYSFSX_openWriteBuffered(filename);

	if (!file)
		return;

	//Write out player's info
	PHYSFS_writeULE32(file, SAVE_FILE_ID);
#if defined(DXX_BUILD_DESCENT_I)
	PHYSFS_writeULE16(file, SAVED_GAME_VERSION);
	PHYSFS_writeULE16(file, PLAYER_STRUCT_VERSION);
	PHYSFS_writeSLE32(file, PlayerCfg.NHighestLevels);
	PHYSFS_writeSLE32(file, PlayerCfg.DefaultDifficulty);
	PHYSFS_writeSLE32(file, PlayerCfg.AutoLeveling);
	errno_ret = EZERO;

	//write higest level info
	if ((PHYSFS_write( file, PlayerCfg.HighestLevels, sizeof(hli), PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels)) {
		errno_ret = errno;
		PHYSFS_close(file);
		return;
	}

	if (PHYSFS_write( file, saved_games,sizeof(saved_games),1) != 1) {
		errno_ret = errno;
		PHYSFS_close(file);
		return;
	}

	if ((PHYSFS_write( file, PlayerCfg.NetworkMessageMacro, MAX_MESSAGE_LEN, 4) != 4)) {
		errno_ret = errno;
		PHYSFS_close(file);
		return;
	}

	//write kconfig info
	{
		if (PHYSFS_write(file, PlayerCfg.KeySettings[0], sizeof(PlayerCfg.KeySettings[0]), 1) != 1)
			errno_ret=errno;
		if (PHYSFS_write(file, PlayerCfg.KeySettings[1], sizeof(PlayerCfg.KeySettings[1]), 1) != 1)
			errno_ret=errno;
		for (unsigned i = 0; i < MAX_CONTROLS*3; i++)
			if (PHYSFS_write(file, "0", sizeof(ubyte), 1) != 1) // Skip obsolete Flightstick/Thrustmaster/Gravis map fields
				errno_ret=errno;
		if (PHYSFS_write(file, PlayerCfg.KeySettings[2], sizeof(PlayerCfg.KeySettings[2]), 1) != 1)
			errno_ret=errno;
		for (unsigned i = 0; i < MAX_CONTROLS; i++)
			if (PHYSFS_write(file, "0", sizeof(ubyte), 1) != 1) // Skip obsolete Cyberman map field
				errno_ret=errno;
	
		if(errno_ret == EZERO)
		{
			ubyte old_avg_joy_sensitivity = 8;
			if (PHYSFS_write( file,  &PlayerCfg.ControlType, sizeof(ubyte), 1 )!=1)
				errno_ret=errno;
			else if (PHYSFS_write( file, &old_avg_joy_sensitivity, sizeof(ubyte), 1 )!=1)
				errno_ret=errno;
		}
	}

	if (!PHYSFS_close(file))
		errno_ret = errno;

	if (errno_ret != EZERO) {
		PHYSFS_delete(filename);			//delete bogus file
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s\n\n%s",TXT_ERROR_WRITING_PLR, strerror(errno_ret));
	}
#elif defined(DXX_BUILD_DESCENT_II)
	(void)errno_ret;
	PHYSFS_writeULE16(file, PLAYER_FILE_VERSION);

	
	PHYSFS_seek(file,PHYSFS_tell(file)+2*(sizeof(PHYSFS_uint16))); // skip Game_window_w, Game_window_h
	PHYSFSX_writeU8(file, PlayerCfg.DefaultDifficulty);
	PHYSFSX_writeU8(file, PlayerCfg.AutoLeveling);
	PHYSFSX_writeU8(file, PlayerCfg.ReticleType==RET_TYPE_NONE?0:1);
	PHYSFSX_writeU8(file, PlayerCfg.CockpitMode[0]);
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(PHYSFS_uint8)); // skip Default_display_mode
	PHYSFSX_writeU8(file, PlayerCfg.MissileViewEnabled);
	PHYSFSX_writeU8(file, PlayerCfg.HeadlightActiveDefault);
	PHYSFSX_writeU8(file, PlayerCfg.GuidedInBigWindow);
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(PHYSFS_uint8)); // skip Automap_always_hires

	//write higest level info
	PHYSFS_writeULE16(file, PlayerCfg.NHighestLevels);
	if ((PHYSFS_write(file, PlayerCfg.HighestLevels, sizeof(hli), PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels))
		goto write_player_file_failed;

	if ((PHYSFS_write(file, PlayerCfg.NetworkMessageMacro, MAX_MESSAGE_LEN, 4) != 4))
		goto write_player_file_failed;

	//write kconfig info
	{

		ubyte old_avg_joy_sensitivity = 8;
		ubyte control_type_dos = PlayerCfg.ControlType;

		if (PHYSFS_write(file, PlayerCfg.KeySettings[0], sizeof(PlayerCfg.KeySettings[0]), 1) != 1)
			goto write_player_file_failed;
		if (PHYSFS_write(file, PlayerCfg.KeySettings[1], sizeof(PlayerCfg.KeySettings[1]), 1) != 1)
			goto write_player_file_failed;
		for (unsigned i = 0; i < MAX_CONTROLS*3; i++)
			if (PHYSFS_write(file, "0", sizeof(ubyte), 1) != 1) // Skip obsolete Flightstick/Thrustmaster/Gravis map fields
				goto write_player_file_failed;
		if (PHYSFS_write(file, PlayerCfg.KeySettings[2], sizeof(PlayerCfg.KeySettings[2]), 1) != 1)
			goto write_player_file_failed;
		for (unsigned i = 0; i < MAX_CONTROLS*2; i++)
			if (PHYSFS_write(file, "0", sizeof(ubyte), 1) != 1) // Skip obsolete Cyberman/Winjoy map fields
				goto write_player_file_failed;
		if (PHYSFS_write(file, &control_type_dos, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;
		ubyte control_type_win = 0;
		if (PHYSFS_write(file, &control_type_win, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;
		if (PHYSFS_write(file, &old_avg_joy_sensitivity, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;

		for (unsigned i = 0; i < 11; i++)
		{
			PHYSFS_write(file, &PlayerCfg.PrimaryOrder[i], sizeof(ubyte), 1);
			PHYSFS_write(file, &PlayerCfg.SecondaryOrder[i], sizeof(ubyte), 1);
		}

		PHYSFS_writeULE32(file, PlayerCfg.Cockpit3DView[0]);
		PHYSFS_writeULE32(file, PlayerCfg.Cockpit3DView[1]);

		PHYSFS_writeULE32(file, PlayerCfg.NetlifeKills);
		PHYSFS_writeULE32(file, PlayerCfg.NetlifeKilled);
		int i=get_lifetime_checksum (PlayerCfg.NetlifeKills,PlayerCfg.NetlifeKilled);
		PHYSFS_writeULE32(file, i);
	}

	//write guidebot name
	PHYSFSX_writeString(file, PlayerCfg.GuidebotNameReal);

	{
		char buf[128];
		strcpy(buf, "DOS joystick");
		PHYSFSX_writeString(file, buf);		// Write out current joystick for player.
	}

	if (!PHYSFS_close(file))
		goto write_player_file_failed;

	return;

 write_player_file_failed:
	nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s\n\n%s", TXT_ERROR_WRITING_PLR, PHYSFS_getLastError());
	if (file)
	{
		PHYSFS_close(file);
		PHYSFS_delete(filename);        //delete bogus file
	}
#endif
}

#if defined(DXX_BUILD_DESCENT_II)
static int get_lifetime_checksum (int a,int b)
{
  int num;

  // confusing enough to beat amateur disassemblers? Lets hope so

  num=(a<<8 ^ b);
  num^=(a | b);
  num*=num>>2;
  return (num);
}
#endif

// read stored values from ngp file to netgame_info
void read_netgame_profile(netgame_info *ng)
{
	char filename[PATH_MAX], line[50], *token, *ptr;
	PHYSFS_file *file;

	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%.8s.ngp"), Players[Player_num].callsign);
	if (!PHYSFSX_exists(filename,0))
		return;

	file = PHYSFSX_openReadBuffered(filename);

	if (!file)
		return;

	// NOTE that we do not set any defaults here or even initialize netgame_info. For flexibility, leave that to the function calling this.
	while (!PHYSFS_eof(file))
	{
		memset(line, 0, 50);
		PHYSFSX_gets(file, line);
		ptr = &(line[0]);
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') {
			const char *value;
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
			if (!value)
				value = "";
			if (!strcmp(token, "game_name"))
			{
				char * p;
				strncpy( ng->game_name, value, NETGAME_NAME_LEN+1 );
				p = strchr( ng->game_name, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, "gamemode"))
				ng->gamemode = strtol(value, NULL, 10);
			else if (!strcmp(token, "RefusePlayers"))
				ng->RefusePlayers = strtol(value, NULL, 10);
			else if (!strcmp(token, "difficulty"))
				ng->difficulty = strtol(value, NULL, 10);
			else if (!strcmp(token, "game_flags"))
			{
				packed_game_flags p;
				p.value = strtol(value, NULL, 10);
				ng->game_flag = unpack_game_flags(&p);
			}
			else if (!strcmp(token, "AllowedItems"))
				ng->AllowedItems = strtol(value, NULL, 10);
#if defined(DXX_BUILD_DESCENT_II)
			else if (!strcmp(token, "Allow_marker_view"))
				ng->Allow_marker_view = strtol(value, NULL, 10);
			else if (!strcmp(token, "AlwaysLighting"))
				ng->AlwaysLighting = strtol(value, NULL, 10);
#endif
			else if (!strcmp(token, "ShowEnemyNames"))
				ng->ShowEnemyNames = strtol(value, NULL, 10);
			else if (!strcmp(token, "BrightPlayers"))
				ng->BrightPlayers = strtol(value, NULL, 10);
			else if (!strcmp(token, "InvulAppear"))
				ng->InvulAppear = strtol(value, NULL, 10);
			else if (!strcmp(token, "KillGoal"))
				ng->KillGoal = strtol(value, NULL, 10);
			else if (!strcmp(token, "PlayTimeAllowed"))
				ng->PlayTimeAllowed = strtol(value, NULL, 10);
			else if (!strcmp(token, "control_invul_time"))
				ng->control_invul_time = strtol(value, NULL, 10);
			else if (!strcmp(token, "PacketsPerSec"))
				ng->PacketsPerSec = strtol(value, NULL, 10);
			else if (!strcmp(token, "ShortPackets"))
				ng->ShortPackets = strtol(value, NULL, 10);
			else if (!strcmp(token, "NoFriendlyFire"))
				ng->NoFriendlyFire = strtol(value, NULL, 10);
#ifdef USE_TRACKER
			else if (!strcmp(token, "Tracker"))
				ng->Tracker = strtol(value, NULL, 10);
#endif
		}
	}

	PHYSFS_close(file);
}

// write values from netgame_info to ngp file
void write_netgame_profile(netgame_info *ng)
{
	char filename[PATH_MAX];
	PHYSFS_file *file;

	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%.8s.ngp"), Players[Player_num].callsign);
	file = PHYSFSX_openWriteBuffered(filename);

	if (!file)
		return;

	PHYSFSX_printf(file, "game_name=%s\n", ng->game_name);
	PHYSFSX_printf(file, "gamemode=%i\n", ng->gamemode);
	PHYSFSX_printf(file, "RefusePlayers=%i\n", ng->RefusePlayers);
	PHYSFSX_printf(file, "difficulty=%i\n", ng->difficulty);
	PHYSFSX_printf(file, "game_flags=%i\n", pack_game_flags(&ng->game_flag).value);
	PHYSFSX_printf(file, "AllowedItems=%i\n", ng->AllowedItems);
#if defined(DXX_BUILD_DESCENT_II)
	PHYSFSX_printf(file, "Allow_marker_view=%i\n", ng->Allow_marker_view);
	PHYSFSX_printf(file, "AlwaysLighting=%i\n", ng->AlwaysLighting);
#endif
	PHYSFSX_printf(file, "ShowEnemyNames=%i\n", ng->ShowEnemyNames);
	PHYSFSX_printf(file, "BrightPlayers=%i\n", ng->BrightPlayers);
	PHYSFSX_printf(file, "InvulAppear=%i\n", ng->InvulAppear);
	PHYSFSX_printf(file, "KillGoal=%i\n", ng->KillGoal);
	PHYSFSX_printf(file, "PlayTimeAllowed=%i\n", ng->PlayTimeAllowed);
	PHYSFSX_printf(file, "control_invul_time=%i\n", ng->control_invul_time);
	PHYSFSX_printf(file, "PacketsPerSec=%i\n", ng->PacketsPerSec);
	PHYSFSX_printf(file, "ShortPackets=%i\n", ng->ShortPackets);
	PHYSFSX_printf(file, "NoFriendlyFire=%i\n", ng->NoFriendlyFire);
#ifdef USE_TRACKER
	PHYSFSX_printf(file, "Tracker=%i\n", ng->Tracker);
#else
	PHYSFSX_printf(file, "Tracker=0\n");
#endif
	PHYSFSX_printf(file, "ngp version=%s\n",VERSION);

	PHYSFS_close(file);
}
