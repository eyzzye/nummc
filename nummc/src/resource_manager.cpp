#include <fstream>
#include "game_common.h"
#include "resource_manager.h"

#include "game_window.h"
#include "game_utils.h"
#include "game_log.h"

std::vector<ResourceImg*>   g_resource_manager_imgs;
std::vector<ResourceMusic*> g_resource_manager_musics;
std::vector<ResourceChunk*> g_resource_manager_chunks;

ResourceProfile g_resource_manager_profile[RESOURCE_MANAGER_PROFILE_LIST_SIZE] = {
	// name, unit, icon, portrait, opening, ending
	{
		"unlock",
		"",
		"images/units/player/unlock/unlock_icon.png",
		"images/units/player/unlock/unlock_portrait.png",
		"",
		"",
	},
	{
		"player1",
		"units/player/player1/player1.unit",
		"images/units/player/player1/player_idle6.png",
		"images/units/player/player1/infinity.png",
		"scenes/story/infinity/opening.dat",
		"scenes/story/infinity/ending.dat",
	},
	{
		"player2",
		"units/player/player2/player2.unit",
		"images/units/player/player2/player_idle5.png",
		"images/units/player/player2/zero.png",
		"scenes/story/zero/opening.dat",
		"scenes/story/zero/ending.dat",
	},
};

void resource_manager_init()
{
	g_resource_manager_imgs.clear();
	g_resource_manager_musics.clear();
	g_resource_manager_chunks.clear();
}

void resource_manager_unload()
{
	// Delete Texture
	for (ResourceImg* resImg : g_resource_manager_imgs)
	{
		delete resImg;
	}
	g_resource_manager_imgs.clear();

	// Delete Music
	for (ResourceMusic* resMusic : g_resource_manager_musics)
	{
		delete resMusic;
	}
	g_resource_manager_musics.clear();

	// Delete Chunk
	for (ResourceChunk* resChunk : g_resource_manager_chunks)
	{
		delete resChunk;
	}
	g_resource_manager_chunks.clear();
}

