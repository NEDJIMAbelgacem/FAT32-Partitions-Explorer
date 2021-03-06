#include <string.h>

#ifdef LINUX
#define DISK_FILE_PATH "/dev/sdc"
#endif
#ifdef WINDOWS
#define DISK_FILE_PATH "\\\\.\\PhysicalDrive1"
#endif

#include "fat_explorer.h"

#define wait() printf("Press any key to continue\n");\
			 getchar(); getchar()

// find currently availlable disks
void probe_disks(char* disks[], int* nb_disks) {
	system("ls /dev/ | grep [sh]d.$ > tmp");
	FILE* tmp = fopen("tmp", "r");
	int max_disks_count = *nb_disks, n = 1;
	*nb_disks = 0;
	while (n > 0) {
		n = fscanf(tmp, "%s", disks[(*nb_disks)++]);
	}
	(*nb_disks)--;
	fclose(tmp);
}

FAT32VolumeID* load_VolumeID(FILE* disk, MasterBootRecord* mbr) {
	if (is_using_gpt(mbr)) {
		GPTHeader* gpt = load_GPT_header(disk);
		// TODO : make it so I can read any partition
		GPTPartitionEntry* partition_entry = load_gpt_partition_entry(disk, gpt, 0);
		FAT32VolumeID* volume_id = load_VolumeID_table_gpt(disk, partition_entry);
		free(gpt);
		free(partition_entry);
		return volume_id;
	}
	return load_VolumeID_table(disk, mbr);
}

