/*
	Copyright (c) 2017-2020 ByteBit

	This file is part of BetterSpades.

	BetterSpades is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	BetterSpades is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with BetterSpades.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>

#include "lodepng/lodepng.h"
#include "log.h"
#include "main.h"
#include "file.h"
#include "common.h"
#include "list.h"
#include "matrix.h"
#include "texture.h"
#include "hud.h"
#include "http.h"
#include "parson.h"
#include "config.h"
#include "network.h"
#include "rpc.h"
#include "map.h"
#include "player.h"
#include "camera.h"
#include "cameracontroller.h"
#include "ping.h"
#include "chunk.h"
#include "utils.h"
#include "weapon.h"
#include "tracer.h"
#include "font.h"
#include "soundtest.h"

struct hud hud_mapload;

struct hud* hud_active;
struct window_instance* hud_window;

static int is_inside_centered(double mx, double my, int x, int y, int w, int h) {
	return mx >= x - w / 2 && mx < x + w / 2 && my >= y - h / 2 && my < y + h / 2;
}

static int is_inside(double mx, double my, int x, int y, int w, int h) {
	return mx >= x && mx < x + w && my >= y && my < y + h;
}

void hud_init() {
	hud_change(&hud_serverlist);
}

void hud_change(struct hud* new) {
	config_key_reset_togglestates();
	hud_active = new;
	if(hud_active->init)
		hud_active->init();
}

/*          HUD_INGAME START           */

static float hud_ingame_touch_x = 0.0F;
static float hud_ingame_touch_y = 0.0F;

int screen_current = SCREEN_NONE;
int show_exit = 0;
static void hud_ingame_init() {
	window_textinput(0);
	chat_input_mode = CHAT_NO_INPUT;
	show_exit = 0;
	window_mousemode(WINDOW_CURSOR_DISABLED);
}

struct player_table {
	unsigned char id;
	unsigned int score;
};

static int playertable_sort(const void* a, const void* b) {
	struct player_table* aa = (struct player_table*)a;
	struct player_table* bb = (struct player_table*)b;
	return bb->score - aa->score;
}

static void hud_ingame_render3D() {
	glDepthRange(0.0F, 0.05F);

	matrix_select(matrix_projection);
	matrix_identity();
	matrix_perspective(camera_fov, ((float)settings.window_width) / ((float)settings.window_height), 0.1F, 128.0F);
	matrix_select(matrix_view);
	matrix_identity();
	matrix_upload_p();
	matrix_select(matrix_model);

	if(!network_map_transfer) {
		if(camera_mode == CAMERAMODE_FPS && players[local_player_id].items_show) {
			players[local_player_id].input.buttons.rmb = 0;

			matrix_identity();
			matrix_translate(-2.25F, -1.5F - (players[local_player_id].held_item == TOOL_SPADE) * 0.5F, -6.0F);
			matrix_rotate(window_time() * 57.4F, 0.0F, 1.0F, 0.0F);
			matrix_translate((model_spade.xpiv - model_spade.xsiz / 2) * 0.05F,
							 (model_spade.zpiv - model_spade.zsiz / 2) * 0.05F,
							 (model_spade.ypiv - model_spade.ysiz / 2) * 0.05F);
			if(players[local_player_id].held_item == TOOL_SPADE) {
				matrix_scale(1.5F, 1.5F, 1.5F);
			}
			matrix_upload();
			kv6_render(&model_spade, players[local_player_id].team);

			if(local_player_blocks > 0) {
				matrix_identity();
				matrix_translate(-2.25F, -1.5F - (players[local_player_id].held_item == TOOL_BLOCK) * 0.5F, -6.0F);
				matrix_translate(1.5F, 0.0F, 0.0F);
				matrix_rotate(window_time() * 57.4F, 0.0F, 1.0F, 0.0F);
				matrix_translate((model_block.xpiv - model_block.xsiz / 2) * 0.05F,
								 (model_block.zpiv - model_block.zsiz / 2) * 0.05F,
								 (model_block.ypiv - model_block.ysiz / 2) * 0.05F);
				if(players[local_player_id].held_item == TOOL_BLOCK) {
					matrix_scale(1.5F, 1.5F, 1.5F);
				}
				model_block.red = players[local_player_id].block.red / 255.0F;
				model_block.green = players[local_player_id].block.green / 255.0F;
				model_block.blue = players[local_player_id].block.blue / 255.0F;
				matrix_upload();
				kv6_render(&model_block, players[local_player_id].team);
			}

			if(local_player_ammo + local_player_ammo_reserved > 0) {
				struct kv6_t* gun;
				switch(players[local_player_id].weapon) {
					default:
					case WEAPON_RIFLE: gun = &model_semi; break;
					case WEAPON_SMG: gun = &model_smg; break;
					case WEAPON_SHOTGUN: gun = &model_shotgun; break;
				}
				matrix_identity();
				matrix_translate(-2.25F, -1.5F - (players[local_player_id].held_item == TOOL_GUN) * 0.5F, -6.0F);
				matrix_translate(3.0F, 0.0F, 0.0F);
				matrix_rotate(window_time() * 57.4F, 0.0F, 1.0F, 0.0F);
				matrix_translate((gun->xpiv - gun->xsiz / 2) * 0.05F, (gun->zpiv - gun->zsiz / 2) * 0.05F,
								 (gun->ypiv - gun->ysiz / 2) * 0.05F);
				if(players[local_player_id].held_item == TOOL_GUN) {
					matrix_scale(1.5F, 1.5F, 1.5F);
				}
				matrix_upload();
				kv6_render(gun, players[local_player_id].team);
			}

			if(local_player_grenades > 0) {
				matrix_identity();
				matrix_translate(-2.25F, -1.5F - (players[local_player_id].held_item == TOOL_GRENADE) * 0.5F, -6.0F);
				matrix_translate(4.5F, 0.0F, 0.0F);
				matrix_rotate(window_time() * 57.4F, 0.0F, 1.0F, 0.0F);
				matrix_translate((model_grenade.xpiv - model_grenade.xsiz / 2) * 0.05F,
								 (model_grenade.zpiv - model_grenade.zsiz / 2) * 0.05F,
								 (model_grenade.ypiv - model_grenade.ysiz / 2) * 0.05F);
				if(players[local_player_id].held_item == TOOL_GRENADE) {
					matrix_scale(1.5F, 1.5F, 1.5F);
				}
				matrix_upload();
				kv6_render(&model_grenade, players[local_player_id].team);
			}
		}

		if(screen_current == SCREEN_TEAM_SELECT) {
			matrix_identity();
			matrix_translate(-1.4F, -2.0F, -3.0F);
			matrix_rotate(-90.0F + 22.5F, 0.0F, 1.0F, 0.0F);
			matrix_upload();
			struct Player p_hud;
			memset(&p_hud, 0, sizeof(struct Player));
			p_hud.spade_use_timer = FLT_MAX;
			p_hud.input.keys.packed = 0;
			p_hud.held_item = TOOL_SPADE;
			p_hud.input.buttons.packed = 0;
			p_hud.physics.eye.x = p_hud.pos.x = 0;
			p_hud.physics.eye.y = p_hud.pos.y = 0;
			p_hud.physics.eye.z = p_hud.pos.z = 0;
			p_hud.physics.velocity.x = 0.0F;
			p_hud.physics.velocity.y = 0.0F;
			p_hud.physics.velocity.z = 0.0F;
			p_hud.orientation.x = p_hud.orientation_smooth.x = 1.0F;
			p_hud.orientation.y = p_hud.orientation_smooth.y = 0.0F;
			p_hud.orientation.z = p_hud.orientation_smooth.z = 0.0F;
			p_hud.alive = 1;

			p_hud.team = TEAM_1;
			player_render(&p_hud, PLAYERS_MAX, NULL, 1, NULL);
			matrix_identity();
			matrix_translate(1.4F, -2.0F, -3.0F);
			matrix_rotate(-90.0F - 22.5F, 0.0F, 1.0F, 0.0F);
			matrix_upload();
			p_hud.team = TEAM_2;
			player_render(&p_hud, PLAYERS_MAX, NULL, 1, NULL);
		}

		if(screen_current == SCREEN_GUN_SELECT) {
			matrix_identity();
			matrix_translate(-1.5F, -1.25F, -3.25F);
			matrix_rotate(window_time() * 90.0F, 0.0F, 1.0F, 0.0F);
			matrix_translate((model_semi.xpiv - model_semi.xsiz / 2.0F) * model_semi.scale,
							 (model_semi.zpiv - model_semi.zsiz / 2.0F) * model_semi.scale,
							 (model_semi.ypiv - model_semi.ysiz / 2.0F) * model_semi.scale);
			matrix_upload();
			kv6_render(&model_semi, TEAM_SPECTATOR);

			matrix_identity();
			matrix_translate(0.0F, -1.25F, -3.25F);
			matrix_rotate(window_time() * 90.0F, 0.0F, 1.0F, 0.0F);
			matrix_translate((model_smg.xpiv - model_smg.xsiz / 2.0F) * model_smg.scale,
							 (model_smg.zpiv - model_smg.zsiz / 2.0F) * model_smg.scale,
							 (model_smg.ypiv - model_smg.ysiz / 2.0F) * model_smg.scale);
			matrix_upload();
			kv6_render(&model_smg, TEAM_SPECTATOR);

			matrix_identity();
			matrix_translate(1.5F, -1.25F, -3.25F);
			matrix_rotate(window_time() * 90.0F, 0.0F, 1.0F, 0.0F);
			matrix_translate((model_shotgun.xpiv - model_shotgun.xsiz / 2.0F) * model_shotgun.scale,
							 (model_shotgun.zpiv - model_shotgun.zsiz / 2.0F) * model_shotgun.scale,
							 (model_shotgun.ypiv - model_shotgun.ysiz / 2.0F) * model_shotgun.scale);
			matrix_upload();
			kv6_render(&model_shotgun, TEAM_SPECTATOR);
		}

		struct kv6_t* rotating_model = NULL;
		int rotating_model_team = TEAM_SPECTATOR;
		if(gamestate.gamemode_type == GAMEMODE_CTF) {
			switch(players[local_player_id].team) {
				case TEAM_1:
					if(gamestate.gamemode.ctf.team_2_intel
					   && gamestate.gamemode.ctf.team_2_intel_location.held.player_id == local_player_id) {
						rotating_model = &model_intel;
						rotating_model_team = TEAM_2;
					}
					break;
				case TEAM_2:
					if(gamestate.gamemode.ctf.team_1_intel
					   && gamestate.gamemode.ctf.team_1_intel_location.held.player_id == local_player_id) {
						rotating_model = &model_intel;
						rotating_model_team = TEAM_1;
					}
					break;
			}
		}
		if(gamestate.gamemode_type == GAMEMODE_TC) {
			for(int k = 0; k < gamestate.gamemode.tc.territory_count; k++) {
				float l = pow(gamestate.gamemode.tc.territory[k].x - players[local_player_id].pos.x, 2.0F)
					+ pow((63.0F - gamestate.gamemode.tc.territory[k].z) - players[local_player_id].pos.y, 2.0F)
					+ pow(gamestate.gamemode.tc.territory[k].y - players[local_player_id].pos.z, 2.0F);
				if(l <= 20.0F * 20.0F) {
					rotating_model = &model_tent;
					rotating_model_team = gamestate.gamemode.tc.territory[k].team;
					break;
				}
			}
		}
		if(rotating_model) {
			matrix_identity();
			matrix_translate(0.0F, -(rotating_model->zsiz * 0.5F + rotating_model->zpiv) * rotating_model->scale,
							 -10.0F);
			matrix_rotate(window_time() * 90.0F, 0.0F, 1.0F, 0.0F);
			matrix_upload();
			glViewport(-settings.window_width * 0.4F, settings.window_height * 0.2F, settings.window_width,
					   settings.window_height);
			kv6_render(rotating_model, rotating_model_team);
			glViewport(0.0F, 0.0F, settings.window_width, settings.window_height);
		}
	}
}

static void hud_ingame_keyboard(int key, int action, int mods, int internal);

static int hud_ingame_onscreencontrol(int index, char* str, int activate) {
	if(chat_input_mode == CHAT_NO_INPUT) {
		if(show_exit) {
			switch(index) {
				case 0:
					if(str)
						strcpy(str, "Yes");
					if(activate == 0)
						hud_ingame_keyboard(WINDOW_KEY_YES, WINDOW_RELEASE, 0, 0);
					if(activate == 1)
						hud_ingame_keyboard(WINDOW_KEY_YES, WINDOW_PRESS, 0, 0);
					return 1;
				case 1:
					if(str)
						strcpy(str, "No");
					if(activate == 0)
						hud_ingame_keyboard(WINDOW_KEY_NO, WINDOW_RELEASE, 0, 0);
					if(activate == 1)
						hud_ingame_keyboard(WINDOW_KEY_NO, WINDOW_PRESS, 0, 0);
					return 1;
			}
		} else {
			if(!network_connected || (network_connected && network_logged_in)) {
				switch(index) {
					case 0:
						if(str)
							strcpy(str, "G-Chat");
						if(activate == 0)
							hud_ingame_keyboard(WINDOW_KEY_CHAT, WINDOW_RELEASE, 0, 0);
						if(activate == 1)
							hud_ingame_keyboard(WINDOW_KEY_CHAT, WINDOW_PRESS, 0, 0);
						return 1;
					case 1:
						if(str)
							strcpy(str, "T-Chat");
						if(activate == 0)
							hud_ingame_keyboard(WINDOW_KEY_YES, WINDOW_RELEASE, 0, 0);
						if(activate == 1)
							hud_ingame_keyboard(WINDOW_KEY_YES, WINDOW_PRESS, 0, 0);
						return 1;
					case 2:
						if(str)
							strcpy(str, "Score");
						if(activate == 0)
							keys(hud_window, WINDOW_KEY_TAB, 0, WINDOW_RELEASE, 0);
						if(activate == 1)
							keys(hud_window, WINDOW_KEY_TAB, 0, WINDOW_PRESS, 0);
						return 1;
					case 3:
						if(str)
							strcpy(str, "Team");
						if(activate == 0)
							hud_ingame_keyboard(WINDOW_KEY_CHANGETEAM, WINDOW_RELEASE, 0, 0);
						if(activate == 1)
							hud_ingame_keyboard(WINDOW_KEY_CHANGETEAM, WINDOW_PRESS, 0, 0);
						return 1;
					case 4:
						if(str)
							strcpy(str, "Weapon");
						if(activate == 0)
							hud_ingame_keyboard(WINDOW_KEY_CHANGEWEAPON, WINDOW_RELEASE, 0, 0);
						if(activate == 1)
							hud_ingame_keyboard(WINDOW_KEY_CHANGEWEAPON, WINDOW_PRESS, 0, 0);
						return 1;
					case 5:
						if(str)
							strcpy(str, "Network");
						if(activate == 0)
							keys(hud_window, WINDOW_KEY_NETWORKSTATS, 0, WINDOW_RELEASE, 0);
						if(activate == 1)
							keys(hud_window, WINDOW_KEY_NETWORKSTATS, 0, WINDOW_PRESS, 0);
						return 1;
					case 6:
						if(str)
							strcpy(str, "Tool");
						if(activate == 1)
							mouse_scroll(hud_window, 0, -1);
						return 1;
					case 64:
						if(str)
							strcpy(str, "LMB");
						if(activate == 0)
							mouse_click(hud_window, WINDOW_MOUSE_LMB, WINDOW_RELEASE, 0);
						if(activate == 1)
							mouse_click(hud_window, WINDOW_MOUSE_LMB, WINDOW_PRESS, 0);
						return 1;
					case 65:
						if(str)
							strcpy(str, "RMB");
						if(activate == 0)
							mouse_click(hud_window, WINDOW_MOUSE_RMB, WINDOW_RELEASE, 0);
						if(activate == 1)
							mouse_click(hud_window, WINDOW_MOUSE_RMB, WINDOW_PRESS, 0);
						return 1;
				}
			}
		}
	} else {
		switch(index) {
			case 0:
				if(str)
					strcpy(str, "Send");
				if(activate == 0)
					hud_ingame_keyboard(WINDOW_KEY_ENTER, WINDOW_RELEASE, 0, 0);
				if(activate == 1)
					hud_ingame_keyboard(WINDOW_KEY_ENTER, WINDOW_PRESS, 0, 0);
				return 1;
			case 1:
				if(str)
					strcpy(str, "Close");
				if(activate == 0)
					hud_ingame_keyboard(WINDOW_KEY_ESCAPE, WINDOW_RELEASE, 0, 0);
				if(activate == 1)
					hud_ingame_keyboard(WINDOW_KEY_ESCAPE, WINDOW_PRESS, 0, 0);
				return 1;
		}
	}
	return 0;
}

