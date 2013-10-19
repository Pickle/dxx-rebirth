#pragma once

#include "vecmat.h"

#ifdef __cplusplus
struct dxxobject;
#ifdef DXX_USE_STRICT_TYPESAFE
struct objnum_t;
#else
typedef unsigned short objnum_t;
#endif

#ifdef USE_PYTHON
void cxx_script_get_guided_missile_rotang(vms_angvec *);
void cxx_script_get_player_ship_rotthrust(dxxobject *);
unsigned cxx_script_get_player_ship_vecthrust(dxxobject *, fix);
void cxx_script_hook_controls();
void py_load_level_hit();
void py_notify_read_player_file();
void py_notify_create_player_appearance_effect(dxxobject *);
void py_notify_player_join(unsigned);
void py_notify_player_death_reactor(unsigned, objnum_t);
void py_notify_player_death_mine(unsigned, objnum_t);
void py_notify_player_death_robot(unsigned, objnum_t);
void py_notify_player_death_player(unsigned, unsigned);
void py_notify_player_ship_destroyed();
void py_notify_start_endlevel_sequence();
void scripting_init();
void scripting_close();
void scripting_input_enter(const char *);
#else
static inline void cxx_script_get_guided_missile_rotang(vms_angvec *) {}
static inline void cxx_script_get_player_ship_rotthrust(dxxobject *) {}
static inline unsigned cxx_script_get_player_ship_vecthrust(dxxobject *, fix) { return 0; }
static inline void cxx_script_hook_controls() {}
static inline void py_load_level_hit() {}
static inline void py_notify_read_player_file() {}
static inline void py_notify_create_player_appearance_effect(dxxobject *) {}
static inline void py_notify_player_join(unsigned) {}
static inline void py_notify_player_death_reactor(unsigned, const objnum_t&) {}
static inline void py_notify_player_death_mine(unsigned, const objnum_t&) {}
static inline void py_notify_player_death_robot(unsigned, const objnum_t&) {}
static inline void py_notify_player_death_player(unsigned, unsigned) {}
static inline void py_notify_player_ship_destroyed() {}
static inline void py_notify_start_endlevel_sequence() {}
static inline void scripting_init() {}
static inline void scripting_close() {}
static inline void scripting_input_enter(const char *) {}
#endif

#endif
