#include <allegro.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <allegro/keyboard.h>

#define CELL_SIZE   	10  	// size of a cell (must divide field height and width)

#include "multimedia.h"

#define BG_PATH             "img/solarsysbg.bmp"
#define ICON_IDLE			"img/icon/idle.bmp"
#define ICON_ADD_planet 	"img/icon/addplanet.bmp"
#define ICON_KILL_planet	"img/icon/killplanet.bmp"
#define ICON_EXIT        	"img/icon/exit.bmp"

#define SUN_PATH		"img/sun.bmp"

#define MERCURY_PATH	"img/mercury.bmp"
#define VENUS_PATH	    "img/venus.bmp"
#define EARTH_PATH		"img/earth.bmp"
#define MARS_PATH		"img/mars.bmp"
#define JUPITER_PATH	"img/jupiter.bmp"
#define SATURN_PATH		"img/saturn.bmp"
#define URANUS_PATH		"img/uranus.bmp"
#define PLUTO_PATH		"img/pluto.bmp"
#define NEPTUNE_PATH	"img/neptune.bmp"


pthread_cond_t terminate = PTHREAD_COND_INITIALIZER;
pthread_mutex_t terminate_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t action_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t selected_planet_mtx = PTHREAD_MUTEX_INITIALIZER;

unsigned int graphics_tid, keyboard_tid, mouse_tid;     // threads IDs

BITMAP* surface;
BITMAP* fieldbmp;
//BITMAP *planetbmp;

BITMAP *sunbmp;
BITMAP *mercurybmp;
BITMAP *venusbmp;
BITMAP *earthbmp;
BITMAP *marsbmp;
BITMAP *jupiterbmp;
BITMAP *saturnbmp;
BITMAP *uranusbmp;
BITMAP *plutobmp;
BITMAP *neptunebmp;




typedef enum action {IDLE, ADD_planet, KILL_planet, EXIT, N_ACTIONS} action;

action current_action = IDLE;

char* action_keybind[N_ACTIONS] = {
    "Q",
    "E",
    "R",
    "ESC"
};

char* action_desc[N_ACTIONS] = {
    "Select",
    "Add planet",
    " Remove planet",
    "Quit"
};

int selected_planet = -1;

char current_message[80];

typedef struct icon {
    int x;
    int y;
    BITMAP* bmp;
} icon;

char* icon_bmp_paths[N_ACTIONS] = {
    ICON_IDLE,
    ICON_ADD_planet,
    ICON_KILL_planet,
    ICON_EXIT
};

icon icons[N_ACTIONS];


/* ======================================
*  ============== UTILITY ===============
*  ====================================== */



/* Get the selected planet index atomically */
int get_selected(void) {

    int sel_planet;
    pthread_mutex_lock(&selected_planet_mtx);
    sel_planet = selected_planet;
    pthread_mutex_unlock(&selected_planet_mtx);
    return sel_planet;
}


/* Set the selected planet index atomically */
void set_selected(int id) {

    pthread_mutex_lock(&selected_planet_mtx);
    selected_planet = id;
    pthread_mutex_unlock(&selected_planet_mtx);
}


/* Get the current action atomically */
action get_action(void) {

    action a;
    pthread_mutex_lock(&action_mtx);
    a = current_action;
    pthread_mutex_unlock(&action_mtx);
    return a;
}


/* Set the action atomically */
void set_action(action a) {

    pthread_mutex_lock(&action_mtx);
    current_action = a;
    pthread_mutex_unlock(&action_mtx);
}



/* ======================================
*  ============== GRAPHICS ==============
*  ====================================== */


void init_icons() {

    int left_padding = (SPACE_WIDTH - (2 * N_ACTIONS - 1) * ICON_SIZE) / 2;

    for (int i = 0; i < N_ACTIONS; i++) {
        icons[i].x = left_padding + 2 * i * ICON_SIZE - ICON_SIZE / 2;
        icons[i].y = SPACE_HEIGHT + TOOLBAR_H / 2 - ICON_SIZE / 2;
        icons[i].bmp = load_bitmap(icon_bmp_paths[i], NULL);
    }
}

