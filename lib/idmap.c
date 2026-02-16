/*
 * idmap - Map user/group IDs between systems
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include "idmap.h"

struct idmap {
	id_t (*uids) [2],
	     (*gids) [2],
	     (*ugids)[2][2];
	size_t nuids, ngids, nugids;
};

static inline bool add_id(id_t from_id, id_t to_id, id_t (**ids)[2], size_t* size) {
	id_t (*newids)[2] = realloc(*ids, sizeof(**ids)*(*size+1));
	if(!newids)
		return false;
	*ids = newids;
	newids += *size;
	(*newids)[0] = from_id;
	(*newids)[1] = to_id;
	(*size)++;
	return true;
}

bool idmap_add_user(struct idmap* map, uid_t from_user, uid_t to_user) {
	return add_id(from_user, to_user, &map->uids, &map->nuids);
}

bool idmap_add_group(struct idmap* map, gid_t from_group, gid_t to_group) {
	return add_id(from_group, to_group, &map->gids, &map->ngids);
}

bool idmap_add_user_group_pair(struct idmap* map, uid_t from_user, gid_t from_group, uid_t to_user, gid_t to_group) {
	id_t (*ids)[2][2] = realloc(map->ugids, sizeof(*map->ugids)*(map->nugids+1));
	if(!ids)
		return false;
	map->ugids = ids;
	ids += map->nugids;
	(*ids)[0][0] = from_user;
	(*ids)[0][1] = from_group;
	(*ids)[1][0] = to_user;
	(*ids)[1][1] = to_group;
	map->nugids++;
	return true;
}

static inline bool read_ids(FILE* f, id_t (**ids)[2], size_t* size) {
	// No format specifier for id_t, so scan these separately as %u
	unsigned int from_id, to_id;
	while(fscanf(f, "%u %u\n", &from_id, &to_id) == 2)
		if(!add_id(from_id, to_id, ids, size))
			return false;
	return feof(f);
}

bool idmap_read_users(struct idmap* map, FILE* mapfile) {
	return read_ids(mapfile, &map->uids, &map->nuids);
}

bool idmap_read_groups(struct idmap* map, FILE* mapfile) {
	return read_ids(mapfile, &map->gids, &map->ngids);
}

bool idmap_read_user_group_pairs(struct idmap* map, FILE* mapfile) {
	unsigned int from_user, from_group, to_user, to_group;
	while(fscanf(mapfile, "%u:%u %u:%u\n", &from_user, &from_group, &to_user, &to_group) == 4)
		if(!idmap_add_user_group_pair(map, from_user, from_group, to_user, to_group))
			return false;
	return feof(mapfile);
}

bool idmap_read_mapfiles(struct idmap* map, const char* user_map, const char* group_map, const char* user_group_map) {
	FILE* f;
	if(user_map) {
		if(!((f = fopen(user_map,"r")) && idmap_read_users(map, f)))
			goto err;
		fclose(f);
	}
	if(group_map) {
		if(!((f = fopen(group_map,"r")) && idmap_read_groups(map, f)))
			goto err;
		fclose(f);
	}
	if(user_group_map) {
		if(!((f = fopen(user_group_map,"r")) && idmap_read_user_group_pairs(map, f)))
			goto err;
		fclose(f);
	}
	return true;

err:
	if(f)
		fclose(f);
	return false;
}

struct idmap* idmap_open(void) {
	return calloc(1, sizeof(struct idmap));
}

void idmap_close(struct idmap* map) {
	free(map->uids);
	free(map->gids);
	free(map->ugids);
	free(map);
}

struct idmap* idmap_open_with_mapfiles(const char* user_map, const char* group_map, const char* user_group_map) {
	struct idmap* map = idmap_open();
	if(!map)
		return NULL;
	if(idmap_read_mapfiles(map, user_map, group_map, user_group_map))
		return map;
	idmap_close(map);
	return NULL;
}

void idmap_map(struct idmap* map, uid_t* restrict uid, gid_t* restrict gid, bool invert) {
	for(int i = 0; i < map->nugids; i++)
		if(map->ugids[i][!!invert][0] == *uid && map->ugids[i][!!invert][1] == *gid) {
			*uid = map->ugids[i][!invert][0];
			*gid = map->ugids[i][!invert][1];
			return;
		}
	for(int i = 0; i < map->nuids; i++)
		if(map->uids[i][!!invert] == *uid) {
			*uid = map->uids[i][!invert];
			break;
		}
	for(int i = 0; i < map->ngids; i++)
		if(map->gids[i][!!invert] == *gid) {
			*gid = map->gids[i][!invert];
			break;
		}
}
