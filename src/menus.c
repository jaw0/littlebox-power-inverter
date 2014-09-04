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
const struct Menu menu_aaae;
const struct Menu menu_aaaf;
const char * arg_aaag[] = { "-ui",  "6", "13"  };
const char * argv_empty[] = { "-ui" };

const struct Menu menu_aaaa = {
    "Demo", &guitop, 0, {
	{ "wander", MTYP_FUNC, (void*)ui_f_wander, sizeof(argv_empty)/4, argv_empty },
	{ "dance", MTYP_FUNC, (void*)ui_f_dance, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaab = {
    "Diag Menu", &guitop, 0, {
	{ "battery", MTYP_FUNC, (void*)ui_f_power, sizeof(argv_empty)/4, argv_empty },
	{ "sensors", MTYP_FUNC, (void*)ui_f_testsensors, sizeof(argv_empty)/4, argv_empty },
	{ "imu", MTYP_FUNC, (void*)ui_f_testimu, sizeof(argv_empty)/4, argv_empty },
	{ "encoders", MTYP_FUNC, (void*)ui_f_testencoders, sizeof(argv_empty)/4, argv_empty },
	{ "motors", MTYP_FUNC, (void*)ui_f_testmotors, sizeof(argv_empty)/4, argv_empty },
	{ "motors/jam", MTYP_FUNC, (void*)ui_f_testforwrev, sizeof(argv_empty)/4, argv_empty },
	{ "distance", MTYP_FUNC, (void*)ui_f_testdistance, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaac = {
    "Profile", &guitop, 0, {
	{ "encoders", MTYP_FUNC, (void*)ui_f_prof_encoders, sizeof(argv_empty)/4, argv_empty },
	{ "distance", MTYP_FUNC, (void*)ui_f_distance_profile, sizeof(argv_empty)/4, argv_empty },
	{ "spin", MTYP_FUNC, (void*)ui_f_prof_gyro, sizeof(argv_empty)/4, argv_empty },
	{ "spin2", MTYP_FUNC, (void*)ui_f_prof_gyro2, sizeof(argv_empty)/4, argv_empty },
	{ "go", MTYP_FUNC, (void*)ui_f_prof_accx, sizeof(argv_empty)/4, argv_empty },
	{ "sencal", MTYP_FUNC, (void*)ui_f_sensor_calibrate, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaad = {
    "Test", &guitop, 0, {
	{ "path", MTYP_FUNC, (void*)ui_f_testpath, sizeof(argv_empty)/4, argv_empty },
	{ "straight", MTYP_FUNC, (void*)ui_f_testgo, sizeof(argv_empty)/4, argv_empty },
	{ "turn", MTYP_FUNC, (void*)ui_f_testturn, sizeof(argv_empty)/4, argv_empty },
	{ "spin", MTYP_FUNC, (void*)ui_f_testspin, sizeof(argv_empty)/4, argv_empty },
	{ "search", MTYP_FUNC, (void*)ui_f_search, sizeof(argv_empty)/4, argv_empty },
	{ "dance", MTYP_FUNC, (void*)ui_f_testdance, sizeof(argv_empty)/4, argv_empty },
	{ "botstop", MTYP_FUNC, (void*)ui_f_testbotstop, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaae = {
    "Settings", &guitop, 0, {
	{ "volume", MTYP_FUNC, (void*)ui_f_volume, sizeof(argv_empty)/4, argv_empty },
	{ "testspeed", MTYP_FUNC, (void*)ui_f_settestspeed, sizeof(argv_empty)/4, argv_empty },
	{ "save", MTYP_FUNC, (void*)ui_f_updateboot, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu menu_aaaf = {
    "Dazzle", &guitop, 0, {
	{ "music", MTYP_FUNC, (void*)ui_f_playsong, sizeof(arg_aaag)/4, arg_aaag },
	{ "disco", MTYP_FUNC, (void*)ui_f_disco, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu guitop = {
    "Main Menu", &guitop, 0, {
	{ "demo", MTYP_MENU, (void*)&menu_aaaa },
	{ "diag", MTYP_MENU, (void*)&menu_aaab },
	{ "profile", MTYP_MENU, (void*)&menu_aaac },
	{ "tests", MTYP_MENU, (void*)&menu_aaad },
	{ "settings", MTYP_MENU, (void*)&menu_aaae },
	{ "dazzle", MTYP_MENU, (void*)&menu_aaaf },
	{}
    }
};