static void hud_ingame_render(float scalex, float scalef) {
	// window_mousemode(camera_mode==CAMERAMODE_SELECTION?WINDOW_CURSOR_ENABLED:WINDOW_CURSOR_DISABLED);
	hud_active->render_localplayer = players[local_player_id].team != TEAM_SPECTATOR
		&& (screen_current == SCREEN_NONE || camera_mode != CAMERAMODE_FPS);

	if(window_key_down(WINDOW_KEY_NETWORKSTATS)) {
		if(network_map_transfer)
			glColor3f(1.0F, 1.0F, 1.0F);
		else
			glColor3f(0.0F, 0.0F, 0.0F);
		glEnable(GL_DEPTH_TEST);
		glColorMask(0, 0, 0, 0);
		texture_draw_empty(8.0F * scalex, 380.0F * scalef, 160.0F * scalef, 160.0F * scalef);
		glColorMask(1, 1, 1, 1);
		glDepthFunc(GL_NOTEQUAL);
		texture_draw_empty(7.0F * scalex, 381.0F * scalef, 162.0F * scalef, 162.0F * scalef);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_DEPTH_TEST);
		font_select(FONT_SMALLFNT);
		char dbg_str[32];

		int max = 0;
		for(int k = 0; k < 40; k++) {
			max = max(max, network_stats[k].ingoing + network_stats[k].outgoing);
		}
		for(int k = 0; k < 40; k++) {
			float in_h = (float)(network_stats[39 - k].ingoing) / max * 160.0F;
			float out_h = (float)(network_stats[39 - k].ingoing + network_stats[39 - k].outgoing) / max * 160.0F;
			float ping_h = min(network_stats[39 - k].avg_ping / 25.0F, 160.0F);

			glColor3f(0.0F, 0.0F, 1.0F);
			texture_draw_empty(8.0F * scalex + 4 * k * scalef, (220.0F + out_h) * scalef, 4.0F * scalef,
							   out_h * scalef);
			if(!k) {
				sprintf(dbg_str, "out: %i b/s", network_stats[1].outgoing);
				font_render(8.0F * scalex + 80 * scalef, 212.0F * scalef, 8.0F * scalef, dbg_str);
			}

			glColor3f(0.0F, 1.0F, 0.0F);
			texture_draw_empty(8.0F * scalex + 4 * k * scalef, (220.0F + in_h) * scalef, 4.0F * scalef, in_h * scalef);
			if(!k) {
				sprintf(dbg_str, "in: %i b/s", network_stats[1].ingoing);
				font_render(8.0F * scalex, 212.0F * scalef, 8.0F * scalef, dbg_str);
			}

			glColor3f(1.0F, 0.0F, 0.0F);
			texture_draw_empty(8.0F * scalex + 4 * k * scalef, (220.0F + ping_h) * scalef, 4.0F * scalef,
							   ping_h * scalef);
			if(!k) {
				sprintf(dbg_str, "ping: %i", network_stats[1].avg_ping);
				font_render(8.0F * scalex, 202.0F * scalef, 8.0F * scalef, dbg_str);
			}
		}
		font_select(FONT_FIXEDSYS);
		glColor3f(1.0F, 1.0F, 1.0F);
	}

	if(network_map_transfer) {
		glColor3f(1.0F, 1.0F, 1.0F);
		texture_draw(&texture_splash, (settings.window_width - settings.window_height * 4.0F / 3.0F * 0.7F) * 0.5F,
					 560 * scalef, settings.window_height * 4.0F / 3.0F * 0.7F, settings.window_height * 0.7F);

		float p = (compressed_chunk_data_estimate > 0) ?
			((float)compressed_chunk_data_offset / (float)compressed_chunk_data_estimate) :
			0.0F;
		;
		glColor3ub(68, 68, 68);
		texture_draw(&texture_white, (settings.window_width - 440.0F * scalef) / 2.0F + 440.0F * scalef * p,
					 settings.window_height * 0.25F, 440.0F * scalef * (1.0F - p), 20.0F * scalef);
		glColor3ub(255, 255, 50);
		texture_draw(&texture_white, (settings.window_width - 440.0F * scalef) / 2.0F, settings.window_height * 0.25F,
					 440.0F * scalef * p, 20.0F * scalef);
		glColor3ub(69, 69, 69);
		char str[128];
		sprintf(str, "Loading Map %iKB/%iKB", compressed_chunk_data_offset / 1024,
				compressed_chunk_data_estimate / 1024);
		font_centered(settings.window_width / 2.0F, 130 * scalef, 27 * scalef, str);

		font_select(FONT_SMALLFNT);
		glColor3f(1.0F, 1.0F, 0.0F);
		font_render(0.0F, 8.0F * scalef, 8.0F * scalef,
					"Created by ByteBit, visit https://github.com/xtreme8000/BetterSpades");
		font_select(FONT_FIXEDSYS);
	} else {
		if(window_key_down(WINDOW_KEY_HIDEHUD))
			return;

		if(screen_current == SCREEN_TEAM_SELECT) {
			glColor3f(1.0F, 0.0F, 0.0F);
			char join_str[48];
			sprintf(join_str, "Press 1 to join %s", gamestate.team_1.name);
			font_centered(settings.window_width / 4.0F, 61 * scalef, 18.0F * scalef, join_str);
			sprintf(join_str, "Press 2 to join %s", gamestate.team_2.name);
			font_centered(settings.window_width / 4.0F * 3.0F, 61 * scalef, 18.0F * scalef, join_str);
			font_centered(settings.window_width / 2.0F, 61 * scalef, 18.0F * scalef, "Press 3 to spectate");
			glColor3f(1.0F, 1.0F, 1.0F);
		}

		if(screen_current == SCREEN_GUN_SELECT) {
			glColor3f(1.0F, 0.0F, 0.0F);
			font_centered(settings.window_width / 4.0F * 1.0F, 61 * scalef, 18.0F * scalef, "Press 1 to select");
			font_centered(settings.window_width / 4.0F * 2.0F, 61 * scalef, 18.0F * scalef, "Press 2 to select");
			font_centered(settings.window_width / 4.0F * 3.0F, 61 * scalef, 18.0F * scalef, "Press 3 to select");
			glColor3f(1.0F, 1.0F, 1.0F);
		}

		if(window_key_down(WINDOW_KEY_TAB) || camera_mode == CAMERAMODE_SELECTION) {
			if(network_connected && network_logged_in) {
				char ping_str[16];
				sprintf(ping_str, "PING: %ims", network_ping());
				font_select(FONT_SMALLFNT);
				glColor3f(1.0F, 0.0F, 0.0F);
				font_centered(settings.window_width / 2.0F, settings.window_height * 0.92F, 8.0F * scalef, ping_str);
				font_select(FONT_FIXEDSYS);
			}

			char score_str[8];
			glColor3ub(gamestate.team_1.red, gamestate.team_1.green, gamestate.team_1.blue);
			switch(gamestate.gamemode_type) {
				case GAMEMODE_CTF:
					sprintf(score_str, "%i-%i", gamestate.gamemode.ctf.team_1_score,
							gamestate.gamemode.ctf.capture_limit);
					break;
				case GAMEMODE_TC: {
					int t = 0;
					for(int k = 0; k < gamestate.gamemode.tc.territory_count; k++)
						if(gamestate.gamemode.tc.territory[k].team == TEAM_1)
							t++;
					sprintf(score_str, "%i-%i", t, gamestate.gamemode.tc.territory_count);
					break;
				}
			}
			font_centered(settings.window_width / 4.0F, 487 * scalef, 53.0F * scalef, score_str);
			glColor3ub(gamestate.team_2.red, gamestate.team_2.green, gamestate.team_2.blue);
			switch(gamestate.gamemode_type) {
				case GAMEMODE_CTF:
					sprintf(score_str, "%i-%i", gamestate.gamemode.ctf.team_2_score,
							gamestate.gamemode.ctf.capture_limit);
					break;
				case GAMEMODE_TC: {
					int t = 0;
					for(int k = 0; k < gamestate.gamemode.tc.territory_count; k++)
						if(gamestate.gamemode.tc.territory[k].team == TEAM_2)
							t++;
					sprintf(score_str, "%i-%i", t, gamestate.gamemode.tc.territory_count);
					break;
				}
			}
			font_centered(settings.window_width / 4.0F * 3.0F, 487 * scalef, 53.0F * scalef, score_str);

			struct player_table pt[PLAYERS_MAX];
			int connected = 0;
			for(int k = 0; k < PLAYERS_MAX; k++) {
				if(players[k].connected) {
					pt[connected].id = k;
					pt[connected++].score = players[k].score;
				}
			}
			qsort(pt, connected, sizeof(struct player_table), playertable_sort);

			int cntt[3] = {0};
			for(int k = 0; k < connected; k++) {
				int mul = 0;
				switch(players[pt[k].id].team) {
					case TEAM_1: mul = 1; break;
					case TEAM_2: mul = 3; break;
					default:
					case TEAM_SPECTATOR: mul = 2; break;
				}
				if(pt[k].id == local_player_id)
					glColor3f(1.0F, 1.0F, 0.0F);
				else if(!players[pt[k].id].alive)
					glColor3f(0.6F, 0.6F, 0.6F);
				else
					glColor3f(1.0F, 1.0F, 1.0F);
				char id_str[16];
				sprintf(id_str, "#%i", pt[k].id);
				font_render(settings.window_width / 4.0F * mul - font_length(18.0F * scalef, players[pt[k].id].name),
							(427 - 18 * cntt[mul - 1]) * scalef, 18.0F * scalef, players[pt[k].id].name);
				font_render(settings.window_width / 4.0F * mul + 8.82F * scalef, (427 - 18 * cntt[mul - 1]) * scalef,
							18.0F * scalef, id_str);
				if(mul != 2) {
					sprintf(id_str, "%i", pt[k].score);
					font_render(settings.window_width / 4.0F * mul + 44.1F * scalef,
								(427 - 18 * cntt[mul - 1]) * scalef, 18.0F * scalef, id_str);
				}
				cntt[mul - 1]++;
			}
		}

		int is_local = (camera_mode == CAMERAMODE_FPS) || (cameracontroller_bodyview_player == local_player_id);
		int local_id = (camera_mode == CAMERAMODE_FPS) ? local_player_id : cameracontroller_bodyview_player;

		if(camera_mode == CAMERAMODE_BODYVIEW
		   || (camera_mode == CAMERAMODE_SPECTATOR && cameracontroller_bodyview_mode)) {
			if(cameracontroller_bodyview_player != local_player_id) {
				font_select(FONT_SMALLFNT);
				switch(players[cameracontroller_bodyview_player].team) {
					case TEAM_1: glColor3ub(gamestate.team_1.red, gamestate.team_1.green, gamestate.team_1.blue); break;
					case TEAM_2: glColor3ub(gamestate.team_2.red, gamestate.team_2.green, gamestate.team_2.blue); break;
				}
				font_centered(settings.window_width / 2.0F, settings.window_height * 0.25F, 8.0F * scalef,
							  players[cameracontroller_bodyview_player].name);
			}
			font_select(FONT_FIXEDSYS);
			glColor3f(1.0F, 1.0F, 0.0F);
			font_centered(settings.window_width / 2.0F, settings.window_height, 18.0F * scalef,
						  "Click to switch players");
			if(window_time() - local_player_death_time <= local_player_respawn_time) {
				glColor3f(1.0F, 0.0F, 0.0F);
				int cnt = local_player_respawn_time - (int)(window_time() - local_player_death_time);
				char coin[16];
				sprintf(coin, "INSERT COIN:%i", cnt);
				font_centered(settings.window_width / 2.0F,
							  53.0F * scalef * (cameracontroller_bodyview_mode ? 2.0F : 1.0F), 53.0F * scalef, coin);
				if(local_player_respawn_cnt_last != cnt) {
					if(cnt < 4) {
						sound_create(NULL, SOUND_LOCAL, (cnt == 1) ? &sound_beep1 : &sound_beep2, 0.0F, 0.0F, 0.0F);
					}
					local_player_respawn_cnt_last = cnt;
				}
			}
			glColor3f(1.0F, 1.0F, 1.0F);
		}

		if(camera_mode == CAMERAMODE_FPS
		   || ((camera_mode == CAMERAMODE_BODYVIEW || camera_mode == CAMERAMODE_SPECTATOR)
			   && cameracontroller_bodyview_mode)) {
			glColor3f(1.0F, 1.0F, 1.0F);

			if(players[local_id].held_item == TOOL_GUN && players[local_id].input.buttons.rmb
			   && players[local_id].alive) {
				struct texture* zoom;
				switch(players[local_id].weapon) {
					case WEAPON_RIFLE: zoom = &texture_zoom_semi; break;
					case WEAPON_SMG: zoom = &texture_zoom_smg; break;
					case WEAPON_SHOTGUN: zoom = &texture_zoom_shotgun; break;
				}
				float last_shot = is_local ? weapon_last_shot : players[local_id].gun_shoot_timer;
				float zoom_factor = fmax(
					0.25F * (1.0F - ((window_time() - last_shot) / weapon_delay(players[local_id].weapon))) + 1.0F,
					1.0F);
				float aspect_ratio = (float)zoom->width / (float)zoom->height;

				texture_draw(zoom, (settings.window_width - settings.window_height * aspect_ratio * zoom_factor) / 2.0F,
							 settings.window_height * (zoom_factor * 0.5F + 0.5F),
							 settings.window_height * aspect_ratio * zoom_factor, settings.window_height * zoom_factor);
				texture_draw_sector(zoom, 0, settings.window_height * (zoom_factor * 0.5F + 0.5F),
									(settings.window_width - settings.window_height * aspect_ratio * zoom_factor)
										/ 2.0F,
									settings.window_height * zoom_factor, 0.0F, 0.0F, 1.0F / (float)zoom->width, 1.0F);
				texture_draw_sector(
					zoom, (settings.window_width + settings.window_height * aspect_ratio * zoom_factor) / 2.0F,
					settings.window_height * (zoom_factor * 0.5F + 0.5F),
					(settings.window_width - settings.window_height * aspect_ratio * zoom_factor) / 2.0F,
					settings.window_height * zoom_factor, (float)(zoom->width - 1) / (float)zoom->width, 0.0F,
					1.0F / (float)zoom->width, 1.0F);
			} else {
				texture_draw(&texture_target, (settings.window_width - 16) / 2.0F, (settings.window_height + 16) / 2.0F,
							 16, 16);
			}

			if(window_time() - local_player_last_damage_timer <= 0.5F && is_local) {
				float ang = atan2(players[local_player_id].orientation.z, players[local_player_id].orientation.x)
					- atan2(camera_z - local_player_last_damage_z, camera_x - local_player_last_damage_x) + PI;
				texture_draw_rotated(&texture_indicator, settings.window_width / 2.0F, settings.window_height / 2.0F,
									 200, 200, ang);
			}

			int health
				= is_local ? (players[local_id].alive ? local_player_health : 0) : (players[local_id].alive ? 100 : 0);
			if(health <= 30)
				glColor3f(1, 0, 0);
			else
				glColor3f(1, 1, 1);
			char hp[4];
			sprintf(hp, "%i", health);
			font_render(settings.window_width / 2.0F - font_length(53.0F * scalef, hp), 53.0F * scalef, 53.0F * scalef,
						hp);
			texture_draw(&texture_health, settings.window_width / 2.0F, 44.0F * scalef, 32.0F * scalef, 32.0F * scalef);

			char item_mini_str[32];
			struct texture* item_mini;
			int off = 0;
			glColor3f(1.0F, 1.0F, 0.0F);
			switch(players[local_id].held_item) {
				default:
				case TOOL_BLOCK: off = 64 * scalef;
				case TOOL_SPADE:
					item_mini = &texture_block;
					sprintf(item_mini_str, "%i", is_local ? local_player_blocks : 50);
					break;
				case TOOL_GRENADE:
					item_mini = &texture_grenade;
					sprintf(item_mini_str, "%i", is_local ? local_player_grenades : 3);
					break;
				case TOOL_GUN: {
					int ammo = is_local ? local_player_ammo : players[local_id].ammo;
					int ammo_reserve = is_local ? local_player_ammo_reserved : players[local_id].ammo_reserved;
					sprintf(item_mini_str, "%i-%i", ammo, ammo_reserve);
					switch(players[local_id].weapon) {
						case WEAPON_RIFLE: item_mini = &texture_ammo_semi; break;
						case WEAPON_SMG: item_mini = &texture_ammo_smg; break;
						case WEAPON_SHOTGUN: item_mini = &texture_ammo_shotgun; break;
					}
					if(ammo == 0)
						glColor3f(1.0F, 0.0F, 0.0F);
					break;
				}
			}

			texture_draw(item_mini, settings.window_width - 44.0F * scalef - off, 44.0F * scalef, 32.0F * scalef,
						 32.0F * scalef);
			font_render(settings.window_width - font_length(53.0F * scalef, item_mini_str) - 44.0F * scalef - off,
						53.0F * scalef, 53.0F * scalef, item_mini_str);
			glColor3f(1.0F, 1.0F, 1.0F);

			if(players[local_id].held_item == TOOL_BLOCK) {
				for(int y = 0; y < 8; y++) {
					for(int x = 0; x < 8; x++) {
						if(texture_block_color(x, y) == players[local_id].block.packed) {
							unsigned char g = (((int)(window_time() * 4)) & 1) * 0xFF;
							glColor3ub(g, g, g);
							texture_draw_empty(settings.window_width + (x * 8 - 65) * scalef, (65 - y * 8) * scalef,
											   8 * scalef, 8 * scalef);
							y = 10; // to break outer loop too
							break;
						}
					}
				}
				glColor3f(1.0F, 1.0F, 1.0F);

				texture_draw(&texture_color_selection, settings.window_width - 64 * scalef, 64 * scalef, 64 * scalef,
							 64 * scalef);
			}
		}

		if(camera_mode != CAMERAMODE_SELECTION) {
			glColor3f(1.0F, 1.0F, 1.0F);
			font_select(FONT_SMALLFNT);
			if(chat_input_mode != CHAT_NO_INPUT) {
				switch(chat_input_mode) {
					case CHAT_ALL_INPUT:
						font_render(11.0F * scalef, settings.window_height * 0.15F + 20.0F * scalef, 8.0F * scalef,
									"Global:");
						break;
					case CHAT_TEAM_INPUT:
						font_render(11.0F * scalef, settings.window_height * 0.15F + 20.0F * scalef, 8.0F * scalef,
									"Team:");
						break;
				}
				int l = strlen(chat[0][0]);
				chat[0][0][l] = '_';
				chat[0][0][l + 1] = 0;
				font_render(11.0F * scalef, settings.window_height * 0.15F + 10.0F * scalef, 8.0F * scalef, chat[0][0]);
				chat[0][0][l] = 0;
			}
			for(int k = 0; k < 6; k++) {
				if(window_time() - chat_timer[0][k + 1] < 10.0F || chat_input_mode != CHAT_NO_INPUT) {
					glColor3ub(red(chat_color[0][k + 1]), green(chat_color[0][k + 1]), blue(chat_color[0][k + 1]));
					font_render(11.0F * scalef, settings.window_height * 0.15F - 10.0F * scalef * k, 8.0F * scalef,
								chat[0][k + 1]);
				}

				if(window_time() - chat_timer[1][k + 1] < 10.0F) {
					glColor3ub(red(chat_color[1][k + 1]), green(chat_color[1][k + 1]), blue(chat_color[1][k + 1]));
					font_render(11.0F * scalef, settings.window_height - 22.0F * scalef - 10.0F * scalef * k,
								8.0F * scalef, chat[1][k + 1]);
				}
			}

			font_select(FONT_FIXEDSYS);
			glColor3f(1.0F, 1.0F, 1.0F);
		} else {
			glColor3f(1.0F, 1.0F, 1.0F);
			texture_draw(&texture_splash, (settings.window_width - 240 * scalef) * 0.5F, 599 * scalef, 240 * scalef,
						 180 * scalef);
			glColor3f(1.0F, 1.0F, 0.0F);
			font_centered(settings.window_width / 2.0F, 420 * scalef, 27 * scalef, "CONTROLS");
			char help_str[2][18][16] = {{"Movement", "Fire", "Gunsight", "Weapons", "Reload", "Jump", "Crouch", "Sneak",
										 "Sprint", "View Score", "Map", "Change Team", "Change Weapon", "Global Chat",
										 "Team Chat", "Color Select", "Grab Color", "Quit"},
										{"W S A D", "L. Mouse", "R. Mouse", "1-4/Wheel", "R", "Space", "CTRL", "V",
										 "SHIFT", "TAB", "M", ",", ".", "T", "Y", "Arrow Keys", "E", "ESC"}};
			for(int k = 0; k < 18; k++) {
				font_render(settings.window_width / 2.0F - font_length(18 * scalef, help_str[0][k]),
							(420 - 27 - 18 * k) * scalef, 18 * scalef, help_str[0][k]);
				font_render(settings.window_width / 2.0F + font_length(18 * scalef, " "), (420 - 27 - 18 * k) * scalef,
							18 * scalef, help_str[1][k]);
			}
			glColor3f(1.0F, 1.0F, 1.0F);
		}

		if(gamestate.gamemode_type == GAMEMODE_TC && gamestate.progressbar.tent < gamestate.gamemode.tc.territory_count
		   && gamestate.gamemode.tc.territory[gamestate.progressbar.tent].team
			   != gamestate.progressbar.team_capturing) {
			float p = max(min(gamestate.progressbar.progress
								  + 0.05F * gamestate.progressbar.rate * (window_time() - gamestate.progressbar.update),
							  1.0F),
						  0.0F);
			float l
				= pow(gamestate.gamemode.tc.territory[gamestate.progressbar.tent].x - players[local_player_id].pos.x,
					  2.0F)
				+ pow((63.0F - gamestate.gamemode.tc.territory[gamestate.progressbar.tent].z)
						  - players[local_player_id].pos.y,
					  2.0F)
				+ pow(gamestate.gamemode.tc.territory[gamestate.progressbar.tent].y - players[local_player_id].pos.z,
					  2.0F);
			if(p < 1.0F && l < 20.0F * 20.0F) {
				switch(gamestate.gamemode.tc.territory[gamestate.progressbar.tent].team) {
					case TEAM_1: glColor3ub(gamestate.team_1.red, gamestate.team_1.green, gamestate.team_1.blue); break;
					case TEAM_2: glColor3ub(gamestate.team_2.red, gamestate.team_2.green, gamestate.team_2.blue); break;
					default: glColor3ub(0, 0, 0);
				}
				texture_draw(&texture_white, (settings.window_width - 440.0F * scalef) / 2.0F + 440.0F * scalef * p,
							 settings.window_height * 0.25F, 440.0F * scalef * (1.0F - p), 20.0F * scalef);
				switch(gamestate.progressbar.team_capturing) {
					case TEAM_1: glColor3ub(gamestate.team_1.red, gamestate.team_1.green, gamestate.team_1.blue); break;
					case TEAM_2: glColor3ub(gamestate.team_2.red, gamestate.team_2.green, gamestate.team_2.blue); break;
					default: glColor3ub(0, 0, 0);
				}
				texture_draw(&texture_white, (settings.window_width - 440.0F * scalef) / 2.0F,
							 settings.window_height * 0.25F, 440.0F * scalef * p, 20.0F * scalef);
			}
		}

		// draw the minimap
		if(camera_mode != CAMERAMODE_SELECTION) {
			glColor3f(1.0F, 1.0F, 1.0F);
			// large
			if(window_key_down(WINDOW_KEY_MAP)) {
				float minimap_x = (settings.window_width - (map_size_x + 1) * scalef) / 2.0F;
				float minimap_y = ((600 - map_size_z - 1) / 2.0F + map_size_z + 1) * scalef;

				texture_draw(&texture_minimap, minimap_x, minimap_y, 512 * scalef, 512 * scalef);

				font_select(FONT_SMALLFNT);
				char c[2] = {0};
				for(int k = 0; k < 8; k++) {
					c[0] = 'A' + k;
					font_centered(minimap_x + (64 * k + 32) * scalef, minimap_y + 8.0F * scalef, 8.0F * scalef, c);
					c[0] = '1' + k;
					font_centered(minimap_x - 8 * scalef, minimap_y - (64 * k + 32 - 4) * scalef, 8.0F * scalef, c);
				}
				font_select(FONT_FIXEDSYS);

				tracer_minimap(1, scalef, minimap_x, minimap_y);

				if(gamestate.gamemode_type == GAMEMODE_CTF) {
					if(!gamestate.gamemode.ctf.team_1_intel) {
						glColor3ub(gamestate.team_1.red, gamestate.team_1.green, gamestate.team_1.blue);
						texture_draw_rotated(
							&texture_intel, minimap_x + gamestate.gamemode.ctf.team_1_intel_location.dropped.x * scalef,
							minimap_y - gamestate.gamemode.ctf.team_1_intel_location.dropped.y * scalef, 12 * scalef,
							12 * scalef, 0.0F);
					}
					if(map_object_visible(gamestate.gamemode.ctf.team_1_base.x, 0.0F,
										  gamestate.gamemode.ctf.team_1_base.y)) {
						glColor3f(gamestate.team_1.red * 0.94F, gamestate.team_1.green * 0.94F,
								  gamestate.team_1.blue * 0.94F);
						texture_draw_empty_rotated(minimap_x + gamestate.gamemode.ctf.team_1_base.x * scalef,
												   minimap_y - gamestate.gamemode.ctf.team_1_base.y * scalef,
												   12 * scalef, 12 * scalef, 0.0F);
						glColor3f(1.0F, 1.0F, 1.0F);
						texture_draw_rotated(
							&texture_medical, minimap_x + gamestate.gamemode.ctf.team_1_base.x * scalef,
							minimap_y - gamestate.gamemode.ctf.team_1_base.y * scalef, 12 * scalef, 12 * scalef, 0.0F);
					}

					if(!gamestate.gamemode.ctf.team_2_intel) {
						glColor3ub(gamestate.team_2.red, gamestate.team_2.green, gamestate.team_2.blue);
						texture_draw_rotated(
							&texture_intel, minimap_x + gamestate.gamemode.ctf.team_2_intel_location.dropped.x * scalef,
							minimap_y - gamestate.gamemode.ctf.team_2_intel_location.dropped.y * scalef, 12 * scalef,
							12 * scalef, 0.0F);
					}
					if(map_object_visible(gamestate.gamemode.ctf.team_2_base.x, 0.0F,
										  gamestate.gamemode.ctf.team_2_base.y)) {
						glColor3f(gamestate.team_2.red * 0.94F, gamestate.team_2.green * 0.94F,
								  gamestate.team_2.blue * 0.94F);
						texture_draw_empty_rotated(minimap_x + gamestate.gamemode.ctf.team_2_base.x * scalef,
												   minimap_y - gamestate.gamemode.ctf.team_2_base.y * scalef,
												   12 * scalef, 12 * scalef, 0.0F);
						glColor3f(1.0F, 1.0F, 1.0F);
						texture_draw_rotated(
							&texture_medical, minimap_x + gamestate.gamemode.ctf.team_2_base.x * scalef,
							minimap_y - gamestate.gamemode.ctf.team_2_base.y * scalef, 12 * scalef, 12 * scalef, 0.0F);
					}
				}
				if(gamestate.gamemode_type == GAMEMODE_TC) {
					for(int k = 0; k < gamestate.gamemode.tc.territory_count; k++) {
						switch(gamestate.gamemode.tc.territory[k].team) {
							case TEAM_1:
								glColor3f(gamestate.team_1.red * 0.94F, gamestate.team_1.green * 0.94F,
										  gamestate.team_1.blue * 0.94F);
								break;
							case TEAM_2:
								glColor3f(gamestate.team_2.red * 0.94F, gamestate.team_2.green * 0.94F,
										  gamestate.team_2.blue * 0.94F);
								break;
							default:
							case TEAM_SPECTATOR: glColor3ub(0, 0, 0);
						}
						texture_draw_rotated(
							&texture_command, minimap_x + gamestate.gamemode.tc.territory[k].x * scalef,
							minimap_y - gamestate.gamemode.tc.territory[k].y * scalef, 12 * scalef, 12 * scalef, 0.0F);
					}
				}

				for(int k = 0; k < PLAYERS_MAX; k++) {
					if(players[k].connected && players[k].alive && k != local_player_id
					   && players[k].team != TEAM_SPECTATOR
					   && (players[k].team == players[local_player_id].team || camera_mode == CAMERAMODE_SPECTATOR)) {
						switch(players[k].team) {
							case TEAM_1:
								glColor3ub(gamestate.team_1.red, gamestate.team_1.green, gamestate.team_1.blue);
								break;
							case TEAM_2:
								glColor3ub(gamestate.team_2.red, gamestate.team_2.green, gamestate.team_2.blue);
								break;
						}
						float ang = -atan2(players[k].orientation.z, players[k].orientation.x) - HALFPI;
						texture_draw_rotated(&texture_player, minimap_x + players[k].pos.x * scalef,
											 minimap_y - players[k].pos.z * scalef, 12 * scalef, 12 * scalef, ang);
					}
				}

				glColor3f(0.0F, 1.0F, 1.0F);
				texture_draw_rotated(&texture_player, minimap_x + camera_x * scalef, minimap_y - camera_z * scalef,
									 12 * scalef, 12 * scalef, camera_rot_x + PI);
				glColor3f(1.0F, 1.0F, 1.0F);
			} else {
				// minimized, top right
				float view_x = camera_x - 64.0F; // min(max(camera_x-64.0F,0.0F),map_size_x+1-128.0F);
				float view_z = camera_z - 64.0F; // min(max(camera_z-64.0F,0.0F),map_size_z+1-128.0F);

				switch(players[local_player_id].team) {
					case TEAM_1: glColor3ub(gamestate.team_1.red, gamestate.team_1.green, gamestate.team_1.blue); break;
					case TEAM_2: glColor3ub(gamestate.team_2.red, gamestate.team_2.green, gamestate.team_2.blue); break;
					case TEAM_SPECTATOR:
					default: glColor3f(0.0F, 0.0F, 0.0F); // same as chat
				}

				char sector_str[3] = {(int)(camera_x / 64.0F) + 'A', (int)(camera_z / 64.0F) + '1', 0};
				font_centered(settings.window_width - 79 * scalef, 456 * scalef, 20.0F * scalef, sector_str);

				glColor3ub(0, 0, 0);
				texture_draw_empty(settings.window_width - 144 * scalef, 586 * scalef, 130 * scalef, 130 * scalef);
				glColor3f(1.0F, 1.0F, 1.0F);

				texture_draw_sector(&texture_minimap, settings.window_width - 143 * scalef, 585 * scalef, 128 * scalef,
									128 * scalef, (camera_x - 64.0F) / 512.0F, (camera_z - 64.0F) / 512.0F, 0.25F,
									0.25F);

				tracer_minimap(0, scalef, view_x, view_z);

				if(gamestate.gamemode_type == GAMEMODE_CTF) {
					float tent1_x = min(max(gamestate.gamemode.ctf.team_1_base.x, view_x), view_x + 128.0F) - view_x;
					float tent1_y = min(max(gamestate.gamemode.ctf.team_1_base.y, view_z), view_z + 128.0F) - view_z;

					float tent2_x = min(max(gamestate.gamemode.ctf.team_2_base.x, view_x), view_x + 128.0F) - view_x;
					float tent2_y = min(max(gamestate.gamemode.ctf.team_2_base.y, view_z), view_z + 128.0F) - view_z;

					if(map_object_visible(gamestate.gamemode.ctf.team_1_base.x, 0.0F,
										  gamestate.gamemode.ctf.team_1_base.y)) {
						glColor3f(gamestate.team_1.red * 0.94F, gamestate.team_1.green * 0.94F,
								  gamestate.team_1.blue * 0.94F);
						texture_draw_empty_rotated(settings.window_width - 143 * scalef + tent1_x * scalef,
												   (585 - tent1_y) * scalef, 12 * scalef, 12 * scalef, 0.0F);
						glColor3f(1.0F, 1.0F, 1.0F);
						texture_draw_rotated(&texture_medical, settings.window_width - 143 * scalef + tent1_x * scalef,
											 (585 - tent1_y) * scalef, 12 * scalef, 12 * scalef, 0.0F);
					}
					if(!gamestate.gamemode.ctf.team_1_intel) {
						float intel_x
							= min(max(gamestate.gamemode.ctf.team_1_intel_location.dropped.x, view_x), view_x + 128.0F)
							- view_x;
						float intel_y
							= min(max(gamestate.gamemode.ctf.team_1_intel_location.dropped.y, view_z), view_z + 128.0F)
							- view_z;
						glColor3ub(gamestate.team_1.red, gamestate.team_1.green, gamestate.team_1.blue);
						texture_draw_rotated(&texture_intel, settings.window_width - 143 * scalef + intel_x * scalef,
											 (585 - intel_y) * scalef, 12 * scalef, 12 * scalef, 0.0F);
					}

					if(map_object_visible(gamestate.gamemode.ctf.team_2_base.x, 0.0F,
										  gamestate.gamemode.ctf.team_2_base.y)) {
						glColor3f(gamestate.team_2.red * 0.94F, gamestate.team_2.green * 0.94F,
								  gamestate.team_2.blue * 0.94F);
						texture_draw_empty_rotated(settings.window_width - 143 * scalef + tent2_x * scalef,
												   (585 - tent2_y) * scalef, 12 * scalef, 12 * scalef, 0.0F);
						glColor3f(1.0F, 1.0F, 1.0F);
						texture_draw_rotated(&texture_medical, settings.window_width - 143 * scalef + tent2_x * scalef,
											 (585 - tent2_y) * scalef, 12 * scalef, 12 * scalef, 0.0F);
					}
					if(!gamestate.gamemode.ctf.team_2_intel) {
						float intel_x
							= min(max(gamestate.gamemode.ctf.team_2_intel_location.dropped.x, view_x), view_x + 128.0F)
							- view_x;
						float intel_y
							= min(max(gamestate.gamemode.ctf.team_2_intel_location.dropped.y, view_z), view_z + 128.0F)
							- view_z;
						glColor3ub(gamestate.team_2.red, gamestate.team_2.green, gamestate.team_2.blue);
						texture_draw_rotated(&texture_intel, settings.window_width - 143 * scalef + intel_x * scalef,
											 (585 - intel_y) * scalef, 12 * scalef, 12 * scalef, 0.0F);
					}
				}
				if(gamestate.gamemode_type == GAMEMODE_TC) {
					for(int k = 0; k < gamestate.gamemode.tc.territory_count; k++) {
						switch(gamestate.gamemode.tc.territory[k].team) {
							case TEAM_1:
								glColor3f(gamestate.team_1.red * 0.94F, gamestate.team_1.green * 0.94F,
										  gamestate.team_1.blue * 0.94F);
								break;
							case TEAM_2:
								glColor3f(gamestate.team_2.red * 0.94F, gamestate.team_2.green * 0.94F,
										  gamestate.team_2.blue * 0.94F);
								break;
							default:
							case TEAM_SPECTATOR: glColor3ub(0, 0, 0);
						}
						float t_x = min(max(gamestate.gamemode.tc.territory[k].x, view_x), view_x + 128.0F) - view_x;
						float t_y = min(max(gamestate.gamemode.tc.territory[k].y, view_z), view_z + 128.0F) - view_z;
						texture_draw_rotated(&texture_command, settings.window_width - 143 * scalef + t_x * scalef,
											 (585 - t_y) * scalef, 12 * scalef, 12 * scalef, 0.0F);
					}
				}

				for(int k = 0; k < PLAYERS_MAX; k++) {
					if(players[k].connected && players[k].alive
					   && (players[k].team == players[local_player_id].team
						   || (camera_mode == CAMERAMODE_SPECTATOR
							   && (k == local_player_id || players[k].team != TEAM_SPECTATOR)))) {
						if(k == local_player_id) {
							glColor3ub(0, 255, 255);
						} else {
							switch(players[k].team) {
								case TEAM_1:
									glColor3ub(gamestate.team_1.red, gamestate.team_1.green, gamestate.team_1.blue);
									break;
								case TEAM_2:
									glColor3ub(gamestate.team_2.red, gamestate.team_2.green, gamestate.team_2.blue);
									break;
							}
						}
						float player_x = ((k == local_player_id) ? camera_x : players[k].pos.x) - view_x;
						float player_y = ((k == local_player_id) ? camera_z : players[k].pos.z) - view_z;
						if(player_x > 0.0F && player_x < 128.0F && player_y > 0.0F && player_y < 128.0F) {
							float ang = (k == local_player_id) ?
								camera_rot_x + PI :
								-atan2(players[k].orientation.z, players[k].orientation.x) - HALFPI;
							texture_draw_rotated(&texture_player,
												 settings.window_width - 143 * scalef + player_x * scalef,
												 (585 - player_y) * scalef, 12 * scalef, 12 * scalef, ang);
						}
					}
				}
			}
		}

		if(player_intersection_type >= 0
		   && (players[local_player_id].team == TEAM_SPECTATOR
			   || players[player_intersection_player].team == players[local_player_id].team)) {
			font_select(FONT_SMALLFNT);
			char* th[4] = {"torso", "head", "arms", "legs"};
			char str[32];
			switch(players[player_intersection_player].team) {
				case TEAM_1: glColor3ub(gamestate.team_1.red, gamestate.team_1.green, gamestate.team_1.blue); break;
				case TEAM_2: glColor3ub(gamestate.team_2.red, gamestate.team_2.green, gamestate.team_2.blue); break;
				default: glColor3f(1.0F, 1.0F, 1.0F);
			}
			sprintf(str, "%s's %s", players[player_intersection_player].name, th[player_intersection_type]);
			font_centered(settings.window_width / 2.0F, settings.window_height * 0.2F, 8.0F * scalef, str);
			font_select(FONT_FIXEDSYS);
		}

		if(show_exit) {
			glColor3f(1.0F, 0.0F, 0.0F);
			font_render((settings.window_width - font_length(53.0F * scalef, "EXIT GAME? Y/N")) / 2.0F,
						settings.window_height / 2.0F + 53.0F * scalef, 53.0F * scalef, "EXIT GAME? Y/N");

			char play_time[128];
			sprintf(play_time, "Playing for %im%is", (int)window_time() / 60, (int)window_time() % 60);
			font_render(settings.window_width - font_length(27.0F * scalef, play_time), settings.window_height,
						27.0F * scalef, play_time);
		}
		if(window_time() - chat_popup_timer < chat_popup_duration) {
			glColor3ub(red(chat_popup_color), green(chat_popup_color), blue(chat_popup_color));
			font_render((settings.window_width - font_length(53.0F * scalef, chat_popup)) / 2.0F,
						settings.window_height / 2.0F, 53.0F * scalef, chat_popup);
		}
		glColor3f(1.0F, 1.0F, 1.0F);
	}

	if(settings.show_fps) {
		char debug_str[16];
		font_select(FONT_FIXEDSYS);
		glColor3f(1.0F, 1.0F, 1.0F);
		sprintf(debug_str, "PING: %ims", network_ping());
		font_render(11.0F * scalef, settings.window_height * 0.33F, 20.0F * scalef, debug_str);
		sprintf(debug_str, "FPS: %i", (int)fps);
		font_render(11.0F * scalef, settings.window_height * 0.33F - 20.0F * scalef, 20.0F * scalef, debug_str);
	}

