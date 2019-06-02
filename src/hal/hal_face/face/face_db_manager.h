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

#ifndef FACE_DB_MANAGER_H_
#define FACE_DB_MANAGER_H_

#include <mutex>
#include <adk/base/definition_magic.h>

#include "face_feature.h"
#include "database/DBCtrl.h"

namespace rk {

#define DB_FEATURE_SIZE 128

#define INSERT_INTO "INSERT INTO "
#define DELETE_FROM "DELETE from "

class FaceDbManager {
public:
    FaceDbManager() : mDBC(NULL) {}
    virtual ~FaceDbManager() {
         DbExit();
    }
    FaceDbManager* GetInstance(void) {
        static FaceDbManager instance_;
        return &instance_;
    }

    ADK_DECLARE_SHARED_PTR(FaceDbManager);

    void DbInit();
    void DbExit();
    void DbReset();

    const char *GetCurTableName();
    void SetCurTableName(const char *tableName);

    int DbRecordCount(void);
    int DbAddFeature(FaceFeature& feature, int* user);
    void DbDeleteUser(int user);
    void DbGetFeatureByIndex(int index, FaceFeature& feature);
    void DbGetUserByIndex(int index, int* user);

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
