/* File: cmd-rvip.c */

/*
 * RVIP additions (see ~/Games/RVIP.md):
 * - the command menu on Enter (every command, grouped like the help screen)
 * - the inventory / equipment screen with a cursor and item menus
 *
 * Every command keeps its own checks and prompts: the menu hands back the
 * command key, and item actions preselect the item for get_item().
 */

#include "angband.h"

/* Item that the next get_item() takes without asking (-1000: none) */
int item_preselect = ITEM_PRESELECT_NONE;

/* Item under the cursor in show_inven()/show_equip()/show_floor() */
int item_cursor = ITEM_PRESELECT_NONE;

/* Reopen the inventory ('i') or equipment ('e') after the next command */
char inven_reopen = 0;

/* Is a visible monster in line of sight? */
bool monster_in_view(void)
{
    int i;

    for (i = 1; i < mon_max; i++)
    {
        monster_type* m_ptr = &mon_list[i];

        if (m_ptr->r_idx && m_ptr->ml
            && player_has_los_bold(m_ptr->fy, m_ptr->fx))
            return (TRUE);
    }
    return (FALSE);
}

/*
 * A pop-up box sized to its content: one space of padding and a border
 * around the widest line.  Row 'cur' is highlighted.  Scrolls only when
 * the lines don't fit on the screen.
 */
static void rvip_box(int y, int x, cptr title, cptr* lines, int n, int cur)
{
    int wid = 0, hgt, top = 0, i, j, scr_w, scr_h;
    char buf[120];

    Term_get_size(&scr_w, &scr_h);

    if (title)
        wid = strlen(title);
    for (i = 0; i < n; i++)
        if ((int)strlen(lines[i]) > wid)
            wid = strlen(lines[i]);
    if (wid > scr_w - 4)
        wid = scr_w - 4;

    hgt = n;
    if (hgt > scr_h - 2)
        hgt = scr_h - 2;
    if (cur >= hgt)
        top = cur - hgt + 1;

    if (x + wid + 4 > scr_w)
        x = scr_w - wid - 4;
    if (x < 0)
        x = 0;
    if (y + hgt + 2 > scr_h)
        y = scr_h - hgt - 2;
    if (y < 0)
        y = 0;

    /*
     * Big tiles are two cells wide: cover whole tiles, or restoring the
     * screen leaves half-tiles of the border behind
     */
    if (use_bigtile)
    {
        if ((x - COL_MAP) & 1)
            x += (x + wid + 5 > scr_w) ? -1 : 1;
        if ((wid + 4) & 1)
            wid++;
    }

    /* Border */
    buf[0] = '+';
    for (j = 1; j <= wid + 2; j++)
        buf[j] = '-';
    buf[j++] = '+';
    buf[j] = '\0';
    Term_putstr(x, y, -1, TERM_SLATE, buf);
    Term_putstr(x, y + hgt + 1, -1, TERM_SLATE, buf);
    if (title)
        Term_putstr(x + 2, y, -1, TERM_L_WHITE, title);

    for (i = 0; i < hgt; i++)
    {
        cptr s = lines[top + i];
        byte a = ((top + i) == cur) ? TERM_L_BLUE : TERM_WHITE;

        strnfmt(buf, sizeof(buf), " %-*.*s ", wid, wid, s);
        Term_putstr(x, y + 1 + i, -1, TERM_SLATE, "|");
        Term_putstr(x + 1, y + 1 + i, -1, a, buf);
        Term_putstr(x + wid + 3, y + 1 + i, -1, TERM_SLATE, "|");
    }

    /* Cursor on the chosen line */
    Term_gotoxy(x + 1, y + 1 + cur - top);
}

/*** Command menu ***/

typedef struct
{
    int key;
    cptr name;
} rvip_cmd;

typedef struct
{
    cptr name;
    const rvip_cmd* cmds;
} rvip_group;

static const rvip_cmd cmds_move[]
    = { { ';', "Walk" }, { '.', "Run" }, { 'z', "Hold still" },
          { 'Z', "Rest" }, { 'P', "Auto-explore" },
          { '<', "Go up (walks to known stairs)" },
          { '>', "Go down (walks to known stairs)" },
          { 'X', "Exchange places" }, { 'S', "Stealth mode" },
          { 'n', "Repeat last command" }, { 0, NULL } };

static const rvip_cmd cmds_terrain[]
    = { { '/', "Alter (default action)" }, { 'T', "Tunnel" },
          { 'o', "Open" }, { 'c', "Close" }, { 'b', "Bash a door" },
          { 'D', "Disarm a trap" }, { 'g', "Pick up" }, { 0, NULL } };

