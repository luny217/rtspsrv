#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rtspsrv_json.h"
#include "libavutil/avutil_log.h"
#include "rtspsrv_defines.h"
#include "rtspsrv_config.h"

static rsrv_config_t config;

rsrv_config_t * rsrv_get_config(void)
{
    return &config;
}

rsrv_param_t * rsrv_get_param(void)
{
    return &config.param;
}

static void rsrv_param_init(rsrv_param_t * param)
{
    memset(&config.param, 0, sizeof(rsrv_param_t));
}

int32_t rsrv_config_init(const char * filename)
{
	rsrv_param_init(&config.param);
	return rsrv_parse_param(filename);
}

#if 0
void rsrv_param_free(struct rsrv_param * param)
{
    if (param->local_interface)
        free(param->local_interface);

    if (param->remote_addr)
        free(param->remote_addr);

    if (param->key)
        free(param->key);

    if (param->crypt)
        free(param->crypt);

    if (param->mode)
        free(param->mode);
}
#endif

static int parse_json_int(const json_value * value)
{
    if (value->type == json_integer)
    {
        return (int)value->u.integer;
    }
    else if (value->type == json_boolean)
    {
        return value->u.boolean;
    }
    else
    {
		rsrv_log(AV_LOG_ERROR, "Need type :%d or %d, now get wrong type: %d", json_integer, json_boolean, value->type);
		rsrv_log(AV_LOG_ERROR, "Invalid config format.");
    }
    return 0;
}

static char * parse_json_string(const json_value * value)
{
    if (value->type == json_string)
    {
        return _strdup(value->u.string.ptr);
    }
    else if (value->type == json_null)
    {
        return NULL;
    }
    else
    {
        rsrv_log(AV_LOG_ERROR, "Need type :%d or %d, now get wrong type: %d", json_string, json_null, value->type);
        rsrv_log(AV_LOG_ERROR, "Invalid config format.");
    }
    return 0;
}

int rsrv_parse_param(const char * filename)
{
    return rsrv_parse_json_param(&config.param, filename);
}

// 1: error; 0, success
int rsrv_parse_json_param(rsrv_param_t * param, const char * filename)
{
    if (!param)
        return 1;

    char * buf;
    json_value * obj;

    FILE * f = fopen(filename, "rb");
    if (f == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "Invalid config path.");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long pos = ftell(f);
    fseek(f, 0, SEEK_SET);

    buf = malloc(pos + 1);
    if (buf == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "No enough memory.");
        return 1;
    }

    int nread = fread(buf, pos, 1, f);
    if (!nread)
    {
        rsrv_log(AV_LOG_ERROR, "Failed to read the config file.");
        return 1;
    }
    fclose(f);

    buf[pos] = '\0'; // end of string

    json_settings settings = { 0UL, 0, NULL, NULL, NULL };
    char error_buf[512];
    obj = json_parse_ex(&settings, buf, pos, error_buf);

    if (obj == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "%s", error_buf);
        return 1;
    }

    if (obj->type == json_object)
    {
        unsigned int i;
        for (i = 0; i < obj->u.object.length; i++)
        {
            char * name		= obj->u.object.values[i].name;
            json_value * value = obj->u.object.values[i].value;
            if (strncmp(name, "main_uri", 256) == 0)
            {
                param->main_uri = parse_json_string(value);
                rsrv_log(AV_LOG_WARNING, "main_uri is %s\n", param->main_uri);
            }
            else if (strncmp(name, "main_usr", 32) == 0)
            {
                param->main_usr = parse_json_string(value);
                rsrv_log(AV_LOG_WARNING, "main_usr is %s\n", param->main_usr);
            }
            else if (strncmp(name, "main_pwd", 32) == 0)
            {
                param->main_pwd = parse_json_string(value);
                rsrv_log(AV_LOG_WARNING, "main_pwd is %s\n", param->main_pwd);
            }
            else if (strncmp(name, "sub_uri", 256) == 0)
            {
                param->sub_uri = parse_json_string(value);
                rsrv_log(AV_LOG_WARNING, "sub_uri is %s\n", param->sub_uri);
            }
            else if (strncmp(name, "sub_usr", 32) == 0)
            {
                param->sub_usr = parse_json_string(value);
                rsrv_log(AV_LOG_WARNING, "sub_usr is %s\n", param->sub_usr);
            }
            else if (strncmp(name, "sub_pwd", 32) == 0)
            {
                param->sub_pwd = parse_json_string(value);
                rsrv_log(AV_LOG_WARNING, "sub_pwd is %s\n", param->sub_pwd);
            }
            else if (strncmp(name, "ip_prefix", 32) == 0)
            {
                param->ip_prefix = parse_json_string(value);
                rsrv_log(AV_LOG_WARNING, "ip_prefix is %s\n", param->ip_prefix);
            }
			else if (strncmp(name, "ip_begin", 32) == 0)
			{
				param->ip_begin = parse_json_int(value);
				rsrv_log(AV_LOG_WARNING, "ip_begin is %d\n", param->ip_begin);
			}
			else if (strncmp(name, "ip_num", 32) == 0)
			{
				param->ip_num = parse_json_int(value);
				rsrv_log(AV_LOG_WARNING, "ip_num is %d\n", param->ip_num);
			}            
        }
    }
    else
    {
        rsrv_log(AV_LOG_DEBUG, "Invalid config file");
        return 1;
    }

    free(buf);
    json_value_free(obj);

    return 0;
}
