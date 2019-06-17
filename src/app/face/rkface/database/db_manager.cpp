/*
 * Database manager class
 *
 * Copyright (C) 2018 Rockchip Electronics Co., Ltd.
 * author: huaping.liao@rock-chips.com
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL), available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "db_manager.h"

#include <string.h>
#include <adk/utils/logger.h>
// static struct sql_tb gFaceTable[] = {
//   {"id",         "INTEGER PRIMARY KEY"}, // main key
//   {"isuse",      "INTEGER"},
//   {"type",       "INTEGER"},
//   {"permission", "INTEGER"},
//   {"tv_sec",     "INTEGER"},
//   {"username",   "VARCHAR(32)"},
//   {"password",   "VARCHAR(16)"},
//   {"face_data",  "VARCHAR(100)"},
// };

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])
namespace rk
{

#define db_debug(...) pr_dbg(__VA_ARGS__)

static DBC_TYPE gDBType[] = {
    DB_UINT32,
    DB_TEXT,
};

static SQL_TABLE gBaseTable[] = {
    {"id",    "INTEGER"}, // Don't need declare PRIMARY KEY
    {"name",  "VARCHAR(32)"},
};

void addSingleQuotes(string &str)
{
    str = "'" + str + "'";
}

void addWhereDescri(string &str)
{
    str = "where " + str + "=";
}

DbManager::DbManager()
    : mDBC(NULL)
{
    //DbInit();
}

DbManager::~DbManager()
{
    DbExit();
}

void DbManager::DbInit()
{
    if (mDBC == NULL) {
        mDBC = new DBCtrl();
    }
    mDBC->createTable(kDbTable, gBaseTable, sizeof(gBaseTable) / sizeof(gBaseTable[0]));
    mDBC->setColumnType(gDBType, 0);
    //dbUpdate();
}

void DbManager::DbExit()
{
    if (mDBC) {
        delete mDBC;
        mDBC = NULL;
    }
}

void DbManager::DbReset()
{
    db_debug("%s %d %p\n", __FUNCTION__, __LINE__, mDBC);
    mDBC->deleteTable(string(kDbTable));

    mDbUpdated = false;
    DbInit();
    mDbUpdated = true;
}

int DbManager::DbUpdate(const char *path)
{
    db_debug("------dbUpate from path\n", path);
}

/*
 * Add method
 * execute include COMMIT_TRANSACTION, slow
 * This is base method, children need rewrite this method.
 */
int DbManager::DbAddRecord(int id, const char *name)
{
    int ret = 0;
    char str[DB_COM_LEN];
    snprintf(str, DB_COM_LEN, "INSERT INTO %s VALUES(%d,'%s')",
             mDBC->getTableName().c_str(), id, name);
    //db_msg("sql is %s\n", str);
    std::lock_guard<std::mutex> lock(mutex_);
    mDBC->setSQL(string(str));
    ret = mDBC->executeSQL();
    if (ret != SQLITE_OK) {
        db_debug("error to add file:%d\n", ret);
    }
    return ret;
}

/*
 * Add method
 * Execute exclude COMMIT_TRANSACTION, fast.
 * This is base method, children need rewrite this method.
 */
int DbManager::DbAddRecordByTransaction(int id, const char *name)
{
    int ret = 0;
    char str[DB_COM_LEN];
    // old is String8::format
    snprintf(str, DB_COM_LEN, "INSERT INTO %s VALUES(%d,'%s')",
             mDBC->getTableName().c_str(), id, name);
    //db_msg("sql is %s\n", str);
    mDBC->setSQL(string(str));
    ret = mDBC->sqlite3Exec();
    if (ret != SQLITE_OK) {
        db_debug("error to add file:%d\n", ret);
    }
    return ret;
}

int DbManager::DbAddRecord(std::vector<BaseRecord> &base_record)
{
    int ret = 0;
    mDBC->setSQL(string(BEGIN_TRANSACTION));
    mDBC->sqlite3Exec();
    //ALOGD("begin transaction");
    for (auto & record : base_record) {
        ret = DbAddRecordByTransaction(record.id, record.name);
        if (ret != SQLITE_OK) {
            db_debug("error to add file:%d\n", ret);
            break;
        }
    }
    //ALOGD("commit transaction");
    mDBC->setSQL(string(COMMIT_TRANSACTION));
    mDBC->sqlite3Exec();
    return ret;
}

