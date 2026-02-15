/*
 * fuse-idmap - FUSE module for inter-system user/group ID mapping
 */

// FUSE modules must implement every operation they can support, but we only need to intercept getattr/chown
// so we try to pass all others along. This set of operations is complete as of FUSE version 2.9.1.

#ifdef __APPLE__
static int idmapfuse_setvolname(const char *volname) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_setvolname(fs, volname);
}

static int idmapfuse_setxattr(const char *path, const char *name, const char *value, size_t size, int flags, uint32_t position) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_setxattr(fs, path, name, value, size, flags, position);
}

static int idmapfuse_getxattr(const char *path, const char *name, char *value, size_t size, uint32_t position) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_getxattr(fs, path, name, value, size, position);
}

#if FUSE_VERSION < 30
static int idmapfuse_setattr_x(const char *path, struct setattr_x *attr) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_setattr_x(fs, path, attr);
}

static int idmapfuse_fsetattr_x(const char *path, struct setattr_x *attr, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_fsetattr_x(fs, path, attr, fi);
}

static int idmapfuse_exchange(const char *oldpath, const char *newpath, unsigned long flags) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_exchange(fs, oldpath, newpath, flags);
}

static int idmapfuse_chflags(const char *path, uint32_t flags) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_chflags(fs, path, flags);
}

static int idmapfuse_getxtimes(const char *path, struct timespec *bkuptime, struct timespec *crtime) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_getxtimes(fs, path, bkuptime, crtime);
}

static int idmapfuse_setbkuptime(const char *path, const struct timespec *tv) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_setbkuptime(fs, path, tv);
}

static int idmapfuse_setchgtime(const char *path, const struct timespec *tv) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_setchgtime(fs, path, tv);
}

static int idmapfuse_setcrtime(const char *path, const struct timespec *tv) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_setcrtime(fs, path, tv);
}

static int idmapfuse_statfs(const char *path, struct statvfs *buf) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_statfs(fs, path, buf);
}
#else /* FUSE_VERSION >= 30 */
static int idmapfuse_chflags(const char *path, struct fuse_file_info *fi, uint32_t flags) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_chflags(fs, path, fi, flags);
}

static int idmapfuse_statfs(const char *path, struct statfs *buf) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_statfs(fs, path, buf);
}
#endif
#else /* !__APPLE__ */
static int idmapfuse_statfs(const char *path, struct statvfs *buf) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_statfs(fs, path, buf);
}

static int idmapfuse_setxattr(const char *path, const char *name, const char *value, size_t size, int flags) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_setxattr(fs, path, name, value, size, flags);
}

static int idmapfuse_getxattr(const char *path, const char *name, char *value, size_t size) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_getxattr(fs, path, name, value, size);
}
#endif /* __APPLE__ */


#if FUSE_VERSION < 30
static int idmapfuse_rename(const char *oldpath, const char *newpath) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_rename(fs, oldpath, newpath);
}

static int idmapfuse_utimens(const char *path, const struct timespec tv[2]) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_utimens(fs, path, tv);
}

static int idmapfuse_chmod(const char *path, mode_t mode) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_chmod(fs, path, mode);
}
static int idmapfuse_ftruncate(const char *path, off_t size, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_ftruncate(fs, path, size, fi);
}

static int idmapfuse_truncate(const char *path, off_t size) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_truncate(fs, path, size);
}

static void *idmapfuse_init(struct fuse_conn_info *conn) {
	struct idmapfuse *ctx = fuse_get_context()->private_data;
	fuse_fs_init(ctx->next, conn);
	return ctx;
}
#else /* FUSE_VERSION >= 30 */
static int idmapfuse_rename(const char *oldpath, const char *newpath, unsigned int flags) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_rename(fs, oldpath, newpath, flags);
}

static int idmapfuse_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_utimens(fs, path, tv, fi);
}

static int idmapfuse_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_chmod(fs, path, mode, fi);
}

static int idmapfuse_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_truncate(fs, path, size, fi);
}

