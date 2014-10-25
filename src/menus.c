// automatically generated - do not edit

#include "conf.h"
#include "menu.h"
#include "cli.h"
#include "defproto.h"

const struct Menu guitop;
const struct Menu menu_aaaa;
const struct Menu menu_aaab;
const struct Menu menu_aaac;
const struct Menu menu_aaad;
const struct Menu menu_aaaf;
const struct Menu menu_aaag;
const char * arg_aaae[] = { "-ui", "log/diag" };
const char * arg_aaah[] = { "-ui",  "6", "13"  };
const char * argv_empty[] = { "-ui" };

const struct Menu menu_aaaa = {
    "Diag Menu", &guitop, 0, {
	{ "sensors", MTYP_FUNC, (void*)ui_f_testsensors, sizeof(argv_empty)/4, argv_empty },
	{ "power", MTYP_FUNC, (void*)ui_f_testpowersensors, sizeof(argv_empty)/4, argv_empty },
	{ "vin", MTYP_FUNC, (void*)ui_f_testvi, sizeof(argv_empty)/4, argv_empty },
	{ "imu", MTYP_FUNC, (void*)ui_f_testimu, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaab = {
    "Profile", &guitop, 0, {
	{ "Sine", MTYP_FUNC, (void*)ui_f_profsine, sizeof(argv_empty)/4, argv_empty },
	{ "H-Br", MTYP_FUNC, (void*)ui_f_profhpwm, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaac = {
    "Test", &guitop, 0, {
	{ "DC", MTYP_FUNC, (void*)ui_f_testdc, sizeof(argv_empty)/4, argv_empty },
	{ "serial", MTYP_FUNC, (void*)ui_f_debser, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaad = {
    "Logging", &guitop, 0, {
	{ "enable", MTYP_FUNC, (void*)ui_f_logging, sizeof(arg_aaae)/4, arg_aaae },
	{ "disable", MTYP_FUNC, (void*)ui_f_logging, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaaf = {
    "Settings", &guitop, 0, {
	{ "volume", MTYP_FUNC, (void*)ui_f_volume, sizeof(argv_empty)/4, argv_empty },
	{ "battle short", MTYP_FUNC, (void*)ui_f_set_battle_short, sizeof(argv_empty)/4, argv_empty },
	{ "save", MTYP_FUNC, (void*)ui_f_save_all, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaag = {
    "Dazzle", &guitop, 0, {
	{ "music", MTYP_FUNC, (void*)ui_f_playsong, sizeof(arg_aaah)/4, arg_aaah },
	{}
    }
};
const struct Menu guitop = {
    "Test Menu", &guitop, 0, {
	{ "diag", MTYP_MENU, (void*)&menu_aaaa },
	{ "profile", MTYP_MENU, (void*)&menu_aaab },
	{ "test", MTYP_MENU, (void*)&menu_aaac },
	{ "logging", MTYP_MENU, (void*)&menu_aaad },
	{ "settings", MTYP_MENU, (void*)&menu_aaaf },
	{ "dazzle", MTYP_MENU, (void*)&menu_aaag },
	{ "Exit", MTYP_EXIT },
	{}
    }
};