#ifdef USE_TOUCH
	glColor3f(1.0F, 1.0F, 1.0F);
	if(camera_mode == CAMERAMODE_FPS || camera_mode == CAMERAMODE_SPECTATOR) {
		texture_draw_rotated(&texture_ui_joystick, settings.window_height * 0.3F, settings.window_height * 0.3F,
							 settings.window_height * 0.4F, settings.window_height * 0.4F, 0.0F);
		texture_draw_rotated(&texture_ui_knob, settings.window_height * 0.3F, settings.window_height * 0.3F,
							 settings.window_height * 0.075F, settings.window_height * 0.075F, 0.0F);
		texture_draw_rotated(&texture_ui_knob, hud_ingame_touch_x + settings.window_height * 0.3F,
							 hud_ingame_touch_y + settings.window_height * 0.3F, settings.window_height * 0.1F,
							 settings.window_height * 0.1F, 0.0F);
	}

	int k = 0;
	char str[128];
	while(hud_ingame_onscreencontrol(k, str, -1)) {
		texture_draw_rotated(&texture_ui_input, settings.window_height * (0.2F + 0.175F * k),
							 settings.window_height * 0.96F, settings.window_height * 0.15F,
							 settings.window_height * 0.1F, 0.0F);
		font_centered(settings.window_height * (0.2F + 0.175F * k), settings.window_height * 0.98F,
					  settings.window_height * 0.04F, str);
		k++;
	}
	if(hud_ingame_onscreencontrol(64, str, -1)) {
		texture_draw_rotated(&texture_ui_input, settings.window_width - settings.window_height * 0.075F,
							 settings.window_height * 0.6F, settings.window_height * 0.15F,
							 settings.window_height * 0.1F, 0.0F);
		font_centered(settings.window_width - settings.window_height * 0.075F, settings.window_height * 0.62F,
					  settings.window_height * 0.04F, str);
	}
	if(hud_ingame_onscreencontrol(65, str, -1)) {
		texture_draw_rotated(&texture_ui_input, settings.window_width - settings.window_height * 0.075F,
							 settings.window_height * 0.45F, settings.window_height * 0.15F,
							 settings.window_height * 0.1F, 0.0F);
		font_centered(settings.window_width - settings.window_height * 0.075F, settings.window_height * 0.47F,
					  settings.window_height * 0.04F, str);
	}
