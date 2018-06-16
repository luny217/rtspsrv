#ifndef	_NID_CONFIG_H__
#define	_NID_CONFIG_H__

#include <stdint.h>

typedef struct _rsrv_param
{
	uint8_t * main_uri;
	uint8_t * main_usr;
	uint8_t * main_pwd;
	uint8_t * sub_uri;
	uint8_t * sub_usr;
	uint8_t * sub_pwd;
	uint8_t * ip_prefix;
	int32_t ip_begin;
	int32_t ip_num;
} rsrv_param_t;

typedef struct _rsrv_config
{
    char * config_file;
    rsrv_param_t param;
} rsrv_config_t;

int32_t rsrv_config_init(const char * filename);

rsrv_config_t * rsrv_get_config(void);

int32_t rsrv_parse_param(const char * filename);

int32_t rsrv_parse_json_param(rsrv_param_t * config, const char * filename);

rsrv_param_t * rsrv_get_param(void);

#endif
