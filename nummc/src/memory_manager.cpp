#include <memory.h>
#include "game_common.h"
#include "memory_manager.h"

#include "game_utils.h"
#include "game_log.h"

// -- memory assigned --
//           
// +-------- <char_buff> name
//  2048*N
// +-------- <char_buff> char
//  2048*N
// +-------- <char_buff> line
//  2048*N
// +-------- <node_buff> resource
//  2048*N
// +-------- <node_buff> unit, map
//  2048*N
// +-------- <void_buff>
//  2048*N
// +-------- <end>

// -- char_buff --
#define CHAR_INFO_LINE_SIZE_MAX     32
#define CHAR_INFO_STRING_SIZE_MAX  128
#define CHAR_INFO_NAME_SIZE_MAX     16

#define CHAR_INFO_LINE_SIZE_DEFAULT    16
#define CHAR_INFO_STRING_SIZE_DEFAULT  96
#define CHAR_INFO_NAME_SIZE_DEFAULT     8

typedef int memory_manager_new_block_func(int index);

static memory_buffer_info_t char_info_line_inst[CHAR_INFO_LINE_SIZE_MAX];
static memory_buffer_info_t char_info_string_inst[CHAR_INFO_STRING_SIZE_MAX];
static memory_buffer_info_t char_info_name_inst[CHAR_INFO_NAME_SIZE_MAX];

static int char_info_line_size;
static int char_info_string_size;
static int char_info_name_size;

#ifdef MEMORY_MANAGER_DEBUG
static char char_buff_line[CHAR_INFO_LINE_SIZE_MAX][MEMORY_MANAGER_ALLOCK_BLOCK_SIZE]     = { 0 };  // (256[byte] *  8) * N(Max)
static char char_buff_string[CHAR_INFO_STRING_SIZE_MAX][MEMORY_MANAGER_ALLOCK_BLOCK_SIZE] = { 0 };  // (128[byte] * 16) * N(Max)
static char char_buff_name[CHAR_INFO_NAME_SIZE_MAX][MEMORY_MANAGER_ALLOCK_BLOCK_SIZE]     = { 0 };  // ( 32[byte] * 64) * N(Max)

static memory_block_t char_block_line[CHAR_INFO_LINE_SIZE_MAX][MEMORY_MANAGER_LINE_LIST_SIZE]       = { 0 };  // ( 24[byte] *  8) * N(Max)
static memory_block_t char_block_string[CHAR_INFO_STRING_SIZE_MAX][MEMORY_MANAGER_STRING_LIST_SIZE] = { 0 };  // ( 24[byte] * 16) * N(Max)
static memory_block_t char_block_name[CHAR_INFO_NAME_SIZE_MAX][MEMORY_MANAGER_NAME_LIST_SIZE]       = { 0 };  // ( 24[byte] * 64) * N(Max)

static memory_buffer_info_t* char_info_line   = NULL;  // ( 32[byte] * 1) * N(Max)
static memory_buffer_info_t* char_info_string = NULL;  // ( 32[byte] * 1) * N(Max)
static memory_buffer_info_t* char_info_name   = NULL;  // ( 32[byte] * 1) * N(Max)
#else
static char* char_buff_line[CHAR_INFO_LINE_SIZE_MAX]     = { NULL };  // (256[byte] *  8) * N(Max)
static char* char_buff_string[CHAR_INFO_STRING_SIZE_MAX] = { NULL };  // (128[byte] * 16) * N(Max)
static char* char_buff_name[CHAR_INFO_NAME_SIZE_MAX]     = { NULL };  // ( 32[byte] * 64) * N(Max)

static memory_block_t* char_block_line[CHAR_INFO_LINE_SIZE_MAX]     = { NULL };  // ( 24[byte] *  8) * N(Max)
static memory_block_t* char_block_string[CHAR_INFO_STRING_SIZE_MAX] = { NULL };  // ( 24[byte] * 16) * N(Max)
static memory_block_t* char_block_name[CHAR_INFO_NAME_SIZE_MAX]     = { NULL };  // ( 24[byte] * 64) * N(Max)