unsigned int init_graphics() {

	if (allegro_init()!=0)
		return 1;

    set_color_depth(32);
    if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, SPACE_WIDTH + STATS_PANEL_W,
                SPACE_HEIGHT + TOOLBAR_H , 0, 0))
        return 2;

    surface = create_bitmap(SCREEN_W, SCREEN_H);
    fieldbmp = load_bitmap(BG_PATH, NULL);
    //planetbmp = load_bitmap(planet_PATH, NULL);
    sunbmp = load_bitmap(SUN_PATH, NULL);
    mercurybmp = load_bitmap(MERCURY_PATH, NULL);
    venusbmp = load_bitmap(VENUS_PATH, NULL);
    earthbmp = load_bitmap(EARTH_PATH, NULL);
    marsbmp = load_bitmap(MARS_PATH, NULL);
    jupiterbmp = load_bitmap(JUPITER_PATH, NULL);
    saturnbmp = load_bitmap(SATURN_PATH, NULL);
    uranusbmp = load_bitmap(URANUS_PATH, NULL);
    plutobmp = load_bitmap(PLUTO_PATH, NULL);
    neptunebmp = load_bitmap(NEPTUNE_PATH, NULL);
    init_icons();
    snprintf(current_message, 80, "%s", "Left-click on a planet to view info about it");

    clear_to_color(surface, COLOR_GREEN);
    blit(surface, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

    return 0;
}


void clear_field() {

    blit(fieldbmp, surface, 0, 0, 0, 0, fieldbmp->w, fieldbmp->h);//TO DO...
}



void draw_toolbar(void) {

    int x0 = 0;
    int y0 = SPACE_HEIGHT;

    action a = get_action();

    // Draw background and border
    rectfill(surface, 0, SPACE_HEIGHT, SPACE_WIDTH, SCREEN_H - 1, COLOR_TOOLBAR_BORDER);
    rectfill(surface, 2, SPACE_HEIGHT + 2, SPACE_WIDTH - 2, SCREEN_H - 3, COLOR_TOOLBAR);

    // Display toolbar elements
    for (int i = 0; i < N_ACTIONS; i++) {
        // Draw icon
        if (a == i) {
            rectfill(surface, icons[i].x - 3, icons[i].y - 3, 
                    icons[i].x + ICON_SIZE + 3, icons[i].y + ICON_SIZE + 3, COLOR_ICON_BORDER);
            rectfill(surface, icons[i].x, icons[i].y, 
                    icons[i].x + ICON_SIZE, icons[i].y + ICON_SIZE, COLOR_TOOLBAR);
        } else {
            rect(surface, icons[i].x - 1, icons[i].y - 1, 
                    icons[i].x + ICON_SIZE + 1, icons[i].y + ICON_SIZE + 1, COLOR_ICON_BORDER);
        }
        draw_sprite(surface, icons[i].bmp, icons[i].x, icons[i].y);

        // Draw keybind
        textout_centre_ex(surface, font, action_keybind[i], 
            icons[i].x + ICON_SIZE / 2, icons[i].y - 12, COLOR_TEXT, COLOR_TOOLBAR);

        // Draw description
        textout_centre_ex(surface, font, action_desc[i], 
            icons[i].x + ICON_SIZE / 2, icons[i].y + ICON_SIZE + 8, COLOR_TEXT, COLOR_TOOLBAR);
    }

    // Print the current message
    textout_centre_ex(surface, font, current_message, 
            x0 + SPACE_WIDTH / 2, y0 + 7, COLOR_TEXT, COLOR_TOOLBAR);
}