static const rvip_cmd cmds_fight[]
    = { { 'f', "Fire from quiver 1" }, { 'F', "Fire from quiver 2" },
          { 't', "Throw an item" },
          { KTRL('T'), "Throw at nearest" },
          { 's', "Sing (change song)" }, { 'l', "Look / target" },
          { 0, NULL } };

static const rvip_cmd cmds_items[]
    = { { 'i', "Inventory" }, { 'e', "Equipment" }, { 'u', "Use an item" },
          { 'w', "Wear / wield" }, { 'r', "Take off" }, { 'd', "Drop" },
          { 'k', "Ignore / destroy" }, { 'x', "Examine" }, { 'E', "Eat" },
          { 'q', "Quaff a potion" }, { 'a', "Use a staff" },
          { 'p', "Play an instrument" }, { '{', "Inscribe" },
          { '-', "Fletchery" }, { '0', "Smithing" }, { 0, NULL } };

static const rvip_cmd cmds_info[]
    = { { '@', "Character sheet" }, { '\t', "Abilities" },
          { 'M', "Map of the level" }, { 'L', "Locate / scroll map" },
          { '~', "Knowledge" }, { '[', "Visible monsters" },
          { ']', "Visible objects" }, { KTRL('O'), "Previous message" },
          { KTRL('P'), "Message history" }, { '?', "Help" },
          { 'V', "Version" }, { 0, NULL } };

static const rvip_cmd cmds_game[]
    = { { 'm', "Main menu" }, { 'O', "Options" }, { '$', "Macros" },
          { '&', "Colours" }, { ':', "Write a note" },
          { ')', "Screenshot" }, { KTRL('R'), "Redraw" },
          { KTRL('E'), "Swap inven/equip windows" }, { KTRL('S'), "Save" },
          { KTRL('X'), "Save and quit" }, { 'Q', "Abort this game" },
          { 0, NULL } };

static const rvip_group cmd_groups[]
    = { { "Moving", cmds_move }, { "Doors and terrain", cmds_terrain },
          { "Fighting and songs", cmds_fight }, { "Items", cmds_items },
          { "Information", cmds_info }, { "Game", cmds_game } };

#define CMD_GROUPS ((int)N_ELEMENTS(cmd_groups))

static cptr key_name(int k)
{
    static char buf[8];

    if (k == '\t')
        return ("Tab");
    if (k < ' ')
        strnfmt(buf, sizeof(buf), "^%c", k + '@');
    else
        strnfmt(buf, sizeof(buf), "%c", k);
    return (buf);
}

/* Is k one of the menu's command keys? */
static bool menu_key(int k)
{
    int g, i;

    for (g = 0; g < CMD_GROUPS; g++)
        for (i = 0; cmd_groups[g].cmds[i].name; i++)
            if (cmd_groups[g].cmds[i].key == k)
                return (TRUE);
    return (FALSE);
}

/* Commands of one group; returns the key, 0 for "back", -1 for "close" */
static int cmd_menu_group(int g, int y, int x)
{
    const rvip_cmd* c = cmd_groups[g].cmds;
    char text[24][60];
    cptr lines[24];
    int n, cur = 0;

    for (n = 0; c[n].name; n++)
    {
        strnfmt(text[n], sizeof(text[n]), "%-3s %s", key_name(c[n].key),
            c[n].name);
        lines[n] = text[n];
    }

    while (1)
    {
        char ch;

        rvip_box(y, x, cmd_groups[g].name, lines, n, cur);
        Term_fresh();
        ch = inkey();

        if ((ch == ESCAPE) || (ch == '4'))
            return (ch == ESCAPE ? -1 : 0);
        if (ch == '8')
            cur = (cur + n - 1) % n;
        else if (ch == '2')
            cur = (cur + 1) % n;
        else if ((ch == '\r') || (ch == '\n') || (ch == ' ') || (ch == '5')
            || (ch == '6'))
            return (c[cur].key);
        else if (menu_key(ch))
            return (ch);
    }
}

/*
 * The command menu (Enter).  Returns the chosen command key, or 0.
 */
