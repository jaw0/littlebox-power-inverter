// automatically generated - do not edit

#include "conf.h"
#include "menu.h"
#include "cli.h"
#include "defproto.h"

const struct Menu guitop;
const struct Menu menu_aaaa;
const struct Menu menu_aaac;
const struct Menu menu_aaad;
const struct Menu menu_aaae;
const struct Menu menu_aaaf;
const char * arg_aaab[] = { "-ui", "log/diag" };
const char * arg_aaag[] = { "-ui",  "6", "13"  };
const char * argv_empty[] = { "-ui" };

const struct Menu menu_aaaa = {
    "Logging", &guitop, 0, {
	{ "enable", MTYP_FUNC, (void*)ui_f_logging, sizeof(arg_aaab)/4, arg_aaab },
	{ "disable", MTYP_FUNC, (void*)ui_f_logging, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaac = {
    "Profile", &guitop, 0, {
	{ "Sine", MTYP_FUNC, (void*)ui_f_profsine, sizeof(argv_empty)/4, argv_empty },
	{ "H-Br", MTYP_FUNC, (void*)ui_f_profhpwm, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaad = {
    "Test", &guitop, 0, {
	{ "DC", MTYP_FUNC, (void*)ui_f_testdc, sizeof(argv_empty)/4, argv_empty },
	{ "serial", MTYP_FUNC, (void*)ui_f_debser, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaae = {
    "Settings", &guitop, 0, {
	{ "volume", MTYP_FUNC, (void*)ui_f_volume, sizeof(argv_empty)/4, argv_empty },
	{ "battle short", MTYP_FUNC, (void*)ui_f_set_battle_short, sizeof(argv_empty)/4, argv_empty },
	{ "save", MTYP_FUNC, (void*)ui_f_save_all, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaaf = {
    "Dazzle", &guitop, 0, {
	{ "music", MTYP_FUNC, (void*)ui_f_playsong, sizeof(arg_aaag)/4, arg_aaag },
	{}
    }
};
const struct Menu guitop = {
    "Test Menu", &guitop, 0, {
	{ "logging", MTYP_MENU, (void*)&menu_aaaa },
	{ "profile", MTYP_MENU, (void*)&menu_aaac },
	{ "test", MTYP_MENU, (void*)&menu_aaad },
	{ "settings", MTYP_MENU, (void*)&menu_aaae },
	{ "dazzle", MTYP_MENU, (void*)&menu_aaaf },
	{ "Exit", MTYP_EXIT },
	{}
    }
};