static memory_buffer_info_t* char_info_line   = NULL;  // ( 32[byte] * 1) * N(Max)
static memory_buffer_info_t* char_info_string = NULL;  // ( 32[byte] * 1) * N(Max)
static memory_buffer_info_t* char_info_name   = NULL;  // ( 32[byte] * 1) * N(Max)
#endif

static int memory_manager_new_char_block_line(int index);
static int memory_manager_new_char_block_string(int index);
static int memory_manager_new_char_block_name(int index);

// -- node_buff --
//static ResourceImg resource_img_buffer[RESOURCE_IMG_BUFFER_SIZE];
//static ResourceMusic resource_music_buffer[RESOURCE_MUSIC_BUFFER_SIZE];
//static ResourceChunk resource_chunk_buffer[RESOURCE_CHUNK_BUFFER_SIZE];

//static node_buffer_info_t bgm_list_buffer_info[SECTION_DATA_BUFFER_SIZE];
//static node_buffer_info_t enemy_list_buffer_info[SECTION_DATA_BUFFER_SIZE * SECTION_ENEMY_PHASE_SIZE];
//static node_buffer_info_t trap_list_buffer_info[SECTION_DATA_BUFFER_SIZE];
//static node_buffer_info_t items_list_buffer_info[SECTION_DATA_BUFFER_SIZE];
//static node_buffer_info_t section_stock_item_buffer_info[SECTION_DATA_BUFFER_SIZE];

//static tile_instance_data_t map_wall[COLLISION_STATIC_WALL_NUM]; // for invisible col_shape
//static tile_instance_data_t map_raw_data_buffer[MAP_RAW_DATA_BUFFER_SIZE];
//static tile_data_t tile_tex[TILE_TEX_NUM];

//static unit_player_data_t player_base[UNIT_PLAYER_BASE_LIST_SIZE];
//static unit_enemy_data_t enemy_base[UNIT_ENEMY_BASE_LIST_SIZE];
//static unit_enemy_data_t enemy[UNIT_ENEMY_LIST_SIZE];
//static unit_items_data_t items_base[UNIT_ITEMS_BASE_LIST_SIZE];
//static unit_items_data_t items[UNIT_ITEMS_LIST_SIZE];
//static unit_trap_data_t trap_base[UNIT_TRAP_BASE_LIST_SIZE];
//static unit_trap_data_t trap[UNIT_TRAP_LIST_SIZE];
//static unit_player_bullet_data_t player_bullet_base[UNIT_PLAYER_BULLET_BASE_LIST_SIZE];
//static unit_player_bullet_data_t player_bullet[UNIT_PLAYER_BULLET_LIST_SIZE];
//static unit_enemy_bullet_data_t enemy_bullet_base[UNIT_ENEMY_BULLET_BASE_LIST_SIZE];
//static unit_enemy_bullet_data_t enemy_bullet[UNIT_ENEMY_BULLET_LIST_SIZE];
//static unit_effect_data_t effect_base[UNIT_EFFECT_BASE_LIST_SIZE];
//static unit_effect_data_t effect[UNIT_EFFECT_LIST_SIZE];

#define NODE_INFO_SMALL_SIZE_MAX      (ANIM_DATA_LIST_SIZE / 8)
#define NODE_INFO_SMALL_SIZE_DEFAULT  (128)
static memory_buffer_info_t node_info_small_inst[NODE_INFO_SMALL_SIZE_MAX];
static int node_info_small_size;

#ifdef MEMORY_MANAGER_DEBUG
static char node_buff_small[NODE_INFO_SMALL_SIZE_MAX][MEMORY_MANAGER_ALLOCK_BLOCK_SIZE]           = { 0 };  // (256[byte] * 8) * N(Max)
static memory_block_t node_block_small[NODE_INFO_SMALL_SIZE_MAX][MEMORY_MANAGER_NODE_S_LIST_SIZE] = { 0 };  // ( 24[byte] * 8) * N(Max)
static memory_buffer_info_t* node_info_small = NULL;                                                        // ( 32[byte] * 1) * N(Max)
#else
static char* node_buff_small[NODE_INFO_SMALL_SIZE_MAX] = { NULL };             // (256[byte] * 8) * N(Max)
static memory_block_t* node_block_small[NODE_INFO_SMALL_SIZE_MAX] = { NULL };  // ( 24[byte] * 8) * N(Max)
static memory_buffer_info_t* node_info_small = NULL;                           // ( 32[byte] * 1) * N(Max)
#endif

