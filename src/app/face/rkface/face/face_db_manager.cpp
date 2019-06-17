/*
 * Copyright (C) 2019 Rockchip Electronics Co., Ltd.
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

#define TAG "face_service"

#include "face_db_manager.h"

#include <vector>
#include <string.h>
#include <sstream>

#include <adk/utils/logger.h>

using namespace rk;

static DBC_TYPE gDBType[] = {
    DB_UINT32,
    DB_TEXT,
};

static SQL_TABLE gBaseTable[] = {
    {"id",    "INTEGER"},
    {"feature",  "VARCHAR(128)"},
};

int FaceDbManager::DbRecordCount(void)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return mDBC->getRecordCnt();
}

void FaceDbManager::SetCurTableName(const char *tableName)
{
    table_name_ = string(tableName);
}

const char *FaceDbManager::GetCurTableName()
{
    return table_name_.c_str();
}

void FaceDbManager::DbInit()
{
    if (mDBC == NULL) {
        mDBC = new DBCtrl("/data/face.db");
    }
    mDBC->createTable(kDbTable, gBaseTable, sizeof(gBaseTable) / sizeof(gBaseTable[0]));
    mDBC->setColumnType(gDBType, 0);
}

void FaceDbManager::DbExit() {
    if (mDBC) {
        delete mDBC;
        mDBC = NULL;
    }
}

void FaceDbManager::DbReset()
{
    mDBC->deleteTable(string(kDbTable));

    mDbUpdated = false;
    DbInit();
    mDbUpdated = true;
}

static string VALUES(const char* item, FaceFeature& feature) {
    std::stringstream stream;

    stream << item;
    stream << " VALUES('";

    for (int i = 0; i < feature.size(); i++) {
        stream << feature.feature()[i];
        stream << " ";
    }
    stream << "')";

    return stream.str();
}

int FaceDbManager::DbAddFeature(FaceFeature& feature, int* user)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::stringstream stream;

    stream << INSERT_INTO;
    stream << mDBC->getTableName();
    stream << VALUES(" (feature)", feature);
    string str(stream.str());

    mDBC->setSQL(stream.str());
    int ret = mDBC->executeSQL();
    if (ret != SQLITE_OK) {
        pr_err("error to add file:%d\n", ret);
        return -1;
    }

    int user_id = -1;
    std::stringstream request;

    request << "where feature='";
    for (int i = 0; i < feature.size(); i++) {
        request << feature.feature()[i];
        request << " ";
    }
    request << "'";

    string condition(request.str());

    mDBC->setFilter(condition);
    mDBC->bufPrepare();

    void *get_record[2];
    int num = mDBC->getFilterLine(&get_record[0], 0);
    if (num > 0) {
        *user = (int)get_record[0];
    }

    mDBC->bufRelease();
    return 0;
}

void FaceDbManager::DbDeleteUser(int user)
{
    std::stringstream stream;

    stream << DELETE_FROM;
    stream << mDBC->getTableName();
    stream << " WHERE id='";
    stream << user;
    stream << "'";

    std::lock_guard<std::mutex> lock(mutex_);
    mDBC->setSQL(stream.str());
    mDBC->executeSQL();
}

static vector<string> &split(const string &str, char delim,
                             vector<string> &elems, bool skip_empty = true)
{
    istringstream iss(str);
    for (string item; getline(iss, item, delim); )
        if (skip_empty && item.empty()) continue;
        else elems.push_back(item);
    return elems;
}

void FaceDbManager::DbGetFeatureByIndex(int user, FaceFeature& dst_feature)
{
    int total = DbRecordCount();
    if (user > total) {
        pr_err("user error, total = %d, user = %d\n", total, user);
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    mDBC->bufPrepare();

    void *get_record[2];
    float feature[DB_FEATURE_SIZE] = {0};

    int num = mDBC->getFilterLine(&get_record[0], user);
    if (num > 0) {
        std::string str((char*)get_record[1]);
        vector<string> result;
        split(str, ' ', result);

        for (int i = 0; i < result.size(); i++) {
            stringstream stream(result[i]);
            stream >> feature[i];
            dst_feature.push_back(feature[i]);
        }
    }

    mDBC->bufRelease();
}

void FaceDbManager::DbGetUserByIndex(int index, int* user)
{
    int total = DbRecordCount();
    if (index > total) {
        pr_err("id error, total = %d, index = %d\n", total, index);
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    mDBC->bufPrepare();

    void *get_record[2];
    float feature[DB_FEATURE_SIZE] = {0};

    int num = mDBC->getFilterLine(&get_record[0], index);
    if (num > 0)
        *user = (int)get_record[0];

    mDBC->bufRelease();
}

