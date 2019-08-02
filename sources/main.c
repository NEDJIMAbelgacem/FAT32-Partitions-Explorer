
#ifdef LINUX
#define DISK_FILE_PATH "/dev/sdc"
#endif
#ifdef WINDOWS
#define DISK_FILE_PATH "\\\\.\\PhysicalDrive1"
#endif

#include "fat_explorer.h"

int main(int argc, char* argv[]) {
	int cluster_num = 2;
	if (argc > 1) cluster_num = atoi(argv[1]);
	FILE* disk = NULL;
	disk = fopen(DISK_FILE_PATH, "r");
	if (disk == NULL) {
		printf("ERROR : couldn\'t open disk at \"%s\"\n", DISK_FILE_PATH);
		exit(-1);
	}
	MasterBootRecord* mbr = load_MBR_sector(disk);
	// print_MBR_sector_info(mbr);
	FAT32VolumeID* volume_id = load_VolumeID_table(disk, mbr);
	if (volume_id == NULL) {
		printf("Failed to load Volume ID table \n");
		exit(-1);
	}
	// print_FAT32_volume_id(volume_id);
	FAT32_FileSystem_Handle* fs_handle = load_FAT32_filesystem_root(disk, volume_id);
	FileSystem_Node* current_node = fs_handle->fs_node;
	while (true) {
		printf("=====================================================\n");
		printf("Choose one of the commands below : \n");
		printf("1 -> Print Master Boot Record of the current disk \n");
		printf("2 -> Print the Boot Sector (aka volume ID) of the first partition in disk (FAT32) \n");
		printf("3 -> Print all file name records informations of current directory (defaults to root) \n");
		printf("4 -> Print all subdirectories names of the current directory \n");
		printf("5 -> Print all file names of the current directory \n");
		printf("6 -> navigate to the parent directory \n");
		printf("7 -> navigate to a subdirectory \n");
		printf("8 -> Print sector data \n");
		printf("9 -> Delete a file/directory \n");
		printf("99 -> exit \n");
		int command, subdir_num, lba_adr;
		byte sector_buffer[SECTOR_SIZE];
		printf(" --> input command number : ");
		scanf("%d", &command);
		switch (command) {
		case 1:
			printf("=====================================================\n");
			printf("Printing some MBR sector info\n");
			printf("=====================================================\n");
			print_MBR_sector_info(mbr);
			printf("=====================================================\n");
			break;
		case 2:
			printf("=====================================================\n");
			printf("Printing some FAT32 boot sector info\n");
			printf("=====================================================\n");
			print_FAT32_volume_id(volume_id);
			printf("=====================================================\n");
			break;
		case 3:
			printf("=====================================================\n");
			printf("Printing current directory records info\n");
			printf("=====================================================\n");
			for (int i = 0; i < current_node->nb_subdirectories; ++i) print_dir_record(current_node->directories_records[i]);
			for (int i = 0; i < current_node->nb_files; ++i) print_dir_record(current_node->files_records[i]);
			printf("=====================================================\n");
			break;
		case 4:
			printf("=====================================================\n");
			printf("Printing current directory subdirectories names\n");
			printf("=====================================================\n");
			for (int i = 0; i < current_node->nb_subdirectories; ++i) {
				if (current_node->directories_records[i]->long_file_name[0] == 0) {
					printf("%d (short file name) : %s \n", i, current_node->directories_records[i]->short_file_name);
				} else {
					printf("%d (long file name) : %ls \n", i, current_node->directories_records[i]->long_file_name);
				}
			}
			printf("=====================================================\n");
			break;
		case 5:
			printf("=====================================================\n");
			printf("Printing current directory files names\n");
			printf("=====================================================\n");
			for (int i = 0; i < current_node->nb_files; ++i) {
				if (current_node->files_records[i]->long_file_name[0] == 0) {
					printf("%d (short file name) : %s \n", i, current_node->files_records[i]->short_file_name);
				} else {
					printf("%d (long file name) : %ls \n", i, current_node->files_records[i]->long_file_name);
				}
			}
			printf("=====================================================\n");
			break;
		case 6:
			current_node = current_node->parent;
			break;
		case 7:
			printf("=====================================================\n");
			printf("Printing current directory files names\n");
			printf("=====================================================\n");
			printf("Input subdirectory number : ");
			scanf("%d", &subdir_num);
			load_subdirectory(fs_handle, current_node, subdir_num);
			if (subdir_num < current_node->nb_subdirectories) current_node = current_node->directories_nodes[subdir_num];
			printf("=====================================================\n");
			break;
		case 8:
			printf("=====================================================\n");
			printf("Printing sector data \n");
			printf("=====================================================\n");
			printf("Input sector LBA : ");
			scanf("%d", &lba_adr);
			load_sector(disk, volume_id, lba_adr, sector_buffer);
			print_sector(lba_adr, sector_buffer);
			printf("=====================================================\n");
			break;
		case 9:
			// delete a file or directory
			break;
		case 10:
			// display deleted files
			break;
		case 11:
			// rename a file
			break;
		default:
			exit(0);
			break;
		}
	}
}