static ssize_t idmapfuse_copy_file_range(const char *path_in, struct fuse_file_info *fi_in, off_t offset_in, const char *path_out, struct fuse_file_info *fi_out, off_t offset_out, size_t size, int flags) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_copy_file_range(fs, path_in, fi_in, offset_in, path_out, fi_out, offset_out, size, flags);
}

static off_t idmapfuse_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_lseek(fs, path, off, whence, fi);
}

static void *idmapfuse_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
	struct idmapfuse *ctx = fuse_get_context()->private_data;
	fuse_fs_init(ctx->next, conn, cfg);
	return ctx;
}
#endif

#if FUSE_VERSION < 35
static int idmapfuse_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi, unsigned int flags, void *data) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_ioctl(fs, path, cmd, arg, fi, flags, data);
}
#else
static int idmapfuse_ioctl(const char *path, unsigned int cmd, void *arg, struct fuse_file_info *fi, unsigned int flags, void *data) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_ioctl(fs, path, cmd, arg, fi, flags, data);
}
#endif

static int idmapfuse_unlink(const char *path) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_unlink(fs, path);
}

static int idmapfuse_rmdir(const char *path) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_rmdir(fs, path);
}

static int idmapfuse_symlink(const char *linkname, const char *path) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_symlink(fs, linkname, path);
}

static int idmapfuse_link(const char *oldpath, const char *newpath) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_link(fs, oldpath, newpath);
}

static int idmapfuse_release(const char *path, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_release(fs, path, fi);
}

static int idmapfuse_open(const char *path, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_open(fs, path, fi);
}

static int idmapfuse_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_read(fs, path, buf, size, off, fi);
}

static int idmapfuse_read_buf(const char *path, struct fuse_bufvec **bufp, size_t size, off_t off, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_read_buf(fs, path, bufp, size, off, fi);
}

static int idmapfuse_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_write(fs, path, buf, size, off, fi);
}

static int idmapfuse_write_buf(const char *path, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_write_buf(fs, path, buf, off, fi);
}

static int idmapfuse_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_fsync(fs, path, datasync, fi);
}

static int idmapfuse_flush(const char *path, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_flush(fs, path, fi);
}

static int idmapfuse_opendir(const char *path, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_opendir(fs, path, fi);
}

static int idmapfuse_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_fsyncdir(fs, path, datasync, fi);
}

static int idmapfuse_releasedir(const char *path, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_releasedir(fs, path, fi);
}

static int idmapfuse_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_create(fs, path, mode, fi);
}

static int idmapfuse_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *lock) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_lock(fs, path, fi, cmd, lock);
}

static int idmapfuse_flock(const char *path, struct fuse_file_info *fi, int op) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_flock(fs, path, fi, op);
}

static int idmapfuse_access(const char *path, int mask) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_access(fs, path, mask);
}

static int idmapfuse_readlink(const char *path, char *buf, size_t len) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_readlink(fs, path, buf, len);
}

static int idmapfuse_mknod(const char *path, mode_t mode, dev_t rdev) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_mknod(fs, path, mode, rdev);
}

static int idmapfuse_mkdir(const char *path, mode_t mode) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_mkdir(fs, path, mode);
}

static int idmapfuse_listxattr(const char *path, char *list, size_t size) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_listxattr(fs, path, list, size);
}

static int idmapfuse_removexattr(const char *path, const char *name) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_removexattr(fs, path, name);
}

static int idmapfuse_bmap(const char *path, size_t blocksize, uint64_t *idx) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_bmap(fs, path, blocksize, idx);
}

static int idmapfuse_poll(const char *path, struct fuse_file_info *fi, struct fuse_pollhandle *ph, unsigned *reventsp) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_poll(fs, path, fi, ph, reventsp);
}

static int idmapfuse_fallocate(const char *path, int mode, off_t offset, off_t length, struct fuse_file_info *fi) {
	struct fuse_fs *fs = ((struct idmapfuse*)fuse_get_context()->private_data)->next;
	return fuse_fs_fallocate(fs, path, mode, offset, length, fi);
}
