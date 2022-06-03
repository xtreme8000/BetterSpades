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

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lodepng/lodepng.h"
#include "common.h"
#include "file.h"
#include "font.h"
#include "weapon.h"
#include "window.h"
#include "rpc.h"
#include "network.h"
#include "sound.h"
#include "map.h"
#include "particle.h"
#include "tracer.h"
#include "camera.h"
#include "cameracontroller.h"
#include "grenade.h"
#include "player.h"
#include "hud.h"
#include "config.h"
#include "log.h"
#include "ping.h"
#include "matrix.h"
#include "texture.h"
#include "chunk.h"
#include "main.h"

int fps = 0;

int ms_seed = 1;
int ms_rand() {
	ms_seed = ms_seed * 0x343FD + 0x269EC3;
	return (ms_seed >> 0x10) & 0x7FFF;
}

int chat_input_mode = CHAT_NO_INPUT;

char chat[2][10][256] = {0}; // chat[0] is current input
unsigned int chat_color[2][10];
float chat_timer[2][10];
void chat_add(int channel, unsigned int color, const char* msg) {
	for(int k = 9; k > 1; k--) {
		strcpy(chat[channel][k], chat[channel][k - 1]);
		chat_color[channel][k] = chat_color[channel][k - 1];
		chat_timer[channel][k] = chat_timer[channel][k - 1];
	}
	strcpy(chat[channel][1], msg);
	chat_color[channel][1] = color;
	chat_timer[channel][1] = window_time();
	if(channel == 0)
		log_info("%s", msg);
}
char chat_popup[256] = {};
int chat_popup_color;
float chat_popup_timer = 0.0F;
float chat_popup_duration = 0.0F;

void chat_showpopup(const char* msg, float duration, int color) {
	strcpy(chat_popup, msg);
	chat_popup_timer = window_time();
	chat_popup_duration = duration;
	chat_popup_color = color;
}