/* Draw the status information and statistics about the currently selected planet.
*  x0 and y0 are the coordinate of the upper-left corner of the panel  */
void draw_selected_planet_stats(double x0, double y0) {

    char buf[60];
    int sel_planet;
    unsigned int sel_tid;
    position sel_pos;


    hline(surface, SPACE_WIDTH, STATS_PLANET_OFF_H, SCREEN_W - 1, COLOR_TOOLBAR_BORDER);
    textout_centre_ex(surface, font, "planet",
            x0 + (SCREEN_W - SPACE_WIDTH) / 2, STATS_PLANET_OFF_H + 10, COLOR_TEXT, COLOR_STATS_PANEL);

    sel_planet = get_selected();
    if (sel_planet >= 0){
    	switch(sel_planet){
    	case 0:
    		sprintf(buf, "%-16s %s", "Selected: ", "Mercury");
    		break;
    	case 1:
			sprintf(buf, "%-16s %s", "Selected: ", "Venus");
			break;
    	case 2:
			sprintf(buf, "%-16s %s", "Selected: ", "Earth");
			break;
		case 3:
			sprintf(buf, "%-16s %s", "Selected: ", "Mars");
			break;
		case 4:
			sprintf(buf, "%-16s %s", "Selected: ", "Jupiter");
			break;
		case 5:
			sprintf(buf, "%-16s %s", "Selected: ", "Saturn");
			break;
		case 6:
			sprintf(buf, "%-16s %s", "Selected: ", "Uranus");
			break;
		case 7:
			sprintf(buf, "%-16s %s", "Selected: ", "Neptune");

			break;
		case 8:
			sprintf(buf, "%-16s %s", "Selected: ", "Pluto");
			break;
    	}
    }
    else
    	sprintf(buf, "%-16s %s", "Selected: ", "none");
    textout_ex(surface, font, buf, x0 + 10, STATS_PLANET_OFF_H + 40, COLOR_TEXT, COLOR_STATS_PANEL);

    if (sel_planet >= 0 && sel_planet < MAX_PLANETS) {
        pthread_mutex_lock(&planets[sel_planet].mtx);

        if (!planets[sel_planet].alive) {
            pthread_mutex_unlock(&planets[sel_planet].mtx);
            set_selected(-1);
            return;
        }
            
        sel_tid = planets[sel_planet].tid;
        sel_pos = planets[sel_planet].pos;
        pthread_mutex_unlock(&planets[sel_planet].mtx);
    } else
        return;

    sprintf(buf, "%-12s (%.2f %.2f)","Pos (x,y):", sel_pos.x,sel_pos.y);
    textout_ex(surface, font, buf, x0 + 10, STATS_PLANET_OFF_H + 60, COLOR_TEXT, COLOR_STATS_PANEL);
    sprintf(buf, "%-12s %.2f", "Angle (°):",fmod((360 / TWO_PI * sel_pos.angle),360));
    textout_ex(surface, font, buf, x0 + 10, STATS_PLANET_OFF_H + 80, COLOR_TEXT, COLOR_STATS_PANEL);

    textout_centre_ex(surface, font, "Thread Execution Time Stats:",
                x0 + (SCREEN_W - SPACE_WIDTH) / 2, STATS_PLANET_OFF_H + 110, COLOR_TEXT, COLOR_STATS_PANEL);
    sprintf(buf, "%-16s %d", "Exec.Phase:", get_exec_phase(sel_tid));
    textout_ex(surface, font, buf, x0 + 10, STATS_PLANET_OFF_H + 130, COLOR_TEXT, COLOR_STATS_PANEL);
    sprintf(buf, "%-16s %d", "Deadline misses:", num_of_dl_misses(sel_tid));
    textout_ex(surface, font, buf, x0 + 10, STATS_PLANET_OFF_H + 150, COLOR_TEXT, COLOR_STATS_PANEL);
    sprintf(buf, "%-16s %lld", "Min.Exec Time(us):", get_min_exec_time(sel_tid)/NS_PER_US);
    textout_ex(surface, font, buf, x0 + 10, STATS_PLANET_OFF_H + 170, COLOR_TEXT, COLOR_STATS_PANEL);
    sprintf(buf, "%-16s %lld", "Max.exec Time(us):", get_max_exec_time(sel_tid)/NS_PER_US);
    textout_ex(surface, font, buf, x0 + 10, STATS_PLANET_OFF_H + 190, COLOR_TEXT, COLOR_STATS_PANEL);
    sprintf(buf, "%-16s %.2f", "Avg.Exec Time(us):", get_avg_exec_time(sel_tid)/NS_PER_US);
    textout_ex(surface, font, buf, x0 + 10, STATS_PLANET_OFF_H + 210, COLOR_TEXT, COLOR_STATS_PANEL);



}

