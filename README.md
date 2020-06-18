fuse-idmap is a [FUSE](https://github.com/libfuse/libfuse) module for mapping UNIX user and group IDs from the mounted filesystem to corresponding values on the host.

fuse-idmap may be loaded over any FUSE filesystem and offers a more flexible alternative to the `uid=...` and `gid=...` options provided by standard FUSE implementations.

# Function
When loaded, this module intercepts `getattr` FUSE calls and overrides the user/group IDs that are returned to the host system. If the mounted target filesystem is writable, it also reverses this procedure for `chown`.  
In practice this means that the host system will use the mapped IDs for determining access and when reading or copying file attributes from the target filesystem, but translate these back when using the host system to change ownership over a file on the target.

The mapping may be reversed, so for instance the same map file(s) used to mount an HFS+ partition on Linux may be used when mounting the corresponding ext4 partition under macOS.

# Installation
## Building
    make
    make install

Makefile dialect is GNU, so substitute `gmake` as needed.

By default, this will install libfusemod_idmap.so to /usr/lib so that it can be found by FUSE. You can change this by setting the PREFIX environment variable before running `make install` but make sure the destination is in the appropriate search path for loadable modules (see `man 3 dlopen`)

## Use
Add `modules=idmap` to the options string when mounting a FUSE filesystem.  
fuse-idmap's options will be printed as part of FUSE's module help where supported.

    [idmap]
        -o umap=user.map       Path to UID remapping file
        -o gmap=group.map      Path to GID remapping file
        -o pairmap=pairs.map   Path to remapping file for specific user:group pairs
        -o invert              invert the mapping

Any of the 3 mappings may be omitted if they are not needed, and the same file may be specified for both `umap` and `gmap` if the user and group IDs are identical.

# Map file format
## user.map and group.map
User and group mapping files are simple text files containing whitespace-separated pairs of foreign and local IDs, with one pair per line.  
A typical user.map and group.map for an HFS+ filesystem with two users being mounted on Linux might look like:

    501 1000
    502 1001

Where `501` and `502` are IDs on the HFS+ filesystem, and `1000` and `1001` are their corresponding IDs on the host system. Note that fuse-idmap doesn't assume users and their groups have identical IDs, so to map both using this file it must be specified for both the `umap` and `gmap` options.

IDs that are not mapped are simply passed through as-is, so for instance mapping the root user (0), which is the same on both systems, is unnecessary.

## pairs.map
At times it may be desirable to specify that users and groups should only be mapped when they occur together. For this the third `pairmap` option can be used. These files instead map colon-separated user and group pairs together.  
As a practical example, macOS files are typically created under the "staff" group (20), but Linux systems generally prefer to use the user's own group (100x). We probably don't want to map the staff group ID as a whole to our own group, so we can instead instruct fuse-idmap to only map "staff" grouped files to our user when the UID is also our user:

    501:20 1000:1000
	502:20 1001:1001

This way our two different users retain the group of their own files, and files that are root/staff will not become root/1000, neither of which are possible with only a group.map.

These more specific mappings take priority over the user and group mappings specified individually, but otherwise can be combined with them. In this example, we would still include separate user and group maps as otherwise only files that match the u+g map exactly will be translated.

# libidmap
For filesystems not wanting the (minimal) overhead of a module, the same id mapping functions are available by including idmap.h and linking with libidmap.a. See the fuse-idmap module code for reference usage.

# Similar Projects

* https://github.com/bAndie91/libnss_idmap