void drawScene() {
	if(settings.ambient_occlusion) {
		glShadeModel(GL_SMOOTH);
	} else {
		glShadeModel(GL_FLAT);
	}

	matrix_upload();
	chunk_draw_visible();

	if(settings.smooth_fog) {
#ifdef OPENGL_ES
		glFogx(GL_FOG_MODE, GL_EXP2);
#else
		glFogi(GL_FOG_MODE, GL_EXP2);
#endif
		glFogf(GL_FOG_DENSITY, 0.015F);
		glFogfv(GL_FOG_COLOR, fog_color);
		glEnable(GL_FOG);
	}

	glShadeModel(GL_FLAT);
	kv6_calclight(-1, -1, -1);
	matrix_upload();
	particle_render();
	tracer_render();
	grenade_render();
	map_damaged_voxels_render();
	matrix_upload();

	if(gamestate.gamemode_type == GAMEMODE_CTF) {
		if(!gamestate.gamemode.ctf.team_1_intel) {
			float x = gamestate.gamemode.ctf.team_1_intel_location.dropped.x;
			float y = 63.0F - gamestate.gamemode.ctf.team_1_intel_location.dropped.z + 1.0F;
			float z = gamestate.gamemode.ctf.team_1_intel_location.dropped.y;
			matrix_push(matrix_model);
			matrix_translate(matrix_model, x, y, z);
			kv6_calclight(x, y, z);
			matrix_upload();
			kv6_render(&model_intel, TEAM_1);
			matrix_pop(matrix_model);
		}
		if(!gamestate.gamemode.ctf.team_2_intel) {
			float x = gamestate.gamemode.ctf.team_2_intel_location.dropped.x;
			float y = 63.0F - gamestate.gamemode.ctf.team_2_intel_location.dropped.z + 1.0F;
			float z = gamestate.gamemode.ctf.team_2_intel_location.dropped.y;
			matrix_push(matrix_model);
			matrix_translate(matrix_model, x, y, z);
			kv6_calclight(x, y, z);
			matrix_upload();
			kv6_render(&model_intel, TEAM_2);
			matrix_pop(matrix_model);
		}
		if(map_object_visible(gamestate.gamemode.ctf.team_1_base.x, 63.0F - gamestate.gamemode.ctf.team_1_base.z + 1.0F,
							  gamestate.gamemode.ctf.team_1_base.y)) {
			matrix_push(matrix_model);
			matrix_translate(matrix_model, gamestate.gamemode.ctf.team_1_base.x,
							 63.0F - gamestate.gamemode.ctf.team_1_base.z + 1.0F, gamestate.gamemode.ctf.team_1_base.y);
			kv6_calclight(gamestate.gamemode.ctf.team_1_base.x, 63.0F - gamestate.gamemode.ctf.team_1_base.z + 1.0F,
						  gamestate.gamemode.ctf.team_1_base.y);
			matrix_upload();
			kv6_render(&model_tent, TEAM_1);
			matrix_pop(matrix_model);
		}
		if(map_object_visible(gamestate.gamemode.ctf.team_2_base.x, 63.0F - gamestate.gamemode.ctf.team_2_base.z + 1.0F,
							  gamestate.gamemode.ctf.team_2_base.y)) {
			matrix_push(matrix_model);
			matrix_translate(matrix_model, gamestate.gamemode.ctf.team_2_base.x,
							 63.0F - gamestate.gamemode.ctf.team_2_base.z + 1.0F, gamestate.gamemode.ctf.team_2_base.y);
			kv6_calclight(gamestate.gamemode.ctf.team_2_base.x, 63.0F - gamestate.gamemode.ctf.team_2_base.z + 1.0F,
						  gamestate.gamemode.ctf.team_2_base.y);
			matrix_upload();
			kv6_render(&model_tent, TEAM_2);
			matrix_pop(matrix_model);
		}
	}
	if(gamestate.gamemode_type == GAMEMODE_TC) {
		for(int k = 0; k < gamestate.gamemode.tc.territory_count; k++) {
			matrix_push(matrix_model);
			matrix_translate(matrix_model, gamestate.gamemode.tc.territory[k].x,
							 63.0F - gamestate.gamemode.tc.territory[k].z + 1.0F, gamestate.gamemode.tc.territory[k].y);
			kv6_calclight(gamestate.gamemode.tc.territory[k].x, 63.0F - gamestate.gamemode.tc.territory[k].z + 1.0F,
						  gamestate.gamemode.tc.territory[k].y);
			matrix_upload();
			kv6_render(&model_tent, min(gamestate.gamemode.tc.territory[k].team, 2));
			matrix_pop(matrix_model);
		}
	}
}

