#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sqlite3.h"
#include "mqtt_cache.h"
#include "log.h"

#define BUF_PAYLOAD_MAX_LEN (8000)
#define BUF_TOPIC_MAX_LEN (128)

static int callback_read_one_item(void *para, int ncolumn, char **columnvalue, char *columnname[]);
static int callback_count(void *para, int ncolumn, char **columnvalue, char *columnname[]);
static int callback_pageSize(void *para, int ncolumn, char **columnvalue, char *columnname[]);
static int callback_pageCount(void *para, int ncolumn, char **columnvalue, char *columnname[]);

struct mqtt_cache_sqlite_data
{
    int *rowid;
    void *data;
};

int mqtt_cache_open(const char *dir, const char *table_name, void **handle)
{
    printf("%s,dir:%s,table:%s\n", __func__, dir, table_name);
    int nResult = sqlite3_open(dir, (sqlite3 **)handle);
    char sql_creat_table[128];
    if (nResult != SQLITE_OK)
    {
        // printf("open success\n");
        log_info("%s,dir:%s,table:%s\n", __func__, dir, table_name);
        return -1;
    }
    // open table
    memset(sql_creat_table, 0, 128);
    sprintf(sql_creat_table, "CREATE TABLE [%s] ([topic] TEXT(%d),[payload] TEXT(%d))", table_name, BUF_TOPIC_MAX_LEN, BUF_PAYLOAD_MAX_LEN);

    if (sqlite3_exec((sqlite3 *)(*handle), sql_creat_table, NULL, NULL, NULL) != SQLITE_OK)
    {
        // exit
        return 0;
    }
    else
    {
        // create table success
        return 0;
    }
    return 0;
}

int mqtt_cache_read_one_payload(void *handle, const char *table_name, mqtt_ringbuffer_element_t *e, int *idx)
{
    char sql_select[128];
    struct mqtt_cache_sqlite_data stTemp;

    memset(sql_select, 0, 128);
    sprintf(sql_select, "select rowid,* from %s limit 1", table_name);

    stTemp.rowid = idx;
    stTemp.data = (void *)e;

    if (sqlite3_exec((sqlite3 *)handle, sql_select, callback_read_one_item, (void *)&stTemp, NULL) != SQLITE_OK)
    {
        // printf("select data from db error.\n");
        log_info("%s,read from table(%s) failed\n", __func__, table_name);
        return -1;
    }
    return 0;
}

int mqtt_cache_write_one_payload(void *handle, const char *table_name, mqtt_ringbuffer_element_t e)
{
    char sql_insert[BUF_PAYLOAD_MAX_LEN + BUF_TOPIC_MAX_LEN + 128];
    memset(sql_insert, 0, BUF_PAYLOAD_MAX_LEN + BUF_TOPIC_MAX_LEN + 128);
    sprintf(sql_insert, "insert into %s values(\"%s\",\"%s\")", table_name, e.sztopic, e.szpayload);
    if (sqlite3_exec((sqlite3 *)handle, sql_insert, NULL, NULL, NULL) != SQLITE_OK)
    {
        // printf("insert data from db error.\n");
        log_info("%s,write to table(%s) failed\n", __func__, table_name);
        return -1;
    }
    return 0;
}

int mqtt_cache_delete_one_payload(void *handle, const char *table_name, int rowid)
{
    char sql_delete[128];

    memset(sql_delete, 0, 128);
    sprintf(sql_delete, "delete from %s where rowid = %d", table_name, rowid);

    if (sqlite3_exec((sqlite3 *)handle, sql_delete, NULL, NULL, NULL) != SQLITE_OK)
    {
        // printf("delete data from db error.\n");
        log_info("%s,delete from table(%s) rowid(%d) failed\n", __func__, table_name, rowid);
        return -1;
    }
    return 0;
}

int mqtt_cache_get_payload_nb(void *handle, const char *table_name, int *nb)
{
    char sql_select[128];

    memset(sql_select, 0, 128);
    sprintf(sql_select, "select count(1) from %s", table_name);

    if (sqlite3_exec((sqlite3 *)handle, sql_select, callback_count, (void *)nb, NULL) != SQLITE_OK)
    {
        // printf("get payload nb error.\n");
        log_info("%s,get itmNb from table(%s) failed\n", __func__, table_name);
        return -1;
    }
    return 0;
}

int mqtt_cache_free_memory(void *handle)
{
    char sql_select[128];

    memset(sql_select, 0, 128);
    sprintf(sql_select, "vacuum");

    if (sqlite3_exec((sqlite3 *)handle, sql_select, NULL, NULL, NULL) != SQLITE_OK)
    {
        log_info("free memory exec failed\n");
        return -1;
    }
    return 0;
}

int mqtt_cache_get_memory_size(void *handle, long *size)
{
    char sql_page_size[128];
    char sql_page_count[128];
    int page_size = 0;
    int page_count = 0;

    memset(sql_page_size, 0, 128);
    sprintf(sql_page_size, "PRAGMA PAGE_SIZE");

    memset(sql_page_count, 0, 128);
    sprintf(sql_page_count, "PRAGMA PAGE_COUNT");

    if (sqlite3_exec((sqlite3 *)handle, sql_page_size, callback_pageSize, (void *)&page_size, NULL) != SQLITE_OK)
    {
        // printf("get payload nb error.\n");
        return -1;
    }
    else if (sqlite3_exec((sqlite3 *)handle, sql_page_count, callback_pageCount, (void *)&page_count, NULL) != SQLITE_OK)
    {
        return -2;
    }

    *size = page_size * page_count;
    return 0;
}

void mqtt_cache_close(void *handle)
{
    sqlite3_close((sqlite3 *)handle);
}

static int callback_read_one_item(void *para, int ncolumn, char **columnvalue, char *columnname[])
{
    struct mqtt_cache_sqlite_data *stTemp = (struct mqtt_cache_sqlite_data *)para;
    mqtt_ringbuffer_element_t *e = (mqtt_ringbuffer_element_t *)(stTemp->data);

    int i = 0;
    for (i = 0; i < ncolumn; i++)
    {
        if (strcmp("rowid", (const char *)columnname[i]) == 0)
        {
            *(stTemp->rowid) = atoi(columnvalue[i]);
        }
        else if (strcmp("topic", (const char *)columnname[i]) == 0)
        {
            strcpy(e->sztopic, columnvalue[i]);
        }
        else if (strcmp("payload", (const char *)columnname[i]) == 0)
        {
            strcpy(e->szpayload, columnvalue[i]);
        }
    }
    return 0;
}

static int callback_count(void *para, int ncolumn, char **columnvalue, char *columnname[])
{
    if (ncolumn > 0)
    {
        *((int *)para) = atoi(columnvalue[0]);
        return 0;
    }
    else
    {
        return -1;
    }
}

static int callback_pageSize(void *para, int ncolumn, char **columnvalue, char *columnname[])
{
    if (ncolumn > 0)
    {
        *((int *)para) = atoi(columnvalue[0]);
        return 0;
    }
    else
    {
        return -1;
    }
}

static int callback_pageCount(void *para, int ncolumn, char **columnvalue, char *columnname[])
{
    if (ncolumn > 0)
    {
        *((int *)para) = atoi(columnvalue[0]);
        return 0;
    }
    else
    {
        return -1;
    }
}