/* Draw the status and statistics panel */
void draw_stats_panelbox(void) {

    int x0 = SPACE_WIDTH;
    int y0 = 0;
    char buf[60];

    // Draw background and border
    rectfill(surface, SPACE_WIDTH, 0, SCREEN_W - 1, SCREEN_H - 1, COLOR_STATS_BORDER);
    rectfill(surface, SPACE_WIDTH + 2, 2, SCREEN_W - 3, SCREEN_H - 3, COLOR_STATS_PANEL);

    textout_centre_ex(surface, font, "STATUS", 
            x0 + (SCREEN_W - SPACE_WIDTH) / 2, y0 + 10, COLOR_TEXT, COLOR_STATS_PANEL);

    // Print stats
    sprintf(buf, "%-16s %d", "planets:", n_planets);
    textout_ex(surface, font, buf, x0 + 10, y0 + 40, COLOR_TEXT, COLOR_STATS_PANEL);
    sprintf(buf, "%-16s", "Deadline misses");
    textout_ex(surface, font, buf, x0 + 10, y0 + 80, COLOR_TEXT, COLOR_STATS_PANEL);
    sprintf(buf, "%-16s %d", " - Graphics:", num_of_dl_misses(graphics_tid));
    textout_ex(surface, font, buf, x0 + 10, y0 + 100, COLOR_TEXT, COLOR_STATS_PANEL);
    sprintf(buf, "%-16s %d", " - Mouse:", num_of_dl_misses(mouse_tid));
    textout_ex(surface, font, buf, x0 + 10, y0 + 120, COLOR_TEXT, COLOR_STATS_PANEL);
    sprintf(buf, "%-16s %d", " - Keyboard:", num_of_dl_misses(keyboard_tid));
    textout_ex(surface, font, buf, x0 + 10, y0 + 140, COLOR_TEXT, COLOR_STATS_PANEL);

    draw_selected_planet_stats(x0, y0);
}

/*Draw The Sun*/
static inline void draw_sun(){
    draw_sprite(surface, sunbmp, SPACE_WIDTH/2-baseSize/2, SPACE_HEIGHT/2-baseSize/2);
}

/*Draw Orbits*/

static inline void draw_orbits(){
	int col=makecol(166,94,27);//Golden Brown
	for (int i = 0; i <MAX_PLANETS ; i++) {
		ellipse(surface,SPACE_WIDTH/2, SPACE_HEIGHT/2, orbits[i].diametrx/2, orbits[i].diametry/2,col);
	}
}