int resource_manager_load_dat(std::string path)
{
	bool img_flg = false;
	bool font_flg = false;
	bool music_flg = false;
	bool snd_flg = false;

	std::ifstream inFile(g_base_path + "data/" + path);
	if (inFile.is_open()) {
		std::string line;
		while (std::getline(inFile, line)) {
			if (line == "") continue;
			if (line[0] == '#') continue;

			if (line == "[img]")    { img_flg   = true;  continue; }
			if (line == "[/img]")   { img_flg   = false; continue; }
			if (line == "[font]")   { font_flg  = true;  continue; }
			if (line == "[/font]")  { font_flg  = false; continue; }
			if (line == "[music]")  { music_flg = true;  continue; }
			if (line == "[/music]") { music_flg = false; continue; }
			if (line == "[snd]")    { snd_flg   = true;  continue; }
			if (line == "[/snd]")   { snd_flg   = false; continue; }

			if (img_flg) {
				resource_manager_load_img(line);
			}
			if (font_flg) {
				resource_manager_load_font(line);
			}
			if (music_flg) {
				resource_manager_load_music(line);
			}
			if (snd_flg) {
				resource_manager_load_chunk(line);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("resource_manager_load_dat %s error\n", path.c_str());
		return 1;
	}
	return 0;
}

//
// Image
//
ResourceImg* resource_manager_load_img(std::string path, int type)
{
	int scale_mode = -1;
	bool effect_color = false;
	SDL_Color src_col, dst_col;

	bool header_found = false;
	int read_index = 1;
	int read_flg = 0; // 0:effect

	// read effect header {effect}
	if (path[0] == '{') {
		header_found = true;
		while ((path[read_index] != '}') && (read_index < path.size())) {
			read_index++;
		}

		std::vector<std::string> str_list;
		std::string header_str = path.substr(1, read_index - 1);
		game_utils_split_conmma(header_str, str_list);

		for (int i = 0; i < str_list.size(); i++) {
			std::vector<std::string> effect_list;
			game_utils_split_colon(str_list[i], effect_list);

			// scale_mode:linear ... SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear)
			if (effect_list[0] == "scale_mode") {
				if (effect_list[1] == "linear") {
					scale_mode = (int)SDL_ScaleModeLinear;
				}
			}
			else if (effect_list[0] == "color") {
				if (effect_list.size() != 9) {
					LOG_ERROR("Error: resource_manager_load_img color param are wrong.\n");
					continue;
				}

				// effect_list[1] == "S"
				src_col.r = atoi(effect_list[2].c_str());
				src_col.g = atoi(effect_list[3].c_str());
				src_col.b = atoi(effect_list[4].c_str());

				// effect_list[1] == "D"
				dst_col.r = atoi(effect_list[6].c_str());
				dst_col.g = atoi(effect_list[7].c_str());
				dst_col.b = atoi(effect_list[8].c_str());

				effect_color = true;
			}
		}
	}

	std::string* raw_path = NULL;
	std::string new_path;
	if (header_found) {
		new_path = path.substr((size_t)read_index + 1, path.size() - read_index);
		raw_path = &new_path;
	}
	else {
		raw_path = &path;
	}

	g_render_mtx.lock();
	SDL_Texture* tex;
	std::string full_path = g_base_path + "data/" + *raw_path;
	if (effect_color) {
		tex = game_utils_render_img_tex(full_path, src_col, dst_col);
	}
	else {
		tex = IMG_LoadTexture(g_ren, full_path.c_str());
	}
	g_render_mtx.unlock();
	if (tex == NULL) {
		LOG_ERROR("SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
		return NULL;
	}

	if (scale_mode == (int)SDL_ScaleModeLinear) {
		SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
	}
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	ResourceImg* resImg = new ResourceImg();
	resImg->path = path;
	resImg->tex = tex;
	resImg->type = type;
	g_resource_manager_imgs.push_back(resImg);
	return resImg;
}

ResourceImg* resource_manager_getTextureFromPath(std::string path)
{
	for (ResourceImg* resImg : g_resource_manager_imgs)
	{
		if (resImg->path == path) {
			return resImg;
		}
	}
	return resource_manager_load_img(path);
}

//
// Font
//
ResourceImg* resource_manager_load_font(std::string message, int type)
{
	int font_size = 0;
	int font_color[4] = { 0 };
	std::string font_file_name = "";

	// read effect header {size,color,font_name,effect}
	bool header_found = false;
	int read_index = 1;
	int read_flg = 0; // 0:size, 1:color, 2:font_name, 3:effect
	int read_color_i = 0;

	if (message[0] == '{') {
		header_found = true;
		while ((message[read_index] != '}') && (read_index < message.size())) {
			if (read_flg > 3) continue;
			if (message[read_index] == ',') {
				read_flg++;
				read_index++;
				continue;
			}

			// read font size
			if (read_flg == 0) {
				if (message[read_index] != '-') {
					char num_str = message[read_index];
					font_size = (font_size * 10) + (num_str - 0x30); // ascii -> int
				}
			}
			// read font color
			else if (read_flg == 1) {
				if (message[read_index] == ':') {
					read_color_i++;
				}
				else if (message[read_index] != '-') {
					char num_str = message[read_index];
					font_color[read_color_i] = font_color[read_color_i] * 10 + (num_str - 0x30);
				}
			}
			// read font name
			else if (read_flg == 2) {
				if (message[read_index] != '-') {
					font_file_name = font_file_name + message[read_index];
				}
			}
			// read effect
			else if (read_flg == 3) {
				// reserve
			}

			read_index++;
		}
	}

	if ((header_found == true) && (read_index == message.size())) {
		LOG_ERROR("resource_manager_load_font size error\n");
		return NULL;
	}

	// set default font
	if (font_size == 0) font_size = 32;
	if (read_color_i == 0) font_color[3] = 255;
	if (font_file_name == "") font_file_name = "DejaVuSans.ttf";

	// set message
	std::string* disp_msg = NULL;
	std::string new_message;
	if (header_found) {
		new_message = message.substr((size_t)read_index + 1, message.size() - read_index);
		disp_msg = &new_message;
	}
	else {
		disp_msg = &message;
	}

	const std::string fontFile = g_base_path + "data/font/" + font_file_name;
	SDL_Color color = { (Uint8)font_color[0], (Uint8)font_color[1], (Uint8)font_color[2], (Uint8)font_color[3] };
	g_render_mtx.lock();
	SDL_Texture* tex = game_utils_render_font_tex(*disp_msg, fontFile, color, font_size);
	g_render_mtx.unlock();
	if (tex == NULL) {
		LOG_ERROR("SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
		return NULL;
	}
	SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	ResourceImg* resFont = new ResourceImg();
	resFont->path = "font:" + message;
	resFont->tex = tex;
	resFont->type = type;
	g_resource_manager_imgs.push_back(resFont);
	return resFont;
}

ResourceImg* resource_manager_getFontTextureFromPath(std::string message)
{
	for (ResourceImg* resFont : g_resource_manager_imgs)
	{
		if (resFont->path == "font:" + message) {
			return resFont;
		}
	}
	return resource_manager_load_font(message);
}

//
// Music
//
ResourceMusic* resource_manager_load_music(std::string path, int type)
{
	g_render_mtx.lock();
	Mix_Music* music = Mix_LoadMUS((g_base_path + "data/" + path).c_str());
	g_render_mtx.unlock();
	if (music == NULL) {
		LOG_ERROR("Mix_LoadMUS Error: %s\n", SDL_GetError());
		return NULL;
	}
	ResourceMusic* resMusic = new ResourceMusic();
	resMusic->path = path;
	resMusic->music = music;
	resMusic->type = type;
	g_resource_manager_musics.push_back(resMusic);
	return resMusic;
}

ResourceMusic* resource_manager_getMusicFromPath(std::string path)
{
	for (ResourceMusic* resMusic : g_resource_manager_musics)
	{
		if (resMusic->path == path) {
			return resMusic;
		}
	}
	return resource_manager_load_music(path);
}

//
// Chunk
//
ResourceChunk* resource_manager_load_chunk(std::string path, int type)
{
	g_render_mtx.lock();
	Mix_Chunk* chunk = Mix_LoadWAV((g_base_path + "data/" + path).c_str());
	g_render_mtx.unlock();
	if (chunk == NULL) {
		LOG_ERROR("Mix_LoadWAV Error: %s\n", SDL_GetError());
		return NULL;
	}
	ResourceChunk* resChunk = new ResourceChunk();
	resChunk->path = path;
	resChunk->chunk = chunk;
	resChunk->type = type;
	g_resource_manager_chunks.push_back(resChunk);
	return resChunk;
}

ResourceChunk* resource_manager_getChunkFromPath(std::string path)
{
	for (ResourceChunk* resChunk : g_resource_manager_chunks)
	{
		if (resChunk->path == path) {
			return resChunk;
		}
	}
	return resource_manager_load_chunk(path);
}
