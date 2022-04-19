/*
 * test-my-malloc.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

/* Constants */
#define NORMAL_FLAG 0x00
#define ALL_FLAG 0x01
#define LONG_FLAG 0x02

/* Prototypes */
int parse_flags(int argc, char *argv[]);
void list_files(int flags, char *path);
void print_long(struct stat current_stat, char *file_name);

struct stat input_stat;

int main(int argc, char *argv[]) {
	int flags = NORMAL_FLAG;

	/* Determines the flags from the option arguments and assigns that
	value to the flags */
	flags = parse_flags(argc, argv);

	/* If there no nonoption arguments */
	if (argc <= optind) {
		/* If there are no nonoption arguments */
		list_files(flags, ".");
	}
	else {
		/* If there is only one given non option input so that it does
		   not print ".:" */
		if ((optind + 1) == argc) {
			list_files(flags, argv[optind]);
		}
		/* Because there are multiple inputs it has changed formatting */
		else {
			for (int i = optind; i < argc; i ++) {
				printf ("%s:\n", argv[i]);
				list_files(flags, argv[i]);
				printf ("\n");
			}
		}
	}
	return 0;
}


int parse_flags(int argc, char *argv[]) {
	/* parse command line arguments and set flags:
	 * flags 0b00 being no flagss or show_normal_files()
	 * flags 0b01 being -a or show_hidden_files()
	 * flags 0b10 being -l or show_long_files()
	 * flags 0b11 being -a and -l or show_hidden_long_files() */

	int c;
	int flags = NORMAL_FLAG;

	opterr = 0;
	while ((c = getopt (argc, argv, "al")) != -1) {
		switch (c) {
			case 'a':
				flags = flags | ALL_FLAG;
			 	break;
			case 'l':
				flags = flags | LONG_FLAG;
				break;
			case '?':
				fprintf(stderr, "Unknown option '-%c'.\n",optopt);
				exit(1);
		}
	}
	return flags;
}


void list_files(int flags, char *path) {
	/* Calls different flags functions based on given flags */

	/* Initializing variables */
	DIR *curr_dir;
	struct dirent *curr_dirent;
	struct stat input_stat;
	struct stat current_stat;
	char absolute_path[1024];
	int column_counter = 0;

	if (stat(path, &input_stat) == -1) {
		/* If the path cannot be found and stat returns -1 */
		perror("stat");
		return;
	}

	if (S_ISREG(input_stat.st_mode)) {
		/* If the error is a "Not a directory"
		Get stats of element in target dir*/
		if (lstat(path, &current_stat) == -1) {
			perror("lstat");
			return;
		}
		else if (flags & LONG_FLAG) {
			print_long(current_stat, path);
		}
		else{
			printf("%s\n", path);
		}
	}
	else if (S_ISDIR(input_stat.st_mode)) {
		/* Attempt to open the path as a directory */
		curr_dir = opendir(path);
		if (curr_dir == NULL){
			perror("opendir");
			return;
		}
		while (((curr_dirent = readdir(curr_dir)) != NULL)&&
			(column_counter <= 80)) {
			if ((flags & ALL_FLAG) || (curr_dirent->d_name[0] != '.')) {
				/* For every element in the directory that does not start
				with a "." get info and print it */

				/* concat together path and directory name to get abs path*/
				snprintf(absolute_path, sizeof absolute_path, "%s%s%s", path, "/",
					curr_dirent->d_name);

				/* Get stats of element in target dir*/
				if (lstat(absolute_path, &current_stat) == -1) {
					perror("lstat");
					return;
				}

				/* Given a current stat and the name to be printed, it parses and pretty
				 prints a long line*/
				if (flags & LONG_FLAG) {
					print_long(current_stat, curr_dirent->d_name);
				}
				else{
					printf("%s\n", curr_dirent->d_name);
				}

				/* Incrememnt column number*/
				column_counter ++;
			}
		}
	}
	else{
		perror("lstat");
		return;
	}
}

void print_long(struct stat current_stat, char *file_name) {
	/* Given an input stat struct and file name it creates
	and pretty prints that string with the human readable stats */
	struct tm long_time;
	struct passwd *pwd;
	struct group *grp;
	char passwd_name[32];
	char group_name[32];
	char perm_string[11];
	char time_buff[80];

	/* Gets human readable sequence of permission characters*/
	perm_string[0] = (S_ISDIR(current_stat.st_mode)) ? 'd' : '-';
	perm_string[1] = (current_stat.st_mode & S_IRUSR) ? 'r' : '-';
	perm_string[2] = (current_stat.st_mode & S_IWUSR) ? 'w' : '-';
	perm_string[3] = (current_stat.st_mode & S_IXUSR) ? 'x' : '-';
	perm_string[4] = (current_stat.st_mode & S_IRGRP) ? 'r' : '-';
	perm_string[5] = (current_stat.st_mode & S_IWGRP) ? 'w' : '-';
	perm_string[6] = (current_stat.st_mode & S_IXGRP) ? 'x' : '-';
	perm_string[7] = (current_stat.st_mode & S_IROTH) ? 'r' : '-';
	perm_string[8] = (current_stat.st_mode & S_IWOTH) ? 'w' : '-';
	perm_string[9] = (current_stat.st_mode & S_IXOTH) ? 'x' : '-';
	perm_string[10] = '\0';

	/* Make time human readable*/
	localtime_r(&current_stat.st_mtime, &long_time);
	strftime(time_buff, sizeof(time_buff), "%b %d %H:%M", &long_time);

	/* Get PWD names from ids*/
	pwd = getpwuid(current_stat.st_uid);
	if(pwd == NULL) {
		sprintf(passwd_name, "%d", current_stat.st_uid);
	}
	else {
		sprintf(passwd_name, "%s", pwd->pw_name);

	}

	/* Get Group names from ids*/
	grp = getgrgid(current_stat.st_gid);
	if(grp == NULL) {
		sprintf(group_name, "%d", current_stat.st_gid);
	}
	else {
		sprintf(group_name, "%s", grp->gr_name);
	}

	/* Print formatted entry*/
	printf("%s %ld %s %s %5ld %s %s\n", perm_string,
	current_stat.st_nlink, passwd_name, group_name,
	current_stat.st_size, time_buff, file_name);
}
