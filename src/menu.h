/*
  Copyright (c) 2013
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2013-Apr-18 21:42 (EDT)
  Function: menu system
*/

struct MenuOption {
    const char *text;
    long	type;
#  define MTYP_DONE	0
#  define MTYP_FUNC	1
#  define MTYP_MENU	2
#  define MTYP_EXIT	3

    const void	*action;	// func or menu
    int		argc;
    const char  **argv;
};


struct Menu {
    const char *title;
    const struct Menu *prev;
    int *startval;
    struct MenuOption el[];
};

extern void menu(const struct Menu *);