int do_cmd_command_menu(void)
{
    static int cur = 0;
    cptr lines[CMD_GROUPS];
    int g, key = 0;

    for (g = 0; g < CMD_GROUPS; g++)
        lines[g] = cmd_groups[g].name;

    screen_save();

    while (1)
    {
        char ch;

        screen_load();
        screen_save();
        rvip_box(1, COL_MAP, "Commands", lines, CMD_GROUPS, cur);
        Term_fresh();
        ch = inkey();

        if ((ch == ESCAPE) || (ch == '4'))
            break;
        if (ch == '8')
            cur = (cur + CMD_GROUPS - 1) % CMD_GROUPS;
        else if (ch == '2')
            cur = (cur + 1) % CMD_GROUPS;
        else if ((ch == '\r') || (ch == '\n') || (ch == ' ') || (ch == '5')
            || (ch == '6'))
        {
            /* Group box to the right of the group list */
            key = cmd_menu_group(cur, 1 + cur,
                COL_MAP + (int)strlen("Fighting and songs") + 5);
            if (key)
                break;
        }
        else if (menu_key(ch))
        {
            key = ch;
            break;
        }
    }

    screen_load();

    return ((key > 0) ? key : 0);
}

/*** Inventory with a cursor ***/

typedef struct
{
    int key;
    cptr name;
} rvip_action;

/* Main action of an item: its usual command key */
static int item_main_action(int item)
{
    object_type* o_ptr = &inventory[item];

    if (item >= INVEN_WIELD)
        return ('r');

    switch (o_ptr->tval)
    {
    case TV_FOOD:
        return ('E');
    case TV_POTION:
        return ('q');
    case TV_STAFF:
        return ('a');
    case TV_HORN:
        return ('p');
    case TV_FLASK:
        return ('u');
    }

    if (wield_slot(o_ptr) >= INVEN_WIELD)
        return ('w');

    return ('x');
}

static cptr action_name(int key)
{
    switch (key)
    {
    case 'E':
        return ("Eat");
    case 'q':
        return ("Quaff");
    case 'a':
        return ("Use staff");
    case 'p':
        return ("Play");
    case 'u':
        return ("Use");
    case 'w':
        return ("Wear / wield");
    case 'r':
        return ("Take off");
    case 't':
        return ("Throw");
    case 'd':
        return ("Drop");
    case 'k':
        return ("Ignore / destroy");
    case '{':
        return ("Inscribe");
    case 'x':
        return ("Examine");
    }
    return ("?");
}

/* Every action that fits the item */
static int item_actions(int item, int* keys)
{
    int n = 0, main_key = item_main_action(item);

    if (main_key != 'x')
        keys[n++] = main_key;
    if ((item < INVEN_WIELD) && (main_key == 'x'))
        keys[n++] = 'u';
    if (item < INVEN_WIELD)
        keys[n++] = 't';
    keys[n++] = 'd';
    keys[n++] = 'k';
    keys[n++] = '{';
    keys[n++] = 'x';
    return (n);
}

/* Run one item command on a given item */
static void item_do(int key, int item)
{
    item_preselect = item;

    switch (key)
    {
    case 'E':
        do_cmd_eat_food(NULL, 0);
        break;
    case 'q':
        do_cmd_quaff_potion(NULL, 0);
        break;
    case 'a':
        do_cmd_activate_staff(NULL, 0);
        break;
    case 'p':
        do_cmd_play_instrument(NULL, 0);
        break;
    case 'u':
        do_cmd_use_item();
        break;
    case 'w':
        do_cmd_wield(NULL, 0);
        break;
    case 'r':
        do_cmd_takeoff(NULL, 0);
        break;
    case 't':
        do_cmd_throw(FALSE);
        break;
    case 'd':
        do_cmd_drop();
        break;
    case 'k':
        do_cmd_destroy();
        break;
    case '{':
        do_cmd_inscribe();
        break;
    case 'x':
        do_cmd_observe();
        break;
    }

    item_preselect = ITEM_PRESELECT_NONE;
}

/* Item menu: returns the chosen action key, or 0 */
static int item_menu(int item, int y)
{
    int keys[16], n = item_actions(item, keys), cur = 0, i;
    char text[16][40];
    cptr lines[16];
    char o_name[80];

    object_desc(o_name, sizeof(o_name), &inventory[item], TRUE, 0);
    o_name[30] = '\0';

    for (i = 0; i < n; i++)
    {
        strnfmt(text[i], sizeof(text[i]), "%c  %s", keys[i],
            action_name(keys[i]));
        lines[i] = text[i];
    }

    while (1)
    {
        char ch;

        rvip_box(y, COL_MAP, o_name, lines, n, cur);
        Term_fresh();
        ch = inkey();

        if ((ch == ESCAPE) || (ch == '4') || (ch == '0') || (ch == '.'))
            return (0);
        if (ch == '8')
            cur = (cur + n - 1) % n;
        else if (ch == '2')
            cur = (cur + 1) % n;
        else if ((ch == '\r') || (ch == '\n') || (ch == ' ') || (ch == '5')
            || (ch == '6'))
            return (keys[cur]);
        else
            for (i = 0; i < n; i++)
                if (keys[i] == ch)
                    return (ch);
    }
}

