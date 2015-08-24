/*! \file main.c
 *  \brief Main file of SuperDerpy engine.
 *
 *  Contains basic functions shared by all views.
 */
/*
 * Copyright (c) Sebastian Krzyszkowiak <dos@dosowisko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Also, ponies.
 */
#include <stdio.h>
#include <math.h>
#include <getopt.h>
#include <locale.h>
#include <signal.h>
#include <dlfcn.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_ttf.h>
#include "utils.h"
#include "config.h"
#include "main.h"

void DrawGamestates(struct Game *game) {
	al_set_target_backbuffer(game->display);
	al_clear_to_color(al_map_rgb(0,0,0));
	struct Gamestate *tmp = game->_priv.gamestates;
	while (tmp) {
		if ((tmp->loaded) && (tmp->started)) {
			(*tmp->api.Gamestate_Draw)(game, tmp->data);
		}
		tmp = tmp->next;
	}
}

void LogicGamestates(struct Game *game) {
	struct Gamestate *tmp = game->_priv.gamestates;
	while (tmp) {
		if ((tmp->loaded) && (tmp->started) && (!tmp->paused)) {
			(*tmp->api.Gamestate_Logic)(game, tmp->data);
		}
		tmp = tmp->next;
	}
}

void EventGamestates(struct Game *game, ALLEGRO_EVENT *ev) {
	struct Gamestate *tmp = game->_priv.gamestates;
	while (tmp) {
		if ((tmp->loaded) && (tmp->started) && (!tmp->paused)) {
			(*tmp->api.Gamestate_ProcessEvent)(game, tmp->data, ev);
		}
		tmp = tmp->next;
	}
}

void derp(int sig) {
	int __attribute__((unused)) n = write(STDERR_FILENO, "Segmentation fault\nI just don't know what went wrong!\n", 54);
	abort();
}

