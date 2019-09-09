#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <uchar.h>
#include <errno.h>
#include <string.h>

#define cast_to_int(b) (*((int*)(&b)))
#define cast_to_llong(b) (*((long long*)(&b)))


#define PARTITION_MBR_OFFSET 0x1be
#define bool int
#define true 1
#define false 0
#define byte unsigned char
#define SECTOR_SIZE 512
#define FAT32_ENTRY_SIZE 4
#define DIRECTORY_ENTRY_SIZE 32

// File system types signatures
#define FAT32_ID 0x0B
#define NTFS 0x07
#define GPT_ID 0xEE

#define FAT32_UNUSED_DIRECTORY_RECORD_MARKER 0xE5
#define FAT32_DIRECTORY_RECORDS_END_MARKER 0x00

// Directory record attributes flags testing
#define FAT32_IS_READ_ONLY(attr) ((attr) & 0x1)
#define FAT32_IS_HIDDEN(attr) ((attr) & 0x2)
#define FAT32_IS_SYSTEM(attr) ((attr) & 0x4)
#define FAT32_IS_VOLUME_LABEL(attr) ((attr) & 0x8)
#define FAT32_IS_SUBDIRECTORY(attr) ((attr) & 0x10)
#define FAT32_IS_ARCHIVE(attr) ((attr) & 0x20)
#define LFN_SEQ_NUM(attr) ((attr) & 0x1F)

#define FAT32_ATTR_LONG_FILENAME_MASK 0x3F
#define FAT32_IS_LONG_FILENAME(attr) (((attr) & 0x3F) == 0xF)

#define FAT32_IS_LFN_LAST_LONG_ENTRY(seq_num) (((seq_num) & 0x40) != 0)

#define MAX_CLUSTER_CHAIN_LEN 32
#define MAX_FILESYSTEM_DIRECTORY_FILES 250

#define FAT32_CURRENT_DIR_SHORT_FILENAME ".\0\0\0\0\0\0\0\0\0\0"
#define FAT32_PARENT_DIR_SHORT_FILENAME "..\0\0\0\0\0\0\0\0\0"
#define BLANK_STR "\0\0\0\0"
#define FAT32_ROOT_DIR_MAX_SIZE 512

typedef struct PartitionEntry PartitionEntry;
typedef struct MasterBootRecord MasterBootRecord;
typedef struct FAT32VolumeID FAT32VolumeID;
typedef struct Directory_Entry Directory_Entry;
typedef struct FileSystem_Node FileSystem_Node;
typedef struct LFN_Directory_Entry LFN_Directory_Entry;
typedef struct Directory_Record Directory_Record;
typedef struct FAT32_FileSystem_Handle FAT32_FileSystem_Handle;

typedef struct PartitionEntry {
	byte state;
	byte chs_begin[3];
	byte type;
	byte chs_end[3];
	unsigned int lba_begin;
	unsigned int nb_sectors;
} PartitionEntry;

typedef struct MasterBootRecord {
	char oem_name[9];
	PartitionEntry entries[4];
	byte signature[2];
} MasterBootRecord;

typedef struct GPTHeader {
	byte signature[8];
	byte version[4];
	int current_size; // little indian
	int current_crc32;
	long long header_start_lba;
	long long backup_start_lba;
	long long data_start_lba;
	long long data_end_lba;
	byte guid[16];
	long long partitions_table_lba;
	int partitions_table_length;
	int partition_entry_size;
	int partitions_table_crc32;
} GPTHeader;

typedef struct GPTPartitionEntry {
	byte guid_type[16];
	byte guid[16];
	long long first_lba;
	long long last_lba;
	long long attr_flags;
	byte partition_name[72];
} GPTPartitionEntry;

typedef struct FAT32VolumeID {
	// some needed FAT32 VolumeID table data
	short bytes_per_sector; // always 512
	byte sectors_per_cluster; // power of 2
	short nb_reserved_sectors; // usually 0x20
	byte nb_FAT_tables; // always 2
	int sectors_per_FAT; // depends on disk size
	int root_dir_first_cluster; // usually 0x0..2
	byte signature[2];
	// some calculated data
	unsigned long fat_begin_lba;
	unsigned long clusters_begin_lba;
} FAT32VolumeID;

// TODO : use unions to handle long file names entries
typedef struct Directory_Entry {
	char filename[8];
	char filename_extension[3];
	byte file_attribs;
	int starting_cluster_num;
	int file_size;
	// time data
	short creation_time[3];
	short creation_date[3];
	short last_access_date[3];
	short last_write_time[3];
	short last_write_date[3];
} Directory_Entry;