#endif
}

static void hud_ingame_scroll(double yoffset) {
	if(camera_mode == CAMERAMODE_FPS && yoffset != 0.0F) {
		int h = players[local_player_id].held_item;
		if(!players[local_player_id].items_show)
			local_player_lasttool = h;
		h += (yoffset < 0) ? 1 : -1;
		if(h < 0)
			h = 3;
		if(h == TOOL_BLOCK && local_player_blocks == 0)
			h += (yoffset < 0) ? 1 : -1;
		if(h == TOOL_GUN && local_player_ammo + local_player_ammo_reserved == 0)
			h += (yoffset < 0) ? 1 : -1;
		if(h == TOOL_GRENADE && local_player_grenades == 0)
			h += (yoffset < 0) ? 1 : -1;
		if(h > 3)
			h = 0;
		players[local_player_id].held_item = h;
		sound_create(NULL, SOUND_LOCAL, &sound_switch, 0.0F, 0.0F, 0.0F)->stick_to_player = local_player_id;
		players[local_player_id].item_disabled = window_time();
		players[local_player_id].items_show_start = window_time();
		players[local_player_id].items_show = 1;
	}
}

static double last_x, last_y;
static void hud_ingame_mouselocation(double x, double y) {
	if(show_exit) {
		last_x = x;
		last_y = y;
		return;
	}
	float dx = x - last_x;
	float dy = y - last_y;
	last_x = x;
	last_y = y;

	float s = 1.0F;
	if(camera_mode == CAMERAMODE_FPS && players[local_player_id].held_item == TOOL_GUN
	   && players[local_player_id].input.buttons.rmb) {
		s = 0.5F;
	}

	if(settings.invert_y)
		dy *= -1.0F;

	camera_rot_x -= dx * settings.mouse_sensitivity / 5.0F * (float)MOUSE_SENSITIVITY * s;
	camera_rot_y += dy * settings.mouse_sensitivity / 5.0F * (float)MOUSE_SENSITIVITY * s;

	camera_overflow_adjust();
}

static void hud_ingame_mouseclick(double x, double y, int button, int action, int mods) {
	if(button == WINDOW_MOUSE_LMB) {
		button_map[0] = (action == WINDOW_PRESS);
	}
	if(button == WINDOW_MOUSE_RMB) {
		if(action == WINDOW_PRESS && players[local_player_id].held_item == TOOL_GUN) {
			players[local_player_id].input.buttons.rmb ^= 1;
			if(players[local_player_id].items_show) {
				players[local_player_id].input.buttons.rmb = 0;
			}
		}
		if(local_player_drag_active && action == WINDOW_RELEASE && players[local_player_id].held_item == TOOL_BLOCK) {
			int* pos = camera_terrain_pick(0);
			if(pos != NULL && pos[1] > 1
			   && (pow(pos[0] - camera_x, 2) + pow(pos[1] - camera_y, 2) + pow(pos[2] - camera_z, 2)) < 5 * 5) {
				int amount = map_cube_line(local_player_drag_x, local_player_drag_z, 63 - local_player_drag_y, pos[0],
										   pos[2], 63 - pos[1], NULL);
				if(amount <= local_player_blocks) {
					struct PacketBlockLine line;
					line.player_id = local_player_id;
					line.sx = local_player_drag_x;
					line.sy = local_player_drag_z;
					line.sz = 63 - local_player_drag_y;
					line.ex = pos[0];
					line.ey = pos[2];
					line.ez = 63 - pos[1];
					network_send(PACKET_BLOCKLINE_ID, &line, sizeof(line));
					local_player_blocks -= amount;
				}
				players[local_player_id].item_showup = window_time();
			}
		}
		local_player_drag_active = 0;
		if(action == WINDOW_PRESS && players[local_player_id].held_item == TOOL_BLOCK
		   && window_time() - players[local_player_id].item_showup >= 0.5F) {
			int* pos = camera_terrain_pick(0);
			if(pos != NULL && pos[1] > 1
			   && distance3D(camera_x, camera_y, camera_z, pos[0], pos[1], pos[2]) < 5.0F * 5.0F) {
				local_player_drag_active = 1;
				local_player_drag_x = pos[0];
				local_player_drag_y = pos[1];
				local_player_drag_z = pos[2];
			}
		}
		button_map[1] = (action == WINDOW_PRESS);
	}
	if(button == WINDOW_MOUSE_MMB) {
		button_map[2] = (action == WINDOW_PRESS);
	}
	if(camera_mode == CAMERAMODE_BODYVIEW && button == WINDOW_MOUSE_MMB && action == WINDOW_PRESS) {
		float nearest_dist = FLT_MAX;
		int nearest_player = -1;
		for(int k = 0; k < PLAYERS_MAX; k++)
			if(player_can_spectate(&players[k]) && players[k].alive && k != cameracontroller_bodyview_player
			   && distance3D(camera_x, camera_y, camera_z, players[k].pos.x, players[k].pos.y, players[k].pos.z)
				   < nearest_dist) {
				nearest_dist
					= distance3D(camera_x, camera_y, camera_z, players[k].pos.x, players[k].pos.y, players[k].pos.z);
				nearest_player = k;
			}
		if(nearest_player >= 0)
			cameracontroller_bodyview_player = nearest_player;
	}
	if(button == WINDOW_MOUSE_RMB && action == WINDOW_PRESS) {
		players[local_player_id].input.buttons.rmb_start = window_time();
		if(camera_mode == CAMERAMODE_BODYVIEW || camera_mode == CAMERAMODE_SPECTATOR) {
			if(camera_mode == CAMERAMODE_SPECTATOR)
				cameracontroller_bodyview_mode = 1;
			for(int k = 0; k < PLAYERS_MAX * 2; k++) {
				cameracontroller_bodyview_player = (cameracontroller_bodyview_player + 1) % PLAYERS_MAX;
				if(player_can_spectate(&players[cameracontroller_bodyview_player]))
					break;
			}
			cameracontroller_bodyview_zoom = 0.0F;
		}
	}
	if(button == WINDOW_MOUSE_LMB) {
		if(camera_mode == CAMERAMODE_FPS && window_time() - players[local_player_id].item_showup >= 0.5F) {
			if(players[local_player_id].held_item == TOOL_GRENADE && local_player_grenades > 0) {
				if(action == WINDOW_RELEASE) {
					local_player_grenades = max(local_player_grenades - 1, 0);
					struct PacketGrenade g;
					g.player_id = local_player_id;
					g.fuse_length
						= max(3.0F - (window_time() - players[local_player_id].input.buttons.lmb_start), 0.0F);
					g.x = players[local_player_id].pos.x;
					g.y = players[local_player_id].pos.z;
					g.z = 63.0F - players[local_player_id].pos.y;
					g.vx = (g.fuse_length == 0.0F) ?
						0.0F :
						(players[local_player_id].orientation.x + players[local_player_id].physics.velocity.x);
					g.vy = (g.fuse_length == 0.0F) ?
						0.0F :
						(players[local_player_id].orientation.z + players[local_player_id].physics.velocity.z);
					g.vz = (g.fuse_length == 0.0F) ?
						0.0F :
						(-players[local_player_id].orientation.y - players[local_player_id].physics.velocity.y);
					network_send(PACKET_GRENADE_ID, &g, sizeof(g));
					read_PacketGrenade(&g, sizeof(g)); // server won't loop packet back
					players[local_player_id].item_showup = window_time();
				}
				if(action == WINDOW_PRESS) {
					sound_create(NULL, SOUND_LOCAL, &sound_grenade_pin, 0.0F, 0.0F, 0.0F)->stick_to_player
						= local_player_id;
				}
			}
		}
	}
	if(button == WINDOW_MOUSE_LMB && action == WINDOW_PRESS) {
		players[local_player_id].input.buttons.lmb_start = window_time();

		if(camera_mode == CAMERAMODE_FPS) {
			if(players[local_player_id].held_item == TOOL_GUN) {
				if(weapon_reloading()) {
					weapon_reload_abort();
				}
				if(local_player_ammo == 0 && window_time() - players[local_player_id].item_showup >= 0.5F) {
					sound_create(NULL, SOUND_LOCAL, &sound_empty, 0.0F, 0.0F, 0.0F);
					chat_showpopup("RELOAD", 0.4F, rgb(255, 0, 0));
				}
			}
		}

		if(camera_mode == CAMERAMODE_BODYVIEW || camera_mode == CAMERAMODE_SPECTATOR) {
			if(camera_mode == CAMERAMODE_SPECTATOR)
				cameracontroller_bodyview_mode = 1;
			for(int k = 0; k < PLAYERS_MAX * 2; k++) {
				cameracontroller_bodyview_player = (cameracontroller_bodyview_player - 1) % PLAYERS_MAX;
				if(cameracontroller_bodyview_player < 0)
					cameracontroller_bodyview_player = PLAYERS_MAX - 1;
				if(player_can_spectate(&players[cameracontroller_bodyview_player]))
					break;
			}
			cameracontroller_bodyview_zoom = 0.0F;
		}
	}
}

struct autocomplete_type {
	const char* str;
	int acceptance;
};

static int autocomplete_type_cmp(const void* a, const void* b) {
	struct autocomplete_type* aa = (struct autocomplete_type*)a;
	struct autocomplete_type* bb = (struct autocomplete_type*)b;
	return bb->acceptance - aa->acceptance;
}

static const char* hud_ingame_completeword(const char* s) {
	// find most likely player name or command

	struct autocomplete_type candidates[PLAYERS_MAX * 2 + 64] = {0};
	int candidates_cnt = 0;

	for(int k = 0; k < PLAYERS_MAX; k++) {
		if(players[k].connected) {
			candidates[candidates_cnt++] = (struct autocomplete_type) {
				players[k].name,
				0,
			};
			char nmbr[8];
			sprintf(nmbr, "#%i", k);
			candidates[candidates_cnt++] = (struct autocomplete_type) {
				nmbr,
				0,
			};
		}
	}

	candidates[candidates_cnt++] = (struct autocomplete_type) {
		gamestate.team_1.name,
		0,
	};
	candidates[candidates_cnt++] = (struct autocomplete_type) {
		gamestate.team_2.name,
		0,
	};
	candidates[candidates_cnt++] = (struct autocomplete_type) {"/help", 0};
	candidates[candidates_cnt++] = (struct autocomplete_type) {"/medkit", 0};
	candidates[candidates_cnt++] = (struct autocomplete_type) {"/squad", 0};
	candidates[candidates_cnt++] = (struct autocomplete_type) {"/votekick", 0};
	candidates[candidates_cnt++] = (struct autocomplete_type) {"/login", 0};
	candidates[candidates_cnt++] = (struct autocomplete_type) {"/airstrike", 0};
	candidates[candidates_cnt++] = (struct autocomplete_type) {"/streak", 0};
	candidates[candidates_cnt++] = (struct autocomplete_type) {"/ratio", 0};
	candidates[candidates_cnt++] = (struct autocomplete_type) {"/intel", 0};
	candidates[candidates_cnt++] = (struct autocomplete_type) {"/time", 0};
	candidates[candidates_cnt++] = (struct autocomplete_type) {"/admin", 0};
	candidates[candidates_cnt++] = (struct autocomplete_type) {"/ping", 0};

	// valuate all strings
	for(int k = 0; k < candidates_cnt; k++) {
		for(int i = 0; i < strlen(candidates[k].str) && i < strlen(s); i++) {
			if(candidates[k].str[i] == s[i])
				candidates[k].acceptance += 2;
			else if(tolower(candidates[k].str[i]) == tolower(s[i]) || s[i] == '*') {
				candidates[k].acceptance++;
			} else {
				candidates[k].acceptance = 0;
				break;
			}
		}
	}

	qsort(candidates, candidates_cnt, sizeof(struct autocomplete_type), autocomplete_type_cmp);
	return (strlen(candidates[0].str) > 0 && candidates[0].acceptance > 0) ? candidates[0].str : NULL;
}

