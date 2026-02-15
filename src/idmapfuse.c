/*
 * fuse-idmap - FUSE module for inter-system user/group ID mapping
 */

#ifdef __APPLE__
#include <sys/types.h>
#endif
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#if FUSE_USE_VERSION < 30
#include <fuse.h>
#else
#include <fuse3/fuse.h>
#endif

#include "idmap.h"

struct idmapfuse {
	struct fuse_fs* next;
	struct idmap* map;
	bool invert;
};

#if FUSE_DARWIN_ENABLE_EXTENSIONS
typedef struct fuse_darwin_attr stat_type;
typedef fuse_darwin_fill_dir_t fill_dir_type;
#define stat_type_uid(stbuf) (stbuf)->uid
#define stat_type_gid(stbuf) (stbuf)->gid
#else
typedef struct stat stat_type;
typedef fuse_fill_dir_t fill_dir_type;
#define stat_type_uid(stbuf) (stbuf)->st_uid
#define stat_type_gid(stbuf) (stbuf)->st_gid
#endif

#if FUSE_VERSION < 30
static int idmapfuse_getattr(const char* path, struct stat* buf) {
	struct idmapfuse* ctx = fuse_get_context()->private_data;
	int ret = fuse_fs_getattr(ctx->next, path, buf);
	idmap_map(ctx->map, &buf->st_uid, &buf->st_gid, ctx->invert);
	return ret;
}
#else
static int idmapfuse_getattr(const char* path, stat_type* buf, struct fuse_file_info *fi) {
	struct idmapfuse* ctx = fuse_get_context()->private_data;
	int ret = fuse_fs_getattr(ctx->next, path, buf, fi);
	idmap_map(ctx->map, &stat_type_uid(buf), &stat_type_gid(buf), ctx->invert);
	return ret;
}
#endif

#if FUSE_VERSION < 30
static int idmapfuse_fgetattr(const char* path, stat_type* buf, struct fuse_file_info* fi) {
	struct idmapfuse* ctx = fuse_get_context()->private_data;
	int ret = fuse_fs_fgetattr(ctx->next, path, buf, fi);
	idmap_map(ctx->map, &stat_type_uid(buf), &stat_type_gid(buf), ctx->invert);
	return ret;
}
#endif

#if HAVE_STATX && FUSE_VERSION >= 318
static int idmapfuse_statx(const char* path, int flags, int mask, struct statx* stx, struct fuse_file_info* fi) {
	struct idmapfuse* ctx = fuse_get_context()->private_data;
	int ret = fuse_fs_statx(ctx->next, path, flags, mask, stx, fi);
	idmap_map(ctx->map, &stx->stx_uid, &stx->stx_gid, ctx->invert);
	return ret;
}
#endif

#if FUSE_VERSION >= 30
struct intercept_filler {
	struct idmapfuse* ctx;
	fill_dir_type original_filler;
	void* original_buf;
};

static int idmapfuse_filler(void* buf, const char* name, const stat_type* stbuf, off_t off, enum fuse_fill_dir_flags flags) {
	struct intercept_filler* intercept_buf = buf;
	if(flags & FUSE_FILL_DIR_PLUS)
		idmap_map(intercept_buf->ctx->map, (uid_t*)&stat_type_uid(stbuf), (gid_t*)&stat_type_gid(stbuf), intercept_buf->ctx->invert);

	return intercept_buf->original_filler(intercept_buf->original_buf, name, stbuf, off, flags);
}

static int idmapfuse_readdir(const char* path, void* buf, fill_dir_type filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
	struct idmapfuse* ctx = fuse_get_context()->private_data;

	struct intercept_filler intercept_buf = { ctx, filler, buf };
	int ret = fuse_fs_readdir(ctx->next, path, &intercept_buf, idmapfuse_filler, offset, fi, flags);
	return ret;
}
#else
static int idmapfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_readdir(fs, path, buf,  filler, off, fi);
}
#endif

#if FUSE_VERSION < 30
static int idmapfuse_chown(const char* path, uid_t uid, gid_t gid) {
	struct idmapfuse* ctx = fuse_get_context()->private_data;
	idmap_map(ctx->map, &uid, &gid, !ctx->invert);
	return fuse_fs_chown(ctx->next, path, uid, gid);
}
#else
static int idmapfuse_chown(const char* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) {
	struct idmapfuse* ctx = fuse_get_context()->private_data;
	idmap_map(ctx->map, &uid, &gid, !ctx->invert);
	return fuse_fs_chown(ctx->next, path, uid, gid, fi);
}
#endif

static void idmapfuse_destroy(void* opaque) {
	struct idmapfuse* ctx = opaque;
	fuse_fs_destroy(ctx->next);
	idmap_close(ctx->map);
	free(ctx);
}

#include "fuse_stubs.h"

static const struct fuse_operations idmapfuse_ops = {
	.getattr  = idmapfuse_getattr,
#if FUSE_VERSION < 30
	.fgetattr    = idmapfuse_fgetattr,
#endif
#if HAVE_STATX && FUSE_VERSION >= 318
	.statx       = idmapfuse_statx,
#endif
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
#if FUSE_VERSION < 30
	.ftruncate = idmapfuse_ftruncate,
#else
	.copy_file_range = idmapfuse_copy_file_range,
	.lseek = idmapfuse_lseek,
#endif
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
#if FUSE_VERSION < 30
	.setattr_x = idmapfuse_setattr_x,
	.fsetattr_x = idmapfuse_fsetattr_x,
	.exchange = idmapfuse_exchange,
	.getxtimes = idmapfuse_getxtimes,
	.setbkuptime = idmapfuse_setbkuptime,
	.setchgtime = idmapfuse_setchgtime,
	.setcrtime = idmapfuse_setcrtime,
#endif
	.setvolname = idmapfuse_setvolname,
	.chflags = idmapfuse_chflags,
#endif

#if FUSE_VERSION >= 29 && FUSE_VERSION < 30
	.flag_nopath = 1,
#endif
#if FUSE_VERSION >= 28 && FUSE_VERSION < 30
	.flag_nullpath_ok = 1
#endif
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

#if FUSE_VERSION >= 30
FUSE_REGISTER_MODULE(idmap, idmapfuse_new);
#else
FUSE_REGISTER_MODULE(idmap, idmapfuse_new)
#endif