void display() {
	if(network_map_transfer) {
		glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
	} else {
		glClearColor(fog_color[0], fog_color[1], fog_color[2], fog_color[3]);
	}

	if(hud_active->render_world) {
		glEnable(GL_DEPTH_TEST);
		glDepthRange(0.0F, 1.0F);

		chunk_update_all();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if(settings.opengl14) {
			matrix_identity(matrix_projection);
			matrix_perspective(matrix_projection, camera_fov_scaled(),
							   ((float)settings.window_width) / ((float)settings.window_height), 0.1F,
							   settings.render_distance + CHUNK_SIZE * 4.0F);
			matrix_upload_p();

			matrix_identity(matrix_view);
			camera_apply();
			matrix_identity(matrix_model);
			matrix_upload();

			float lpos[4] = {0.0F, -1.0F, 1.0F, 0.0F};
			glLightfv(GL_LIGHT0, GL_POSITION, lpos);
		}

		camera_ExtractFrustum();

		if(!network_map_transfer) {
			glx_enable_sphericalfog();
			drawScene();

			int render_fpv = (camera_mode == CAMERAMODE_FPS)
				|| ((camera_mode == CAMERAMODE_BODYVIEW || camera_mode == CAMERAMODE_SPECTATOR)
					&& cameracontroller_bodyview_mode);
			int is_local = (camera_mode == CAMERAMODE_FPS) || (cameracontroller_bodyview_player == local_player_id);
			int local_id = (camera_mode == CAMERAMODE_FPS) ? local_player_id : cameracontroller_bodyview_player;

			if(players[local_player_id].items_show && window_time() - players[local_player_id].items_show_start >= 0.5F)
				players[local_player_id].items_show = 0;

			if(camera_mode == CAMERAMODE_FPS) {
				weapon_update();
				if(players[local_player_id].input.buttons.lmb && players[local_player_id].held_item == TOOL_BLOCK
				   && (window_time() - players[local_player_id].item_showup) >= 0.5F && local_player_blocks > 0) {
					int* pos = camera_terrain_pick(0);
					if(pos != NULL && pos[1] > 1
					   && distance3D(camera_x, camera_y, camera_z, pos[0], pos[1], pos[2]) < 5.0F * 5.0F
					   && !(pos[0] == (int)camera_x && pos[1] == (int)camera_y + 0 && pos[2] == (int)camera_z)
					   && !(pos[0] == (int)camera_x && pos[1] == (int)camera_y - 1 && pos[2] == (int)camera_z)) {
						players[local_player_id].item_showup = window_time();
						local_player_blocks = max(local_player_blocks - 1, 0);

						struct PacketBlockAction blk;
						blk.player_id = local_player_id;
						blk.action_type = ACTION_BUILD;
						blk.x = pos[0];
						blk.y = pos[2];
						blk.z = 63 - pos[1];
						network_send(PACKET_BLOCKACTION_ID, &blk, sizeof(blk));
						// read_PacketBlockAction(&blk,sizeof(blk));
					}
				}
				if(players[local_player_id].input.buttons.lmb && players[local_player_id].held_item == TOOL_GRENADE
				   && window_time() - players[local_player_id].input.buttons.lmb_start > 3.0F) {
					local_player_grenades = max(local_player_grenades - 1, 0);
					struct PacketGrenade g;
					g.player_id = local_player_id;
					g.x = players[local_player_id].pos.x;
					g.y = players[local_player_id].pos.z;
					g.z = 63.0F - players[local_player_id].pos.y;
					g.fuse_length = g.vx = g.vy = g.vz = 0.0F;
					network_send(PACKET_GRENADE_ID, &g, sizeof(g));
					read_PacketGrenade(&g, sizeof(g));
					players[local_player_id].input.buttons.lmb_start = window_time();
				}
			}

			int* pos = NULL;
			switch(players[local_id].held_item) {
				case TOOL_BLOCK:
					if(!players[local_id].input.keys.sprint && render_fpv) {
						if(is_local)
							pos = camera_terrain_pick(0);
						else
							pos = camera_terrain_pickEx(
								0, camera_x, camera_y, camera_z, players[local_id].orientation_smooth.x,
								players[local_id].orientation_smooth.y, players[local_id].orientation_smooth.z);
					}
					break;
				default: pos = NULL;
			}
			if(pos != NULL && pos[1] > 1
			   && (pow(pos[0] - camera_x, 2) + pow(pos[1] - camera_y, 2) + pow(pos[2] - camera_z, 2)) < 5 * 5) {
				matrix_upload();
				glColor3f(1.0F, 0.0F, 0.0F);
				glLineWidth(1.0F);
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				struct Point cubes[64];
				int amount = 0;
				if(is_local && local_player_drag_active && players[local_player_id].input.buttons.rmb
				   && players[local_player_id].held_item == TOOL_BLOCK) {
					amount = map_cube_line(local_player_drag_x, local_player_drag_z, 63 - local_player_drag_y, pos[0],
										   pos[2], 63 - pos[1], cubes);
				} else {
					amount = 1;
					cubes[0].x = pos[0];
					cubes[0].y = pos[2];
					cubes[0].z = 63 - pos[1];
				}
				while(amount > 0) {
					int tmp = cubes[amount - 1].y;
					cubes[amount - 1].y = 63 - cubes[amount - 1].z;
					cubes[amount - 1].z = tmp;
					if(amount <= (is_local ? local_player_blocks : 50))
						glColor3f(1.0F, 1.0F, 1.0F);

					short vertices[72] = {cubes[amount - 1].x,	   cubes[amount - 1].y,		cubes[amount - 1].z,
										  cubes[amount - 1].x,	   cubes[amount - 1].y,		cubes[amount - 1].z + 1,
										  cubes[amount - 1].x,	   cubes[amount - 1].y,		cubes[amount - 1].z,
										  cubes[amount - 1].x + 1, cubes[amount - 1].y,		cubes[amount - 1].z,
										  cubes[amount - 1].x + 1, cubes[amount - 1].y,		cubes[amount - 1].z + 1,
										  cubes[amount - 1].x + 1, cubes[amount - 1].y,		cubes[amount - 1].z,
										  cubes[amount - 1].x + 1, cubes[amount - 1].y,		cubes[amount - 1].z + 1,
										  cubes[amount - 1].x,	   cubes[amount - 1].y,		cubes[amount - 1].z + 1,

										  cubes[amount - 1].x,	   cubes[amount - 1].y + 1, cubes[amount - 1].z,
										  cubes[amount - 1].x,	   cubes[amount - 1].y + 1, cubes[amount - 1].z + 1,
										  cubes[amount - 1].x,	   cubes[amount - 1].y + 1, cubes[amount - 1].z,
										  cubes[amount - 1].x + 1, cubes[amount - 1].y + 1, cubes[amount - 1].z,
										  cubes[amount - 1].x + 1, cubes[amount - 1].y + 1, cubes[amount - 1].z + 1,
										  cubes[amount - 1].x + 1, cubes[amount - 1].y + 1, cubes[amount - 1].z,
										  cubes[amount - 1].x + 1, cubes[amount - 1].y + 1, cubes[amount - 1].z + 1,
										  cubes[amount - 1].x,	   cubes[amount - 1].y + 1, cubes[amount - 1].z + 1,

										  cubes[amount - 1].x,	   cubes[amount - 1].y,		cubes[amount - 1].z,
										  cubes[amount - 1].x,	   cubes[amount - 1].y + 1, cubes[amount - 1].z,
										  cubes[amount - 1].x + 1, cubes[amount - 1].y,		cubes[amount - 1].z,
										  cubes[amount - 1].x + 1, cubes[amount - 1].y + 1, cubes[amount - 1].z,
										  cubes[amount - 1].x + 1, cubes[amount - 1].y,		cubes[amount - 1].z + 1,
										  cubes[amount - 1].x + 1, cubes[amount - 1].y + 1, cubes[amount - 1].z + 1,
										  cubes[amount - 1].x,	   cubes[amount - 1].y,		cubes[amount - 1].z + 1,
										  cubes[amount - 1].x,	   cubes[amount - 1].y + 1, cubes[amount - 1].z + 1};
					glEnableClientState(GL_VERTEX_ARRAY);
					glVertexPointer(3, GL_SHORT, 0, vertices);
					glDrawArrays(GL_LINES, 0, 24);
					glDisableClientState(GL_VERTEX_ARRAY);
					amount--;
				}
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
			}

			if(window_time() - players[local_player_id].item_disabled < 0.3F) {
				players[local_player_id].item_showup = window_time();
				if(players[local_player_id].input.buttons.lmb)
					players[local_player_id].input.buttons.lmb_start = window_time();
				players[local_player_id].input.buttons.rmb = 0;
			} else {
				if(hud_active->render_localplayer) {
					float tmp2 = players[local_player_id].physics.eye.y;
					players[local_player_id].physics.eye.y = last_cy;
					if(camera_mode == CAMERAMODE_FPS)
						glDepthRange(0.0F, 0.05F);
					matrix_push(matrix_projection);
					matrix_translate(matrix_projection, 0.0F, -0.25F, 0.0F);
					matrix_upload_p();
#ifdef OPENGL_ES
					if(camera_mode == CAMERAMODE_FPS)
						glx_disable_sphericalfog();
#endif
					player_render(&players[local_player_id], local_player_id);
#ifdef OPENGL_ES
					if(camera_mode == CAMERAMODE_FPS)
						glx_enable_sphericalfog();
#endif
					matrix_pop(matrix_projection);
					glDepthRange(0.0F, 1.0F);
					players[local_player_id].physics.eye.y = tmp2;
				}
			}

			matrix_upload_p();
			matrix_upload();
			player_render_all();

			matrix_upload();
			map_collapsing_render();
			matrix_upload();

			if(!map_isair(camera_x, camera_y, camera_z))
				glClear(GL_COLOR_BUFFER_BIT);

			glx_disable_sphericalfog();
			if(settings.smooth_fog)
				glDisable(GL_FOG);
		}
	}

	if(hud_active->render_3D)
		hud_active->render_3D();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_MULTISAMPLE);
	matrix_identity(matrix_projection);
	matrix_ortho(matrix_projection, 0.0F, settings.window_width, 0.0F, settings.window_height, -1.0F, 1.0F);
	matrix_identity(matrix_view);
	matrix_identity(matrix_model);
	matrix_upload();
	matrix_upload_p();
	float scalex = settings.window_width / 800.0F;
	float scalef = settings.window_height / 600.0F;

	if(hud_active->render_2D) {
		mu_Context* ctx = hud_active->ctx;

		if(ctx) {
			hud_active->ctx->style->padding = 10 * scalef - 5;
			hud_active->ctx->style->spacing = 8 * scalef - 4;
			hud_active->ctx->style->title_height = 48 * scalef - 24;
			hud_active->ctx->style->scrollbar_size = 12 * scalef;
			hud_active->ctx->style->thumb_size = 8 * scalef;

			mu_begin(ctx);
		}

		hud_active->render_2D(ctx, scalex, scalef);

		if(ctx) {
			mu_end(ctx);

			glEnable(GL_BLEND);
			glEnable(GL_SCISSOR_TEST);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			mu_Command* cmd = NULL;
			while(mu_next_command(ctx, &cmd)) {
				switch(cmd->type) {
					case MU_COMMAND_TEXT:
						glColor4ub(cmd->text.color.r, cmd->text.color.g, cmd->text.color.b, cmd->text.color.a);
						font_render(cmd->text.pos.x, settings.window_height - cmd->text.pos.y,
									ctx->text_height(cmd->text.font), cmd->text.str);
						glEnable(GL_BLEND);
						break;
					case MU_COMMAND_RECT:
						glColor4ub(cmd->rect.color.r, cmd->rect.color.g, cmd->rect.color.b, cmd->rect.color.a);
						texture_draw_empty(cmd->rect.rect.x, settings.window_height - cmd->rect.rect.y,
										   cmd->rect.rect.w, cmd->rect.rect.h);
						break;
					case MU_COMMAND_ICON:
						glColor4ub(cmd->icon.color.r, cmd->icon.color.g, cmd->icon.color.b, cmd->icon.color.a);
						int size = min(cmd->icon.rect.w, cmd->icon.rect.h);

						if(cmd->icon.id >= HUD_FLAG_INDEX_START - 1) {
							float u, v;
							texture_flag_offset(cmd->icon.id - HUD_FLAG_INDEX_START, &u, &v);

							texture_draw_sector(&texture_ui_flags, cmd->icon.rect.x,
												settings.window_height - cmd->icon.rect.y - size * 0.167F, size,
												size * 0.667F, u, v, 18.0F / 256.0F, 12.0F / 256.0F);
							glEnable(GL_BLEND);
						} else if(hud_active->ui_images) {
							bool resize = false;
							struct texture* img = hud_active->ui_images(cmd->icon.id, &resize);

							if(img) {
								texture_draw(img, cmd->icon.rect.x, settings.window_height - cmd->icon.rect.y,
											 resize ? size : cmd->icon.rect.w, resize ? size : cmd->icon.rect.h);
								glEnable(GL_BLEND);
							}
						}

						break;
					case MU_COMMAND_CLIP:
						glScissor(cmd->clip.rect.x, settings.window_height - (cmd->clip.rect.y + cmd->clip.rect.h),
								  cmd->clip.rect.w, cmd->clip.rect.h);
						break;
				}
			}
			glDisable(GL_SCISSOR_TEST);
			glDisable(GL_BLEND);
		}
	}

	if(settings.multisamples > 0)
		glEnable(GL_MULTISAMPLE);
}