static int memory_manager_new_node_block_small(int index);

// -- void_buff --
// work in progress

static char* memory_manager_new_memory_block(int index, memory_buffer_info_t* memory_info, int memory_info_size);
static int memory_manager_delete_memory_block(void* addr, memory_buffer_info_t* memory_info, int memory_info_size);

int memory_manager_init()
{
	int ret = 0;

	//
	// char_buff
	//
	char_info_line_size   = 0;
	char_info_string_size = 0;
	char_info_name_size   = 0;

#if 0
	char_info_line = (memory_buffer_info_t*)malloc(sizeof(memory_buffer_info_t) * CHAR_INFO_LINE_SIZE_MAX);
	char_info_string = (memory_buffer_info_t*)malloc(sizeof(memory_buffer_info_t) * CHAR_INFO_STRING_SIZE_MAX);
	char_info_name = (memory_buffer_info_t*)malloc(sizeof(memory_buffer_info_t) * CHAR_INFO_NAME_SIZE_MAX);
	if ((char_info_line == NULL) || (char_info_string == NULL) || (char_info_name == NULL)) {
		return 1;
	}
#else
	char_info_line   = &char_info_line_inst[0];
	char_info_string = &char_info_string_inst[0];
	char_info_name   = &char_info_name_inst[0];
#endif

#ifdef MEMORY_MANAGER_DEBUG
	int init_line_size = CHAR_INFO_LINE_SIZE_MAX;
	int init_string_size = CHAR_INFO_STRING_SIZE_MAX;
	int init_name_size = CHAR_INFO_NAME_SIZE_MAX;
#else
	int init_line_size = CHAR_INFO_LINE_SIZE_DEFAULT;
	int init_string_size = CHAR_INFO_STRING_SIZE_DEFAULT;
	int init_name_size = CHAR_INFO_NAME_SIZE_DEFAULT;
#endif

	for (int i = 0; i < init_line_size; i++) {
		ret = memory_manager_new_char_block_line(i);
		if (ret != 0) return 1;
	}

	for (int i = 0; i < init_string_size; i++) {
		ret = memory_manager_new_char_block_string(i);
		if (ret != 0) return 1;
	}

	for (int i = 0; i < init_name_size; i++) {
		ret = memory_manager_new_char_block_name(i);
		if (ret != 0) return 1;
	}

	//
	// node_buff
	//
	node_info_small_size = 0;

#if 0
	node_info_small = (memory_buffer_info_t*)malloc(sizeof(memory_buffer_info_t) * NODE_INFO_SMALL_SIZE_MAX);
	if (node_info_small == NULL) {
		return 1;
	}
#else
	node_info_small = &node_info_small_inst[0];
#endif

#ifdef MEMORY_MANAGER_DEBUG
	int init_node_small_size = NODE_INFO_SMALL_SIZE_MAX;
#else
	int init_node_small_size = NODE_INFO_SMALL_SIZE_DEFAULT;
#endif

	for (int i = 0; i < init_node_small_size; i++) {
		ret = memory_manager_new_node_block_small(i);
		if (ret != 0) return 1;
	}

	return 0;
}

