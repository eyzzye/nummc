#include "game_common.h"
#include "resource_manager.h"

#include "game_window.h"
#include "game_utils.h"
#include "game_log.h"
#include "memory_manager.h"

#define RESOURCE_TAG_IMG        0
#define RESOURCE_TAG_FONT       1
#define RESOURCE_TAG_MUSIC      2
#define RESOURCE_TAG_SOUND      3
#define RESOURCE_TAG_END        4

static node_buffer_info_t resource_img_buffer_info;
static node_buffer_info_t resource_music_buffer_info;
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
	game_utils_node_init(&resource_img_buffer_info,   (int)sizeof(ResourceImg));
	game_utils_node_init(&resource_music_buffer_info, (int)sizeof(ResourceMusic));
	game_utils_node_init(&resource_chunk_buffer_info, (int)sizeof(ResourceChunk));
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

void resource_manager_clean_up()
{
	// Delete Texture
	node_data_t* node = resource_img_buffer_info.start_node;
	while (node != NULL) {
		node_data_t* del_node = node;
		node = node->next;
		if (del_node->type == RESOURCE_MANAGER_TYPE_STATIC) continue;

		SDL_Texture* tmp_tex = ((ResourceImg*)del_node)->tex;
		if (tmp_tex != NULL) SDL_DestroyTexture(tmp_tex);
		game_utils_node_delete(del_node, &resource_img_buffer_info);
	}

	// Delete Music
	node = resource_music_buffer_info.start_node;
	while (node != NULL) {
		node_data_t* del_node = node;
		node = node->next;
		if (del_node->type == RESOURCE_MANAGER_TYPE_STATIC) continue;

		Mix_Music* tmp_music = ((ResourceMusic*)del_node)->music;
		if (tmp_music != NULL) Mix_FreeMusic(tmp_music);
		game_utils_node_delete(del_node, &resource_music_buffer_info);
	}

	// Delete Chunk
	node = resource_chunk_buffer_info.start_node;
	while (node != NULL) {
		node_data_t* del_node = node;
		node = node->next;
		if (del_node->type == RESOURCE_MANAGER_TYPE_STATIC) continue;

		Mix_Chunk* tmp_chunk = ((ResourceChunk*)del_node)->chunk;
		if (tmp_chunk != NULL) Mix_FreeChunk(tmp_chunk);
		game_utils_node_delete(del_node, &resource_chunk_buffer_info);
	}
}

typedef struct _load_dat_callback_data_t load_dat_callback_data_t;
struct _load_dat_callback_data_t {
	bool read_flg[RESOURCE_TAG_END];
};
static load_dat_callback_data_t load_dat_callback_data;
static void load_dat_callback(char* line, int line_size, int line_num, void* argv)
{
	load_dat_callback_data_t* data = (load_dat_callback_data_t*)argv;

	if (line[0] == '\0') return;
	if (line[0] == '#') return;

	if (line[0] == '[') {
		if (STRCMP_EQ(line, "[img]"))    { data->read_flg[RESOURCE_TAG_IMG]   = true;  return; }
		if (STRCMP_EQ(line, "[/img]"))   { data->read_flg[RESOURCE_TAG_IMG]   = false; return; }
		if (STRCMP_EQ(line, "[font]"))   { data->read_flg[RESOURCE_TAG_FONT]  = true;  return; }
		if (STRCMP_EQ(line, "[/font]"))  { data->read_flg[RESOURCE_TAG_FONT]  = false; return; }
		if (STRCMP_EQ(line, "[music]"))  { data->read_flg[RESOURCE_TAG_MUSIC] = true;  return; }
		if (STRCMP_EQ(line, "[/music]")) { data->read_flg[RESOURCE_TAG_MUSIC] = false; return; }
		if (STRCMP_EQ(line, "[snd]"))    { data->read_flg[RESOURCE_TAG_SOUND] = true;  return; }
		if (STRCMP_EQ(line, "[/snd]"))   { data->read_flg[RESOURCE_TAG_SOUND] = false; return; }
	}

	if (data->read_flg[RESOURCE_TAG_IMG]) {
		resource_manager_getTextureFromPath(line, RESOURCE_MANAGER_TYPE_STATIC);
	}
	if (data->read_flg[RESOURCE_TAG_FONT]) {
		resource_manager_getFontTextureFromPath(line, RESOURCE_MANAGER_TYPE_STATIC);
	}
	if (data->read_flg[RESOURCE_TAG_MUSIC]) {
		resource_manager_getMusicFromPath(line, RESOURCE_MANAGER_TYPE_STATIC);
	}
	if (data->read_flg[RESOURCE_TAG_SOUND]) {
		resource_manager_getChunkFromPath(line, RESOURCE_MANAGER_TYPE_STATIC);
	}
}