static void hud_ingame_keyboard(int key, int action, int mods, int internal) {
	if(chat_input_mode != CHAT_NO_INPUT && action == WINDOW_PRESS && key == WINDOW_KEY_TAB && strlen(chat[0][0]) > 0) {
		// autocomplete word
		char* incomplete = strrchr(chat[0][0], ' ') + 1;
		if(incomplete == (char*)1)
			incomplete = chat[0][0];
		const char* match = hud_ingame_completeword(incomplete);
		if(match && strlen(match) + strlen(chat[0][0]) < 128)
			strcpy(incomplete, match);
	}

	if(chat_input_mode == CHAT_NO_INPUT) {
		if(action == WINDOW_PRESS) {
			if(!network_connected) {
				if(key == WINDOW_KEY_F1) {
					camera_mode = CAMERAMODE_SELECTION;
				}
				if(key == WINDOW_KEY_F2) {
					camera_mode = CAMERAMODE_FPS;
				}
				if(key == WINDOW_KEY_F3) {
					camera_mode = CAMERAMODE_SPECTATOR;
				}
				if(key == WINDOW_KEY_F4) {
					camera_mode = CAMERAMODE_BODYVIEW;
				}
				if(key == WINDOW_KEY_SNEAK) {
					log_debug("%f,%f,%f,%f,%f", camera_x, camera_y, camera_z, camera_rot_x, camera_rot_y);
					players[local_player_id].pos.x = 256.0F;
					players[local_player_id].pos.y = 63.0F;
					players[local_player_id].pos.z = 256.0F;
				}
				if(key == WINDOW_KEY_CROUCH) {
					// particle_create(0xFF00FF,players[local_player_id].pos.x,players[local_player_id].pos.y,players[local_player_id].pos.z,2.0F,1.0F,1024,0.1F,0.25F);
					players[local_player_id].alive = !players[local_player_id].alive;
				}
			}

			if(key == WINDOW_KEY_LASTTOOL) {
				int tmp = players[local_player_id].held_item;
				players[local_player_id].held_item = local_player_lasttool;
				local_player_lasttool = tmp;
				players[local_player_id].item_disabled = window_time();
				players[local_player_id].items_show_start = window_time();
				players[local_player_id].items_show = 1;
			}

			if(key == WINDOW_KEY_VOLUME_UP) {
				settings.volume = min(settings.volume + 1, 10);
			}
			if(key == WINDOW_KEY_VOLUME_DOWN) {
				settings.volume = max(settings.volume - 1, 0);
			}
			if(key == WINDOW_KEY_VOLUME_UP || key == WINDOW_KEY_VOLUME_DOWN) {
				sound_volume(settings.volume / 10.0F);
				char volstr[64];
				sprintf(volstr, "Volume: %i", settings.volume);
				chat_add(0, 0x0000FF, volstr);
			}

			if(key == WINDOW_KEY_COMMAND) {
				window_textinput(1);
				chat_input_mode = CHAT_ALL_INPUT;
				strcpy(chat[0][0], "/");
			}

			if(key == WINDOW_KEY_CHAT) {
				window_textinput(1);
				chat_input_mode = CHAT_ALL_INPUT;
				chat[0][0][0] = 0;
			}

			if(show_exit && key == WINDOW_KEY_NO) {
				show_exit = 0;
				window_mousemode(WINDOW_CURSOR_DISABLED);
			}

			if(key == WINDOW_KEY_YES) {
				if(show_exit) {
					hud_change(&hud_serverlist);
				} else {
					window_textinput(1);
					chat_input_mode = CHAT_TEAM_INPUT;
					chat[0][0][0] = 0;
				}
			}

			if((key == WINDOW_KEY_CURSOR_UP || key == WINDOW_KEY_CURSOR_DOWN || key == WINDOW_KEY_CURSOR_LEFT
				|| key == WINDOW_KEY_CURSOR_RIGHT)
			   && camera_mode == CAMERAMODE_FPS && players[local_player_id].held_item == TOOL_BLOCK) {
				int y;
				for(y = 0; y < 8; y++) {
					for(int x = 0; x < 8; x++) {
						if(texture_block_color(x, y) == players[local_player_id].block.packed) {
							switch(key) {
								case WINDOW_KEY_CURSOR_LEFT:
									x--;
									if(x < 0)
										x = 7;
									break;
								case WINDOW_KEY_CURSOR_RIGHT:
									x++;
									if(x > 7)
										x = 0;
									break;
								case WINDOW_KEY_CURSOR_UP:
									y--;
									if(y < 0)
										y = 7;
									break;
								case WINDOW_KEY_CURSOR_DOWN:
									y++;
									if(y > 7)
										y = 0;
									break;
							}
							players[local_player_id].block.packed = texture_block_color(x, y);
							network_updateColor();
							y = 10;
							break;
						}
					}
				}
				if(y < 10) {
					players[local_player_id].block.packed = texture_block_color(3, 0);
					network_updateColor();
				}
			}

			if(key == WINDOW_KEY_RELOAD && camera_mode == CAMERAMODE_FPS
			   && players[local_player_id].held_item == TOOL_GUN) {
				weapon_reload();
			}

			if(key == WINDOW_KEY_SNEAK && (camera_mode == CAMERAMODE_BODYVIEW || camera_mode == CAMERAMODE_SPECTATOR)) {
				cameracontroller_bodyview_mode = !cameracontroller_bodyview_mode;
			}

			if(screen_current == SCREEN_NONE && camera_mode == CAMERAMODE_FPS) {
				unsigned char tool_switch = 0;
				switch(key) {
					case WINDOW_KEY_TOOL1:
						if(players[local_player_id].held_item != TOOL_SPADE) {
							local_player_lasttool = players[local_player_id].held_item;
							players[local_player_id].held_item = TOOL_SPADE;
							tool_switch = 1;
						}
						break;
					case WINDOW_KEY_TOOL2:
						if(players[local_player_id].held_item != TOOL_BLOCK) {
							local_player_lasttool = players[local_player_id].held_item;
							players[local_player_id].held_item = TOOL_BLOCK;
							tool_switch = 1;
						}
						break;
					case WINDOW_KEY_TOOL3:
						if(players[local_player_id].held_item != TOOL_GUN) {
							local_player_lasttool = players[local_player_id].held_item;
							players[local_player_id].held_item = TOOL_GUN;
							tool_switch = 1;
						}
						break;
					case WINDOW_KEY_TOOL4:
						if(players[local_player_id].held_item != TOOL_GRENADE) {
							local_player_lasttool = players[local_player_id].held_item;
							players[local_player_id].held_item = TOOL_GRENADE;
							tool_switch = 1;
						}
						break;
				}

				if(tool_switch) {
					sound_create(NULL, SOUND_LOCAL, &sound_switch, 0.0F, 0.0F, 0.0F)->stick_to_player = local_player_id;
					players[local_player_id].item_disabled = window_time();
					players[local_player_id].items_show_start = window_time();
					players[local_player_id].items_show = 1;
				}
			}

			if(screen_current == SCREEN_NONE) {
				if(key == WINDOW_KEY_CHANGETEAM) {
					screen_current = SCREEN_TEAM_SELECT;
					return;
				}
				if(key == WINDOW_KEY_CHANGEWEAPON) {
					screen_current = SCREEN_GUN_SELECT;
					return;
				}
			}

			if(screen_current == SCREEN_TEAM_SELECT) {
				int new_team = 256;
				switch(key) {
					case WINDOW_KEY_TOOL1: new_team = TEAM_1; break;
					case WINDOW_KEY_TOOL2: new_team = TEAM_2; break;
					case WINDOW_KEY_TOOL3: new_team = TEAM_SPECTATOR; break;
				}
				if(new_team <= 255) {
					if(network_logged_in) {
						struct PacketChangeTeam p;
						p.player_id = local_player_id;
						p.team = new_team;
						network_send(PACKET_CHANGETEAM_ID, &p, sizeof(p));
						screen_current = SCREEN_NONE;
						return;
					} else {
						local_player_newteam = new_team;
						if(new_team == TEAM_SPECTATOR) {
							struct PacketExistingPlayer login;
							login.player_id = local_player_id;
							login.team = local_player_newteam;
							login.weapon = WEAPON_RIFLE;
							login.held_item = TOOL_GUN;
							login.kills = 0;
							login.blue = players[local_player_id].block.blue;
							login.green = players[local_player_id].block.green;
							login.red = players[local_player_id].block.red;
							strcpy(login.name, settings.name);
							network_send(PACKET_EXISTINGPLAYER_ID, &login,
										 sizeof(login) - sizeof(login.name) + strlen(settings.name) + 1);
							screen_current = SCREEN_NONE;
						} else {
							screen_current = SCREEN_GUN_SELECT;
						}
						return;
					}
				}
				if((key == WINDOW_KEY_CHANGETEAM || key == WINDOW_KEY_ESCAPE)
				   && (!network_connected || (network_connected && network_logged_in))) {
					screen_current = SCREEN_NONE;
					return;
				}
			}
			if(screen_current == SCREEN_GUN_SELECT) {
				int new_gun = 255;
				switch(key) {
					case WINDOW_KEY_TOOL1: new_gun = WEAPON_RIFLE; break;
					case WINDOW_KEY_TOOL2: new_gun = WEAPON_SMG; break;
					case WINDOW_KEY_TOOL3: new_gun = WEAPON_SHOTGUN; break;
				}
				if(new_gun < 255) {
					if(network_logged_in) {
						struct PacketChangeWeapon p;
						p.player_id = local_player_id;
						p.weapon = new_gun;
						network_send(PACKET_CHANGEWEAPON_ID, &p, sizeof(p));
					} else {
						struct PacketExistingPlayer login;
						login.player_id = local_player_id;
						login.team = local_player_newteam;
						login.weapon = new_gun;
						login.held_item = TOOL_GUN;
						login.kills = 0;
						login.blue = players[local_player_id].block.blue;
						login.green = players[local_player_id].block.green;
						login.red = players[local_player_id].block.red;
						strcpy(login.name, settings.name);
						network_send(PACKET_EXISTINGPLAYER_ID, &login,
									 sizeof(login) - sizeof(login.name) + strlen(settings.name) + 1);
					}
					screen_current = SCREEN_NONE;
					return;
				}
				if((key == WINDOW_KEY_CHANGEWEAPON || key == WINDOW_KEY_ESCAPE)
				   && (!network_connected || (network_connected && network_logged_in))) {
					screen_current = SCREEN_NONE;
					return;
				}
			}

			if(key == WINDOW_KEY_ESCAPE) {
				show_exit ^= 1;
				window_mousemode(show_exit ? WINDOW_CURSOR_ENABLED : WINDOW_CURSOR_DISABLED);
				return;
			}

			if(key == WINDOW_KEY_PICKCOLOR && players[local_player_id].held_item == TOOL_BLOCK) {
				players[local_player_id].item_disabled = window_time();
				players[local_player_id].items_show_start = window_time();
				players[local_player_id].items_show = 1;

				struct Camera_HitType hit;
				camera_hit_fromplayer(&hit, local_player_id, 128.0F);

				switch(hit.type) {
					case CAMERA_HITTYPE_BLOCK:
						players[local_player_id].block.packed = map_get(hit.x, hit.y, hit.z);
						float dmg = (100.0F - map_damage_get(hit.x, hit.y, hit.z)) / 100.0F * 0.75F + 0.25F;
						players[local_player_id].block.red *= dmg;
						players[local_player_id].block.green *= dmg;
						players[local_player_id].block.blue *= dmg;
						break;
					case CAMERA_HITTYPE_PLAYER:
						players[local_player_id].block.packed = players[hit.player_id].block.packed;
						break;
					case CAMERA_HITTYPE_NONE:
					default:
						players[local_player_id].block.red = fog_color[0] * 255;
						players[local_player_id].block.green = fog_color[1] * 255;
						players[local_player_id].block.blue = fog_color[2] * 255;
						break;
				}
				network_updateColor();
			}
		}
	} else {
		if(action != WINDOW_RELEASE) {
			if(key == WINDOW_KEY_V && mods) {
				const char* clipboard = window_clipboard();
				if(clipboard)
					strcat(chat[0][0], clipboard);
			}
			if(key == WINDOW_KEY_ESCAPE || key == WINDOW_KEY_ENTER) {
				if(key == WINDOW_KEY_ENTER && strlen(chat[0][0]) > 0) {
					struct PacketChatMessage msg;
					msg.player_id = local_player_id;
					msg.chat_type = (chat_input_mode == CHAT_ALL_INPUT) ? CHAT_ALL : CHAT_TEAM;
					strcpy(msg.message, chat[0][0]);
					network_send(PACKET_CHATMESSAGE_ID, &msg,
								 sizeof(msg) - sizeof(msg.message) + strlen(chat[0][0]) + 1);
				}
				window_textinput(0);
				chat_input_mode = CHAT_NO_INPUT;
			}
			if(key == WINDOW_KEY_BACKSPACE) {
				size_t text_len = strlen(chat[0][0]);
				if(text_len > 0) {
					chat[0][0][text_len - 1] = 0;
				}
			}
		}
	}
}

static void hud_ingame_touch(void* finger, int action, float x, float y, float dx, float dy) {
	window_setmouseloc(x, y);
	struct window_finger* f = (struct window_finger*)finger;

	if(action != TOUCH_MOVE) {
		int k = 0;
		while(hud_ingame_onscreencontrol(k, NULL, -1)) {
			if(is_inside_centered(f->start.x, settings.window_height - f->start.y,
								  settings.window_height * (0.2F + 0.175F * k), settings.window_height * 0.96F,
								  settings.window_height * 0.15F, settings.window_height * 0.1F)) {
				hud_ingame_onscreencontrol(k, NULL, (action == TOUCH_DOWN) ? 1 : 0);
				return;
			}
			k++;
		}
		if(is_inside_centered(f->start.x, settings.window_height - f->start.y,
							  settings.window_width - settings.window_height * 0.075F, settings.window_height * 0.6F,
							  settings.window_height * 0.15F, settings.window_height * 0.1F)) {
			hud_ingame_onscreencontrol(64, NULL, (action == TOUCH_DOWN) ? 1 : 0);
			return;
		}
		if(is_inside_centered(f->start.x, settings.window_height - f->start.y,
							  settings.window_width - settings.window_height * 0.075F, settings.window_height * 0.45F,
							  settings.window_height * 0.15F, settings.window_height * 0.1F)) {
			hud_ingame_onscreencontrol(65, NULL, (action == TOUCH_DOWN) ? 1 : 0);
			return;
		}
	}

	if(screen_current == SCREEN_TEAM_SELECT && action == TOUCH_UP) {
		if(x < settings.window_width / 3)
			hud_ingame_keyboard(WINDOW_KEY_TOOL1, WINDOW_PRESS, 0, 0);
		if(x > settings.window_width / 3 * 2)
			hud_ingame_keyboard(WINDOW_KEY_TOOL2, WINDOW_PRESS, 0, 0);
		if(x > settings.window_width / 3 && x < settings.window_width / 3 * 2)
			hud_ingame_keyboard(WINDOW_KEY_TOOL3, WINDOW_PRESS, 0, 0);
		return;
	}
	if(screen_current == SCREEN_GUN_SELECT && action == TOUCH_UP) {
		if(x < settings.window_width / 3)
			hud_ingame_keyboard(WINDOW_KEY_TOOL1, WINDOW_PRESS, 0, 0);
		if(x > settings.window_width / 3 * 2)
			hud_ingame_keyboard(WINDOW_KEY_TOOL3, WINDOW_PRESS, 0, 0);
		if(x > settings.window_width / 3 && x < settings.window_width / 3 * 2)
			hud_ingame_keyboard(WINDOW_KEY_TOOL2, WINDOW_PRESS, 0, 0);
		return;
	}
	if(screen_current == SCREEN_NONE) {
		if(action == TOUCH_DOWN && x > settings.window_width - settings.window_height * 0.25F
		   && y < settings.window_height * 0.25F) {
			window_pressed_keys[WINDOW_KEY_MAP] = !window_pressed_keys[WINDOW_KEY_MAP];
			return;
		}
		if((camera_mode == CAMERAMODE_FPS || camera_mode == CAMERAMODE_SPECTATOR)
		   && distance2D(f->start.x, f->start.y, settings.window_height * 0.3F, settings.window_height * 0.7F)
			   < pow(settings.window_height * 0.15F, 2)) {
			float mx = max(min(x - settings.window_height * 0.3F, settings.window_height * 0.2F),
						   -settings.window_height * 0.2F);
			float my = max(min(y - settings.window_height * 0.7F, settings.window_height * 0.2F),
						   -settings.window_height * 0.2F);
			hud_ingame_touch_x = mx;
			hud_ingame_touch_y = -my;
			if(absf(mx) > settings.window_height * 0.045F) {
				window_pressed_keys[WINDOW_KEY_LEFT] = mx < 0;
				window_pressed_keys[WINDOW_KEY_RIGHT] = mx > 0;
			} else {
				window_pressed_keys[WINDOW_KEY_LEFT] = 0;
				window_pressed_keys[WINDOW_KEY_RIGHT] = 0;
			}
			if(absf(my) > settings.window_height * 0.045F) {
				window_pressed_keys[WINDOW_KEY_UP] = my < 0;
				window_pressed_keys[WINDOW_KEY_DOWN] = my > 0;
			} else {
				window_pressed_keys[WINDOW_KEY_UP] = 0;
				window_pressed_keys[WINDOW_KEY_DOWN] = 0;
			}
			// window_pressed_keys[WINDOW_KEY_CROUCH] = (window_time()-f->down_time)>0.25F &&
			// absf(mx)<settings.window_height*0.06F && absf(my)<settings.window_height*0.06F;
			window_pressed_keys[WINDOW_KEY_SPRINT]
				= absf(mx) > settings.window_height * 0.19F || absf(my) > settings.window_height * 0.19F;
			if(action == TOUCH_UP) {
				window_pressed_keys[WINDOW_KEY_LEFT] = 0;
				window_pressed_keys[WINDOW_KEY_RIGHT] = 0;
				window_pressed_keys[WINDOW_KEY_UP] = 0;
				window_pressed_keys[WINDOW_KEY_DOWN] = 0;
				window_pressed_keys[WINDOW_KEY_SPRINT] = 0;
				// window_pressed_keys[WINDOW_KEY_CROUCH] = 0;
				hud_ingame_touch_x = 0;
				hud_ingame_touch_y = 0;
			}
			return;
		}
		if(camera_mode == CAMERAMODE_BODYVIEW && action == TOUCH_UP) {
			if(x < settings.window_width / 2)
				hud_ingame_mouseclick(0, 0, WINDOW_MOUSE_LMB, WINDOW_PRESS, 0);
			if(x > settings.window_width / 2)
				hud_ingame_mouseclick(0, 0, WINDOW_MOUSE_RMB, WINDOW_PRESS, 0);
			return;
		}
		if(1) {
			camera_rot_x -= dx * 0.002F;
			camera_rot_y += dy * 0.002F;
			camera_overflow_adjust();
			return;
		}
	}
}

struct hud hud_ingame = {
	hud_ingame_init,
	hud_ingame_render3D,
	hud_ingame_render,
	hud_ingame_keyboard,
	hud_ingame_mouselocation,
	hud_ingame_mouseclick,
	hud_ingame_scroll,
	hud_ingame_touch,
	1,
	0,
};

/*         HUD_SERVERLIST START        */

static http_t* request_serverlist = NULL;
static http_t* request_version = NULL;
static http_t* request_news = NULL;
static int server_count = 0;
static int player_count = 0;
static struct serverlist_entry* serverlist;
static float serverlist_scroll;
static float serverlist_news_scroll;
static int serverlist_hover;
static int serverlist_is_outdated;
static int serverlist_con_established;
static pthread_mutex_t serverlist_lock;
static int hud_serverlist_drag = 0;

static struct serverlist_news_entry {
	struct texture image;
	char caption[33];
	char url[129];
	float tile_size;
	int color;
	struct serverlist_news_entry* next;
} serverlist_news;

static struct serverlist_news_entry* serverlist_news_hover;
static int serverlist_news_exists = 0;

static void hud_serverlist_init() {
	ping_stop();
	network_disconnect();
	window_title(NULL);
	rpc_seti(RPC_VALUE_SLOTS, 0);

	window_mousemode(WINDOW_CURSOR_ENABLED);

	hud_serverlist_drag = 0;
	player_count = 0;
	server_count = 0;
	serverlist_scroll = 0.0F;
	serverlist_news_scroll = 0.0F;
	serverlist_news_hover = NULL;
	serverlist_hover = -1;
	request_serverlist = http_get("http://services.buildandshoot.com/serverlist.json", NULL);
	request_version = http_get("http://aos.party/bs/version/", NULL);
	if(!serverlist_news_exists)
		request_news = http_get("http://aos.party/bs/news/", NULL);

	chat_input_mode = CHAT_ALL_INPUT;
	chat[0][0][0] = 0;
	serverlist_is_outdated = 0;
	serverlist_con_established = request_serverlist != NULL;

	pthread_mutex_init(&serverlist_lock, NULL);
	window_textinput(1);
}

