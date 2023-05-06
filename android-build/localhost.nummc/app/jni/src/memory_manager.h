#pragma once
#include "game_common.h"

//#define MEMORY_MANAGER_DEBUG

#define MEMORY_MANAGER_TYPE_NONE      0
#define MEMORY_MANAGER_TYPE_CHAR_BUF  1
#define MEMORY_MANAGER_TYPE_NODE_BUF  2
#define MEMORY_MANAGER_TYPE_VOID_BUF  3
#define MEMORY_MANAGER_TYPE_END       4

#define MEMORY_MANAGER_STAT_NONE      0
#define MEMORY_MANAGER_STAT_USED      1
#define MEMORY_MANAGER_STAT_END       2

#define MEMORY_MANAGER_ALLOCK_BLOCK_SIZE  2048

// memory info
typedef struct _memory_buffer_info_t memory_buffer_info_t;
struct _memory_buffer_info_t {
	int block_num;            // sizeof(buffer) / block_size
	int used_block_count;     // used_buffer < buffer_size
	void* head_addr;          // buffer[]
	void* end_addr;           // buffer[]
	void* current_addr;       // start position to search empty
};

typedef struct _memory_block_t memory_block_t;
struct _memory_block_t {
	int type;          // NONE:0, CHAR_BUF:1, NODE_BUF:2, VOID_BUF:3
	int stat;
	int buffer_size;   // LINE_BUF_SIZE, STRING_BUF_SIZE, NAME_BUF_SIZE, NODE_S_BUF_SIZE
	void* buffer;
};

// char buffer
#define MEMORY_MANAGER_LINE_BUF_SIZE    256
#define MEMORY_MANAGER_STRING_BUF_SIZE  128
#define MEMORY_MANAGER_NAME_BUF_SIZE     32

#define MEMORY_MANAGER_LINE_LIST_SIZE    (MEMORY_MANAGER_ALLOCK_BLOCK_SIZE / MEMORY_MANAGER_LINE_BUF_SIZE)
#define MEMORY_MANAGER_STRING_LIST_SIZE  (MEMORY_MANAGER_ALLOCK_BLOCK_SIZE / MEMORY_MANAGER_STRING_BUF_SIZE)
#define MEMORY_MANAGER_NAME_LIST_SIZE    (MEMORY_MANAGER_ALLOCK_BLOCK_SIZE / MEMORY_MANAGER_NAME_BUF_SIZE)

// node buffer
#define MEMORY_MANAGER_NODE_S_BUF_SIZE    (32 * 8)
#define MEMORY_MANAGER_NODE_S_LIST_SIZE   (MEMORY_MANAGER_ALLOCK_BLOCK_SIZE / MEMORY_MANAGER_LINE_BUF_SIZE)

extern int memory_manager_init();
extern void memory_manager_unload();
extern char* memory_manager_new_char_buff(int size);
extern void memory_manager_delete_char_buff(char* addr);
extern node_data_t* memory_manager_new_node_buff(int size);
extern void memory_manager_delete_node_buff(node_data_t* addr);
extern void* memory_manager_new_void_buff(int size);
extern void memory_manager_delete_void_buff(void* addr);