void memory_manager_unload()
{
	//
	// char_buff
	//
#ifdef MEMORY_MANAGER_DEBUG
		// do nothing
#else
	for (int i = 0; i < char_info_line_size; i++) {
		if (char_buff_line[i]) {
			free(char_buff_line[i]); char_buff_line[i] = NULL;
		}
		if (char_block_line[i]) {
			free(char_block_line[i]); char_block_line[i] = NULL;
		}
	}

	for (int i = 0; i < char_info_string_size; i++) {
		if (char_buff_string[i]) {
			free(char_buff_string[i]); char_buff_string[i] = NULL;
		}
		if (char_block_string[i]) {
			free(char_block_string[i]); char_block_string[i] = NULL;
		}
	}

	for (int i = 0; i < char_info_name_size; i++) {
		if (char_buff_name[i]) {
			free(char_buff_name[i]); char_buff_name[i] = NULL;
		}
		if (char_block_name[i]) {
			free(char_block_name[i]); char_block_name[i] = NULL;
		}
	}
#endif

#if 0
	if (char_info_line) {
		free(char_info_line); char_info_line = NULL;
	}
	if (char_info_string) {
		free(char_info_string); char_info_string = NULL;
	}
	if (char_info_name) {
		free(char_info_name); char_info_name = NULL;
	}
#else
	char_info_line   = NULL;
	char_info_string = NULL;
	char_info_name = NULL;
#endif

	//
	// node_buff
	//
#ifdef MEMORY_MANAGER_DEBUG
		// do nothing
#else
	for (int i = 0; i < node_info_small_size; i++) {
		if (node_buff_small[i]) {
			free(node_buff_small[i]); node_buff_small[i] = NULL;
		}
		if (node_block_small[i]) {
			free(node_block_small[i]); node_block_small[i] = NULL;
		}
	}
#endif

#if 0
	if (node_info_small) {
		free(node_info_small); node_info_small = NULL;
	}
#else
	node_info_small = NULL;
#endif
}

static char* memory_manager_new_memory_block(int index, memory_buffer_info_t* memory_info, int memory_info_size)
{
	char* ret_addr = NULL;

	memory_block_t* block = (memory_block_t*)memory_info[index].current_addr;
	for (int bi = 0; bi < memory_info[index].block_num; bi++) {
		if ((size_t)block > (size_t)memory_info[index].end_addr) {
			block = (memory_block_t*)memory_info[index].head_addr;
		}

		if (block->stat == MEMORY_MANAGER_STAT_NONE) {
			// get block
			ret_addr = (char*)block->buffer;
			memset(block->buffer, 0, block->buffer_size);
			block->stat = MEMORY_MANAGER_STAT_USED;

			// update info
			memory_info[index].used_block_count += 1;
			memory_info[index].current_addr = (block + 1);
			if ((size_t)memory_info[index].current_addr > (size_t)memory_info[index].end_addr) {
				memory_info[index].current_addr = memory_info[index].head_addr;
			}

			return ret_addr;
		}
		block++;
	}

	return ret_addr;
}

static int memory_manager_delete_memory_block(void* addr, memory_buffer_info_t* memory_info, int memory_info_size)
{
	for (int i = 0; i < memory_info_size; i++) {
		memory_block_t* head_addr = (memory_block_t*)memory_info[i].head_addr;
		memory_block_t* end_addr = (memory_block_t*)memory_info[i].end_addr;
		if (((size_t)head_addr->buffer <= (size_t)addr) && ((size_t)addr <= (size_t)end_addr->buffer)) {
			// release block
			int index = (int)(((size_t)addr - (size_t)head_addr->buffer) / head_addr->buffer_size);
			memory_block_t* block = &head_addr[index];
			block->stat = MEMORY_MANAGER_STAT_NONE;

			// update info
			memory_info[i].used_block_count -= 1;
			return 0;
		}
	}
	return 1;
}

static int memory_manager_new_char_block_line(int index)
{
	if (char_info_line_size >= CHAR_INFO_LINE_SIZE_MAX) {
		return 1;
	}

#ifdef MEMORY_MANAGER_DEBUG
	// do nothing
#else
	// memory buffer
	char_buff_line[index] = (char*)malloc(MEMORY_MANAGER_ALLOCK_BLOCK_SIZE);

	// memory block
	char_block_line[index] = (memory_block_t*)malloc(sizeof(memory_block_t) * MEMORY_MANAGER_LINE_LIST_SIZE);
#endif

	// alloc error
	if ((char_buff_line[index] == NULL) || (char_block_line[index] == NULL)) {
		return 1;
	}

	for (int bi = 0; bi < MEMORY_MANAGER_LINE_LIST_SIZE; bi++) {
		char_block_line[index][bi].type        = MEMORY_MANAGER_TYPE_CHAR_BUF;
		char_block_line[index][bi].stat        = MEMORY_MANAGER_STAT_NONE;
		char_block_line[index][bi].buffer_size = MEMORY_MANAGER_LINE_BUF_SIZE;
		char_block_line[index][bi].buffer      = &char_buff_line[index][bi * MEMORY_MANAGER_LINE_BUF_SIZE];
	}

	// memory info
	char_info_line[index].head_addr        = &char_block_line[index][0];
	char_info_line[index].end_addr         = (void*)((size_t)char_info_line[index].head_addr + (sizeof(memory_block_t) * (MEMORY_MANAGER_LINE_LIST_SIZE - 1)));
	char_info_line[index].block_num        = MEMORY_MANAGER_LINE_LIST_SIZE;
	char_info_line[index].current_addr     = char_info_line[index].head_addr;
	char_info_line[index].used_block_count = 0;

	char_info_line_size++;
	return 0;
}