int resource_manager_load_dat(char* path)
{
	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", path);
	if (tmp_path_size == 0) {
		LOG_ERROR("resource_manager_load_dat failed get %s\n", path);
		return 1;
	}

	// read file
	memset(load_dat_callback_data.read_flg, 0, sizeof(bool) * RESOURCE_TAG_END);
	int ret = game_utils_files_read_line(full_path, load_dat_callback, (void*)&load_dat_callback_data);
	if (ret != 0) {
		LOG_ERROR("resource_manager_load_dat %s error\n", path);
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
	resImg->path = memory_manager_new_char_buff((int)path.size());
	game_utils_string_copy(resImg->path, (char*)path.c_str());
	resImg->tex = tex;
	resImg->type = type;
	return resImg;
}

ResourceImg* resource_manager_getTextureFromPath(std::string path, int type)
{
	node_data_t* node = resource_img_buffer_info.start_node;
	while (node != NULL) {
		if (STRCMP_EQ(((ResourceImg*)node)->path, path.c_str())) {
			return (ResourceImg*)node;
		}
		node = node->next;
	}
	return resource_manager_load_img(path, type);
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
	game_utils_string_cat(font_path, (char*)"font:", (char*)message.c_str());
	resFont->path = memory_manager_new_char_buff((int)strlen(font_path));
	game_utils_string_copy(resFont->path, font_path);
	resFont->tex = tex;
	resFont->type = type;
	return resFont;
}

ResourceImg* resource_manager_getFontTextureFromPath(std::string message, int type)
{
	node_data_t* node = resource_img_buffer_info.start_node;
	char font_path[GAME_UTILS_STRING_CHAR_BUF_SIZE];
	game_utils_string_cat(font_path, (char*)"font:", (char*)message.c_str());

	while (node != NULL) {
		if (STRCMP_EQ(((ResourceImg*)node)->path, font_path)) {
			return (ResourceImg*)node;
		}
		node = node->next;
	}
	return resource_manager_load_font(message, type);
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
	resMusic->path = memory_manager_new_char_buff((int)path.size());
	game_utils_string_copy(resMusic->path, (char*)path.c_str());
	resMusic->music = music;
	resMusic->type = type;
	return resMusic;
}

ResourceMusic* resource_manager_getMusicFromPath(std::string path, int type)
{
	node_data_t* node = resource_music_buffer_info.start_node;
	while (node != NULL) {
		if (STRCMP_EQ(((ResourceMusic*)node)->path, path.c_str())) {
			return (ResourceMusic*)node;
		}
		node = node->next;
	}
	return resource_manager_load_music(path, type);
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
	resChunk->path = memory_manager_new_char_buff((int)path.size());
	game_utils_string_copy(resChunk->path, (char*)path.c_str());
	resChunk->chunk = chunk;
	resChunk->type = type;
	return resChunk;
}

ResourceChunk* resource_manager_getChunkFromPath(std::string path, int type)
{
	node_data_t* node = resource_chunk_buffer_info.start_node;
	while (node != NULL) {
		if (STRCMP_EQ(((ResourceChunk*)node)->path, path.c_str())) {
			return (ResourceChunk*)node;
		}
		node = node->next;
	}
	return resource_manager_load_chunk(path, type);
}