/* Next valid slot in the list, moving by dir (+1/-1) */
static int inven_next(int cur, int dir, bool equip)
{
    int lo = equip ? INVEN_WIELD : 0;
    int hi = equip ? INVEN_TOTAL : INVEN_PACK;
    int i, n = hi - lo;

    for (i = 1; i <= n; i++)
    {
        int k = lo + (((cur - lo) + dir * i) % n + n) % n;

        if (inventory[k].k_idx)
            return (k);
    }
    return (inventory[cur].k_idx ? cur : -1);
}

/*
 * 'i' / 'e': the list with a cursor.  Letter = main action, Shift+letter
 * drops, Ctrl+letter examines, Enter/Space/5 opens the item menu; 8/2
 * move, 4/6 switch lists, + main action, - drop, * examine, 0 . Esc close.
 * Any other key is a normal command.
 */
void do_cmd_inven_screen(bool equip)
{
    static int cur_inven = 0, cur_equip = INVEN_WIELD;
    int cur, key = 0, item = -1;
    char ch;

    inven_reopen = 0;

    screen_save();

    while (1)
    {
        cur = equip ? cur_equip : cur_inven;
        if (!inventory[cur].k_idx)
            cur = inven_next(cur, 1, equip);

        screen_load();
        screen_save();

        item_tester_full = TRUE;
        item_cursor = cur;
        if (equip)
            show_equip();
        else
            show_inven();
        item_cursor = ITEM_PRESELECT_NONE;
        item_tester_full = FALSE;

        prt(equip ? "Equipment: letter use, Shift drop, Ctrl examine, "
                    "Enter menu, 4/6 switch"
                  : "Inventory: letter use, Shift drop, Ctrl examine, "
                    "Enter menu, 4/6 switch",
            0, 0);
        Term_fresh();

        ch = inkey();

        if ((ch == ESCAPE) || (ch == '0') || (ch == '.'))
            break;

        if ((ch == '8') || (ch == '2'))
        {
            if (cur >= 0)
                cur = inven_next(cur, (ch == '8') ? -1 : 1, equip);
        }
        else if ((ch == '4') || (ch == '6') || (ch == '/'))
            equip = !equip;
        else if ((ch == '\r') || (ch == '\n') || (ch == ' ') || (ch == '5'))
        {
            if (cur >= 0)
            {
                /* Just below the chosen line */
                int y = 2 + (equip ? cur - INVEN_WIELD : cur);

                key = item_menu(cur, y);
                if (key)
                {
                    item = cur;
                    break;
                }
            }
        }
        else if ((ch == '+') || (ch == '-') || (ch == '*'))
        {
            if (cur >= 0)
            {
                item = cur;
                key = (ch == '+') ? item_main_action(cur)
                    : (ch == '-') ? 'd' : 'x';
                break;
            }
        }
        else if (islower((unsigned char)ch) || isupper((unsigned char)ch)
            || ((ch >= 1) && (ch <= 26) && (ch != '\t')))
        {
            int c = (ch <= 26) ? ch + 'a' - 1 : tolower((unsigned char)ch);
            int k = equip ? label_to_equip(c) : label_to_inven(c);

            if ((k >= 0) && inventory[k].k_idx)
            {
                item = k;
                key = (ch <= 26) ? 'x'
                    : isupper((unsigned char)ch) ? 'd' : item_main_action(k);
                break;
            }
            else
                bell("No such item.");
        }
        else
        {
            /* Any other key: a normal command */
            p_ptr->command_new = ch;
            break;
        }

        if (equip)
            cur_equip = cur;
        else
            cur_inven = cur;
    }

    if (cur >= 0)
    {
        if (equip)
            cur_equip = cur;
        else
            cur_inven = cur;
    }

    screen_load();

    if (key && (item >= 0))
    {
        item_do(key, item);

        /* Reopen the list after the action, unless a monster is in view */
        inven_reopen = equip ? 'e' : 'i';
    }
}