int main(int argc, char **argv){
	signal(SIGSEGV, derp);

	srand(time(NULL));

	al_set_org_name("Super Derpy");
	al_set_app_name("Tickle Monster");

	if(!al_init()) {
		fprintf(stderr, "failed to initialize allegro!\n");
		return -1;
	}

	struct Game game;

	InitConfig(&game);

	game._priv.fps_count.frames_done = 0;
	game._priv.fps_count.fps = 0;
	game._priv.fps_count.old_time = 0;

	game._priv.font_bsod = NULL;
	game._priv.console = NULL;

	game.config.fullscreen = atoi(GetConfigOptionDefault(&game, "SuperDerpy", "fullscreen", "0"));
	game.config.music = atoi(GetConfigOptionDefault(&game, "SuperDerpy", "music", "10"));
	game.config.voice = atoi(GetConfigOptionDefault(&game, "SuperDerpy", "voice", "10"));
	game.config.fx = atoi(GetConfigOptionDefault(&game, "SuperDerpy", "fx", "10"));
	game.config.debug = atoi(GetConfigOptionDefault(&game, "SuperDerpy", "debug", "0"));
	game.config.width = atoi(GetConfigOptionDefault(&game, "SuperDerpy", "width", "1280"));
	if (game.config.width<320) game.config.width=320;
	game.config.height = atoi(GetConfigOptionDefault(&game, "SuperDerpy", "height", "720"));
	if (game.config.height<180) game.config.height=180;

	if(!al_init_image_addon()) {
		fprintf(stderr, "failed to initialize image addon!\n");
		/*al_show_native_message_box(display, "Error", "Error", "Failed to initialize al_init_image_addon!",
															 NULL, ALLEGRO_MESSAGEBOX_ERROR);*/
		return -1;
	}

	if(!al_init_acodec_addon()){
		fprintf(stderr, "failed to initialize audio codecs!\n");
		return -1;
	}

	if(!al_install_audio()){
		fprintf(stderr, "failed to initialize audio!\n");
		return -1;
	}

	if(!al_install_keyboard()){
		fprintf(stderr, "failed to initialize keyboard!\n");
		return -1;
	}

	if(!al_init_primitives_addon()){
		fprintf(stderr, "failed to initialize primitives!\n");
		return -1;
	}
	
	al_init_font_addon();

	if(!al_init_ttf_addon()){
		fprintf(stderr, "failed to initialize fonts!\n");
		return -1;
	}

	if (game.config.fullscreen) al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
	else al_set_new_display_flags(ALLEGRO_WINDOWED);
	al_set_new_display_option(ALLEGRO_VSYNC, 2-atoi(GetConfigOptionDefault(&game, "SuperDerpy", "vsync", "1")), ALLEGRO_SUGGEST);
	al_set_new_display_option(ALLEGRO_OPENGL, atoi(GetConfigOptionDefault(&game, "SuperDerpy", "opengl", "1")), ALLEGRO_SUGGEST);
	al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
	al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
#ifdef ALLEGRO_WINDOWS
	al_set_new_window_position(20, 40); // workaround nasty Windows bug with window being created off-screen
#endif

	game.display = al_create_display(game.config.width, game.config.height);
	if(!game.display) {
		fprintf(stderr, "failed to create display!\n");
		return -1;
	}

	SetupViewport(&game);

	PrintConsole(&game, "Viewport %dx%d", game.viewport.width, game.viewport.height);

	ALLEGRO_BITMAP *icon = al_load_bitmap(GetDataFilePath(&game, "icons/ticklemonster.png"));
	al_set_window_title(game.display, "Tickle Monster vs Suits");
	al_set_display_icon(game.display, icon);
	al_destroy_bitmap(icon);

	if (game.config.fullscreen) al_hide_mouse_cursor(game.display);
	al_inhibit_screensaver(true);

	al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR);

	game._priv.gamestates = NULL;

	game._priv.event_queue = al_create_event_queue();
	if(!game._priv.event_queue) {
		FatalError(&game, true, "Failed to create event queue.");
		al_destroy_display(game.display);
		return -1;
	}

	game.audio.v = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2);
	game.audio.mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
	game.audio.fx = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
	game.audio.music = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
	game.audio.voice = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
	al_attach_mixer_to_voice(game.audio.mixer, game.audio.v);
	al_attach_mixer_to_mixer(game.audio.fx, game.audio.mixer);
	al_attach_mixer_to_mixer(game.audio.music, game.audio.mixer);
	al_attach_mixer_to_mixer(game.audio.voice, game.audio.mixer);
	al_set_mixer_gain(game.audio.fx, game.config.fx/10.0);
	al_set_mixer_gain(game.audio.music, game.config.music/10.0);
	al_set_mixer_gain(game.audio.voice, game.config.voice/10.0);

	al_register_event_source(game._priv.event_queue, al_get_display_event_source(game.display));
	al_register_event_source(game._priv.event_queue, al_get_keyboard_event_source());

	game._priv.showconsole = game.config.debug;

	al_clear_to_color(al_map_rgb(0,0,0));
	game._priv.timer = al_create_timer(ALLEGRO_BPS_TO_SECS(60)); // logic timer
	if(!game._priv.timer) {
		FatalError(&game, true, "Failed to create logic timer.");
		return -1;
	}
	al_register_event_source(game._priv.event_queue, al_get_timer_event_source(game._priv.timer));

	al_flip_display();
	al_start_timer(game._priv.timer);

	setlocale(LC_NUMERIC, "C");

	game.shuttingdown = false;
	game.restart = false;

	char* gamestate = strdup("dosowisko"); // FIXME: don't hardcore gamestate

	int c;
	while ((c = getopt (argc, argv, "l:s:")) != -1)
		switch (c) {
			case 'l':
				free(gamestate);
				gamestate = strdup("levelX");
				gamestate[5] = optarg[0];
				break;
			case 's':
				free(gamestate);
				gamestate = strdup(optarg);
				break;
		}

	LoadGamestate(&game, gamestate);
	game._priv.gamestates->showLoading = false; // we have only one gamestate right now
	StartGamestate(&game, gamestate);
	free(gamestate);

	char libname[1024] = {};
	snprintf(libname, 1024, "libsuperderpy-%s-loading" LIBRARY_EXTENTION, "ticklemonster");
	void *handle = dlopen(libname, RTLD_NOW);
	if (!handle) {
		FatalError(&game, true, "Error while initializing loading screen %s", dlerror());
		exit(1);
	} else {

		#define GS_LOADINGERROR FatalError(&game, true, "Error on resolving loading symbol: %s", dlerror()); exit(1);

		if (!(game._priv.loading.Draw = dlsym(handle, "Draw"))) { GS_LOADINGERROR; }
		if (!(game._priv.loading.Load = dlsym(handle, "Load"))) { GS_LOADINGERROR; }
		if (!(game._priv.loading.Start = dlsym(handle, "Start"))) { GS_LOADINGERROR; }
		if (!(game._priv.loading.Stop = dlsym(handle, "Stop"))) { GS_LOADINGERROR; }
		if (!(game._priv.loading.Unload = dlsym(handle, "Unload"))) { GS_LOADINGERROR; }
	}

	game._priv.loading.data = (*game._priv.loading.Load)(&game);

	bool redraw = false;

	while(1) {
		ALLEGRO_EVENT ev;
		if (redraw && al_is_event_queue_empty(game._priv.event_queue)) {

			struct Gamestate *tmp = game._priv.gamestates;
			int toLoad = 0, loaded = 0;

			// FIXME: move to function
			// TODO: support dependences
			while (tmp) {
				if ((tmp->pending_start) && (tmp->started)) {
					PrintConsole(&game, "Stopping gamestate \"%s\"...", tmp->name);
					(*tmp->api.Gamestate_Stop)(&game, tmp->data);
					tmp->started = false;
					tmp->pending_start = false;
				}

				if ((tmp->pending_load) && (!tmp->loaded)) toLoad++;
				tmp=tmp->next;
			}

			tmp = game._priv.gamestates;
			// FIXME: move to function
			// TODO: support dependences

			while (tmp) {
				if ((tmp->pending_load) && (tmp->loaded)) {
					PrintConsole(&game, "Unloading gamestate \"%s\"...", tmp->name);
					al_stop_timer(game._priv.timer);
					tmp->loaded = false;
					tmp->pending_load = false;
					(*tmp->api.Gamestate_Unload)(&game, tmp->data);
					dlclose(tmp->handle);
					tmp->handle = NULL;
					al_start_timer(game._priv.timer);
				} else if ((tmp->pending_load) && (!tmp->loaded)) {
					PrintConsole(&game, "Loading gamestate \"%s\"...", tmp->name);
					al_stop_timer(game._priv.timer);
					// TODO: take proper game name
					char libname[1024];
					snprintf(libname, 1024, "libsuperderpy-%s-%s" LIBRARY_EXTENTION, "ticklemonster", tmp->name);
					tmp->handle = dlopen(libname,RTLD_NOW);
					if (!tmp->handle) {
						//PrintConsole(&game, "Error while loading gamestate \"%s\": %s", tmp->name, dlerror());
						FatalError(&game, false, "Error while loading gamestate \"%s\": %s", tmp->name, dlerror());

						tmp->pending_load = false;
						tmp->pending_start = false;
					} else {

#define GS_ERROR FatalError(&game, false, "Error on resolving gamestate symbol: %s", dlerror()); tmp->pending_load = false; tmp->pending_start = false; tmp=tmp->next; continue;

						if (!(tmp->api.Gamestate_Draw = dlsym(tmp->handle, "Gamestate_Draw"))) { GS_ERROR; }
						if (!(tmp->api.Gamestate_Logic = dlsym(tmp->handle, "Gamestate_Logic"))) { GS_ERROR; }

						if (!(tmp->api.Gamestate_Load = dlsym(tmp->handle, "Gamestate_Load"))) { GS_ERROR; }
						if (!(tmp->api.Gamestate_Start = dlsym(tmp->handle, "Gamestate_Start"))) { GS_ERROR; }
						if (!(tmp->api.Gamestate_Pause = dlsym(tmp->handle, "Gamestate_Pause"))) { GS_ERROR; }
						if (!(tmp->api.Gamestate_Resume = dlsym(tmp->handle, "Gamestate_Resume"))) { GS_ERROR; }
						if (!(tmp->api.Gamestate_Stop = dlsym(tmp->handle, "Gamestate_Stop"))) { GS_ERROR; }
						if (!(tmp->api.Gamestate_Unload = dlsym(tmp->handle, "Gamestate_Unload"))) { GS_ERROR; }

						if (!(tmp->api.Gamestate_ProcessEvent = dlsym(tmp->handle, "Gamestate_ProcessEvent"))) { GS_ERROR; }
						if (!(tmp->api.Gamestate_Reload = dlsym(tmp->handle, "Gamestate_Reload"))) { GS_ERROR; }

						if (!(tmp->api.Gamestate_ProgressCount = dlsym(tmp->handle, "Gamestate_ProgressCount"))) { GS_ERROR; }

						int p = 0;
						void progress(struct Game *game) {
							p++;
							DrawGamestates(game);
							float progress = ((p / (*(tmp->api.Gamestate_ProgressCount) ? (float)*(tmp->api.Gamestate_ProgressCount) : 1))/(float)toLoad)+(loaded/(float)toLoad);
							if (game->config.debug) PrintConsole(game, "[%s] Progress: %d% (%d/%d)", tmp->name, (int)(progress*100), p, *(tmp->api.Gamestate_ProgressCount));
							if (tmp->showLoading) (*game->_priv.loading.Draw)(game, game->_priv.loading.data, progress);
							DrawConsole(game);
							al_flip_display();
						}

						// initially draw loading screen with empty bar
						DrawGamestates(&game);
						if (tmp->showLoading) {
							(*game._priv.loading.Draw)(&game, game._priv.loading.data, 0);
						}
						DrawConsole(&game);
						al_flip_display();
						tmp->data = (*tmp->api.Gamestate_Load)(&game, &progress); // feel free to replace "progress" with empty function if you want to compile with clang
						loaded++;

						tmp->loaded = true;
						tmp->pending_load = false;
					}
					al_start_timer(game._priv.timer);
				}

				tmp=tmp->next;
			}

			bool gameActive = false;
			tmp=game._priv.gamestates;

			while (tmp) {

				if ((tmp->pending_start) && (!tmp->started) && (tmp->loaded)) {
					PrintConsole(&game, "Starting gamestate \"%s\"...", tmp->name);
					al_stop_timer(game._priv.timer);
					(*tmp->api.Gamestate_Start)(&game, tmp->data);
					al_start_timer(game._priv.timer);
					tmp->started = true;
					tmp->pending_start = false;
				}

				if ((tmp->started) || (tmp->pending_start) || (tmp->pending_load)) gameActive = true;
				tmp=tmp->next;
			}

			if (!gameActive) {
				PrintConsole(&game, "No gamestates left, exiting...");
				break;
			}

			DrawGamestates(&game);
			DrawConsole(&game);
			al_flip_display();
			redraw = false;

		} else {

			al_wait_for_event(game._priv.event_queue, &ev);

			if ((ev.type == ALLEGRO_EVENT_TIMER) && (ev.timer.source == game._priv.timer)) {
				LogicGamestates(&game);
				redraw = true;
			}
			else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
				break;
			}
			else if(ev.type == ALLEGRO_EVENT_DISPLAY_FOUND) {
				SetupViewport(&game);
			}
#ifdef ALLEGRO_MACOSX
			else if ((ev.type == ALLEGRO_EVENT_KEY_DOWN) && (ev.keyboard.keycode == 104)) { //TODO: report to upstream
#else
			else if ((ev.type == ALLEGRO_EVENT_KEY_DOWN) && (ev.keyboard.keycode == ALLEGRO_KEY_TILDE)) {
#endif
				game._priv.showconsole = !game._priv.showconsole;
			}
			else if ((ev.type == ALLEGRO_EVENT_KEY_DOWN) && (game.config.debug) && (ev.keyboard.keycode == ALLEGRO_KEY_F1)) {
				int i;
				for (i=0; i<512; i++) {
					LogicGamestates(&game);
				}
				game._priv.showconsole = true;
				PrintConsole(&game, "DEBUG: 512 frames skipped...");
			}	else if ((ev.type == ALLEGRO_EVENT_KEY_DOWN) && (game.config.debug) && (ev.keyboard.keycode == ALLEGRO_KEY_F10)) {
				double speed = ALLEGRO_BPS_TO_SECS(al_get_timer_speed(game._priv.timer)); // inverting
				speed -= 10;
				if (speed<10) speed = 10;
				al_set_timer_speed(game._priv.timer, ALLEGRO_BPS_TO_SECS(speed));
				game._priv.showconsole = true;
				PrintConsole(&game, "DEBUG: Gameplay speed: %.2fx", speed/60.0);
			}	else if ((ev.type == ALLEGRO_EVENT_KEY_DOWN) && (game.config.debug) && (ev.keyboard.keycode == ALLEGRO_KEY_F11)) {
				double speed = ALLEGRO_BPS_TO_SECS(al_get_timer_speed(game._priv.timer)); // inverting
				speed += 10;
				if (speed>600) speed = 600;
				al_set_timer_speed(game._priv.timer, ALLEGRO_BPS_TO_SECS(speed));
				game._priv.showconsole = true;
				PrintConsole(&game, "DEBUG: Gameplay speed: %.2fx", speed/60.0);
			} else if ((ev.type == ALLEGRO_EVENT_KEY_DOWN) && (game.config.debug) && (ev.keyboard.keycode == ALLEGRO_KEY_F12)) {
				ALLEGRO_PATH *path = al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH);
				char filename[255] = { };
				snprintf(filename, 255, "TickleMonster_%ld_%ld.png", time(NULL), clock());
				al_set_path_filename(path, filename);
				al_save_bitmap(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP), al_get_backbuffer(game.display));
				PrintConsole(&game, "Screenshot stored in %s", al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));
				al_destroy_path(path);
			} else {
				EventGamestates(&game, &ev);
			}
		}
	}
	game.shuttingdown = true;

	// in case of restart
	struct Gamestate *tmp = game._priv.gamestates;
	while (tmp) {
		if (tmp->started) {
			PrintConsole(&game, "Stopping gamestate \"%s\"...", tmp->name);
			(*tmp->api.Gamestate_Stop)(&game, tmp->data);
			tmp->started = false;
		}
		if (tmp->loaded) {
			PrintConsole(&game, "Unloading gamestate \"%s\"...", tmp->name);
			(*tmp->api.Gamestate_Unload)(&game, tmp->data);
			dlclose(tmp->handle);
			tmp->loaded = false;
		}
		tmp=tmp->next;
	}

	al_clear_to_color(al_map_rgb(0,0,0));
	PrintConsole(&game, "Shutting down...");
	DrawConsole(&game);
	al_flip_display();
	al_rest(0.1);
	(*game._priv.loading.Unload)(&game, game._priv.loading.data);
	al_destroy_timer(game._priv.timer);
	Console_Unload(&game);
	al_destroy_display(game.display);
	al_destroy_event_queue(game._priv.event_queue);
	al_destroy_mixer(game.audio.fx);
	al_destroy_mixer(game.audio.music);
	al_destroy_mixer(game.audio.mixer);
	al_destroy_voice(game.audio.v);
	al_uninstall_audio();
	DeinitConfig(&game);
	al_uninstall_system();
	al_shutdown_ttf_addon();
	al_shutdown_font_addon();
	if (game.restart) {
#ifdef ALLEGRO_MACOSX
		return _al_mangled_main(argc, argv);
#else
		return main(argc, argv);
#endif
	}
	return 0;
}
