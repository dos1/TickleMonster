/*! \file dosowisko.c
 *  \brief Init animation with dosowisko.net logo.
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
 */

#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <math.h>
#include "../utils.h"
#include "../timeline.h"
#include "dosowisko.h"

int Gamestate_ProgressCount = 5;

static char* text = "# dosowisko.net";

bool FadeIn(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct dosowiskoResources *data = action->arguments->value;
	if (state == TM_ACTIONSTATE_START) {
		data->fade=0;
	}
	else if (state == TM_ACTIONSTATE_DESTROY) {
		data->fade=255;
	}
	else if (state == TM_ACTIONSTATE_RUNNING) {
		data->fade+=2;
		data->tan++;
		return data->fade >= 255;
	}
	return false;
}

bool FadeOut(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct dosowiskoResources *data = action->arguments->value;
	if (state == TM_ACTIONSTATE_START) {
		data->fadeout = true;
	}
	return true;
}

bool End(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	if (state == TM_ACTIONSTATE_RUNNING) SwitchGamestate(game, "dosowisko", "menu");
	return true;
}

bool Play(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	ALLEGRO_SAMPLE_INSTANCE *data = action->arguments->value;
	if (state == TM_ACTIONSTATE_RUNNING) al_play_sample_instance(data);
	return true;
}

bool Type(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct dosowiskoResources *data = action->arguments->value;
	if (state == TM_ACTIONSTATE_RUNNING) {
		strncpy(data->text, text, data->pos++);
		data->text[data->pos] = 0;
		if (strcmp(data->text, text) != 0) {
			TM_AddBackgroundAction(data->timeline, Type, TM_AddToArgs(NULL, 1, data), 60 + rand() % 60, "type");
		} else{
			al_stop_sample_instance(data->kbd);
		}
		return true;
	}
	return false;
}


void Gamestate_Logic(struct Game *game, struct dosowiskoResources* data) {
	TM_Process(data->timeline);
	data->tick++;
	if (data->tick == 30) {
		data->underscore = !data->underscore;
		data->tick = 0;
	}
}

void Gamestate_Draw(struct Game *game, struct dosowiskoResources* data) {

	if (!data->fadeout) {

		char t[255] = "";
		strcpy(t, data->text);
		if (data->underscore) {
			strncat(t, "_", 1);
		} else {
			strncat(t, " ", 1);
		}

		al_set_target_bitmap(data->bitmap);
		al_clear_to_color(al_map_rgba(0,0,0,0));

		al_draw_text(data->font, al_map_rgba(255,255,255,10), game->viewport.width/2, game->viewport.height*0.4167, ALLEGRO_ALIGN_CENTRE, t);

		double tg = tan(-data->tan/384.0 * ALLEGRO_PI - ALLEGRO_PI/2);

		int fade = data->fadeout ? 255 : data->fade;

		al_set_target_bitmap(data->pixelator);
		al_clear_to_color(al_map_rgb(35, 31, 32));

		al_draw_tinted_scaled_bitmap(data->bitmap, al_map_rgba(fade, fade, fade, fade), 0, 0, al_get_bitmap_width(data->bitmap), al_get_bitmap_height(data->bitmap), -tg*al_get_bitmap_width(data->bitmap)*0.05, -tg*al_get_bitmap_height(data->bitmap)*0.05, al_get_bitmap_width(data->bitmap)+tg*0.1*al_get_bitmap_width(data->bitmap), al_get_bitmap_height(data->bitmap)+tg*0.1*al_get_bitmap_height(data->bitmap), 0);

		al_draw_bitmap(data->checkerboard, 0, 0, 0);

		al_set_target_backbuffer(game->display);

		al_draw_bitmap(data->pixelator, 0, 0, 0);

	}
}

void Gamestate_Start(struct Game *game, struct dosowiskoResources* data) {
	data->pos = 1;
	data->fade = 0;
	data->tan = 64;
	data->tick = 0;
	data->fadeout = false;
	data->underscore=true;
	strcpy(data->text, "#");
	TM_AddDelay(data->timeline, 300);
	TM_AddQueuedBackgroundAction(data->timeline, FadeIn, TM_AddToArgs(NULL, 1, data), 0, "fadein");
	TM_AddDelay(data->timeline, 1500);
	TM_AddAction(data->timeline, Play, TM_AddToArgs(NULL, 1, data->kbd), "playkbd");
	TM_AddQueuedBackgroundAction(data->timeline, Type, TM_AddToArgs(NULL, 1, data), 0, "type");
	TM_AddDelay(data->timeline, 3200);
	TM_AddAction(data->timeline, Play, TM_AddToArgs(NULL, 1, data->key), "playkey");
	TM_AddDelay(data->timeline, 50);
	TM_AddAction(data->timeline, FadeOut, TM_AddToArgs(NULL, 1, data), "fadeout");
	TM_AddDelay(data->timeline, 1000);
	TM_AddAction(data->timeline, End, NULL, "end");
	al_play_sample_instance(data->sound);
}

