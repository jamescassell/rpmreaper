/*
 * Copyright (C) 2008, 2009  Miroslav Lichvar <mlichvar@redhat.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "repo.h"

#include <stdlib.h>
#include <string.h>
#include <libgen.h>

void repos_init(struct repos *repos) {
	array_init(&repos->repos, sizeof (struct repo));
	pkgs_init(&repos->pkgs);
}

void repos_clean(struct repos *repos) {
	uint i;
	struct repo *r;

	for (i = 0; i < array_get_size(&repos->repos); i++) {
		r = repos_getw(repos, i);
		if (r->repo_clean != NULL)
			r->repo_clean(r);
	}
	array_clean(&repos->repos);
	pkgs_clean(&repos->pkgs);
}

struct repo *repos_new(struct repos *repos) {
	struct repo *r;
	uint s;

	s = array_get_size(&repos->repos);
	r = (struct repo *)array_get_wptr(&repos->repos, s);

	r->repo = s;

	return r;
}

const struct repo *repos_get(const struct repos *repos, uint repo) {
	return (const struct repo *)array_get_ptr(&repos->repos, repo);
}

struct repo *repos_getw(struct repos *repos, uint repo) {
	return (struct repo *)array_get_wptr(&repos->repos, repo);
}

int repos_read(struct repos *repos) {
	struct repo *r;
	struct pkgs *p = &repos->pkgs;
	struct array firstpids;
	struct strings files;
	struct strings basenames;
	char buf[1000];
	uint i, s;

	if (pkgs_get_size(&repos->pkgs)) {
		pkgs_clean(&repos->pkgs);
		pkgs_init(&repos->pkgs);
	}

	array_init(&firstpids, 0);
	strings_init(&files);
	strings_init(&basenames);

	for (i = 0; i < array_get_size(&repos->repos); i++) {
		array_set(&firstpids, i, pkgs_get_size(&repos->pkgs));
		r = repos_getw(repos, i);
		if (r->repo_read == NULL)
			return 1;
		r->repo_read(r, &repos->pkgs, array_get(&firstpids, i));
	}

	buf[sizeof (buf) - 1] = '\0';

	/* find file requires */
	for (s = strings_get_first(&p->strings); s != -1; s = strings_get_next(&p->strings, s)) {
		const char *str;

		str = strings_get(&p->strings, s);
		if (str && str[0] == '/') {
			char *base;

			strncpy(buf, str, sizeof (buf) - 1);
			base = basename(buf);
			if (!strcmp(base, ".") || !strcmp(base, "/"))
				continue;
			strings_add(&files, str);
			strings_add(&basenames, base);
		}
	}

	for (i = 0; i < array_get_size(&repos->repos); i++) {
		r = repos_getw(repos, i);
		if (r->repo_read_provs == NULL)
			continue;
		r->repo_read_provs(r, &repos->pkgs, array_get(&firstpids, i), &files, &basenames);
	}

	array_clean(&firstpids);
	strings_clean(&files);
	strings_clean(&basenames);

	pkgs_match_deps(&repos->pkgs);

	return 0;
}

int repos_pkg_info(const struct repos *repos, uint pid) {
	const struct repo *r;

	r = repos_get(repos, pkgs_get(&repos->pkgs, pid)->repo);
	if (r->repo_pkg_info == NULL)
		return 1;
	return r->repo_pkg_info(r, &repos->pkgs, pid);
}

int repos_remove_pkgs(struct repos *repos, const char *options) {
	const struct repo *r;
	uint i;

	for (i = 0; i < array_get_size(&repos->repos); i++) {
		r = repos_get(repos, i);
		if (r->repo_remove_pkgs == NULL)
			continue;
		if (r->repo_remove_pkgs(r, &repos->pkgs, options))
			return 1;
	}
	return 0;
}