void init() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
#ifdef OPENGL_ES
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
#else
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
#endif
	glClearDepth(1.0F);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_FOG);

	map_init();

	glx_init();

	font_init();
	player_init();
	particle_init();
	network_init();
	ping_init();
	kv6_init();
	texture_init();
	sound_init();
	tracer_init();
	hud_init();
	chunk_init();
	grenade_init();

	weapon_set();

	rpc_init();
}

void reshape(struct window_instance* window, int width, int height) {
	font_reset();
	glViewport(0, 0, width, height);
	settings.window_width = width;
	settings.window_height = height;
	if(settings.vsync < 2)
		window_swapping(settings.vsync);
	if(settings.vsync > 1)
		window_swapping(0);
}

static int mu_button_translate(int button) {
	switch(button) {
		case WINDOW_MOUSE_LMB: return MU_MOUSE_LEFT;
		case WINDOW_MOUSE_MMB: return MU_MOUSE_MIDDLE;
		case WINDOW_MOUSE_RMB: return MU_MOUSE_RIGHT;
		default: return 0;
	}
}

static int mu_key_translate(int key) {
	switch(key) {
		case WINDOW_KEY_BACKSPACE: return MU_KEY_BACKSPACE;
		case WINDOW_KEY_ENTER: return MU_KEY_RETURN;
		case WINDOW_KEY_SHIFT: return MU_KEY_SHIFT;
		default: return 0;
	}
}