void Gamestate_ProcessEvent(struct Game *game, struct dosowiskoResources* data, ALLEGRO_EVENT *ev) {
	TM_HandleEvent(data->timeline, ev);
	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		SwitchGamestate(game, "dosowisko", "menu");
	}
}

void* Gamestate_Load(struct Game *game, void (*progress)(struct Game*)) {
	struct dosowiskoResources *data = malloc(sizeof(struct dosowiskoResources));
	data->timeline = TM_Init(game, "main");
	data->bitmap = al_create_bitmap(game->viewport.width, game->viewport.height);
	data->checkerboard = al_create_bitmap(game->viewport.width, game->viewport.height);
	data->pixelator = al_create_bitmap(game->viewport.width, game->viewport.height);

	al_set_target_bitmap(data->checkerboard);
	al_lock_bitmap(data->checkerboard, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
	int x, y;
	for (x = 0; x < al_get_bitmap_width(data->checkerboard); x=x+2) {
		for (y = 0; y < al_get_bitmap_height(data->checkerboard); y=y+2) {
			al_put_pixel(x, y, al_map_rgba(0,0,0,64));
			al_put_pixel(x+1, y, al_map_rgba(0,0,0,0));
			al_put_pixel(x, y+1, al_map_rgba(0,0,0,0));
			al_put_pixel(x+1, y+1, al_map_rgba(0,0,0,0));
		}
	}
	al_unlock_bitmap(data->checkerboard);
	al_set_target_backbuffer(game->display);
	(*progress)(game);

	data->font = al_load_ttf_font(GetDataFilePath(game, "fonts/DejaVuSansMono.ttf"),game->viewport.height*0.1666,0 );
	(*progress)(game);
	data->sample = al_load_sample( GetDataFilePath(game, "dosowisko.flac") );
	data->sound = al_create_sample_instance(data->sample);
	al_attach_sample_instance_to_mixer(data->sound, game->audio.music);
	al_set_sample_instance_playmode(data->sound, ALLEGRO_PLAYMODE_ONCE);
	(*progress)(game);

	data->kbd_sample = al_load_sample( GetDataFilePath(game, "kbd.flac") );
	data->kbd = al_create_sample_instance(data->kbd_sample);
	al_attach_sample_instance_to_mixer(data->kbd, game->audio.fx);
	al_set_sample_instance_playmode(data->kbd, ALLEGRO_PLAYMODE_ONCE);
	(*progress)(game);

	data->key_sample = al_load_sample( GetDataFilePath(game, "key.flac") );
	data->key = al_create_sample_instance(data->key_sample);
	al_attach_sample_instance_to_mixer(data->key, game->audio.fx);
	al_set_sample_instance_playmode(data->key, ALLEGRO_PLAYMODE_ONCE);
	(*progress)(game);


	return data;
}

void Gamestate_Stop(struct Game *game, struct dosowiskoResources* data) {
	al_stop_sample_instance(data->sound);
	al_stop_sample_instance(data->kbd);
	al_stop_sample_instance(data->key);
}

void Gamestate_Unload(struct Game *game, struct dosowiskoResources* data) {
	al_destroy_font(data->font);
	al_destroy_sample_instance(data->sound);
	al_destroy_sample(data->sample);
	al_destroy_sample_instance(data->kbd);
	al_destroy_sample(data->kbd_sample);
	al_destroy_sample_instance(data->key);
	al_destroy_sample(data->key_sample);
	al_destroy_bitmap(data->bitmap);
	al_destroy_bitmap(data->checkerboard);
	al_destroy_bitmap(data->pixelator);
	TM_Destroy(data->timeline);
	free(data);
}

void Gamestate_Reload(struct Game *game, struct dosowiskoResources* data) {}

void Gamestate_Resume(struct Game *game, struct dosowiskoResources* data) {}
void Gamestate_Pause(struct Game *game, struct dosowiskoResources* data) {}
