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

#ifndef DB_MANAGER_H_
#define DB_MANAGER_H_

#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <mutex>
#include <vector>
#include <string>

#include "database/DBCtrl.h"

#define BEGIN_TRANSACTION "begin transaction"
#define COMMIT_TRANSACTION "commit transaction"
#define DB_COM_LEN  256
#define DB_NAME_LEN 32

typedef struct _BaseRecord  {
    int id;
    char name[DB_NAME_LEN];
} BaseRecord;

namespace rk
{

class DbManager
{
public:
    DbManager();
    virtual ~DbManager();
    DbManager* GetInstance(void) {
        static DbManager instance_;
        return &instance_;
    }
    void DbInit();
    void DbExit();
    void DbReset();
    int DbUpdate(const char *path);
    int DbAddRecord(int id, const char *name);
    int DbAddRecordByTransaction(int id, const char *name);
    int DbAddRecord(std::vector<BaseRecord> &base_record);
    void DbDelRecord(const char *id);
    void DbDelRecord(const char *item, const char *value);
    void DbUpdateRecord(const char *item, const char* oldname, const char *newname);
    int DbRecordCnt();
    int DbRecordCnt(const char *item, const char *value);
    void DbGetRecord(vector<string>&record);
    void DbGetRecord(const char *item, const char *value, vector<string>&record);
    void DbGetRecord(const char *item, std::vector<string>&record);
    void DbGetRecord(const char* item, string &file);
    void DbGetRecord(vector<BaseRecord> &record);
    void SetCurTableName(const char *tableName);
    const char *GetCurTableName();
    int GetIdxByItem(SQL_TABLE *table, int count, const char *item);
    DBC_TYPE GetTypeByItem(DBC_TYPE *type, SQL_TABLE *table,
                           int count, const char *item);
private:
    DBCtrl *mDBC;
    std::mutex mutex_;
    bool mDbUpdated;
    string kDbPath = "/data/lock.db";
    string kDbTable = "BaseData";
    string table_name_;
};

} // namespace rk

#endif // DB_MANAGER_H_
