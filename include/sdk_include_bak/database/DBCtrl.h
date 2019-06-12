#ifndef _DBCTRL_H
#define _DBCTRL_H

#include <string>
#include <assert.h>

#include "DBCon.h"
#include "sqlite/sqlite3.h"

using namespace std;

typedef enum
{
	DB_UINT32 = 1,
	DB_INT64,
	DB_UINT16,
	DB_UINT8,
	DB_TEXT,
	DB_BLOB,
	DB_FLOAT,
	DB_DOUBLE,
	DB_DATETIME
} DBC_TYPE;

typedef struct sql_tb
{
    char item[32];
    char type[32];
} SQL_TABLE;

namespace rk {

class DBCtrl
{
public:
	DBCtrl();
	DBCtrl(string dbpath);
	~DBCtrl();
	void setSQL(const string sql);
	string getSQL(void) const;
	string getTableName(void) const;
	void setTableName(const string name);
	int insertRecord(void);
	int bufPrepare(void);
	void bufRelease(void);
	int prepare(void);
	int finish(void);
	int deleteRecord(void);
	int executeSQL(void);
	int sqlite3Exec(void);
	unsigned int getRecordCnt(void);
	int getOneLine(void *pValue[]);
	int getFilterLine(void *pValue[], unsigned int rowNumber);
	int getFilterFirstLine(void *pValue[]);
	void * getFilterFirstElement(string column, int idx);
	void *getElement(string columnName, int columnIdx, unsigned int rowNumber);
	int createTable(string tbName, struct sql_tb *tb, int cnt);
	void setRecord(string columnName, void *pValue);
	void setColumnType(DBC_TYPE *type, int *typeSize);
	void setFilter(const string filter);
	int deleteTable(string tbName);
private:
	void setColumnCnt(const int num);
	void createRequerySQL(void);
	void createAddSQL(void);
	string kDbPath = "/data/rkLock.db";
	string mName;
	string mSQL;
	int mColumnCnt;
	int mTmpNum;
	DBC_TYPE *mColumnType;
	int *mColumnTypeSize;
	unsigned int *mValue;
	string *mColumnName;
	unsigned char *mFieldKey;
	SQLCon *mSQLCon;
	DBCon *mDBCon;
	sqlite3_stmt *mSqlstmt;
	string mFilter;

	unsigned int	result_uint32;
	long long		result_int64;
	unsigned short	result_uint16;
	unsigned char	result_uint8;
	float			result_float;
	double			result_double;
	char (*queryBuf)[10000];
};

} // namespace rk
#endif //DBCtrl
