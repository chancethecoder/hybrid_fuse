/*
 * myfs.c
 * usage : myfs mnt dir_1 dir_2
 * 
 * mount dir_1 and dir_2 to mnt
 * 
 * */

#define FUSE_USE_VERSION 30

#include <fuse.h>

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


/*-----------------------------------------------------------------*/
// Utilizer

#define MY_DATA ((my_data *) fuse_get_context()->private_data)
#define MAX_PATH 256

typedef struct{
    char *rootdir_fir;
    char *rootdir_sec;
	int dir_len;
}my_data;

static void get_path(char fpath[MAX_PATH], const char *path, int idx)
{
	char tpath[MAX_PATH];

	switch(idx)
	{
		case 1 : strcpy(tpath, MY_DATA->rootdir_fir);break;
		case 2 : strcpy(tpath, MY_DATA->rootdir_sec);break;
		default	: strcpy(tpath, path);break;
	}
	strncpy(fpath, tpath, PATH_MAX);
}


/*-----------------------------------------------------------------*/
// Handlers

static void* my_init(struct fuse_conn_info *conn)
{
	return MY_DATA;
}

static int my_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char fpath[MAX_PATH];

	get_path(fpath, path, 1);

	res = lstat(fpath, stbuf);
	if(res == -1)
		return -errno;
	
	return 0;
}

static int my_opendir(const char *path, struct fuse_file_info *fi)
{
	DIR *dp;
	char fpath[MAX_PATH];

	get_path(fpath, path, 1);

	dp = opendir(fpath);
	if(dp == NULL)
		return -errno;

	fi->fh = (intptr_t) dp;

	return 0;
}

static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
					off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	DIR *dp;
	struct dirent *de;

	dp = (DIR *) (uintptr_t) fi->fh;

	if(dp == NULL)
		return -errno;

	while((de = readdir(dp)) != NULL)
		if(filler(buf, de->d_name, NULL, 0) != 0) return -errno;

	return 0;
}

static int my_open(const char *path, struct fuse_file_info *fi)
{
	int fd;
	char fpath[MAX_PATH];

	get_path(fpath, path, 1);

	fd = open(fpath, fi->flags);
	if(fd < 0)
		return -errno;

	fi->fh = fd;
	return 0;
}

static int my_read(const char *path, char *buf, size_t size, off_t offset,
					struct fuse_file_info *fi)
{
	int res = 0;
	(void) fi;

	res = pread(fi->fh, buf, size, offset);
	if(res < 0)
		res = -errno;

	return res;
}

static int my_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
	int res = 0;
	res = ftruncate(fi->fh, offset);
	if(res < 0)
		return -errno;
	return 0;
}

static int my_access(const char *path, int mask)
{
	int res = 0;
	char fpath[PATH_MAX];

	get_path(fpath, path, 1);

	res = access(fpath, mask);
	if(res < 0)
		return -errno;
	
	return 0;
}

static int my_readlink(const char *path, char *link, size_t size)
{
	int res = 0;
	char fpath[MAX_PATH];

	get_path(fpath, path, 1);

	res = readlink(fpath, link, size - 1);
	if(res < 0) 
		return -errno;
	else
		link[res] = '\0';

	return 0;
}

static struct fuse_operations my_oper = {
	.init		= my_init,

	.getattr	= my_getattr,
	.opendir	= my_opendir,
	.readdir	= my_readdir,
	.open		= my_open,
	.read		= my_read,
	.ftruncate	= my_ftruncate,

	.access		= my_access,
	.readlink	= my_readlink,
};

int main(int argc, char **argv)
{
	
	printf("args\n"
			"prog : %s\n"
			"mount point : %s\n"
			"directory_1 : %s\n"
			"directory_2 : %s\n", argv[0]+2, argv[1], argv[2], argv[3]);

	my_data *data;
	data = malloc(sizeof(my_data));

	data->dir_len = argc - 2;
	data->rootdir_fir = realpath(argv[argc-2], NULL);
	data->rootdir_sec = realpath(argv[argc-1], NULL);
	argv[argc-2] = NULL;
	argv[argc-1] = NULL;
	argc -= 2;

	return fuse_main(argc, argv, &my_oper, data);
}