static inline void draw_p(BITMAP *b,planet *p,bool selected){
	draw_sprite(surface, b, (SPACE_WIDTH/2-(p->size)/2 + p->pos.x),
							(SPACE_HEIGHT/2 -(p->size)/2 + p->pos.y));
	if (selected)
		circle(surface, (SPACE_WIDTH/2 + p->pos.x),
				(SPACE_HEIGHT/2 + p->pos.y), 15, COLOR_RED);//if selected draw red circle around the planet
}
/* Draw the planet */
static inline void draw_planet(int i, bool selected) {

    planet *p = &planets[i];
    pthread_mutex_lock(&p->mtx);
    int planet = i;
    if (p->alive) {
    	switch(planet){
    	case 0:
    		if (mercurybmp != NULL)
    			draw_p(mercurybmp,p,selected);
    		else
    			circlefill(surface, (SPACE_WIDTH/2 + p->pos.x),
    					(SPACE_HEIGHT/2 + p->pos.y),CELL_SIZE/2,COLOR_RED);// fallback figure
    		break;
    	case 1:
    		if (venusbmp != NULL)
    			draw_p(venusbmp,p,selected);
    		else
				circlefill(surface, (SPACE_WIDTH/2 + p->pos.x),
						(SPACE_HEIGHT/2 + p->pos.y),CELL_SIZE/2,COLOR_RED);
    		break;
    	case 2:
			if (earthbmp != NULL)
    			draw_p(earthbmp,p,selected);
			else
				circlefill(surface, (SPACE_WIDTH/2 + p->pos.x),
						(SPACE_HEIGHT/2 + p->pos.y),CELL_SIZE/2,COLOR_RED);
			break;
    	case 3:
			if (marsbmp != NULL)
    			draw_p(marsbmp,p,selected);
			else
				circlefill(surface, (SPACE_WIDTH/2 + p->pos.x),
						(SPACE_HEIGHT/2 + p->pos.y),CELL_SIZE/2,COLOR_RED);
			break;
    	case 4:
			if (jupiterbmp != NULL)
    			draw_p(jupiterbmp,p,selected);
			else
				circlefill(surface, (SPACE_WIDTH/2 + p->pos.x),
						(SPACE_HEIGHT/2 + p->pos.y),CELL_SIZE/2,COLOR_RED);
			break;
    	case 5:
			if (saturnbmp != NULL)
    			draw_p(saturnbmp,p,selected);
			else
				circlefill(surface, (SPACE_WIDTH/2 + p->pos.x),
						(SPACE_HEIGHT/2 + p->pos.y),CELL_SIZE/2,COLOR_RED);
			break;
    	case 6:
			if (uranusbmp != NULL)
    			draw_p(uranusbmp,p,selected);
			else
				circlefill(surface, (SPACE_WIDTH/2 + p->pos.x),
						(SPACE_HEIGHT/2 + p->pos.y),CELL_SIZE/2,COLOR_RED);
			break;
    	case 7:
			if (neptunebmp != NULL)
    			draw_p(neptunebmp,p,selected);
			else
				circlefill(surface, (SPACE_WIDTH/2 + p->pos.x),
						(SPACE_HEIGHT/2 + p->pos.y),CELL_SIZE/2,COLOR_RED);
			break;
    	case 8:
			if (plutobmp != NULL)
				draw_p(plutobmp,p,selected);
			else
				circlefill(surface, (SPACE_WIDTH/2 + p->pos.x),
						(SPACE_HEIGHT/2 + p->pos.y),CELL_SIZE/2,COLOR_RED);
			break;

    }
}
    pthread_mutex_unlock(&p->mtx);

}



