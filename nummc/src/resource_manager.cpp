#include <fstream>
#include "game_common.h"
#include "resource_manager.h"

#include "game_window.h"
#include "game_utils.h"
#include "game_log.h"

#define RESOURCE_IMG_BUFFER_SIZE  (ANIM_DATA_LIST_SIZE)
static ResourceImg resource_img_buffer[RESOURCE_IMG_BUFFER_SIZE];
static node_buffer_info_t resource_img_buffer_info;

#define RESOURCE_MUSIC_BUFFER_SIZE  (32)
static ResourceMusic resource_music_buffer[RESOURCE_MUSIC_BUFFER_SIZE];
static node_buffer_info_t resource_music_buffer_info;

#define RESOURCE_CHUNK_BUFFER_SIZE  (64)
static ResourceChunk resource_chunk_buffer[RESOURCE_CHUNK_BUFFER_SIZE];
static node_buffer_info_t resource_chunk_buffer_info;

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
	game_utils_node_init(&resource_img_buffer_info, (node_data_t*)resource_img_buffer, (int)sizeof(ResourceImg), RESOURCE_IMG_BUFFER_SIZE);
	game_utils_node_init(&resource_music_buffer_info, (node_data_t*)resource_music_buffer, (int)sizeof(ResourceMusic), RESOURCE_MUSIC_BUFFER_SIZE);
	game_utils_node_init(&resource_chunk_buffer_info, (node_data_t*)resource_chunk_buffer, (int)sizeof(ResourceChunk), RESOURCE_CHUNK_BUFFER_SIZE);
}

void resource_manager_unload()
{
	// Delete Texture
	node_data_t* node = resource_img_buffer_info.start_node;
	while (node != NULL) {
		node_data_t* del_node = node;
		node = node->next;
		game_utils_node_delete(del_node, &resource_img_buffer_info);
	}

	// Delete Music
	node = resource_music_buffer_info.start_node;
	while (node != NULL) {
		node_data_t* del_node = node;
		node = node->next;
		game_utils_node_delete(del_node, &resource_music_buffer_info);
	}

	// Delete Chunk
	node = resource_chunk_buffer_info.start_node;
	while (node != NULL) {
		node_data_t* del_node = node;
		node = node->next;
		game_utils_node_delete(del_node, &resource_chunk_buffer_info);
	}
}