static int memory_manager_new_char_block_string(int index)
{
	if (char_info_string_size >= CHAR_INFO_STRING_SIZE_MAX) {
		return 1;
	}

#ifdef MEMORY_MANAGER_DEBUG
	// do nothing
#else
	// memory buffer
	char_buff_string[index] = (char*)malloc(MEMORY_MANAGER_ALLOCK_BLOCK_SIZE);

	// memory block
	char_block_string[index] = (memory_block_t*)malloc(sizeof(memory_block_t) * MEMORY_MANAGER_STRING_LIST_SIZE);
#endif

	// alloc error
	if ((char_buff_string[index] == NULL) || (char_block_string[index] == NULL)) {
		return 1;
	}

	for (int bi = 0; bi < MEMORY_MANAGER_STRING_LIST_SIZE; bi++) {
		char_block_string[index][bi].type        = MEMORY_MANAGER_TYPE_CHAR_BUF;
		char_block_string[index][bi].stat        = MEMORY_MANAGER_STAT_NONE;
		char_block_string[index][bi].buffer_size = MEMORY_MANAGER_STRING_BUF_SIZE;
		char_block_string[index][bi].buffer      = &char_buff_string[index][bi * MEMORY_MANAGER_STRING_BUF_SIZE];
	}

	// memory info
	char_info_string[index].head_addr        = &char_block_string[index][0];
	char_info_string[index].end_addr         = (void*)((size_t)char_info_string[index].head_addr + (sizeof(memory_block_t) * (MEMORY_MANAGER_STRING_LIST_SIZE - 1)));
	char_info_string[index].block_num        = MEMORY_MANAGER_STRING_LIST_SIZE;
	char_info_string[index].current_addr     = char_info_string[index].head_addr;
	char_info_string[index].used_block_count = 0;

	char_info_string_size++;
	return 0;
}

static int memory_manager_new_char_block_name(int index)
{
	if (char_info_name_size >= CHAR_INFO_NAME_SIZE_MAX) {
		return 1;
	}

#ifdef MEMORY_MANAGER_DEBUG
	// do nothing
#else
	// memory buffer
	char_buff_name[index] = (char*)malloc(MEMORY_MANAGER_ALLOCK_BLOCK_SIZE);

	// memory block
	char_block_name[index] = (memory_block_t*)malloc(sizeof(memory_block_t) * MEMORY_MANAGER_NAME_LIST_SIZE);
#endif

	// alloc error
	if ((char_buff_name[index] == NULL) || (char_block_name[index] == NULL)) {
		return 1;
	}

	for (int bi = 0; bi < MEMORY_MANAGER_NAME_LIST_SIZE; bi++) {
		char_block_name[index][bi].type        = MEMORY_MANAGER_TYPE_CHAR_BUF;
		char_block_name[index][bi].stat        = MEMORY_MANAGER_STAT_NONE;
		char_block_name[index][bi].buffer_size = MEMORY_MANAGER_NAME_BUF_SIZE;
		char_block_name[index][bi].buffer      = &char_buff_name[index][bi * MEMORY_MANAGER_NAME_BUF_SIZE];
	}

	// memory info
	char_info_name[index].head_addr        = &char_block_name[index][0];
	char_info_name[index].end_addr         = (void*)((size_t)char_info_name[index].head_addr + (sizeof(memory_block_t) * (MEMORY_MANAGER_NAME_LIST_SIZE - 1)));
	char_info_name[index].block_num        = MEMORY_MANAGER_NAME_LIST_SIZE;
	char_info_name[index].current_addr     = char_info_name[index].head_addr;
	char_info_name[index].used_block_count = 0;

	char_info_name_size++;
	return 0;
}