void text_input(struct window_instance* window, unsigned int codepoint) {
	if(hud_active->ctx)
		mu_input_text(hud_active->ctx, (char[2]) {codepoint, 0});

	if(chat_input_mode == CHAT_NO_INPUT)
		return;

	int len = strlen(chat[0][0]);
	if(len < 128) {
		chat[0][0][len] = codepoint;
		chat[0][0][len + 1] = 0;
	}
}

void keys(struct window_instance* window, int key, int scancode, int action, int mods) {
	if(hud_active->ctx) {
		if(mu_key_translate(key)) {
			switch(action) {
				case WINDOW_RELEASE: mu_input_keyup(hud_active->ctx, mu_key_translate(key)); break;
				case WINDOW_REPEAT:
				case WINDOW_PRESS: mu_input_keydown(hud_active->ctx, mu_key_translate(key)); break;
			}
		}

		if(action == WINDOW_PRESS && key == WINDOW_KEY_V && mods) {
			const char* clipboard = window_clipboard();
			if(clipboard)
				mu_input_text(hud_active->ctx, clipboard);
		}
	}

	if(action == WINDOW_PRESS) {
		if(config_key(key)->toggle) {
			if(chat_input_mode == CHAT_NO_INPUT) {
				window_pressed_keys[key] = !window_pressed_keys[key];
			}
		} else {
			window_pressed_keys[key] = 1;
		}
	}
	if(action == WINDOW_RELEASE && !config_key(key)->toggle)
		window_pressed_keys[key] = 0;

#ifdef USE_GLFW
	if(key == WINDOW_KEY_FULLSCREEN && action == WINDOW_PRESS) { // switch between fullscreen
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		if(!settings.fullscreen) {
			glfwSetWindowMonitor(window->impl, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height,
								 mode->refreshRate);
			settings.fullscreen = 1;
		} else {
			glfwSetWindowMonitor(window->impl, NULL, (mode->width - 800) / 2, (mode->height - 600) / 2, 800, 600, 0);
			settings.fullscreen = 0;
		}
	}
#endif

	if(key == WINDOW_KEY_SCREENSHOT && action == WINDOW_PRESS) { // take screenshot
		time_t pic_time;
		time(&pic_time);
		char pic_name[128];
		sprintf(pic_name, "screenshots/%ld.png", (long)pic_time);

		unsigned char* pic_data = malloc(settings.window_width * settings.window_height * 4 * 2);
		CHECK_ALLOCATION_ERROR(pic_data)
		glReadPixels(0, 0, settings.window_width, settings.window_height, GL_RGBA, GL_UNSIGNED_BYTE, pic_data);

		for(int y = 0; y < settings.window_height; y++) { // mirror image (top-bottom)
			for(int x = 0; x < settings.window_width; x++)
				pic_data[(x + (settings.window_height - y - 1) * settings.window_width) * 4 + 3] = 255;
			memcpy(pic_data + settings.window_width * 4 * (y + settings.window_height),
				   pic_data + settings.window_width * 4 * (settings.window_height - y - 1), settings.window_width * 4);
		}

		lodepng_encode32_file(pic_name, pic_data + settings.window_width * settings.window_height * 4,
							  settings.window_width, settings.window_height);
		free(pic_data);

		sprintf(pic_name, "Saved screenshot as screenshots/%ld.png", (long)pic_time);
		chat_add(0, 0x0000FF, pic_name);
	}

	if(key == WINDOW_KEY_SAVE_MAP && action == WINDOW_PRESS) { // save map
		time_t save_time;
		time(&save_time);
		char save_name[128];
		sprintf(save_name, "vxl/%ld.vxl", (long)save_time);

		map_save_file(save_name);

		sprintf(save_name, "Saved map as vxl/%ld.vxl", (long)save_time);
		chat_add(0, 0x0000FF, save_name);
	}
}