/* Graphic task routine */
void *graphics_behaviour(void *arg) {

    clear_field();

    //draw The Sun
    draw_sun();

    // Draw orbits
    draw_orbits();

    // Draw planets
    int sel_planet = get_selected();
    for (int i = 0; i < MAX_PLANETS; ++i)
        draw_planet(i, (i == sel_planet));

    // Draw toolbar and stats
    draw_toolbar();
    draw_stats_panelbox();
    
    blit(surface, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    show_mouse(screen);
}


/* Start the graphics thread */
unsigned int start_graphics(void) {

    unsigned int ret;

    ret = start_thread(graphics_behaviour, NULL, SCHED_FIFO,
            PRD_GRAPHICS, DL_GRAPHICS, PRIO_GRAPHICS);
    if (ret < 0) {
        printf("Failed to initialize the graphics thread!\n");
        return 1;
    } else {
        graphics_tid = ret;
        printf("Initialized the graphics thread with id #%d.\n", graphics_tid);
    }

    return 0;
}


/* Gracefully stop the graphics thread */
void stop_graphics(void) {

    stop_thread(graphics_tid);
    printf("Graphics thread stopped.\n");
}



/* ======================================
*  ============== KEYBOARD ==============
*  ====================================== */


/* Get ASCII code and scan code of the pressed key */
void get_keycodes(char *scan, char *ascii) {

    int k;
    k = readkey();  // read the key
    *ascii = k;     // extract ascii code
    *scan = k >> 8; // extract scan code
}


/* Keyboard thread routine */
void *keyboard_behaviour(void *arg) {

    int key;
    char ascii, scan;
    int ret;

    // Execute the actions corresponding to the pressed keys
    while (keypressed()) {
    	action a = get_action();
        get_keycodes(&scan, &ascii);
        switch (scan) {
        case KEY_ESC:
        	set_action(EXIT);
			snprintf(current_message, 80, "%s", "Closing...");
			pthread_mutex_lock(&terminate_mtx);
			pthread_cond_signal(&terminate);
			pthread_mutex_unlock(&terminate_mtx);
			break;
		case KEY_SPACE:
			printf("Pressed spacebar\n");
			break;
		case KEY_Q:
			set_action(IDLE);
			snprintf(current_message, 80, "%s", "Left-click on a planet to view info about it");
			break;
		case KEY_E:
			set_action(ADD_planet);
			if (spawn_planet() == 0)
				snprintf(current_message, 80, "%s", "planet spawned!");
			else
				snprintf(current_message, 80, "%s", "Failed to spawn a new planet (too many)");
			break;
		case KEY_R:
			set_action(KILL_planet);
			snprintf(current_message, 80, "%s", "Left-click on the planet to kill");
			break;
		default:
			printf("Press ESC to quit!\n");
        }
    }
}


/* Start the keyboard thread */
unsigned int start_keyboard(void) {

    unsigned int ret;

    ret = start_thread(keyboard_behaviour, NULL, SCHED_FIFO,
            PRD_KEYBOARD, DL_KEYBOARD, PRIO_KEYBOARD);
    if (ret < 0) {
        printf("Failed to initialize the keyboard thread!\n");
        return 1;
    } else {
        keyboard_tid = ret;
        printf("Initialized the keyboard thread with id #%d.\n", keyboard_tid);
    }
    return 0;
}


/* Gracefully stop the keyboard thread */
void stop_keyboard(void) {

    stop_thread(keyboard_tid);
    printf("Keyboard thread stopped.\n");
}



/* ======================================
*  =============== MOUSE ================
*  ====================================== */


/* Mouse task routine */
void *mouse_behaviour(void *arg) {

    int x, y;
    int mbutton;

    // Check if any button is being clicked, otherwise return
    mbutton = mouse_b & 3;
    if (mbutton) {
        x = mouse_x;
        y = mouse_y;
    } else
        return NULL;

    action a = get_action();

    switch(mbutton) {
        int planet_id, ret;
        case 1:     // Left-click
            switch(a) {

                case KILL_planet:
                    planet_id = get_planet_id_by_orbit_pos(x, y);
                    if (planet_id >= 0 && kill_planet(planet_id) > 0) {
                        snprintf(current_message, 80, "%s", "Killed planet!");
                        set_action(IDLE);
                    }
                    break;
                case IDLE:
                    planet_id = get_planet_id_by_orbit_pos(x, y);
                    if (planet_id >= 0) {
                        set_selected(planet_id);
                    } else
                        set_selected(-1);
                    break;
                default:
                    break;
            }
            break;
        case 2:     // Right-click
            /* <Actions performed with right click (if any) go here> */
            break;
        case 3:     // Both clicks
            /* <Actions performed with both clicks (if any) go here> */
            break;
        default:
            break;
    }
}


/* Start the mouse thread */
unsigned int start_mouse(void) {

    unsigned int ret;

    ret = start_thread(mouse_behaviour, NULL, SCHED_FIFO,
            PRD_MOUSE, DL_MOUSE, PRIO_MOUSE);
    if (ret < 0) {
        printf("Failed to initialize the mouse thread!\n");
        return 1;
    } else {
        mouse_tid = ret;
        printf("Initialized the mouse thread with id #%d.\n", mouse_tid);
    }

    return 0;
}


/* Gracefully stop the mouse thread */
void stop_mouse(void) {

    stop_thread(mouse_tid);
    printf("Mouse thread stopped.\n");
}


/* Initialize multimedia (graphics, mouse, keyboard) data structures */
unsigned int init_multimedia() {

    if (init_graphics())
        return 1;

    install_mouse();
    install_keyboard();

    if (start_graphics() || start_mouse() || start_keyboard())
    //if (start_graphics())
        return 1;

    return 0;
}


/* Stop all multimedia threads */
void stop_multimedia() {

    stop_keyboard();
    stop_mouse();
    stop_graphics();
}


/* Wait until the application should close (i.e. user pressed ESC) */
void wait_for_termination(void) {

    pthread_mutex_lock(&terminate_mtx);
    pthread_cond_wait(&terminate, &terminate_mtx);
    pthread_mutex_unlock(&terminate_mtx);
}