char* memory_manager_new_char_buff(int size)
{
	char* ret_addr = NULL;

	memory_buffer_info_t* memory_info = NULL;
	int memory_info_size = 0;
	int new_index = 0;
	memory_manager_new_block_func* new_block_func = NULL;
	if (size < MEMORY_MANAGER_NAME_BUF_SIZE) {
		memory_info = char_info_name;
		memory_info_size = CHAR_INFO_NAME_SIZE_MAX;
		new_index = char_info_name_size;
		new_block_func = memory_manager_new_char_block_name;
	}
	else if (size < MEMORY_MANAGER_STRING_BUF_SIZE) {
		memory_info = char_info_string;
		memory_info_size = CHAR_INFO_STRING_SIZE_MAX;
		new_index = char_info_string_size;
		new_block_func = memory_manager_new_char_block_string;
	}
	else if (size < MEMORY_MANAGER_LINE_BUF_SIZE) {
		memory_info = char_info_line;
		memory_info_size = CHAR_INFO_LINE_SIZE_MAX;
		new_index = char_info_line_size;
		new_block_func = memory_manager_new_char_block_line;
	}
	else {
		LOG_ERROR_CONSOLE("Error: memory_manager_new_char_buff() unsupported size\n");
		return ret_addr;
	}

	for (int i = 0; i < memory_info_size; i++) {
		if (memory_info[i].used_block_count >= memory_info[i].block_num) {
			//LOG_DEBUG_CONSOLE("memory_manager_new_char_buff() char_info_name overflow\n");
			continue;
		}

		ret_addr = memory_manager_new_memory_block(i, memory_info, memory_info_size);
		if (ret_addr != NULL) return ret_addr;
	}

	// alloc additional buffer
#ifdef MEMORY_MANAGER_DEBUG
	// do nothing
#else
	if (ret_addr == NULL) {
		int ret = new_block_func(new_index);
		if (ret != 0) {
			LOG_ERROR_CONSOLE("Error: memory_manager_new_char_buff() can't alloc new %d block\n", size);
			return NULL;
		}

		ret_addr = memory_manager_new_memory_block(new_index, memory_info, memory_info_size);
		if (ret_addr != NULL) {
			LOG_DEBUG_CONSOLE("memory_manager_new_char_buff() new %d\n", new_index);
			return ret_addr;
		}
	}
#endif

	if (ret_addr == NULL) {
		LOG_ERROR_CONSOLE("Error: memory_manager_new_char_buff() failed allocation\n");
	}

	return ret_addr;
}

void memory_manager_delete_char_buff(char* addr)
{
	memory_block_t* block = NULL;
	memory_buffer_info_t* memory_info = NULL;
	int memory_info_size = 0;

	for (int char_buf_i = 0; char_buf_i < 3; char_buf_i++) {
		if (char_buf_i == 0) {
			memory_info = char_info_name;
			memory_info_size = char_info_name_size;
		}
		else if (char_buf_i == 1) {
			memory_info = char_info_string;
			memory_info_size = char_info_string_size;
		}
		else if (char_buf_i == 2) {
			memory_info = char_info_line;
			memory_info_size = char_info_line_size;
		}

		int ret = memory_manager_delete_memory_block(addr, memory_info, memory_info_size);
		if (ret == 0) return;
	}

	LOG_ERROR_CONSOLE("memory_manager_delete_char_buff() failed 0x%zx\n", (size_t)addr);
}

