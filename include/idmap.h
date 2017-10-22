/*
 * idmap - Map user/group IDs between systems
 * Copyright 2013-2017 0x09.net.
 */

#ifndef IDMAP_H
#define IDMAP_H

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

struct idmap;

struct idmap* idmap_open(void);
struct idmap* idmap_open_with_mapfiles(const char* user_map, const char* group_map, const char* user_group_map);
void idmap_close(struct idmap* map);

bool idmap_add_user(struct idmap*, uid_t from_user, uid_t to_user);
bool idmap_add_group(struct idmap*, gid_t from_group, gid_t to_group);
bool idmap_add_user_group_pair(struct idmap*, uid_t from_user, gid_t from_group, uid_t to_user, gid_t to_group);

bool idmap_read_users(struct idmap*, FILE* mapfile);
bool idmap_read_groups(struct idmap*, FILE* mapfile);
bool idmap_read_user_group_pairs(struct idmap*, FILE* mapfile);

bool idmap_read_mapfiles(struct idmap*, const char* user_map, const char* group_map, const char* user_group_map);

void idmap_map(struct idmap*, uid_t* restrict uid, gid_t* restrict gid, bool invert);

#endif

