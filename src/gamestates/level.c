/*! \file menu.c
 *  \brief Main Menu view.
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
#include <stdio.h>
#include <math.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include "../config.h"
#include "../utils.h"
#include "../timeline.h"
#include "level.h"

#define TILE_SIZE 20

int Gamestate_ProgressCount = 6;

void AnimateBadguys(struct Game *game, struct LevelResources *data, int i) {
	struct Kid *tmp = data->kids[i];
	while (tmp) {
		AnimateCharacter(game, tmp->character, tmp->melting ? 1 : tmp->speed * data->kidSpeed);
		tmp=tmp->next;
	}
}

void MoveBadguys(struct Game *game, struct LevelResources *data, int i, float dx) {
	struct Kid *tmp = data->kids[i];
	while (tmp) {

		if (!tmp->character->spritesheet->kill) {
			MoveCharacter(game, tmp->character, dx * tmp->speed * data->kidSpeed, 0, 0);
		}

		if (tmp->character->dead) {
			if (tmp->prev) {
				tmp->prev->next = tmp->next;
				if (tmp->next) tmp->next->prev = tmp->prev;
			} else {
				data->kids[i] = tmp->next;
				if (tmp->next) tmp->next->prev = NULL;
			}
			struct Kid *old = tmp;
			tmp = tmp->next;
			old->character->dead = true;
			old->prev = NULL;
			old->next = data->destroyQueue;
			if (data->destroyQueue) data->destroyQueue->prev = old;
			data->destroyQueue = old;
		} else {
			tmp = tmp->next;
		}

	}
}

void CheckForEnd(struct Game *game, struct LevelResources *data) {
	return false;

	int i;
	bool lost = false;
	for (i=0; i<6; i++) {
		struct Kid *tmp = data->kids[i];
		while (tmp) {
			if (tmp->character->x <= (139-(i*10))-10) {
				lost = true;
				break;
			}
			tmp=tmp->next;
		}
		if (lost) break;
	}

	if (lost) {

		data->soloactive=false;
		data->soloanim=0;
		data->soloflash=0;
		data->soloready=0;

		SelectSpritesheet(game, data->monster, "cry");
	}
}

void DrawBadguys(struct Game *game, struct LevelResources *data, int i) {
	struct Kid *tmp = data->kids[i];
	while (tmp) {
		if (tmp->character->x > 20) {
			DrawCharacter(game, tmp->character, al_map_rgb(255,255,255), 0);
		}
		tmp=tmp->next;
	}
}

void Gamestate_Draw(struct Game *game, struct LevelResources* data) {

	al_set_target_bitmap(al_get_backbuffer(game->display));

	al_clear_to_color(al_map_rgb(3, 213, 255));

	al_draw_bitmap(data->bg,0, 0,0);

	DrawCharacter(game, data->suit, al_map_rgb(255,255,255), 0);

	DrawCharacter(game, data->monster, al_map_rgb(255,255,255), 0);

	DrawBadguys(game, data, 0);
	DrawBadguys(game, data, 1);
	DrawBadguys(game, data, 2);
	DrawBadguys(game, data, 3);
	DrawBadguys(game, data, 4);
	DrawBadguys(game, data, 5);

	al_draw_bitmap(data->buildings,0, 0,0);

	if (data->soloflash) {
		al_draw_filled_rectangle(0, 0, 320, 180, al_map_rgb(255,255,255));
	}
}

void AddBadguy(struct Game *game, struct LevelResources* data, int i) {
	struct Kid *n = malloc(sizeof(struct Kid));
	n->next = NULL;
	n->prev = NULL;
	n->speed = (rand() % 3) * 0.25 + 1;
	n->melting = false;
	n->character = CreateCharacter(game, "kid");
	n->character->spritesheets = data->kid->spritesheets;
	n->character->shared = true;
	SelectSpritesheet(game, n->character, "walk");
	SetCharacterPosition(game, n->character, 280, 20+(i*TILE_SIZE), 0);

	if (data->kids[i]) {
		struct Kid *tmp = data->kids[i];
		while (tmp->next) {
			tmp=tmp->next;
		}
		tmp->next = n;
		n->prev = tmp;
	} else {
		data->kids[i] = n;
	}
}

void Fire(struct Game *game, struct LevelResources *data) {

	if (data->tickling) {
		SelectSpritesheet(game, data->monster, "stand");
		MoveCharacter(game, data->monster, 2, -2, 0);
		data->tickling = false;
		return;
	}

	SelectSpritesheet(game, data->monster, "tickle");
	MoveCharacter(game, data->monster, -2, 2, 0);

	data->tickling = true;

	PrintConsole(game, "playing chord");

	struct Kid *tmp = data->kids[data->marky];
	while (tmp) {
		if (!tmp->melting) {
			if ((data->markx >= tmp->character->x - 9) && (data->markx <= tmp->character->x + 1)) {
				data->score += 100 * tmp->speed;
				SelectSpritesheet(game, tmp->character, "melt");
				data->soloready++;
				tmp->melting = true;
			}
		}
		tmp=tmp->next;
	}
}

void Gamestate_Logic(struct Game *game, struct LevelResources* data) {

	if (data->keys.lastkey == data->keys.key) {
		data->keys.delay = data->keys.lastdelay; // workaround for random bugus UP/DOWN events
	}

	if (data->moveup && data->monster->y < 14) {
		data->moveup = false;
	}
	if (data->movedown && data->monster->y > 112) {
		data->movedown = false;
	}

	if (data->moveup) {
		MoveCharacter(game, data->monster, 0, -1, 0);
	} else if (data->movedown) {
		MoveCharacter(game, data->monster, 0, 1, 0);
	}

	if ((int)(data->monster->y + 7) % TILE_SIZE == 0) {
		data->moveup = false;
		data->movedown = false;
	}

	data->cloud_position-=0.1;
	if (data->cloud_position<-40) { data->cloud_position=100; PrintConsole(game, "cloud_position"); }
	AnimateCharacter(game, data->monster, 1);
	AnimateCharacter(game, data->suit, 1);


		if ((data->keys.key) && (data->keys.delay < 3)) {

			if (data->keys.key==ALLEGRO_KEY_LEFT) {
				MoveCharacter(game, data->monster, -1, 0, 0);
			}

			if (data->keys.key==ALLEGRO_KEY_RIGHT) {
				MoveCharacter(game, data->monster, 1, 0, 0);
			}

			if (data->keys.delay == INT_MIN) data->keys.delay = 4;
			else data->keys.delay += 4;

		} else if (data->keys.key) {
			data->keys.delay-=3;
		}

		AnimateBadguys(game, data, 0);
		AnimateBadguys(game, data, 1);
		AnimateBadguys(game, data, 2);
		AnimateBadguys(game, data, 3);
		AnimateBadguys(game, data, 4);
		AnimateBadguys(game, data, 5);

		MoveBadguys(game, data, 0, -0.17);
		MoveBadguys(game, data, 1, -0.18);
		MoveBadguys(game, data, 2, -0.19);
		MoveBadguys(game, data, 3, -0.2);
		MoveBadguys(game, data, 4, -0.21);
		MoveBadguys(game, data, 5, -0.22);

		data->timeTillNextBadguy--;
		if (data->timeTillNextBadguy <= 0) {
			data->timeTillNextBadguy = data->kidRate;
			data->kidRate -= data->kidRate * 0.02;
			if (data->kidRate < 20) {
				data->kidRate = 20;
			}

			data->kidSpeed+= 0.001;
			AddBadguy(game, data, rand() % 6);
		}

		if (data->usage) { data->usage--; }
		if (data->lightanim) { data->lightanim++;}
		if (data->lightanim > 25) { data->lightanim = 0; }

		CheckForEnd(game, data);

	data->soloanim++;
	if (data->soloanim >= 60) data->soloanim=0;

	if (data->soloflash) data->soloflash--;

	data->keys.lastkey = data->keys.key;
	data->keys.lastdelay = data->keys.delay;

	TM_Process(data->timeline);
}

void* Gamestate_Load(struct Game *game, void (*progress)(struct Game*)) {

	struct LevelResources *data = malloc(sizeof(struct LevelResources));

	data->timeline = TM_Init(game, "main");
	(*progress)(game);

	data->bg = al_load_bitmap( GetDataFilePath(game, "bg2.png") );
	data->buildings = al_load_bitmap( GetDataFilePath(game, "buildings.png") );
	data->click_sample = al_load_sample( GetDataFilePath(game, "click.flac") );
	(*progress)(game);

	data->click = al_create_sample_instance(data->click_sample);
	al_attach_sample_instance_to_mixer(data->click, game->audio.fx);
	al_set_sample_instance_playmode(data->click, ALLEGRO_PLAYMODE_ONCE);
	(*progress)(game);


	data->font_title = al_load_ttf_font(GetDataFilePath(game, "fonts/MonkeyIsland.ttf"),game->viewport.height*0.16,0 );
	data->font = al_load_ttf_font(GetDataFilePath(game, "fonts/MonkeyIsland.ttf"),game->viewport.height*0.05,0 );
	(*progress)(game);

	data->monster = CreateCharacter(game, "monster");
	RegisterSpritesheet(game, data->monster, "stand");
	RegisterSpritesheet(game, data->monster, "tickle");
	RegisterSpritesheet(game, data->monster, "jump");
	LoadSpritesheets(game, data->monster);
	(*progress)(game);

	data->suit = CreateCharacter(game, "suit");
	RegisterSpritesheet(game, data->suit, "stand");
	LoadSpritesheets(game, data->suit);
	(*progress)(game);

	data->kid = CreateCharacter(game, "kid");
	RegisterSpritesheet(game, data->kid, "walk");
	RegisterSpritesheet(game, data->kid, "laugh");
	LoadSpritesheets(game, data->kid);

	al_set_target_backbuffer(game->display);
	return data;
}

void DestroyBadguys(struct Game *game, struct LevelResources* data, int i) {
	struct Kid *tmp = data->kids[i];
	if (!tmp) {
		tmp = data->destroyQueue;
		data->destroyQueue = NULL;
	}
	while (tmp) {
		DestroyCharacter(game, tmp->character);
		struct Kid *old = tmp;
		tmp = tmp->next;
		free(old);
		if ((!tmp) && (data->destroyQueue)) {
			tmp = data->destroyQueue;
			data->destroyQueue = NULL;
		}
	}
	data->kids[i] = NULL;
}

void Gamestate_Stop(struct Game *game, struct LevelResources* data) {
	int i;
	for (i=0; i<6; i++) {
		DestroyBadguys(game, data, i);
	}
}

void Gamestate_Unload(struct Game *game, struct LevelResources* data) {
	al_destroy_bitmap(data->bg);
	al_destroy_bitmap(data->buildings);
	al_destroy_font(data->font_title);
	al_destroy_font(data->font);
	//al_destroy_sample_instance(data->music);
	al_destroy_sample_instance(data->click);
	//al_destroy_sample(data->sample);
	al_destroy_sample(data->click_sample);
	DestroyCharacter(game, data->monster);
	DestroyCharacter(game, data->suit);
	DestroyCharacter(game, data->kid);
	TM_Destroy(data->timeline);
}

// TODO: refactor to single Enqueue_Anim
bool Anim_CowLook(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct LevelResources *data = action->arguments->value;
	if (state == TM_ACTIONSTATE_START) {
		ChangeSpritesheet(game, data->suit, "look");
		TM_AddQueuedBackgroundAction(data->timeline, &Anim_CowLook, TM_AddToArgs(NULL, 1, data), 54*1000, "cow_look");
	}
	return true;
}

bool Anim_FixGuitar(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct LevelResources *data = action->arguments->value;
	if (state == TM_ACTIONSTATE_START) {
		ChangeSpritesheet(game, data->monster, "fix");
		TM_AddQueuedBackgroundAction(data->timeline, &Anim_FixGuitar, TM_AddToArgs(NULL, 1, data), 30*1000, "fix_guitar");
	}
	return true;
}

void StartGame(struct Game *game, struct LevelResources *data) {
	TM_CleanQueue(data->timeline);
	TM_CleanBackgroundQueue(data->timeline);
	ChangeSpritesheet(game, data->monster, "stand");
	ChangeSpritesheet(game, data->suit, "stand");
 }

void Gamestate_Start(struct Game *game, struct LevelResources* data) {
	data->cloud_position = 100;
	SetCharacterPosition(game, data->monster, 180, 107, 0);
	SetCharacterPosition(game, data->suit, 65, 88, 0);

	data->score = 0;

	data->tickling = false;

	data->markx = 119;
	data->marky = 2;

	data->soloactive = false;
	data->soloanim = 0;
	data->soloflash = 0;
	data->soloready = 0;

	data->keys.key = 0;
	data->keys.delay = 0;
	data->keys.shift = false;
	data->keys.lastkey = -1;

	data->lightanim=0;

	data->kidSpeed = 1.2;

	data->usage = 0;

	SelectSpritesheet(game, data->monster, "stand");
	SelectSpritesheet(game, data->suit, "stand");
	//TM_AddQueuedBackgroundAction(data->timeline, &Anim_FixGuitar, TM_AddToArgs(NULL, 1, data), 15*1000, "fix_guitar");
	//TM_AddQueuedBackgroundAction(data->timeline, &Anim_CowLook, TM_AddToArgs(NULL, 1, data), 5*1000, "cow_look");

	data->kids[0] = NULL;
	data->kids[1] = NULL;
	data->kids[2] = NULL;
	data->kids[3] = NULL;
	data->kids[4] = NULL;
	data->kids[5] = NULL;
	data->destroyQueue = NULL;

	data->kidRate = 100;
	data->timeTillNextBadguy = 0;
}

void Gamestate_ProcessEvent(struct Game *game, struct LevelResources* data, ALLEGRO_EVENT *ev) {
	TM_HandleEvent(data->timeline, ev);


		if (ev->type == ALLEGRO_EVENT_KEY_DOWN) {

			switch (ev->keyboard.keycode) {
				case ALLEGRO_KEY_LEFT:
				case ALLEGRO_KEY_RIGHT:
					if (data->keys.key != ev->keyboard.keycode) {
						data->keys.key = ev->keyboard.keycode;
						data->keys.delay = INT_MIN;
					}
					break;
				case ALLEGRO_KEY_UP:
					if (!data->moveup && !data->movedown) {
						SelectSpritesheet(game, data->monster, "jump");
					}
					data->moveup = true;
					data->movedown = false;
					break;
				case ALLEGRO_KEY_DOWN:
					if (!data->moveup && !data->movedown) {
						SelectSpritesheet(game, data->monster, "jump");
					}
					data->moveup = false;
					data->movedown = true;
					break;
				case ALLEGRO_KEY_SPACE:
					Fire(game, data);
					break;
				case ALLEGRO_KEY_ESCAPE:
					SwitchGamestate(game, "level", "menu");
					break;
				case ALLEGRO_KEY_LSHIFT:
				case ALLEGRO_KEY_RSHIFT:
					data->keys.shift = true;
					break;
				case ALLEGRO_KEY_ENTER:
					break;
				default:
					data->keys.key = 0;
					break;
			}
		} else if (ev->type == ALLEGRO_EVENT_KEY_UP) {
			switch (ev->keyboard.keycode) {
				case ALLEGRO_KEY_LSHIFT:
				case ALLEGRO_KEY_RSHIFT:
					data->keys.shift = false;
					break;
				default:
					if (ev->keyboard.keycode == data->keys.key) {
						data->keys.key = 0;
					}
					break;
			}
		}


}

void Gamestate_Pause(struct Game *game, struct LevelResources* data) {
	TM_Pause(data->timeline);
}
void Gamestate_Resume(struct Game *game, struct LevelResources* data) {
	TM_Resume(data->timeline);
}
void Gamestate_Reload(struct Game *game, struct LevelResources* data) {}
