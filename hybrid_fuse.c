/** Hybrid file system in FUSE
 *
 * Usage:  myfuse mount_point rootDir rootDir
 * 
 * Mount_point has to be an empty directory.
 * This will show two root directories as one directory.
 *
 * By Youngkyun Kim
 */

#define FUSE_USE_VERSION 30
#define MY_DATA ((my_data *) fuse_get_context()->private_data)

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif



///////////////////////////////////////////////////////////////////

typedef struct{
	char **rootdir;
	int dir_cnt;
}my_data;


static void get_path(char fpath[PATH_MAX], const char *path, int idx)
{
	if(idx >= MY_DATA->dir_cnt)
		idx = 0;
	strncpy(fpath, MY_DATA->rootdir[idx], PATH_MAX);
	strncat(fpath, path, PATH_MAX);
}

////////////////////////////////////////////////////////////////////


/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
static int my_getattr(const char *path, struct stat *statbuf)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = lstat(fpath, statbuf);

		if(res == 0) 
			return res;
	}

	return -errno;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.  If the linkname is too long to fit in the
 * buffer, it should be truncated.  The return value should be 0
 * for success.
 */
static int my_readlink(const char *path, char *link, size_t size)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = readlink(fpath, link, size - 1);
		if(res >= 0)
		{
			link[res] = '\0';
			return 0;
		}
	}

	return -errno;
}

/** Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
static int my_mknod(const char *path, mode_t mode, dev_t dev)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		if (S_ISREG(mode)) 
		{
			res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
			if (res == 0)
				return res;
			res = close(res);
			if (res == 0)
				return res;
		} 
		else if (S_ISFIFO(mode)) 
		{
			res = mkfifo(fpath, mode);
			if (res == 0)
				return res;
		}
		else 
		{
			res = mknod(fpath, mode, dev);
			if (res == 0)
				return res;
		}
	}

	return -errno;
}

/** Create a directory */
static int my_mkdir(const char *path, mode_t mode)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = mkdir(fpath, mode);

		if(res == 0)
			return res;
	}

	return -errno;
}

/** Remove a file */
static int my_unlink(const char *path)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = unlink(fpath);
		if (res == 0)
			return res;
	}

	return -errno;
}

/** Remove a directory */
static int my_rmdir(const char *path)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = rmdir(fpath);
		if (res == 0)
			return res;
	}

	return -errno;
}

/** Create a symbolic link */
static int my_symlink(const char *path, const char *link)
{
	int res = 0;
	char fpath[PATH_MAX];
	char flink[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(flink, link, iter);

		get_path(fpath, path, iter);

		res = symlink(fpath, flink);
		if(res == 0)
			return res;
	}

	return -errno;
}

/** Rename a file */
static int my_rename(const char *path, const char *newpath)
{
	int res = 0;
	char fpath[PATH_MAX];
	char fnewpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		get_path(fnewpath, newpath, iter);

		res = rename(fpath, fnewpath);
		if (res == 0)
			return res;
	}

	return -errno;
}

/** Create a hard link to a file */
static int my_link(const char *path, const char *newpath)
{
	int res = 0;
	char fpath[PATH_MAX];
	char fnewpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		get_path(fnewpath, newpath, iter);

		res = link(fpath, fnewpath);
		if (res == 0)
			return res;
	}

	return -errno;
}

/** Change the permission bits of a file */
static int my_chmod(const char *path, mode_t mode)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = chmod(fpath, mode);
		if (res == 0)
			return res;
	}

	return -errno;
}

/** Change the owner and group of a file */
static int my_chown(const char *path, uid_t uid, gid_t gid)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = chown(fpath, uid, gid);
		if (res == 0)
			return res;
	}

	return -errno;
}

/** Change the size of a file */
static int my_truncate(const char *path, off_t newsize)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = truncate(fpath, newsize);
		if (res == 0)
			return res;
	}

	return -errno;
}

/** Change the access and/or modification times of a file */
static int my_utime(const char *path, struct utimbuf *ubuf)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = utime(fpath, ubuf);
		if (res == 0)
			return res;
	}

	return -errno;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
static int my_open(const char *path, struct fuse_file_info *fi)
{
	int res = 0;
	int fd;
	char fpath[PATH_MAX];


	// just do it for path arg.

	fd = open(path, fi->flags);
	if (fd > 0)
	{
		fi->fh = fd;
		return res;
	}

	// if not opened, try all root directory

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		fd = open(fpath, fi->flags);
		if(fd > 0)
		{
			fi->fh = fd;
			return res;
		}
	}

	return -errno;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */;
static int my_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int res = 0;

	res = pread(fi->fh, buf, size, offset);
	if (res < 0)
		return -errno;

	return res;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
static int my_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
	int res = 0;

	res = pwrite(fi->fh, buf, size, offset);
	if (res < 0)
		return -errno;

	return res;
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
static int my_statfs(const char *path, struct statvfs *statv)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = statvfs(fpath, statv);
		if (res == 0)
			return res;
	}

	return -errno;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
static int my_flush(const char *path, struct fuse_file_info *fi)
{
	int res = 0;	
	return res;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
static int my_release(const char *path, struct fuse_file_info *fi)
{
	int res = 0;

	res = close(fi->fh);

	return res;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
static int my_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
	int res = 0;

#ifdef HAVE_FDATASYNC
	if (datasync)
		res = fdatasync(fi->fh);
	else
#endif	
		res = fsync(fi->fh);

	if (res < 0)
		return -errno;

	return res;
}


#ifdef HAVE_SYS_XATTR_H
/** Set extended attributes */
static int my_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = lsetxattr(fpath, name, value, size, flags);
		if (res == 0)
			return res;
	}

	return -errno;
}