static int hud_serverlist_sort(const void* a, const void* b) {
	struct serverlist_entry* aa = (struct serverlist_entry*)a;
	struct serverlist_entry* bb = (struct serverlist_entry*)b;

	if(strcmp(aa->country, "LAN") == 0) {
		return -1;
	}
	if(strcmp(bb->country, "LAN") == 0) {
		return 1;
	}

	if(abs(aa->current - bb->current) == 0)
		if(abs(aa->ping - bb->ping) == 0)
			return strcmp(aa->name, bb->name);
		else
			return aa->ping - bb->ping;

	return bb->current - aa->current;
}

static void hud_serverlist_pingupdate(void* e, float time_delta, void* user_data) {
	pthread_mutex_lock(&serverlist_lock);
	if(!e) {
		for(int k = 0; k < server_count; k++)
			if(strcmp(serverlist[k].identifier, user_data) == 0) {
				serverlist[k].ping = ceil(time_delta * 1000.0F);
				break;
			}
	} else {
		serverlist = realloc(serverlist, (++server_count) * sizeof(struct serverlist_entry));
		memcpy(&serverlist[server_count - 1], e, sizeof(struct serverlist_entry));
	}
	qsort(serverlist, server_count, sizeof(struct serverlist_entry), hud_serverlist_sort);
	pthread_mutex_unlock(&serverlist_lock);
}

static void hud_serverlist_pingcomplete() {
	pthread_mutex_lock(&serverlist_lock);
	qsort(serverlist, server_count, sizeof(struct serverlist_entry), hud_serverlist_sort);
	pthread_mutex_unlock(&serverlist_lock);
}

static int render_tooltip(char* str, float x, float y, float scaley) {
	if(*str) {
		glColor3f(1.0F, 1.0F, 1.0F);
		texture_draw_empty(x + 5 * scaley, settings.window_height - y + 5 * scaley,
						   font_length(16 * scaley, str) + 10 * scaley, 26 * scaley);
		glColor3f(0.0F, 0.0F, 0.0F);
		font_render(x + 10 * scaley, settings.window_height - y, 16 * scaley, str);
	}
}

static float hud_serverlist_news_height(float scaley) {
	return serverlist_news_exists ? 150 * scaley : 0.0F;
}

static float hud_serverlist_news_width() {
	float width = 0.0F;
	if(serverlist_news_exists) {
		struct serverlist_news_entry* current = &serverlist_news;
		while(current) {
			width += current->tile_size * 128 + 10;
			current = current->next;
		}
	}
	return width;
}

static void hud_serverlist_render(float scalex, float scaley) {
	glColor3f(0.5F, 0.5F, 0.5F);
	float t = window_time() * 0.03125F;
	texture_draw_sector(&texture_ui_bg, 0.0F, settings.window_height, settings.window_width, settings.window_height, t,
						t, settings.window_width / 512.0F, settings.window_height / 512.0F);

	glColor4f(0.0F, 0.0F, 0.0F, 0.66F);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	texture_draw_empty((settings.window_width - 640 * scaley) / 2.0F, 550 * scaley, 640 * scaley, 600 * scaley);
	glDisable(GL_BLEND);

	glColor3f(1.0F, 1.0F, 0.0F);
	font_render((settings.window_width - 600 * scaley) / 2.0F + 0 * scaley, 535 * scaley, 36 * scaley, "Server list");
	glColor3f(0.5F, 0.5F, 0.5F);
	font_centered((settings.window_width - 600 * scaley) / 2.0F + 250 * scaley, 535 * scaley - 12 * scaley, 20 * scaley,
				  "Settings");
	font_centered((settings.window_width - 600 * scaley) / 2.0F + 250 * scaley + 90 * scaley,
				  535 * scaley - 12 * scaley, 20 * scaley, "Controls");

	glColor3f(1.0F, 1.0F, 0.0F);
	char total_str[128];
	sprintf(total_str, "%i players", player_count);
	font_render(settings.window_width / 2.0F + 300 * scaley - font_length(36 * scaley, total_str), 535 * scaley,
				36 * scaley, total_str);
	sprintf(total_str, "on %i servers", server_count);
	font_render(settings.window_width / 2.0F + 300 * scaley - font_length(18 * scaley, total_str), (535 - 36) * scaley,
				18 * scaley, total_str);

	glColor3f(1.0F, 1.0F, 1.0F);
	texture_draw_sector(&texture_ui_input, settings.window_width / 2.0F - 300 * scaley, 485 * scaley, 8 * scaley,
						32 * scaley, 0.0F, 0.0F, 0.25F, 1.0F);
	texture_draw_sector(&texture_ui_input, settings.window_width / 2.0F - 300 * scaley + 8 * scaley, 485 * scaley,
						(400 - 32) * scaley, 32 * scaley, 0.25F, 0.0F, 0.5F, 1.0F);
	texture_draw_sector(&texture_ui_input, settings.window_width / 2.0F - 300 * scaley + (400 - 32 + 8) * scaley,
						485 * scaley, 8 * scaley, 32 * scaley, 0.75F, 0.0F, 0.25F, 1.0F);

	texture_draw(&texture_ui_join, settings.window_width / 2.0F + 90 * scaley, 485 * scaley, 32 * scaley, 32 * scaley);
	texture_draw(&texture_ui_reload, settings.window_width / 2.0F + 130 * scaley, 485 * scaley, 32 * scaley,
				 32 * scaley);

	int a = strlen(chat[0][0]);
	chat[0][0][a] = '_';
	chat[0][0][a + 1] = 0;
	font_render((settings.window_width - 600 * scaley) / 2.0F + font_length(24 * scaley, " "), 481 * scaley,
				24 * scaley, chat[0][0]);
	chat[0][0][a] = 0;

	font_render((settings.window_width - 600 * scaley) / 2.0F + 0 * scaley, 450 * scaley, 18 * scaley, "Players");
	font_render((settings.window_width - 600 * scaley) / 2.0F + 75 * scaley, 450 * scaley, 18 * scaley, "Name");
	font_render((settings.window_width - 600 * scaley) / 2.0F + 335 * scaley, 450 * scaley, 18 * scaley, "Map");
	font_render((settings.window_width - 600 * scaley) / 2.0F + 490 * scaley, 450 * scaley, 18 * scaley, "Mode");
	font_render((settings.window_width - 600 * scaley) / 2.0F + 560 * scaley, 450 * scaley, 18 * scaley, "Ping");

	if(serverlist_scroll > 0)
		serverlist_scroll = 0;
	if(serverlist_scroll < -(server_count * 20 - 430 + 50) * scaley - hud_serverlist_news_height(scaley))
		serverlist_scroll = -(server_count * 20 - 430 + 50) * scaley - hud_serverlist_news_height(scaley);

	float progress
		= serverlist_scroll / (-(server_count * 20 - 430 + 50) * scaley - hud_serverlist_news_height(scaley));
	texture_draw_empty((settings.window_width - 600 * scaley) / 2.0F - 20 * scaley,
					   450 * scaley - (430 - 50) * scaley * progress, 10 * scaley, 20 * scaley);

	glEnable(GL_SCISSOR_TEST);
	glScissor((settings.window_width - 600 * scaley) / 2.0F, 50 * scaley, 600 * scaley, (430 - 50) * scaley);

	double xpos, ypos;
	window_mouseloc(&xpos, &ypos);
	ypos = settings.window_height - ypos;

	if(serverlist_news_exists) {
		struct serverlist_news_entry* current = &serverlist_news;
		float news_offset = (settings.window_width - 600 * scaley) / 2.0F + serverlist_news_scroll * scaley;
		serverlist_news_hover = NULL;
		while(current) {
			float width = current->tile_size * 128 * scaley;
			glColor3f(1.0F, 1.0F, 1.0F);
			texture_draw(&current->image, news_offset, 420 * scaley - serverlist_scroll, width, 128 * scaley);

			int lines = 1;
			for(int k = 0; k < strlen(current->caption); k++)
				if(current->caption[k] == '\n')
					lines++;

			glColor4f(0.0F, 0.0F, 0.0F, 0.5F);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			texture_draw_empty(news_offset, 420 * scaley - serverlist_scroll - (128 - 6 - 18 * lines) * scaley, width,
							   (18 * lines + 6) * scaley);
			glDisable(GL_BLEND);

			char* line = strtok(current->caption, "\n");
			for(int k = 0; line; k++) {
				if(k == 0)
					glColor3ub(red(current->color), green(current->color), blue(current->color));
				else
					glColor3f(1.0F, 1.0F, 1.0F);
				font_render(news_offset + 8 * scaley,
							420 * scaley - serverlist_scroll - (128 - 18 * (lines - k) - 4) * scaley, 18 * scaley,
							line);
				char* line_old = line;
				line = strtok(NULL, "\n");
				// repair string after usage of strtok
				if(line)
					line_old[strlen(line_old)] = '\n';
			}

			if(is_inside(xpos, ypos, news_offset, 420 * scaley - serverlist_scroll - 128 * scaley, width, 128 * scaley))
				serverlist_news_hover = current;

			news_offset += width + 10 * scaley;
			current = current->next;
		}

		glColor3f(1.0F, 1.0F, 1.0F);
		texture_draw_rotated(&texture_ui_arrow2, (settings.window_width - 600 * scaley) / 2.0F + 16 * scaley,
							 420 * scaley - serverlist_scroll - 64 * scaley, 32 * scaley, 32 * scaley, 0.0F);
		texture_draw_rotated(&texture_ui_arrow2, (settings.window_width - 600 * scaley) / 2.0F + (600 - 32) * scaley,
							 420 * scaley - serverlist_scroll - 64 * scaley, 32 * scaley, 32 * scaley, PI);
	}

	serverlist_scroll += hud_serverlist_news_height(scaley);

	int tmp = -1;
	for(int k = 0; k < server_count; k++) {
		if(xpos >= (settings.window_width - 600 * scaley) / 2.0F && xpos < (settings.window_width + 600 * scaley) / 2.0F
		   && ypos < 450 * scaley - 20 * scaley * (k + 0.9F) - serverlist_scroll
		   && ypos >= 450 * scaley - 20 * scaley * (k + 1.9F) - serverlist_scroll && ypos < 430 * scaley
		   && ypos >= 50 * scaley) {
			glColor4f(1.0F, 1.0F, 1.0F, 0.5F);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			texture_draw_empty((settings.window_width - 600 * scaley) / 2.0F,
							   450 * scaley - 20 * scaley * (k + 0.9F) - serverlist_scroll, 600 * scaley, 20 * scaley);
			glDisable(GL_BLEND);
			tmp = k;
		}

		pthread_mutex_lock(&serverlist_lock);

		float f = ((serverlist[k].current && serverlist[k].current < serverlist[k].max) || tmp == k
				   || serverlist[k].current < 0) ?
			1.0F :
			0.5F;
		glColor3f(f, f, f);

		if(serverlist[k].current >= 0)
			sprintf(total_str, "%i/%i", serverlist[k].current, serverlist[k].max);
		else
			strcpy(total_str, "-");
		font_render((settings.window_width - 600 * scaley) / 2.0F + 0 * scaley,
					450 * scaley - 20 * scaley * (k + 1) - serverlist_scroll, 16 * scaley, total_str);
		font_render((settings.window_width - 600 * scaley) / 2.0F + 75 * scaley,
					450 * scaley - 20 * scaley * (k + 1) - serverlist_scroll, 16 * scaley, serverlist[k].name);
		font_render((settings.window_width - 600 * scaley) / 2.0F + 335 * scaley,
					450 * scaley - 20 * scaley * (k + 1) - serverlist_scroll, 16 * scaley, serverlist[k].map);
		font_render((settings.window_width - 600 * scaley) / 2.0F + 490 * scaley,
					450 * scaley - 20 * scaley * (k + 1) - serverlist_scroll, 16 * scaley, serverlist[k].gamemode);

		float u, v;
		texture_flag_offset(serverlist[k].country, &u, &v);
		texture_draw_sector(&texture_ui_flags, (settings.window_width - 600 * scaley) / 2.0F + 55 * scaley,
							448 * scaley - 20 * scaley * (k + 1) - serverlist_scroll, 18 * scaley, 12 * scaley, u, v,
							18.0F / 256.0F, 12.0F / 256.0F);

		if(serverlist[k].ping < 110)
			glColor3f(0.0F, 1.0F * f, 0.0F);
		else if(serverlist[k].ping < 200)
			glColor3f(1.0F * f, 1.0F * f, 0.0F);
		else
			glColor3f(1.0F * f, 0.0F, 0.0F);

		sprintf(total_str, "%ims", serverlist[k].ping);
		font_render((settings.window_width - 600 * scaley) / 2.0F + 560 * scaley,
					450 * scaley - 20 * scaley * (k + 1) - serverlist_scroll, 16 * scaley,
					(serverlist[k].ping >= 0) ? total_str : "?");
		glColor3f(1.0F, 1.0F, 1.0F);

		pthread_mutex_unlock(&serverlist_lock);
	}
	serverlist_hover = tmp;

	serverlist_scroll -= hud_serverlist_news_height(scaley);

	glDisable(GL_SCISSOR_TEST);

	if(serverlist_hover >= 0) {
		pthread_mutex_lock(&serverlist_lock);
		if(serverlist[serverlist_hover].ping < 0) {
			render_tooltip("Server unreachable!", xpos, settings.window_height - ypos, scaley);
		} else {
			if(serverlist[serverlist_hover].current >= serverlist[serverlist_hover].max) {
				render_tooltip("Server full!", xpos, settings.window_height - ypos, scaley);
			} else {
				if(serverlist[serverlist_hover].current == 0) {
					render_tooltip("Server empty!", xpos, settings.window_height - ypos, scaley);
				}
			}
		}
		pthread_mutex_unlock(&serverlist_lock);
	}

	if(serverlist_is_outdated) {
		glColor4f(0.0F, 0.0F, 0.0F, 0.9F);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		texture_draw_empty((settings.window_width - 350 * scaley) / 2.0F,
						   (settings.window_height - 200 * scaley) / 2.0F + 200 * scaley, 350 * scaley, 200 * scaley);
		glDisable(GL_BLEND);
		glColor3f(1.0F, 1.0F, 0.0F);
		font_centered(settings.window_width / 2.0F, (settings.window_height - 200 * scaley) / 2.0F + 150 * scaley,
					  22 * scaley, "NEW CLIENT VERSION AVAILABLE!");
		glColor3f(1.0F, 1.0F, 1.0F);
		font_centered(settings.window_width / 2.0F,
					  (settings.window_height - 200 * scaley) / 2.0F + (100 - 22) * scaley, 16 * scaley,
					  "Your game is outdated and should be");
		font_centered(settings.window_width / 2.0F,
					  (settings.window_height - 200 * scaley) / 2.0F + (100 - 22 - 16) * scaley, 16 * scaley,
					  "updated immediately.");
		font_centered(settings.window_width / 2.0F,
					  (settings.window_height - 200 * scaley) / 2.0F + (100 - 22 - 32) * scaley, 16 * scaley,
					  "Head over to https://aos.party/bs or click");
	}

	if(window_time() - chat_popup_timer < chat_popup_duration) {
		float fade = 1.0F - (window_time() - chat_popup_timer) / chat_popup_duration;
		glColor4f(1.0F, 0.0F, 0.0F, (fade > 0.25F) ? 0.9F : fade / 0.25F * 0.9F);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		texture_draw_empty((settings.window_width - 350 * scaley) / 2.0F,
						   (settings.window_height - 100 * scaley) / 2.0F + 100 * scaley, 350 * scaley, 100 * scaley);
		glDisable(GL_BLEND);
		glColor4f(1.0F, 1.0F, 1.0F, (fade > 0.25F) ? 1.0F : fade / 0.25F);
		char reason_str[32];
		sprintf(reason_str, "Reason: %s.", chat_popup);
		font_centered(settings.window_width / 2.0F, (settings.window_height - 100 * scaley) / 2.0F + 80 * scaley,
					  22 * scaley, "Disconnected from server");
		font_centered(settings.window_width / 2.0F, (settings.window_height - 100 * scaley) / 2.0F + (80 - 40) * scaley,
					  22 * scaley, reason_str);
	}

	if(request_news) {
		switch(http_process(request_news)) {
			case HTTP_STATUS_COMPLETED: {
				JSON_Value* js = json_parse_string(request_news->response_data);
				JSON_Array* news = json_value_get_array(js);
				int news_entries = json_array_get_count(news);

				struct serverlist_news_entry* current = &serverlist_news;
				memset(current, 0, sizeof(struct serverlist_news_entry));

				for(int k = 0; k < news_entries; k++) {
					JSON_Object* s = json_array_get_object(news, k);
					if(json_object_get_string(s, "caption"))
						strncpy(current->caption, json_object_get_string(s, "caption"), sizeof(current->caption) - 1);
					if(json_object_get_string(s, "url"))
						strncpy(current->url, json_object_get_string(s, "url"), sizeof(current->url) - 1);
					current->tile_size = json_object_get_number(s, "tilesize");
					current->color = json_object_get_number(s, "color");
					if(json_object_get_string(s, "image")) {
						char* img = (char*)json_object_get_string(s, "image");
						int size = base64_decode(img, strlen(img));
						unsigned char* buffer;
						int width, height;
						lodepng_decode32(&buffer, &width, &height, img, size);
						texture_create_buffer(&current->image, width, height, buffer, 1);
					}
					current->next = (k < news_entries - 1) ? malloc(sizeof(struct serverlist_news_entry)) : NULL;
					current = current->next;
				}

				json_value_free(js);
				http_release(request_news);
				serverlist_news_exists = 1;
				request_news = NULL;
				break;
			}
			case HTTP_STATUS_FAILED:
				http_release(request_news);
				request_news = NULL;
				break;
		}
	}

	if(request_version) {
		switch(http_process(request_version)) {
			case HTTP_STATUS_COMPLETED:
				serverlist_is_outdated = 1;
				log_info("newest game version: %s", request_version->response_data);
				log_info("current game version: %s", BETTERSPADES_VERSION);
				serverlist_is_outdated = strcmp(request_version->response_data, BETTERSPADES_VERSION) != 0;
				http_release(request_version);
				request_version = NULL;
				break;
			case HTTP_STATUS_FAILED:
				http_release(request_version);
				request_version = NULL;
				break;
		}
	}

	int render_status_icon = !serverlist_con_established;
	if(request_serverlist) {
		switch(http_process(request_serverlist)) {
			case HTTP_STATUS_PENDING: render_status_icon = 1; break;
			case HTTP_STATUS_COMPLETED: {
				JSON_Value* js = json_parse_string(request_serverlist->response_data);
				JSON_Array* servers = json_value_get_array(js);
				server_count = json_array_get_count(servers);

				pthread_mutex_lock(&serverlist_lock);
				serverlist = realloc(serverlist, server_count * sizeof(struct serverlist_entry));
				CHECK_ALLOCATION_ERROR(serverlist)

				ping_stop();

				player_count = 0;
				for(int k = 0; k < server_count; k++) {
					JSON_Object* s = json_array_get_object(servers, k);
					memset(&serverlist[k], 0, sizeof(struct serverlist_entry));

					serverlist[k].current = (int)json_object_get_number(s, "players_current");
					serverlist[k].max = (int)json_object_get_number(s, "players_max");
					serverlist[k].ping = -1;

					strncpy(serverlist[k].name, json_object_get_string(s, "name"), sizeof(serverlist[k].name) - 1);
					strncpy(serverlist[k].map, json_object_get_string(s, "map"), sizeof(serverlist[k].map) - 1);
					strncpy(serverlist[k].gamemode, json_object_get_string(s, "game_mode"),
							sizeof(serverlist[k].gamemode) - 1);
					strncpy(serverlist[k].identifier, json_object_get_string(s, "identifier"),
							sizeof(serverlist[k].identifier) - 1);
					strncpy(serverlist[k].country, json_object_get_string(s, "country"),
							sizeof(serverlist[k].country) - 1);

					int port;
					char ip[32];
					if(network_identifier_split(serverlist[k].identifier, ip, &port))
						ping_check(ip, port, serverlist[k].identifier);

					player_count += serverlist[k].current;
				}

				ping_start(hud_serverlist_pingcomplete, hud_serverlist_pingupdate);

				qsort(serverlist, server_count, sizeof(struct serverlist_entry), hud_serverlist_sort);
				pthread_mutex_unlock(&serverlist_lock);

				http_release(request_serverlist);
				json_value_free(js);
				request_serverlist = NULL;
				break;
			}
			case HTTP_STATUS_FAILED:
				http_release(request_serverlist);
				hud_serverlist_init();
				break;
		}
	}

	double x, y;
	window_mouseloc(&x, &y);
	if(is_inside(x, settings.window_height - y, settings.window_width / 2.0F + 90 * scaley, (485 - 32) * scaley,
				 32 * scaley, 32 * scaley))
		render_tooltip("Join address", x, y, scaley);

	if(is_inside(x, settings.window_height - y, settings.window_width / 2.0F + 130 * scaley, (485 - 32) * scaley,
				 32 * scaley, 32 * scaley))
		render_tooltip("Refresh", x, y, scaley);

	if(render_status_icon) {
		glColor3f(1.0F, 1.0F, 1.0F);
		texture_draw_rotated(serverlist_con_established ? &texture_ui_wait : &texture_ui_alert,
							 settings.window_width / 2.0F, settings.window_height / 2.0F, 48 * scaley, 48 * scaley,
							 serverlist_con_established ? -window_time() * 5.0F : 0.0F);
		font_centered(settings.window_width / 2.0F, settings.window_height / 2.0F - 24 * scaley, 18 * scaley,
					  serverlist_con_established ? "Please wait..." : "No connection");
	}
}