void mouse_click(struct window_instance* window, int button, int action, int mods) {
	if(hud_active->input_mouseclick) {
		double x, y;
		window_mouseloc(&x, &y);
		hud_active->input_mouseclick(x, y, button, action, mods);
	}

	if(hud_active->ctx) {
		double x, y;
		window_mouseloc(&x, &y);
		switch(action) {
			case WINDOW_PRESS: mu_input_mousedown(hud_active->ctx, x, y, mu_button_translate(button)); break;
			case WINDOW_RELEASE: mu_input_mouseup(hud_active->ctx, x, y, mu_button_translate(button)); break;
		}
	}
}

void mouse(struct window_instance* window, double x, double y) {
	if(hud_active->input_mouselocation)
		hud_active->input_mouselocation(x, y);
	if(hud_active->ctx)
		mu_input_mousemove(hud_active->ctx, x, y);
}

void mouse_scroll(struct window_instance* window, double xoffset, double yoffset) {
	if(hud_active->input_mousescroll)
		hud_active->input_mousescroll(yoffset);
	if(hud_active->ctx)
		mu_input_scroll(hud_active->ctx, -xoffset * 50, -yoffset * 50);
}

void deinit() {
	rpc_deinit();
	ping_deinit();
	if(network_connected)
		network_disconnect();
	window_deinit();
}