int resource_manager_load_dat(std::string path)
{
	bool img_flg = false;
	bool font_flg = false;
	bool music_flg = false;
	bool snd_flg = false;

	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", (char*)path.c_str());
	if (tmp_path_size == 0) {
		LOG_ERROR("resource_manager_load_dat failed get %s\n", path.c_str());
		return 1;
	}

	std::ifstream inFile(full_path);
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
				resource_manager_getTextureFromPath(line);
			}
			if (font_flg) {
				resource_manager_getFontTextureFromPath(line);
			}
			if (music_flg) {
				resource_manager_getMusicFromPath(line);
			}
			if (snd_flg) {
				resource_manager_getChunkFromPath(line);
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

		char header_str[GAME_UTILS_STRING_CHAR_BUF_SIZE];
		char str_list[GAME_UTILS_STRING_CHAR_BUF_SIZE * RESOURCE_MANAGER_IMG_OPT_END];
		char effect_list[GAME_UTILS_STRING_CHAR_BUF_SIZE * 9];

		//std::string header_str = path.substr(1, read_index - 1);
		game_utils_string_copy_n(header_str, &path[1], read_index - 1);
		int str_list_size = game_utils_split_conmma(header_str, str_list, RESOURCE_MANAGER_IMG_OPT_END);

		for (int i = 0; i < str_list_size; i++) {
			char* option_str = &str_list[i * GAME_UTILS_STRING_CHAR_BUF_SIZE];
			int effect_list_size = game_utils_split_colon(option_str, effect_list, 9);

			// scale_mode:linear ... SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear)
			if (STRCMP_EQ(&effect_list[0],"scale_mode")) {
				char* mode_str = &effect_list[1 * GAME_UTILS_STRING_CHAR_BUF_SIZE];
				if (STRCMP_EQ(mode_str,"linear")) {
					scale_mode = (int)SDL_ScaleModeLinear;
				}
				continue;
			}

			// color:S:255:255:255:D:255:255:255
			if (STRCMP_EQ(&effect_list[0],"color")) {
				if (effect_list_size != 9) {
					LOG_ERROR("Error: resource_manager_load_img color param are wrong.\n");
					continue;
				}

				// effect_list[1] == "S"
				src_col.r = atoi(&effect_list[2 * GAME_UTILS_STRING_CHAR_BUF_SIZE]);
				src_col.g = atoi(&effect_list[3 * GAME_UTILS_STRING_CHAR_BUF_SIZE]);
				src_col.b = atoi(&effect_list[4 * GAME_UTILS_STRING_CHAR_BUF_SIZE]);

				// effect_list[1] == "D"
				dst_col.r = atoi(&effect_list[6 * GAME_UTILS_STRING_CHAR_BUF_SIZE]);
				dst_col.g = atoi(&effect_list[7 * GAME_UTILS_STRING_CHAR_BUF_SIZE]);
				dst_col.b = atoi(&effect_list[8 * GAME_UTILS_STRING_CHAR_BUF_SIZE]);

				effect_color = true;
				continue;
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

	//std::string full_path = g_base_path + "data/" + *raw_path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", (char*)raw_path->c_str());
	if (tmp_path_size == 0) {
		LOG_ERROR("resource_manager_load_img failed get %s\n", path.c_str());
		return NULL;
	}

	g_render_mtx.lock();
	SDL_Texture* tex;
	if (effect_color) {
		tex = game_utils_render_img_tex(full_path, src_col, dst_col);
	}
	else {
		tex = IMG_LoadTexture(g_ren, full_path);
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
	ResourceImg* resImg = (ResourceImg*)game_utils_node_new(&resource_img_buffer_info);
	resImg->path = path;
	resImg->tex = tex;
	resImg->type = type;
	return resImg;
}

ResourceImg* resource_manager_getTextureFromPath(std::string path)
{
	node_data_t* node = resource_img_buffer_info.start_node;
	while (node != NULL) {
		if (((ResourceImg*)node)->path == path) {
			return (ResourceImg*)node;
		}
		node = node->next;
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

	//const std::string fontFile = g_base_path + "data/font/" + font_file_name;
	char font_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(font_path, g_base_path, (char*)"data/font/", (char*)font_file_name.c_str());
	if (tmp_path_size == 0) {
		LOG_ERROR("SDL_CreateTextureFromSurface failed get %s\n", font_file_name.c_str());
		return NULL;
	}

	SDL_Color color = { (Uint8)font_color[0], (Uint8)font_color[1], (Uint8)font_color[2], (Uint8)font_color[3] };
	g_render_mtx.lock();
	SDL_Texture* tex = game_utils_render_font_tex(*disp_msg, font_path, color, font_size);
	g_render_mtx.unlock();
	if (tex == NULL) {
		LOG_ERROR("SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
		return NULL;
	}
	SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	ResourceImg* resFont = (ResourceImg*)game_utils_node_new(&resource_img_buffer_info);
	resFont->path = "font:" + message;
	resFont->tex = tex;
	resFont->type = type;
	return resFont;
}

ResourceImg* resource_manager_getFontTextureFromPath(std::string message)
{
	node_data_t* node = resource_img_buffer_info.start_node;
	while (node != NULL) {
		if (((ResourceImg*)node)->path == "font:" + message) {
			return (ResourceImg*)node;
		}
		node = node->next;
	}
	return resource_manager_load_font(message);
}

//
// Music
//
ResourceMusic* resource_manager_load_music(std::string path, int type)
{
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", (char*)path.c_str());
	if (tmp_path_size == 0) {
		LOG_ERROR("resource_manager_load_music failed get %s\n", path.c_str());
		return NULL;
	}

	g_render_mtx.lock();
	Mix_Music* music = Mix_LoadMUS(full_path);
	g_render_mtx.unlock();
	if (music == NULL) {
		LOG_ERROR("Mix_LoadMUS Error: %s\n", SDL_GetError());
		return NULL;
	}
	ResourceMusic* resMusic = (ResourceMusic*)game_utils_node_new(&resource_music_buffer_info);
	resMusic->path = path;
	resMusic->music = music;
	resMusic->type = type;
	return resMusic;
}

ResourceMusic* resource_manager_getMusicFromPath(std::string path)
{
	node_data_t* node = resource_music_buffer_info.start_node;
	while (node != NULL) {
		if (((ResourceMusic*)node)->path == path) {
			return (ResourceMusic*)node;
		}
		node = node->next;
	}
	return resource_manager_load_music(path);
}

//
// Chunk
//
ResourceChunk* resource_manager_load_chunk(std::string path, int type)
{
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", (char*)path.c_str());
	if (tmp_path_size == 0) {
		LOG_ERROR("resource_manager_load_chunk failed get %s\n", path.c_str());
		return NULL;
	}

	g_render_mtx.lock();
	Mix_Chunk* chunk = Mix_LoadWAV(full_path);
	g_render_mtx.unlock();
	if (chunk == NULL) {
		LOG_ERROR("Mix_LoadWAV Error: %s\n", SDL_GetError());
		return NULL;
	}
	ResourceChunk* resChunk = (ResourceChunk*)game_utils_node_new(&resource_chunk_buffer_info);
	resChunk->path = path;
	resChunk->chunk = chunk;
	resChunk->type = type;
	return resChunk;
}

ResourceChunk* resource_manager_getChunkFromPath(std::string path)
{
	node_data_t* node = resource_chunk_buffer_info.start_node;
	while (node != NULL) {
		if (((ResourceChunk*)node)->path == path) {
			return (ResourceChunk*)node;
		}
		node = node->next;
	}
	return resource_manager_load_chunk(path);
}