static void hud_serverlist_scroll(double yoffset) {
	if(!hud_serverlist_drag) {
		double x, y;
		window_mouseloc(&x, &y);
		float scaley = settings.window_height / 600.0F;

		if(serverlist_news_exists
		   && is_inside(x, settings.window_height - y, (settings.window_width - 600 * scaley) / 2.0F,
						420 * scaley - serverlist_scroll - 128 * scaley, 600 * scaley, 128 * scaley)) {
			serverlist_news_scroll
				= max(min(serverlist_news_scroll + yoffset * 10.0F, 0.0F), -hud_serverlist_news_width() + 600);
		} else {
			serverlist_scroll += yoffset * 20.0F;
		}
	}
}

static void server_c(char* address, char* name) {
	if(file_exists(address)) {
		void* data = file_load(address);
		map_vxl_load(data, file_size(address));
		free(data);
		chunk_rebuild_all();
		camera_mode = CAMERAMODE_FPS;
		players[local_player_id].pos.x = map_size_x / 2.0F;
		players[local_player_id].pos.y = map_size_y - 1.0F;
		players[local_player_id].pos.z = map_size_z / 2.0F;
		window_title(address);
		hud_change(&hud_ingame);
	} else {
		window_title(name);
		if(name && address) {
			rpc_setv(RPC_VALUE_SERVERNAME, name);
			rpc_setv(RPC_VALUE_SERVERURL, address);
			rpc_seti(RPC_VALUE_SLOTS, 32);
		} else {
			rpc_seti(RPC_VALUE_SLOTS, 0);
		}
		hud_change(network_connect_string(address) ? &hud_ingame : &hud_serverlist);
	}
}

static void hud_serverlist_mouseclick(double x, double y, int button, int action, int mods) {
	float scaley = settings.window_height / 600.0F;

	if(action == WINDOW_PRESS) {
		if(serverlist_is_outdated) {
			serverlist_is_outdated = 0;
			file_url("https://aos.party/bs");
			return;
		}
		if(serverlist_hover >= 0) {
			pthread_mutex_lock(&serverlist_lock);
			server_c(serverlist[serverlist_hover].identifier, serverlist[serverlist_hover].name);
			pthread_mutex_unlock(&serverlist_lock);
		}

		if(x >= (settings.window_width - 600 * scaley) / 2.0F + 250 * scaley - font_length(20 * scaley, "Settings") / 2
		   && x
			   < (settings.window_width - 600 * scaley) / 2.0F + 250 * scaley + font_length(20 * scaley, "Settings") / 2
		   && y >= 77 * scaley && y < 97 * scaley) {
			hud_change(&hud_settings);
		}

		if(x >= (settings.window_width - 600 * scaley) / 2.0F + 340 * scaley - font_length(20 * scaley, "Controls") / 2
		   && x
			   < (settings.window_width - 600 * scaley) / 2.0F + 340 * scaley + font_length(20 * scaley, "Controls") / 2
		   && y >= 77 * scaley && y < 97 * scaley) {
			hud_change(&hud_controls);
		}

		// inside progress bar thingy
		float progress
			= serverlist_scroll / (-(server_count * 20 - 430 + 50) * scaley - hud_serverlist_news_height(scaley));
		if(x >= (settings.window_width - 600 * scaley) / 2.0F - 20 * scaley
		   && x <= (settings.window_width - 600 * scaley) / 2.0F - 20 * scaley + 10 * scaley
		   && y <= settings.window_height - (450 * scaley - (430 - 50) * scaley * progress - 20 * scaley)
		   && y >= settings.window_height - (450 * scaley - (430 - 50) * scaley * progress)) {
			hud_serverlist_drag = 1;
		}

		// texture_draw(&texture_ui_join,settings.window_width/2.0F+90*scaley,485*scaley,32*scaley,32*scaley);
		if(is_inside(x, settings.window_height - y, settings.window_width / 2.0F + 90 * scaley, (485 - 32) * scaley,
					 32 * scaley, 32 * scaley)
		   && strlen(chat[0][0]) > 0)
			server_c(chat[0][0], NULL);

		if(is_inside(x, settings.window_height - y, settings.window_width / 2.0F + 130 * scaley, (485 - 32) * scaley,
					 32 * scaley, 32 * scaley))
			hud_change(&hud_serverlist);

		if(serverlist_news_exists) {
			float news_offset = 0.0F;
			float prev_loc = 0.0F, next_loc = 0.0F;
			struct serverlist_news_entry* current = &serverlist_news;
			while(current) {
				if(-serverlist_news_scroll >= news_offset
				   && -serverlist_news_scroll < news_offset + current->tile_size * 128 + 10) {
					next_loc = news_offset + current->tile_size * 128 + 10;
					break;
				}

				prev_loc = news_offset;
				news_offset += current->tile_size * 128 + 10;
				current = current->next;
			}

			if(is_inside_centered(x, settings.window_height - y,
								  (settings.window_width - 600 * scaley) / 2.0F + 16 * scaley,
								  420 * scaley - serverlist_scroll - 64 * scaley, 32 * scaley, 32 * scaley)
			   && serverlist_news_scroll <= 0.0F) {
				serverlist_news_scroll = -prev_loc;
			}
			if(is_inside_centered(x, settings.window_height - y,
								  (settings.window_width - 600 * scaley) / 2.0F + (600 - 32) * scaley,
								  420 * scaley - serverlist_scroll - 64 * scaley, 32 * scaley, 32 * scaley)
			   && current->next) {
				serverlist_news_scroll = max(-next_loc, -hud_serverlist_news_width() + 600);
			}

			if(serverlist_news_hover) {
				if(!strncmp("aos://", serverlist_news_hover->url, 6))
					server_c(serverlist_news_hover->url, serverlist_news_hover->caption);
				else
					file_url(serverlist_news_hover->url);
			}
		}
	}

	if(hud_serverlist_drag && action == WINDOW_RELEASE)
		hud_serverlist_drag = 0;
}

void hud_serverlist_mouselocation(double x, double y) {
	if(hud_serverlist_drag) {
		float scaley = settings.window_height / 600.0F;
		serverlist_scroll
			= -((y - 160 * scaley) * (20 * server_count * scaley - 380 * scaley + hud_serverlist_news_height(scaley)))
			/ (380 * scaley);
		return;
	}
}

static void hud_serverlist_touch(void* finger, int action, float x, float y, float dx, float dy) {
	window_setmouseloc(x, y);
	switch(action) {
		case TOUCH_DOWN: hud_serverlist_mouseclick(x, y, WINDOW_MOUSE_LMB, WINDOW_PRESS, 0); break;
		case TOUCH_MOVE: hud_serverlist_mouselocation(x, y); break;
		case TOUCH_UP: hud_serverlist_mouseclick(x, y, WINDOW_MOUSE_LMB, WINDOW_RELEASE, 0); break;
	}
}

static void hud_serverlist_keyboard(int key, int action, int mods, int internal) {
	if(action != WINDOW_RELEASE) {
		if(!hud_serverlist_drag) {
			if(key == WINDOW_KEY_UP || key == WINDOW_KEY_CURSOR_UP)
				serverlist_scroll += 20.0F;
			if(key == WINDOW_KEY_DOWN || key == WINDOW_KEY_CURSOR_DOWN)
				serverlist_scroll -= 20.0F;
		}
		if(key == WINDOW_KEY_BACKSPACE) {
			size_t text_len = strlen(chat[0][0]);
			if(text_len > 0)
				chat[0][0][text_len - 1] = 0;
		}
		if(key == WINDOW_KEY_ENTER && strlen(chat[0][0]) > 0)
			server_c(chat[0][0], NULL);
	}
}

struct hud hud_serverlist = {
	hud_serverlist_init,
	(void*)NULL,
	hud_serverlist_render,
	hud_serverlist_keyboard,
	hud_serverlist_mouselocation,
	hud_serverlist_mouseclick,
	hud_serverlist_scroll,
	hud_serverlist_touch,
	0,
	0,
};

/*         HUD_SETTINGS START        */

static struct config_setting* hud_settings_edit = NULL;

static void hud_settings_init() {
	memcpy(&settings_tmp, &settings, sizeof(struct RENDER_OPTIONS));
	chat_input_mode = CHAT_ALL_INPUT;
	chat[0][0][0] = 0;
}

static void hud_settings_render(float scalex, float scaley) {
	glColor3f(0.5F, 0.5F, 0.5F);
	float t = window_time() * 0.03125F;
	texture_draw_sector(&texture_ui_bg, 0.0F, settings.window_height, settings.window_width, settings.window_height, t,
						t, settings.window_width / 512.0F, settings.window_height / 512.0F);

	glColor4f(0.0F, 0.0F, 0.0F, 0.66F);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	texture_draw_empty((settings.window_width - 640 * scaley) / 2.0F, 550 * scaley, 640 * scaley, 600 * scaley);
	glDisable(GL_BLEND);

	glColor3f(1.0F, 1.0F, 0.0F);
	font_render((settings.window_width - 600 * scaley) / 2.0F + 0 * scaley, 535 * scaley, 36 * scaley, "Settings");
	font_centered((settings.window_width - 600 * scaley) / 2.0F + 174 * scaley, 70 * scaley, 30 * scaley, "Apply");
#ifdef USE_SOUNDTEST
#ifdef USE_SOUND
	font_centered((settings.window_width - 600 * scaley) / 2.0F + 500 * scaley, 70 * scaley, 30 * scaley, "Soundtest");
	glColor3f(0.5F, 0.5F, 0.5F);
#endif
#endif
	font_centered((settings.window_width - 600 * scaley) / 2.0F + 210 * scaley, 535 * scaley - 12 * scaley, 20 * scaley,
				  "Controls");
	font_centered((settings.window_width - 600 * scaley) / 2.0F + 320 * scaley, 535 * scaley - 12 * scaley, 20 * scaley,
				  "Server list");

	for(int k = 0; k < list_size(&config_settings); k++) {
		struct config_setting* a = list_get(&config_settings, k);

		float row_offset = (k + 1) / 2;
		float col_offset = 0.0F;
		if(k > 0)
			col_offset = ((k + 1) & 1) ? 314 * scaley : 0;

		glColor3f(1.0F, 1.0F, 1.0F);
		font_render((settings.window_width - 600 * scaley) / 2.0F + col_offset, (460 - 40 * row_offset) * scaley,
					16 * scaley, a->name);

		char tmp[32];
		switch(a->type) {
			case CONFIG_TYPE_STRING:
				glColor3f(1.0F, 1.0F, 1.0F);
				texture_draw_sector(&texture_ui_input,
									(settings.window_width - 600 * scaley) / 2.0F + 154 * scaley + col_offset,
									(466 - 40 * row_offset) * scaley, 8 * scaley, 32 * scaley, 0.0F, 0.0F, 0.25F, 1.0F);
				texture_draw_sector(
					&texture_ui_input, (settings.window_width - 600 * scaley) / 2.0F + 162 * scaley + col_offset,
					(466 - 40 * row_offset) * scaley, 200 * scaley, 32 * scaley, 0.25F, 0.0F, 0.5F, 1.0F);
				texture_draw_sector(
					&texture_ui_input, settings.window_width / 2.0F - 300 * scaley + 362 * scaley + col_offset,
					(466 - 40 * row_offset) * scaley, 8 * scaley, 32 * scaley, 0.75F, 0.0F, 0.25F, 1.0F);
				if(hud_settings_edit == a)
					glColor3f(1.0F, 0.0F, 0.0F);
				font_render((settings.window_width - 600 * scaley) / 2.0F + 162 * scaley + col_offset,
							(460 - 40 * row_offset) * scaley, 20 * scaley,
							(hud_settings_edit == a) ? chat[0][0] : a->value);
				break;
			case CONFIG_TYPE_INT:
				glColor3f(1.0F, 1.0F, 1.0F);
				if(hud_settings_edit == a)
					strcpy(tmp, chat[0][0]);
				else
					sprintf(tmp, "%i", *(int*)a->value);
				if(a->max == 1) {
					texture_draw_rotated(*(int*)a->value ? &texture_ui_box_check : &texture_ui_box_empty,
										 (settings.window_width - 600 * scaley) / 2.0F + 220 * scaley + col_offset,
										 (450 - 40 * row_offset) * scaley, 32 * scaley, 32 * scaley, 0.0F);
				} else {
					texture_draw_rotated(&texture_ui_arrow,
										 (settings.window_width - 600 * scaley) / 2.0F + 270 * scaley + col_offset,
										 (450 - 40 * row_offset) * scaley, 32 * scaley, 32 * scaley, 0.0F);
					texture_draw_rotated(&texture_ui_arrow,
										 (settings.window_width - 600 * scaley) / 2.0F + 170 * scaley + col_offset,
										 (450 - 40 * row_offset) * scaley, 32 * scaley, 32 * scaley, PI);
					if(hud_settings_edit == a)
						glColor3f(1.0F, 0.0F, 0.0F);
					font_centered((settings.window_width - 600 * scaley) / 2.0F + 220 * scaley + col_offset,
								  (460 - 40 * row_offset) * scaley, 20 * scaley, tmp);
				}
				break;
			case CONFIG_TYPE_FLOAT:
				glColor3f(1.0F, 1.0F, 1.0F);
				texture_draw_rotated(&texture_ui_arrow,
									 (settings.window_width - 600 * scaley) / 2.0F + 270 * scaley + col_offset,
									 (450 - 40 * row_offset) * scaley, 32 * scaley, 32 * scaley, 0.0F);
				texture_draw_rotated(&texture_ui_arrow,
									 (settings.window_width - 600 * scaley) / 2.0F + 170 * scaley + col_offset,
									 (450 - 40 * row_offset) * scaley, 32 * scaley, 32 * scaley, PI);
				if(hud_settings_edit == a) {
					glColor3f(1.0F, 0.0F, 0.0F);
					strcpy(tmp, chat[0][0]);
				} else {
					sprintf(tmp, "%0.2f", *(float*)a->value);
				}
				font_centered((settings.window_width - 600 * scaley) / 2.0F + 220 * scaley + col_offset,
							  (460 - 40 * row_offset) * scaley, 20 * scaley, tmp);
				break;
		}
	}

	// now find the correct help str and draw it above everything else
	double x, y;
	window_mouseloc(&x, &y);
	for(int k = 0; k < list_size(&config_settings); k++) {
		struct config_setting* a = list_get(&config_settings, k);

		float row_offset = (k + 1) / 2;
		float col_offset = 0.0F;
		if(k > 0)
			col_offset = ((k + 1) & 1) ? 314 * scaley : 0;

		if(is_inside(x, settings.window_height - y, (settings.window_width - 600 * scaley) / 2.0F + col_offset,
					 (435 - 40 * row_offset) * scaley, 300 * scaley, 32 * scaley)) {
			render_tooltip(a->help, x, y, scaley);
			break;
		}
	}
#ifdef USE_SOUNDTEST
#ifdef USE_SOUND
	if (is_inside(x, settings.window_height - y,
				((settings.window_width - 600 - font_length(32 * scaley, "Soundtest")) * scaley) / 2.0F + 500,
				40 * scaley, font_length(32 * scaley, "Soundtest"), 30 * scaley)) {
		render_tooltip("Run soundtest", x, y, scaley);
	}
#endif
#endif
}