void on_error(int i, const char* s) {
	log_fatal("Major error occured: [%i] %s", i, s);
	getchar();
}

int main(int argc, char** argv) {
	settings.opengl14 = 1;
	settings.color_correction = 0;
	settings.multisamples = 0;
	settings.shadow_entities = 0;
	settings.ambient_occlusion = 0;
	settings.render_distance = 128.0F;
	settings.window_width = 800;
	settings.window_height = 600;
	settings.player_arms = 0;
	settings.fullscreen = 0;
	settings.greedy_meshing = 0;
	settings.mouse_sensitivity = MOUSE_SENSITIVITY;
	settings.show_news = 1;
	settings.show_fps = 0;
	settings.volume = 10;
	settings.voxlap_models = 0;
	settings.force_displaylist = 0;
	settings.invert_y = 0;
	settings.smooth_fog = 0;
	settings.camera_fov = CAMERA_DEFAULT_FOV;
	strcpy(settings.name, "DEV_CLIENT");

#ifdef USE_TOUCH
	mkdir("/sdcard/BetterSpades");
#else
	if(!file_dir_exists("logs"))
		file_dir_create("logs");
	if(!file_dir_exists("cache"))
		file_dir_create("cache");
	if(!file_dir_exists("screenshots"))
		file_dir_create("screenshots");
	if(!file_dir_exists("vxl"))
		file_dir_create("vxl");
#endif

	log_set_level(LOG_INFO);

	time_t t = time(NULL);
	char buf[32];
	strftime(buf, 32, "logs/%m-%d-%Y.log", localtime(&t));
	log_set_fp(fopen(buf, "a"));

	srand(t);

	log_info("Game started!");

	config_reload();

	window_init();

#ifndef OPENGL_ES
	if(glewInit())
		log_error("Could not load extended OpenGL functions!");
#endif

	log_info("Vendor: %s", glGetString(GL_VENDOR));
	log_info("Renderer: %s", glGetString(GL_RENDERER));
	log_info("Version: %s", glGetString(GL_VERSION));

	if(settings.multisamples > 0) {
		glEnable(GL_MULTISAMPLE);
		log_info("MSAAx%i on", settings.multisamples);
	}

	while(glGetError() != GL_NO_ERROR)
		;

	init();
	atexit(deinit);

	if(settings.vsync < 2)
		window_swapping(settings.vsync);
	if(settings.vsync > 1)
		window_swapping(0);

	if(argc > 1) {
		if(!strcmp(argv[1], "--help")) {
			log_info("Usage: client                     [server browser]");
			log_info("       client -aos://<ip>:<port>  [custom address]");
			exit(0);
		}

		if(!network_connect_string(argv[1] + 1)) {
			log_error("Error: Connection failed (use --help for instructions)");
			exit(1);
		} else {
			log_info("Connection to %s successful", argv[1] + 1);
			hud_change(&hud_ingame);
		}
	}

	double last_frame_start = 0.0F;
	double physics_time_fixed = 0.0F;
	double physics_time_fast = 0.0F;

	while(!window_closed()) {
		double dt = window_time() - last_frame_start;
		last_frame_start = window_time();

		if(hud_active->render_world) {
			physics_time_fast += dt;
			physics_time_fixed += dt;

// these run at exactly ~60fps
#define PHYSICS_STEP_TIME (1.0 / 60.0)
			while(physics_time_fixed >= PHYSICS_STEP_TIME) {
				physics_time_fixed -= PHYSICS_STEP_TIME;
				player_update(PHYSICS_STEP_TIME, 1); // just physics tick
				grenade_update(PHYSICS_STEP_TIME);
			}

			// these run at min. ~60fps but as fast as possible
			double step = fmin(dt, PHYSICS_STEP_TIME);
			while(step > 0 && physics_time_fast >= step) {
				physics_time_fast -= step;
				player_update(step, 0); // smooth orientation update
				camera_update(step);
				tracer_update(step);
				particle_update(step);
				map_collapsing_update(step);
			}
		}

		display();

		sound_update();
		network_update();
		window_update();

		rpc_update();

		if(settings.vsync > 1 && (window_time() - last_frame_start) < (1.0 / settings.vsync)) {
			double sleep_s = 1.0 / settings.vsync - (window_time() - last_frame_start);
			struct timespec ts;
			ts.tv_sec = (int)sleep_s;
			ts.tv_nsec = (sleep_s - ts.tv_sec) * 1000000000.0;
			nanosleep(&ts, NULL);
		}

		fps = 1.0F / dt;
	}
}
