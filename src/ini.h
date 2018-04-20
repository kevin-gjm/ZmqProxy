/**
 * Copyright (c) 2016 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `ini.c` for details.
 */

#ifndef INI_H
#define INI_H

#define INI_VERSION "0.1.1"

typedef struct ini_t ini_t;

ini_t*      ini_load(const char *filename);
void        ini_free(ini_t *ini);
const char* ini_get(ini_t *ini, const char *section, const char *key);
int         ini_sget(ini_t *ini, const char *section, const char *key, const char *scanfmt, void *dst);

#endif

//config.ini
/*
; last modified 1 April 2001 by John Doe
[owner]
name = John Doe
organization = Acme Widgets Inc.

[database]
; use IP address in case network name resolution is not working
server = 192.0.2.62
port = 143
file = "payroll.dat"
*/
/*
int main()
{
	ini_t *config = ini_load("config.ini");
	if(config == NULL)
		return -1;
	const char *name = "Leo";
	const char *organization="ht";
	const char *server = "default";
	const char *file = "file";
	int port = 80;

	if(!ini_sget(config, "owner", "name", "%s", &name))
		return -1;
	//if(!ini_sget(config, "owner", "name", NULL, &name))
	//	return -1;
	if(!ini_sget(config, "owner", "organization", NULL, &organization))
		return -1;
	if(!ini_sget(config, "database", "server", NULL, &server))
		return -1;
	if(!ini_sget(config, "database", "port", "%d", &port))
		return -1;
	if(!ini_sget(config, "database", "file", NULL, &file))   // will not have the quotation
		return -1;

	printf("result:\n");
	printf("own: %s:%s\n", name, organization);
	printf("server: %s:%d:%s\n", server, port,file);

	//	result:
	//	own: John Doe:Acme Widgets Inc.
	//	server: 192.0.2.62:143:payroll.dat


	ini_free(config);

}


*/