static int is_int(const char* x) {
	while(*x) {
		if(!(*x >= '0' && *x <= '9'))
			return 0;
		x++;
	}
	return 1;
}

static int is_float(char* x) {
	char* decimalpoint = strchr(x, '.');
	if(!decimalpoint) {
		return is_int(x);
	} else {
		*decimalpoint = 0;
		if(is_int(x)) {
			if(is_int(decimalpoint + 1)) {
				*decimalpoint = '.';
				return 1;
			} else {
				return 0;
			}
		} else {
			return 0;
		}
	}
}

static void hud_settings_keyboard(int key, int action, int mods, int internal) {
	if(hud_settings_edit && action != WINDOW_RELEASE && key == WINDOW_KEY_BACKSPACE) {
		size_t text_len = strlen(chat[0][0]);
		if(text_len > 0) {
			chat[0][0][text_len - 1] = 0;
		}
	}
}

static void hud_settings_mouseclick(double x, double y, int button, int action, int mods) {
	if(action == WINDOW_PRESS) {
		if(hud_settings_edit) {
			if(strlen(chat[0][0])) {
				switch(hud_settings_edit->type) {
					case CONFIG_TYPE_INT:
						if(is_int(chat[0][0]))
							*(int*)hud_settings_edit->value
								= max(min(strtol(chat[0][0], NULL, 10), hud_settings_edit->max), 0);
						break;
					case CONFIG_TYPE_FLOAT:
						if(is_float(chat[0][0]))
							*(float*)hud_settings_edit->value
								= max(min(strtod(chat[0][0], NULL), hud_settings_edit->max), 0);
						break;
					case CONFIG_TYPE_STRING:
						strncpy(hud_settings_edit->value, chat[0][0], hud_settings_edit->max);
						break;
				}
			}
			window_textinput(0);
			hud_settings_edit = NULL;
		}

		float scaley = settings.window_height / 600.0F;

		if(x >= (settings.window_width - 600 * scaley) / 2.0F + 320 * scaley
				   - font_length(20 * scaley, "Server list") / 2
		   && x < (settings.window_width - 600 * scaley) / 2.0F + 320 * scaley
				   + font_length(20 * scaley, "Server list") / 2
		   && y >= 77 * scaley && y < 97 * scaley) {
			hud_change(&hud_serverlist);
		}

		if(x >= (settings.window_width - 600 * scaley) / 2.0F + 210 * scaley - font_length(20 * scaley, "Controls") / 2
		   && x
			   < (settings.window_width - 600 * scaley) / 2.0F + 210 * scaley + font_length(20 * scaley, "Controls") / 2
		   && y >= 77 * scaley && y < 97 * scaley) {
			hud_change(&hud_controls);
		}

		y = settings.window_height - y;

		if(x >= (settings.window_width - 600 * scaley) / 2.0F + 174 * scaley - font_length(30 * scaley, "Apply") / 2
		   && x < (settings.window_width - 600 * scaley) / 2.0F + 174 * scaley + font_length(30 * scaley, "Apply") / 2
		   && y >= 40 * scaley && y < 70 * scaley) {
			memcpy(&settings, &settings_tmp, sizeof(struct RENDER_OPTIONS));
			window_fromsettings();
			sound_volume(settings.volume / 10.0F);
			config_save();
		}
#ifdef USE_SOUNDTEST
#ifdef USE_SOUND
		if(x >= (settings.window_width - 600 * scaley) / 2.0F + 500 * scaley - font_length(30 * scaley, "Soundtest") / 2
		   && x < (settings.window_width - 600 * scaley) / 2.0F + 500 * scaley + font_length(30 * scaley, "Soundtest") / 2
		   && y >= 40 * scaley && y < 70 * scaley) {
			log_info("SOUNDTEST");
			hud_change(&hud_soundtest);
		}
#endif
#endif

		for(int k = 0; k < list_size(&config_settings); k++) {
			struct config_setting* a = list_get(&config_settings, k);

			float row_offset = (k + 1) / 2;
			float col_offset = 0.0F;
			if(k > 0)
				col_offset = ((k + 1) & 1) ? 314 * scaley : 0;

			if(is_inside_centered(x, y, (settings.window_width - 600 * scaley) / 2.0F + 220 * scaley + col_offset,
								  (450 - 40 * row_offset) * scaley, 68 * scaley, 32 * scaley)
			   && !(a->type == CONFIG_TYPE_INT && a->max == 1)) {
				hud_settings_edit = a;
				switch(a->type) {
					case CONFIG_TYPE_INT: sprintf(chat[0][0], "%i", *(int*)a->value); break;
					case CONFIG_TYPE_FLOAT: sprintf(chat[0][0], "%0.2f", *(float*)a->value); break;
					case CONFIG_TYPE_STRING: strcpy(chat[0][0], a->value); break;
				}
				window_textinput(1);
			}
			switch(a->type) {
				case CONFIG_TYPE_INT:
					if(a->max == 1) {
						if(is_inside_centered(
							   x, y, (settings.window_width - 600 * scaley) / 2.0F + 220 * scaley + col_offset,
							   (450 - 40 * row_offset) * scaley, 32 * scaley, 32 * scaley)) { // checkbox pressed
							*(int*)a->value = !*(int*)a->value;
						}
					} else {
						if(is_inside_centered(
							   x, y, (settings.window_width - 600 * scaley) / 2.0F + 270 * scaley + col_offset,
							   (450 - 40 * row_offset) * scaley, 32 * scaley, 32 * scaley)) { // right arrow pressed
							int next = a->defaults_length ? (*(int*)a->value) : ((*(int*)a->value) + 1);
							int diff = INT_MAX;
							for(int l = 0; l < a->defaults_length; l++) {
								if(a->defaults[l] > *(int*)a->value && a->defaults[l] - *(int*)a->value < diff) {
									diff = a->defaults[l] - *(int*)a->value;
									next = a->defaults[l];
								}
							}
							*(int*)a->value = min(next, a->max);
						}
						if(is_inside_centered(
							   x, y, (settings.window_width - 600 * scaley) / 2.0F + 170 * scaley + col_offset,
							   (450 - 40 * row_offset) * scaley, 32 * scaley, 32 * scaley)) { // left arrow pressed
							int next = a->defaults_length ? (*(int*)a->value) : ((*(int*)a->value) - 1);
							int diff = INT_MAX;
							for(int l = 0; l < a->defaults_length; l++) {
								if(a->defaults[l] < *(int*)a->value && *(int*)a->value - a->defaults[l] < diff) {
									diff = *(int*)a->value - a->defaults[l];
									next = a->defaults[l];
								}
							}
							*(int*)a->value = max(next, 0);
						}
					}
					break;
				case CONFIG_TYPE_FLOAT:
					if(is_inside_centered(
						   x, y, (settings.window_width - 600 * scaley) / 2.0F + 270 * scaley + col_offset,
						   (450 - 40 * row_offset) * scaley, 32 * scaley, 32 * scaley)) { // right arrow pressed
						*(float*)a->value = min((*(float*)a->value) + 0.1F, a->max);
					}
					if(is_inside_centered(
						   x, y, (settings.window_width - 600 * scaley) / 2.0F + 170 * scaley + col_offset,
						   (450 - 40 * row_offset) * scaley, 32 * scaley, 32 * scaley)) { // left arrow pressed
						*(float*)a->value = max((*(float*)a->value) - 0.1F, 0);
					}
					break;
			}
		}
	}
}

static void hud_settings_touch(void* finger, int action, float x, float y, float dx, float dy) {
	window_setmouseloc(x, y);
	switch(action) {
		case TOUCH_DOWN: hud_settings_mouseclick(x, y, WINDOW_MOUSE_LMB, WINDOW_PRESS, 0); break;
		case TOUCH_UP: hud_settings_mouseclick(x, y, WINDOW_MOUSE_LMB, WINDOW_RELEASE, 0); break;
	}
}

struct hud hud_settings = {
	hud_settings_init,
	(void*)NULL,
	hud_settings_render,
	hud_settings_keyboard,
	(void*)NULL,
	hud_settings_mouseclick,
	(void*)NULL,
	hud_settings_touch,
	0,
	0,
};

/*         HUD_CONTROLS START        */

static struct config_key_pair* hud_controls_edit = NULL;

static void hud_controls_render(float scalex, float scaley) {
	glColor3f(0.5F, 0.5F, 0.5F);
	float t = window_time() * 0.03125F;
	texture_draw_sector(&texture_ui_bg, 0.0F, settings.window_height, settings.window_width, settings.window_height, t,
						t, settings.window_width / 512.0F, settings.window_height / 512.0F);

	glColor4f(0.0F, 0.0F, 0.0F, 0.66F);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	texture_draw_empty((settings.window_width - 640 * scaley) / 2.0F, 550 * scaley, 640 * scaley, 600 * scaley);
	glDisable(GL_BLEND);

	glColor3f(1.0F, 1.0F, 0.0F);
	font_render((settings.window_width - 600 * scaley) / 2.0F + 0 * scaley, 535 * scaley, 36 * scaley, "Controls");
	glColor3f(0.5F, 0.5F, 0.5F);
	font_centered((settings.window_width - 600 * scaley) / 2.0F + 225 * scaley, 535 * scaley - 12 * scaley, 20 * scaley,
				  "Server list");
	font_centered((settings.window_width - 600 * scaley) / 2.0F + 340 * scaley, 535 * scaley - 12 * scaley, 20 * scaley,
				  "Settings");

	int x_off = 0;
	int y_off = 0;
	for(int k = 0; k < list_size(&config_keys); k++) {
		struct config_key_pair* a = list_get(&config_keys, k);
		if(*a->display) {
			glColor3f(1.0F, 1.0F, 1.0F);
			font_render((settings.window_width - 600 * scaley) / 2.0F + 200 * x_off * scaley,
						(460 - 40 * y_off) * scaley, 16 * scaley, a->display);

			texture_draw_sector(&texture_ui_input,
								(settings.window_width - 600 * scaley) / 2.0F + (200 * x_off + 110) * scaley,
								(466 - 40 * y_off) * scaley, 8 * scaley, 32 * scaley, 0.0F, 0.0F, 0.25F, 1.0F);
			texture_draw_sector(&texture_ui_input,
								(settings.window_width - 600 * scaley) / 2.0F + (200 * x_off + 118) * scaley,
								(466 - 40 * y_off) * scaley, 64 * scaley, 32 * scaley, 0.25F, 0.0F, 0.5F, 1.0F);
			texture_draw_sector(&texture_ui_input,
								(settings.window_width - 600 * scaley) / 2.0F + (200 * x_off + 182) * scaley,
								(466 - 40 * y_off) * scaley, 8 * scaley, 32 * scaley, 0.75F, 0.0F, 0.25F, 1.0F);

			if(hud_controls_edit == a)
				glColor3f(1.0F, 0.0F, 0.0F);
			font_centered((settings.window_width - 600 * scaley) / 2.0F + (200 * x_off + 150) * scaley,
						  (460 - 40 * y_off) * scaley, 20 * scaley, window_keyname(a->def));

			y_off++;
			if(y_off > 10) {
				y_off = 0;
				x_off++;
			}
		}
	}
}

static void hud_controls_mouseclick(double x, double y, int button, int action, int mods) {
	if(action == WINDOW_PRESS) {
		if(hud_controls_edit)
			hud_controls_edit = NULL;

		float scaley = settings.window_height / 600.0F;

		if(x >= (settings.window_width - 600 * scaley) / 2.0F + 225 * scaley
				   - font_length(20 * scaley, "Server list") / 2
		   && x < (settings.window_width - 600 * scaley) / 2.0F + 225 * scaley
				   + font_length(20 * scaley, "Server list") / 2
		   && y >= 77 * scaley && y < 97 * scaley) {
			hud_change(&hud_serverlist);
		}

		if(x >= (settings.window_width - 600 * scaley) / 2.0F + 340 * scaley - font_length(20 * scaley, "Settings") / 2
		   && x
			   < (settings.window_width - 600 * scaley) / 2.0F + 340 * scaley + font_length(20 * scaley, "Settings") / 2
		   && y >= 77 * scaley && y < 97 * scaley) {
			hud_change(&hud_settings);
		}

		y = settings.window_height - y;

		int x_off = 0;
		int y_off = 0;
		for(int k = 0; k < list_size(&config_keys); k++) {
			struct config_key_pair* a = list_get(&config_keys, k);
			if(*a->display) {
				if(is_inside(x, y, (settings.window_width - 600 * scaley) / 2.0F + (200 * x_off + 110) * scaley,
							 (434 - 40 * y_off) * scaley, 80 * scaley, 32 * scaley)) {
					hud_controls_edit = a;
					break;
				}

				y_off++;
				if(y_off > 10) {
					y_off = 0;
					x_off++;
				}
			}
		}
	}
}

static void hud_controls_touch(void* finger, int action, float x, float y, float dx, float dy) {
	window_setmouseloc(x, y);
	switch(action) {
		case TOUCH_DOWN: hud_controls_mouseclick(x, y, WINDOW_MOUSE_LMB, WINDOW_PRESS, 0); break;
		case TOUCH_UP: hud_controls_mouseclick(x, y, WINDOW_MOUSE_LMB, WINDOW_RELEASE, 0); break;
	}
}

static void hud_controls_keyboard(int key, int action, int mods, int internal) {
	if(hud_controls_edit) {
		hud_controls_edit->def = internal;
		hud_controls_edit = NULL;
		config_save();
	}
}

struct hud hud_controls = {
	(void*)NULL,
	(void*)NULL,
	hud_controls_render,
	hud_controls_keyboard,
	(void*)NULL,
	hud_controls_mouseclick,
	(void*)NULL,
	hud_controls_touch,
	0,
	0,
};

#ifdef USE_SOUNDTEST
#ifdef USE_SOUND

/* HUD SOUNDTEST START */
static char tmp[32] = "0";
static void hud_soundtest_init()
{
	strcpy(tmp, "0");
	st_ctx.init(&st_ctx);
}

static void hud_soundtest_render(float scalex, float scaley)
{
	glColor3f(0.5F, 0.5F, 0.5F);
	float t = window_time() * 0.03125F;
	texture_draw_sector(&texture_ui_bg, 0.0F, settings.window_height, settings.window_width, settings.window_height, t,
						t, settings.window_width / 512.0F, settings.window_height / 512.0F);

	glColor4f(0.0F, 0.0F, 0.0F, 0.66F);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	texture_draw_empty((settings.window_width - 640 * scaley) / 2.0F, 550 * scaley, 640 * scaley, 600 * scaley);
	glDisable(GL_BLEND);

	glColor3f(1.0F, 1.0F, 0.0F);
	font_render((settings.window_width - 600 * scaley) / 2.0F + 0 * scaley, 535 * scaley, 36 * scaley, "Soundtest");
	font_centered((settings.window_width * scaley) / 2.0F * scaley, 70 * scaley, 30 * scaley, "Back");
	font_centered((settings.window_width * scaley) / 2.0F * scaley, 300 * scaley, 30 * scaley, "Play");
	
	glColor3f(1.0F, 1.0F, 1.0F);
	font_centered((settings.window_width * scaley) / 2.0F * scaley,
			(settings.window_height + 300 * scaley) / 2.0f * scaley, 30 * scaley, tmp);

	float size = 32.0F;
	float button_y = (settings.window_height + (300 - size) * scaley) / 2.0F * scaley;

	texture_draw_rotated(&texture_ui_arrow,
						 (settings.window_width + 200 * scaley) / 2.0F * scaley,
						 button_y, size * scaley, size * scaley, 0.0F);
	texture_draw_rotated(&texture_ui_arrow,
						 (settings.window_width - 200 * scaley) / 2.0F * scaley,
						 button_y, size * scaley, size * scaley, PI);

}

static void hud_soundtest_mouseclick(double x, double y, int button, int action, int mods)
{
	if (action == WINDOW_PRESS) {
		float scaley = settings.window_height / 600.0F;
		y = settings.window_height - y;

		float button_size = 32.0F;
		float button_y = (settings.window_height + (300 - button_size) * scaley) / 2.0F * scaley;

		float play_size = font_length(30 * scaley, "Play");
		float play_x = (settings.window_width * scaley) / 2.0F * scaley;
		float play_y = 300 * scaley;

		// Back button
		if(x >= (settings.window_width * scaley) / 2.0F * scaley - font_length(30 * scaley, "Back") / 2
		   && x < (settings.window_width * scaley) / 2.0F * scaley + font_length(30 * scaley, "Back") / 2
		   && y >= 40 * scaley && y < 70 * scaley) {
			st_ctx.clean(&st_ctx);
			hud_change(&hud_settings);
		}

		// Play button
		if (x >= play_x - (play_size/2.0F) && x < play_x + (play_size/2.0F)
				&& y >= play_y - (play_size/2.0F) && y < play_y) {
			st_ctx.play_current(&st_ctx);
		}


		// Arrow right
		if (x >= (settings.window_width + 200 * scaley) / 2.0F * scaley - 16
				&& x < (settings.window_width + 200 + (button_size) * scaley) / 2.0F * scaley
				&& y >= button_y - 16 && y < button_y + button_size - 16) {
			st_ctx.next_sound(&st_ctx);
			sprintf(tmp, "%d", st_ctx.current_sound);
		}

		// Arrow left
		if (x >= (settings.window_width - 200 * scaley) / 2.0F * scaley - 16
				&& x < (settings.window_width - 200 + (button_size) * scaley) / 2.0F * scaley
				&& y >= button_y - 16 && y < button_y + button_size - 16) {
			st_ctx.prev_sound(&st_ctx);
			sprintf(tmp, "%d", st_ctx.current_sound);
		}
	}
}

struct hud hud_soundtest = {
	hud_soundtest_init,
	(void*)NULL,
	hud_soundtest_render,
	(void*)NULL,
	(void*)NULL,
	hud_soundtest_mouseclick,
	(void*)NULL,
	(void*)NULL,
	0,
	0,
};
#endif
#endif