// Delete method
void DbManager::DbDelRecord(const char *id)
{
    char str[DB_COM_LEN];
    snprintf(str, DB_COM_LEN, "DELETE from %s WHERE id='%s'",
             mDBC->getTableName().c_str(), id);
    std::lock_guard<std::mutex> lock(mutex_);
    mDBC->setSQL(string(str));
    mDBC->executeSQL();
}

// Delete method
void DbManager::DbDelRecord(const char *item, const char *value)
{
    char str[DB_COM_LEN];
    snprintf(str, DB_COM_LEN, "DELETE from %s WHERE %s='%s'",
             mDBC->getTableName().c_str(), item, value);
    std::lock_guard<std::mutex> lock(mutex_);
    mDBC->setSQL(string(str));
    mDBC->executeSQL();
}

// Change method
void DbManager::DbUpdateRecord(const char *item, const char* oldname, const char *newname)
{
    char str[DB_COM_LEN];
    // Old id String8::format
    snprintf(str, DB_COM_LEN, "UPDATE %s SET %s = '%s' WHERE %s = '%s'",
             mDBC->getTableName().c_str(), item, newname, item, oldname);
    std::lock_guard<std::mutex> lock(mutex_);
    mDBC->setSQL(string(str));
    mDBC->executeSQL();
}

/* Return item count */
int DbManager::DbRecordCnt()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return mDBC->getRecordCnt();
}

/* Return item count by value */
int DbManager::DbRecordCnt(const char *item, const char *value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    string str(item);
    str = "where " + str + "=";
    string value_str(value);
    value_str = "'" + value_str + "'";
    mDBC->setFilter(str + value_str);
    return mDBC->getRecordCnt();
}

void DbManager::DbGetRecord(const char *item, vector<string>&record)
{
    char *ptr = NULL;

    int total = DbRecordCnt();
    std::lock_guard<std::mutex> lock(mutex_);
    mDBC->bufPrepare();
    for (int i = 0; i < total; i++) {
        int idx = GetIdxByItem(&gBaseTable[0], ARRAY_SIZE(gBaseTable), item);
        ptr = (char *)mDBC->getElement(string(item), idx, i);
        if (ptr) {
            record.push_back(string(ptr));
        }
    }
    mDBC->bufRelease();
}

void DbManager::DbGetRecord(const char *item, const char *value,
                            vector<string>&record)
{
    char *ptr = NULL;

    int total = DbRecordCnt(item, value);

    std::lock_guard<std::mutex> lock(mutex_);
    string str(item);
    str = "where " + str + "=";
    string value_str(value);
    value_str = "'" + value_str + "'";
    mDBC->setFilter(str + value_str);
    mDBC->bufPrepare();
    for (int i = 0; i < total; i++) {
        int idx = GetIdxByItem(&gBaseTable[0], ARRAY_SIZE(gBaseTable), item);
        ptr = (char *)mDBC->getElement(string(item), idx, i);
        if (ptr) {
            record.push_back(string(ptr));
        }
    }
    mDBC->bufRelease();
}

void DbManager::DbGetRecord(vector<BaseRecord> &record)
{

    int total = DbRecordCnt();
    std::lock_guard<std::mutex> lock(mutex_);
    mDBC->bufPrepare();
    for (int i = 0; i < total; i++) {
        BaseRecord cur_record;
        memset(&cur_record, 0, sizeof(BaseRecord));
        void *get_record[2]; // To store array
        int num = mDBC->getFilterLine(&get_record[0], i);
        if (num > 0) {
            cur_record.id = *(int *)get_record[0];
            strcpy(cur_record.name, (const char*)get_record[1]);
            record.push_back(cur_record);
        }

    }
    mDBC->bufRelease();
}

void DbManager::SetCurTableName(const char *tableName)
{
    table_name_ = string(tableName);
}

const char *DbManager::GetCurTableName()
{
    return table_name_.c_str();
}

// "id" idx=0; "name" idx=1;
int DbManager::GetIdxByItem(SQL_TABLE *table, int count, const char *item)
{
    unsigned i;
    for (i = 0; i < count; ++i) {
        if (*(table + i)->item == 0)
            continue;
        if (strcmp((table + i)->item, item) == 0) {
            break;
        }
    }
    return (i == count ? -1 : i);
}

// "id" DBC_TYPE=DB_UINT32; "name" DBC_TYPE=DB_TEXT;
DBC_TYPE DbManager::GetTypeByItem(DBC_TYPE *type, SQL_TABLE *table, int count, const char *item)
{
    return *(type + GetIdxByItem(table, count, item));
}

} // namespace rk