/** Get extended attributes */
static int my_getxattr(const char *path, const char *name, char *value, size_t size)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = lgetxattr(fpath, name, value, size);
		if (res == 0)
			return res;
	}

	return -errno;
}

/** List extended attributes */
static int my_listxattr(const char *path, char *list, size_t size)
{
	int res = 0;
	char fpath[PATH_MAX];
	char *ptr;

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = llistxattr(fpath, list, size);
		if (res == 0)
			return res;
	}

	return -errno;
}

/** Remove extended attributes */
static int my_removexattr(const char *path, const char *name)
{
	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = lremovexattr(fpath, name);
		if (res == 0)
			return res;
	}

	return -errno;
}
#endif


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
static int my_opendir(const char *path, struct fuse_file_info *fi)
{
	DIR *dp;
	int res = 0;
	char fpath[PATH_MAX];


	// just do it for path arg.

	dp = opendir(path);
	if (dp != NULL)
	{
		fi->fh = (intptr_t) dp;
		return res;
	}

	// if not opened, try all root directory

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		dp = opendir(fpath);
		if(dp != NULL)
		{
			fi->fh = (intptr_t) dp;
			return res;
		}
	}

	return -errno;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, 
		off_t offset, struct fuse_file_info *fi)
{
	int res = 0;
	char fpath[PATH_MAX];
	DIR *dp;
	struct dirent *de;

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		dp = opendir(fpath);

		if(dp == NULL)
			continue;

		while ((de = readdir(dp)) != NULL)
		{
			res = filler(buf, de->d_name, NULL, 0);
			if(res != 0)
				return -errno;
		}
	}

	return res;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
static int my_releasedir(const char *path, struct fuse_file_info *fi)
{
	int res = 0;

	res = closedir((DIR *) (uintptr_t) fi->fh);

	return res;
}

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
static int my_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
	int res = 0;
	return res;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
static void *my_init(struct fuse_conn_info *conn)
{
	return MY_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
static void my_destroy(void *userdata)
{
	free(userdata);
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
static int my_access(const char *path, int mask)
{

	int res = 0;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		res = access(fpath, mask);

		if(res == 0) 
			return res;
	}

	return -errno;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
static int my_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int res = 0;
	int fd;
	char fpath[PATH_MAX];

	int iter;
	for(iter = 0; iter < MY_DATA->dir_cnt; iter++)
	{
		get_path(fpath, path, iter);

		fd = creat(fpath, mode);
		if (fd > 0)
		{
			fi->fh = fd;
			return res;
		}
	}

	return -errno;
}

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
static int my_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
	int res = 0;

	res = ftruncate(fi->fh, offset);
	if (res < 0)
		return -errno;

	return res;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
static int my_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
	int res = 0;

	if (!strcmp(path, "/"))
		return my_getattr(path, statbuf);
    
	res = fstat(fi->fh, statbuf);
	if (res < 0)
		return -errno;

	return res;
}

static struct fuse_operations my_oper = {
	.getattr	= my_getattr,
	.readlink	= my_readlink,
	.mknod		= my_mknod,
	.mkdir		= my_mkdir,
	.unlink		= my_unlink,
	.rmdir		= my_rmdir,
	.symlink	= my_symlink,
	.rename		= my_rename,
	.link		= my_link,
	.chmod		= my_chmod,
	.chown		= my_chown,
	.truncate	= my_truncate,
	.utime		= my_utime,
	.open		= my_open,
	.read		= my_read,
	.write		= my_write,
	.statfs		= my_statfs,
	.flush		= my_flush,
	.release	= my_release,
	.fsync		= my_fsync,
  
#ifdef HAVE_SYS_XATTR_H
	.setxattr	= my_setxattr,
	.getxattr	= my_getxattr,
	.listxattr	= my_listxattr,
	.removexattr	= my_removexattr,
#endif
  
	.opendir	= my_opendir,
	.readdir	= my_readdir,
	.releasedir	= my_releasedir,
	.fsyncdir	= my_fsyncdir,
	.init		= my_init,
	.destroy	= my_destroy,
	.access		= my_access,
	.create		= my_create,
	.ftruncate	= my_ftruncate,
	.fgetattr	= my_fgetattr
};


int main(int argc, char **argv)
{
	if(argc < 4 || 
		argv[1][0] == '-' || 
		argv[2][0] == '-' || 
		argv[3][0] == '-')
	{
		fprintf(stderr, "usage:  myfuse mount_point rootDir rootDir\n");
		return -errno;
	}

	printf("args\n"
		"prog : %s\n"
		"mount point : %s\n"
		"directory_1 : %s\n"
		"directory_2 : %s\n", argv[0]+2, argv[1], argv[2], argv[3]);


	my_data *data;
	data = malloc(sizeof(my_data));

	data->dir_cnt = argc - 2;
	data->rootdir = malloc(sizeof(char *) * data->dir_cnt);
	argc -= data->dir_cnt;

	int iter;
	for(iter = 0; iter < data->dir_cnt; iter++)
	{
		data->rootdir[iter] = realpath(argv[iter+2], NULL);
		argv[iter+2] = NULL;
	}

	return fuse_main(argc, argv, &my_oper, data);
}
