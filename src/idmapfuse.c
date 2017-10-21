/*
 * fuse-idmap - FUSE module for inter-system user/group ID mapping
 * Copyright 2013-2017 0x09.net.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <fuse/fuse.h>

#include "idmap.h"

struct idmapfuse {
	struct fuse_fs* next;
	struct idmap* map;
	bool invert;
};

static int idmapfuse_getattr(const char* path, struct stat* buf) {
	struct idmapfuse* ctx = fuse_get_context()->private_data;
	int ret = fuse_fs_getattr(ctx->next, path, buf);
	idmap_map(ctx->map, &buf->st_uid, &buf->st_gid, ctx->invert);
	return ret;
}

static int idmapfuse_fgetattr(const char* path, struct stat* buf, struct fuse_file_info* fi) {
	struct idmapfuse* ctx = fuse_get_context()->private_data;
	int ret = fuse_fs_fgetattr(ctx->next, path, buf, fi);
	idmap_map(ctx->map, &buf->st_uid, &buf->st_gid, ctx->invert);
	return ret;
}

static int idmapfuse_chown(const char* path, uid_t uid, gid_t gid) {
	struct idmapfuse* ctx = fuse_get_context()->private_data;
	idmap_map(ctx->map, &uid, &gid, !ctx->invert);
	return fuse_fs_chown(ctx->next, path, uid, gid);
}

static void idmapfuse_destroy(void* opaque) {
	struct idmapfuse* ctx = opaque;
	fuse_fs_destroy(ctx->next);
	idmap_close(ctx->map);
	free(ctx);
}

#include "fuse_stubs.h"

static const struct fuse_operations idmapfuse_ops = {
	.getattr  = idmapfuse_getattr,
	.fgetattr = idmapfuse_fgetattr,
	.chown    = idmapfuse_chown,
	.destroy  = idmapfuse_destroy,

	// FUSE modules must implement every operation they can support, but we only need to intercept getattr/chown
	// so we try to pass all others along. This set of operations is complete as of FUSE version 2.9.1.
	.setxattr = idmapfuse_setxattr,
	.getxattr = idmapfuse_getxattr,
	.rename = idmapfuse_rename,
	.unlink = idmapfuse_unlink,
	.rmdir = idmapfuse_rmdir,
	.symlink = idmapfuse_symlink,
	.link = idmapfuse_link,
	.release = idmapfuse_release,
	.open = idmapfuse_open,
	.read = idmapfuse_read,
	.read_buf = idmapfuse_read_buf,
	.write = idmapfuse_write,
	.write_buf = idmapfuse_write_buf,
	.fsync = idmapfuse_fsync,
	.flush = idmapfuse_flush,
	.statfs = idmapfuse_statfs,
	.opendir = idmapfuse_opendir,
	.readdir = idmapfuse_readdir,
	.fsyncdir = idmapfuse_fsyncdir,
	.releasedir = idmapfuse_releasedir,
	.create = idmapfuse_create,
	.lock = idmapfuse_lock,
	.flock = idmapfuse_flock,
	.chmod = idmapfuse_chmod,
	.truncate = idmapfuse_truncate,
	.ftruncate = idmapfuse_ftruncate,
	.utimens = idmapfuse_utimens,
	.access = idmapfuse_access,
	.readlink = idmapfuse_readlink,
	.mknod = idmapfuse_mknod,
	.mkdir = idmapfuse_mkdir,
	.listxattr = idmapfuse_listxattr,
	.removexattr = idmapfuse_removexattr,
	.bmap = idmapfuse_bmap,
	.ioctl = idmapfuse_ioctl,
	.poll = idmapfuse_poll,
	.fallocate = idmapfuse_fallocate,
	.init = idmapfuse_init,
#ifdef __APPLE__
	.setattr_x = idmapfuse_setattr_x,
	.fsetattr_x = idmapfuse_fsetattr_x,
	.setvolname = idmapfuse_setvolname,
	.exchange = idmapfuse_exchange,
	.chflags = idmapfuse_chflags,
	.getxtimes = idmapfuse_getxtimes,
	.setbkuptime = idmapfuse_setbkuptime,
	.setchgtime = idmapfuse_setchgtime,
	.setcrtime = idmapfuse_setcrtime,
#endif

	.flag_nopath = 1,
	.flag_nullpath_ok = 1
};

struct idmapfuse_opts {
	const char* umap,* gmap,* ugmap;
	int invert;
};
static const struct fuse_opt idmapfuse_opts[] = {
	FUSE_OPT_KEY("-h",    0),
	FUSE_OPT_KEY("--help",0),
	{"umap=%s",   offsetof(struct idmapfuse_opts,umap),  0},
	{"gmap=%s",   offsetof(struct idmapfuse_opts,gmap),  0},
	{"pairmap=%s",offsetof(struct idmapfuse_opts,ugmap), 0},
	{"invert",    offsetof(struct idmapfuse_opts,invert),1},
	FUSE_OPT_END
};

static int idmapfuse_opt_proc(void* data, const char* arg, int key, struct fuse_args* outargs) {
	if(key)
		return 1;

	fprintf(
		stderr,
		"    -o umap=user.map       Path to UID remapping file\n"
		"    -o gmap=group.map      Path to GID remapping file\n"
		"    -o pairmap=pairs.map   Path to remapping file for specific user:group pairs\n"
		"    -o invert              invert the mapping\n"
	);
	return -1;
}

static struct fuse_fs* idmapfuse_new(struct fuse_args* args, struct fuse_fs* next[]) {
	struct idmapfuse_opts opts = {0};
	if(fuse_opt_parse(args, &opts, idmapfuse_opts, idmapfuse_opt_proc) < 0)
		return NULL;

	struct idmapfuse* ctx = malloc(sizeof(*ctx));
	ctx->next = next[0];
	if(!(ctx->map = idmap_open_with_mapfiles(opts.umap, opts.gmap, opts.ugmap)))
		perror("Error initializing idmap");
	ctx->invert = opts.invert;

	struct fuse_fs* fs = fuse_fs_new(&idmapfuse_ops, sizeof(idmapfuse_ops), ctx);
	if(fs)
		return fs;

	idmap_close(ctx->map);
	free(ctx);
	return NULL;
}

FUSE_REGISTER_MODULE(idmap, idmapfuse_new);