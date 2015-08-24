/*! \file loading.c
 *  \brief Loading screen.
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
#include <allegro5/allegro_primitives.h>
#include "../utils.h"
#include "loading.h"

void Progress(struct Game *game, struct LoadingResources *data, float p) {
	al_set_target_bitmap(al_get_backbuffer(game->display));
	al_draw_bitmap(data->loading_bitmap,0,0,0);
	al_draw_filled_rectangle(0, game->viewport.height/2 - 1, p*game->viewport.width, game->viewport.height/2 + 1, al_map_rgba(128,128,128,128));
}

void Draw(struct Game *game, struct LoadingResources *data, float p) {
	al_draw_bitmap(data->loading_bitmap,0,0,0);
	Progress(game, data, p);
}

void* Load(struct Game *game) {
	struct LoadingResources *data = malloc(sizeof(struct LoadingResources));
	al_clear_to_color(al_map_rgb(0,0,0));

	data->loading_bitmap = al_create_bitmap(game->viewport.width, game->viewport.height);
	data->bg = al_load_bitmap( GetDataFilePath(game, "bg.png") );

	al_set_target_bitmap(data->loading_bitmap);
	al_clear_to_color(al_map_rgb(0,0,0));
	al_draw_bitmap(data->bg, 0, 0, 0);
	al_draw_filled_rectangle(0, game->viewport.height/2 - 1, game->viewport.width, game->viewport.height/2 + 1, al_map_rgba(32,32,32,32));
	al_set_target_bitmap(al_get_backbuffer(game->display));
	return data;
}

void Start(struct Game *game, struct LoadingResources *data) {}
void Stop(struct Game *game, struct LoadingResources *data) {}
void Unload(struct Game *game, struct LoadingResources *data) {
	al_destroy_bitmap(data->loading_bitmap);
	al_destroy_bitmap(data->bg);
	free(data);
}