static int memory_manager_new_node_block_small(int index)
{
	if (node_info_small_size >= NODE_INFO_SMALL_SIZE_MAX) {
		return 1;
	}

#ifdef MEMORY_MANAGER_DEBUG
	// do nothing
#else
	// memory buffer
	node_buff_small[index] = (char*)malloc(MEMORY_MANAGER_ALLOCK_BLOCK_SIZE);

	// memory block
	node_block_small[index] = (memory_block_t*)malloc(sizeof(memory_block_t) * MEMORY_MANAGER_NODE_S_LIST_SIZE);
#endif

	// alloc error
	if ((node_buff_small[index] == NULL) || (node_block_small[index] == NULL)) {
		return 1;
	}

	for (int bi = 0; bi < MEMORY_MANAGER_NODE_S_LIST_SIZE; bi++) {
		node_block_small[index][bi].type        = MEMORY_MANAGER_TYPE_NODE_BUF;
		node_block_small[index][bi].stat        = MEMORY_MANAGER_STAT_NONE;
		node_block_small[index][bi].buffer_size = MEMORY_MANAGER_NODE_S_BUF_SIZE;
		node_block_small[index][bi].buffer      = &node_buff_small[index][bi * MEMORY_MANAGER_NODE_S_BUF_SIZE];
	}

	// memory info
	node_info_small[index].head_addr        = &node_block_small[index][0];
	node_info_small[index].end_addr         = (void*)((size_t)node_info_small[index].head_addr + (sizeof(memory_block_t) * (MEMORY_MANAGER_NODE_S_LIST_SIZE - 1)));
	node_info_small[index].block_num        = MEMORY_MANAGER_NODE_S_LIST_SIZE;
	node_info_small[index].current_addr     = node_info_small[index].head_addr;
	node_info_small[index].used_block_count = 0;

	node_info_small_size++;
	return 0;
}

node_data_t* memory_manager_new_node_buff(int size)
{
	node_data_t* ret_addr = NULL;

	memory_buffer_info_t* memory_info = NULL;
	int memory_info_size = 0;
	int new_index = 0;
	memory_manager_new_block_func* new_block_func = NULL;
	if (size < MEMORY_MANAGER_NODE_S_BUF_SIZE) {
		memory_info = node_info_small;
		memory_info_size = NODE_INFO_SMALL_SIZE_MAX;
		new_index = node_info_small_size;
		new_block_func = memory_manager_new_node_block_small;
	}
	else {
		LOG_ERROR_CONSOLE("Error: memory_manager_new_node_buff() unsupported size\n");
		return ret_addr;
	}

	for (int i = 0; i < memory_info_size; i++) {
		if (memory_info[i].used_block_count >= memory_info[i].block_num) {
			//LOG_DEBUG_CONSOLE("memory_manager_new_node_buff() node_info_small overflow\n");
			continue;
		}

		ret_addr = (node_data_t*)memory_manager_new_memory_block(i, memory_info, memory_info_size);
		if (ret_addr != NULL) return ret_addr;
	}

	// alloc additional buffer
#ifdef MEMORY_MANAGER_DEBUG
	// do nothing
#else
	if (ret_addr == NULL) {
		int ret = new_block_func(new_index);
		if (ret != 0) {
			LOG_ERROR_CONSOLE("Error: memory_manager_new_node_buff() can't alloc new %d block\n", size);
			return NULL;
		}

		ret_addr = (node_data_t*)memory_manager_new_memory_block(new_index, memory_info, memory_info_size);
		if (ret_addr != NULL) {
			LOG_DEBUG_CONSOLE("memory_manager_new_node_buff() new %d\n", new_index);
			return ret_addr;
		}
	}
#endif

	if (ret_addr == NULL) {
		LOG_ERROR_CONSOLE("Error: memory_manager_new_node_buff() failed allocation\n");
	}

	return ret_addr;
}

void memory_manager_delete_node_buff(node_data_t* addr)
{
	memory_buffer_info_t* memory_info = NULL;
	int memory_info_size = 0;

	for (int node_buf_i = 0; node_buf_i < 1; node_buf_i++) {
		if (node_buf_i == 0) {
			memory_info = node_info_small;
			memory_info_size = node_info_small_size;
		}

		int ret = memory_manager_delete_memory_block(addr, memory_info, memory_info_size);
		if (ret == 0) return;
	}

	LOG_ERROR_CONSOLE("memory_manager_delete_node_buff() failed 0x%zx\n", (size_t)addr);
}

void* memory_manager_new_void_buff(int size)
{
	return NULL;
}

void memory_manager_delete_void_buff(void* addr)
{

}