int main(int argc, char* argv[]) {
	setlocale(LC_CTYPE,"UTF-16");

	int cluster_num = 2;
	if (argc > 1) cluster_num = atoi(argv[1]);
	FILE* disk = NULL;
	
	char* disk_names[5];
	int nb_disks = 5;
	for (int i = 0; i < nb_disks; ++i) disk_names[i] = (char*) malloc(6 * sizeof(char));
	probe_disks(disk_names, &nb_disks);

	printf("Welcome :D \n");
	int selected_disk_index = -1;
	while (selected_disk_index <= 0 || selected_disk_index > nb_disks) {
		printf("Please select one of the following disks and make sure it is FAT32 formatted \n");
		for (int i = 0; i < nb_disks; ++i) printf("%d-%s\n", i + 1, disk_names[i]);
		printf("Selected disk : ");
		scanf("%d", &selected_disk_index);
	}
	char* disk_path = (char*) malloc(255 * sizeof(char));
	strcpy(disk_path, "/dev/");
	strcpy(disk_path + 5, disk_names[selected_disk_index - 1]);
	printf("Opening %s \n", disk_path);
	disk = fopen(disk_path, "r+b");
	assert(disk != NULL, "openning disk");
	MasterBootRecord* mbr = load_MBR_sector(disk);
	// byte sector_buffer[SECTOR_SIZE];
	// assert(load_sector(disk, 0, sector_buffer), "loading MBR");
	GPTHeader* gpt = NULL;
	FAT32VolumeID* volume_id = NULL;
	bool has_gpt = is_using_gpt(mbr);

	if (has_gpt) {
		gpt = load_GPT_header(disk);
		// TODO : make it so I can read any partition
		GPTPartitionEntry* partition_entry = load_gpt_partition_entry(disk, gpt, 0);
		volume_id = load_VolumeID_table_gpt(disk, partition_entry);
		free(partition_entry);
	} else volume_id = load_VolumeID(disk, mbr);

	assert(volume_id != NULL, "Failled to load FAT32 Volume ID table (aka Boot Sectoor) \n");
	
	FAT32_FileSystem_Handle* fs_handle = load_FAT32_filesystem_root(disk, volume_id);
	FileSystem_Node* current_node = fs_handle->fs_node;
	while (true) {
		system("clear");
		printf("=====================================================\n");
		printf("Choose one of the commands below : \n");
		printf("1 -> Print Master Boot Record of the current disk \n");
		printf("2 -> Print the Boot Sector (aka volume ID) of the first partition in disk (FAT32) \n");
		printf("3 -> Print file name records informations of a file or a subdirectory of the current directory \n");
		printf("4 -> Print all subdirectories names of the current directory \n");
		printf("5 -> Print all file names of the current directory \n");
		printf("6 -> navigate to the parent directory \n");
		printf("7 -> navigate to a subdirectory \n");
		printf("8 -> Print sector data \n");
		printf("9 -> Print all deleted files of the current directory \n");
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
			wait();
			break;
		case 2:
			printf("=====================================================\n");
			printf("Printing some FAT32 boot sector info\n");
			printf("=====================================================\n");
			print_FAT32_volume_id(volume_id);
			printf("=====================================================\n");
			wait();
			break;
		case 3: 
			printf("=====================================================\n");
			printf("Printing current directory records info\n");
			printf("=====================================================\n");
			{
				printf("|-----|-------------------------------------|----------------|------| \n");
				printf("| num | long name                           | short name     | type | \n");
				printf("|-----|-------------------------------------|----------------|------| \n");

				for (int i = 0; i < current_node->nb_subdirectories; ++i) {
					printf("| %3.3d | %35.35ls | %14.14s | dir  | \n", i, current_node->directories_records[i]->long_file_name, 
						current_node->directories_records[i]->short_file_name);
					printf("|-----|-------------------------------------|----------------|------| \n");
				}

				for (int i = 0; i < current_node->nb_files; ++i) {
					printf("| %3.3d | %35.35ls | %14.14s | file | \n", current_node->nb_subdirectories + i, current_node->files_records[i]->long_file_name, 
						current_node->files_records[i]->short_file_name);
					printf("|-----|-------------------------------------|----------------|------| \n");
				}
				int num;
				printf("Input number : ");
				scanf("%d", &num);
				if (num >= 0) {
					if (num < current_node->nb_subdirectories) {
						print_dir_record(current_node->directories_records[num]);
					} else if (num - current_node->nb_subdirectories < current_node->nb_files) {
						print_dir_record(current_node->files_records[num - current_node->nb_subdirectories]);
					}
				}
				printf("=====================================================\n");
			}
			wait();
			break;
		case 4: // List current directory subdirectories
			printf("=====================================================\n");
			printf("Printing current directory subdirectories names\n");
			printf("=====================================================\n");
			printf("|-------------------------------------|----------------| \n");
			printf("| long name                           | short name     | \n");
			printf("|-------------------------------------|----------------| \n");
			for (int i = 0; i < current_node->nb_subdirectories; ++i) {
				printf("| %35.35ls | %14.14s | \n", current_node->directories_records[i]->long_file_name, 
					current_node->directories_records[i]->short_file_name);
				printf("|-------------------------------------|----------------| \n");
			}
			wait();
			break;
		case 5: // List current directory files
			printf("=====================================================\n");
			printf("Printing current directory files names\n");
			printf("=====================================================\n");
			printf("|-------------------------------------|----------------| \n");
			printf("| long name                           | short name     | \n");
			printf("|-------------------------------------|----------------| \n");
			for (int i = 0; i < current_node->nb_files; ++i) {
				if (current_node->files_records[i]->is_deleted_file) continue;
				printf("| %35.35ls | %14.14s | \n", current_node->files_records[i]->long_file_name, 
					current_node->files_records[i]->short_file_name);
				printf("|-------------------------------------|----------------| \n");
			}
			wait();
			break;
		case 6: // navigate to parent directory
			current_node = current_node->parent;
			break;
		case 7: // navigate to subdirectory
			printf("=====================================================\n");
			printf("Printing current directory subdirectories names\n");
			printf("=====================================================\n");
			printf("|-----|-------------------------------------|----------------| \n");
			printf("| num | long name                           | short name     | \n");
			printf("|-----|-------------------------------------|----------------| \n");
			for (int i = 0; i < current_node->nb_subdirectories; ++i) {
				printf("| %3.3d | %35.35ls | %14.14s | \n", i, current_node->directories_records[i]->long_file_name, 
					current_node->directories_records[i]->short_file_name);
				printf("|-----|-------------------------------------|----------------| \n");
			}
			printf("Input subdirectory number : ");
			scanf("%d", &subdir_num);
			load_subdirectory(fs_handle, current_node, subdir_num);
			if (subdir_num < current_node->nb_subdirectories) current_node = current_node->directories_nodes[subdir_num];
			printf("=====================================================\n");
			break;
		case 8: // Print sector data
			printf("=====================================================\n");
			printf("Printing sector data \n");
			printf("=====================================================\n");
			printf("Input sector LBA : ");
			scanf("%d", &lba_adr);
			load_sector(disk, lba_adr, sector_buffer);
			print_sector(lba_adr, sector_buffer);
			printf("=====================================================\n");
			wait();
			break;
		case 9: // display the list of current directory's deleted files
			printf("=====================================================\n");
			printf("Printing current directory deleted files \n");
			printf("=====================================================\n");
			printf("|-------------------------------------|----------------| \n");
			printf("| long name                           | short name     | \n");
			printf("|-------------------------------------|----------------| \n");
			for (int i = 0; i < current_node->nb_files; ++i) {
				if (!current_node->files_records[i]->is_deleted_file) continue;
				printf("| %35.35ls | %14.14s | \n", current_node->files_records[i]->long_file_name, 
					current_node->files_records[i]->short_file_name);
				printf("|-------------------------------------|----------------| \n");
			}
			printf("=====================================================\n");
			wait();
			break;
		case 10:
			for (int i = 0; i < volume_id->sectors_per_cluster; ++i)
			{
				byte sector[512];
				load_sector(disk, volume_id->clusters_begin_lba + i, sector);
				print_sector(volume_id->clusters_begin_lba + i, sector);
			}
			wait();
			break;
		default:
			exit(0);
			break;
		}
	}
}