typedef struct LFN_Directory_Entry {
	byte sequence_number;
	wchar_t file_name_part[13];
	byte file_attribs;
	byte SFN_entry_checksum;
} LFN_Directory_Entry;

typedef struct Directory_Record {
	byte short_file_name[13];
	wchar_t long_file_name[260];
	byte file_attribs;
	int starting_cluster_num;
	int file_size;
	bool is_directory;
	// usefull debug infor
	int nb_LFN_entries;
	// time data
	short creation_time[3];
	short creation_date[3];
	short last_access_date[3];
	short last_write_time[3];
	short last_write_date[3];
	//
	int cluster_offset;
	int cluster_num;
} Directory_Record;

typedef struct FileSystem_Node {
	int cluster_num;
	FileSystem_Node* directories_nodes[FAT32_ROOT_DIR_MAX_SIZE];
	Directory_Record* directories_records[FAT32_ROOT_DIR_MAX_SIZE];
	int nb_subdirectories;

	Directory_Record* files_records[FAT32_ROOT_DIR_MAX_SIZE];
	int nb_files;
	FileSystem_Node* current;
	FileSystem_Node* parent;
} FileSystem_Node;

typedef struct FAT32_FileSystem_Handle {
	FILE* disk;
	FAT32VolumeID* volume_id;
	FileSystem_Node* fs_node;
	Directory_Record* volume_label;
} FAT32_FileSystem_Handle;

Directory_Record* init_directory_record();

wchar_t cast_utf16(char16_t c);

void convert_utf16_to_wchar(byte* str, int str_len, wchar_t* w_str, int* wstr_len);

// cluster pointer should e pointing to already allocated memory of sufficient size
bool load_cluster(FILE* disk, FAT32VolumeID* volume_id, int cluster_num, byte* cluster);

// sector pointer should e pointing to already allocated memory of sufficient size
bool load_sector(FILE* disk, int lba_adr, byte* sector);

// TODO : handle the first 446 bytes of MBR sector 
// disk file should be open
MasterBootRecord* load_MBR_sector(FILE* disk);

GPTHeader* load_GPT_header(FILE* disk);

GPTPartitionEntry* load_gpt_partition_entry(FILE* disk, GPTHeader* gpt_header, int index);

void print_MBR_sector_info(MasterBootRecord* mbr);

// load FAT32 VolumeID table informations using MBR data
FAT32VolumeID* load_VolumeID_table(FILE* disk, MasterBootRecord* mbr);
FAT32VolumeID* load_VolumeID_table_gpt(FILE* disk, GPTPartitionEntry* part_info);

void print_FAT32_volume_id(FAT32VolumeID* volume_id);

void read_time(byte* ptr, short time[3]);

void read_date(byte* ptr, short time[3]);

Directory_Entry* read_short_filename_entry(byte buffer[DIRECTORY_ENTRY_SIZE]);

LFN_Directory_Entry* read_long_filename_entry(byte buffer[DIRECTORY_ENTRY_SIZE]);

void print_dir_record(Directory_Record* entry);

// *nb_clusters is the maximum length of the cluster chain
// cluster_numbers is a previously allocated cluster numbers array
// *nb_clusters will hold the number of the clusters in the chaine
// the 4 ignored MSB of 32bits FAT entries are handled here 
void fetch_cluster_chain(FILE* disk, FAT32VolumeID* volume_id, int start_cluster_number, int* cluster_numbers, int* nb_cluster);

// create SFN entry checksum
byte create_sum(Directory_Entry* entry);

// directory_records should be allocated previously and directory_records_size contains its size
void fetch_directory_records(FILE* disk, FAT32VolumeID* volume_id, int start_cluster_number, Directory_Record** directory_records, int* directory_records_size);

FileSystem_Node* init_FAT32_filesystem_node();

FAT32_FileSystem_Handle* init_FAT32_filesystem_handle();

FAT32_FileSystem_Handle* load_FAT32_filesystem_root(FILE* disk, FAT32VolumeID* volume_id);

void load_subdirectory(FAT32_FileSystem_Handle* fs_handle, FileSystem_Node* node, int subdir_num);

#define LINE_SIZE 32
void print_sector(int lba_adr, byte* buffer);

void delete_file(FAT32_FileSystem_Handle* fs_handle, FileSystem_Node* curent_dir, int file_num);

bool is_fat32_partition(PartitionEntry* partition);
bool is_using_gpt(MasterBootRecord* mbr);

#define assert(expr, msg) if(!(expr)) { \
	printf("Error : %s, at %s:%d \n", msg, __FILE__, __LINE__); \
	exit(-1); \
}

#define silent_assert(expr, msg) if(!(expr)) { \
	printf("Warning : %s, at %s:%d \n", msg, __FILE__, __LINE__); \
}