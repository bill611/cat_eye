#ifndef LOCALSQLCONNECTH
#define LOCALSQLCONNECTH

#ifndef FALSE
    #define FALSE 0
    #define TRUE 1
    #define BOOL int
#endif
//----------------------------------------------------------------------------
struct SqlitePrivate;        //私有数据
#define SQL_NAME_MAX 32 
typedef struct _TSQLiteField
{
	struct SqlitePrivate *Private;
	char Name[SQL_NAME_MAX];											//字段名
	int offset;															//第几个字段
    char * (*AsChar)(struct _TSQLiteField *This,char *Buf,int Size);    //作为字段型读取
    int (*AsInt)(struct _TSQLiteField *This);                           //作为整型读取
    double (*AsFloat)(struct _TSQLiteField *This);                      //作为浮点型读取
} TSQLiteField,*PSQLiteField;
//----------------------------------------------------------------------------

struct Sqlite_mutex; 
typedef struct _TSqlite
{
    struct SqlitePrivate * Private;
	PSQLiteField Fields;
    char ServerIP[16];
    int ServerPort;
	const char *file_name;
	struct Sqlite_mutex *sql_lock;

    void (*Destroy)(struct _TSqlite *This);        //销毁
    BOOL (*Open)(struct _TSqlite *This);            //打开
    BOOL (*ExecSQL)(struct _TSqlite *This);         //执行
    void (*Close)(struct _TSqlite *This);           //关闭
    void (*First)(struct _TSqlite *This);          //首记录
    void (*Last)(struct _TSqlite *This);           //末记录
    void (*Prior)(struct _TSqlite *This);          //上一记录
    void (*Next)(struct _TSqlite *This);           //下一记录
    void (*SetRecNo)(struct _TSqlite *This,int RecNo);       //跳到记录号
    int (*RecNo)(struct _TSqlite *This);          //返回记录号
    PSQLiteField (*FieldByName)(struct _TSqlite *This,char *Name);       //返回字段
    int (*GetSQLText)(struct _TSqlite *This,char *pBuf,int Size);  //取SQL命令行
    void (*SetSQLText)(struct _TSqlite *This,char *SqlCmd);        //设置SQL命令行
	BOOL (*Active)(struct _TSqlite *This);							//是否打开表
    int (*RecordCount)(struct _TSqlite *This);						//读行数
    int (*FieldCount)(struct _TSqlite *This);						//字段数
	int (*LastRowId)(struct _TSqlite *This);						//取得最后插入影响的ID

    void (*prepare)(struct _TSqlite *This,char *SqlCmd);
    void (*reset)(struct _TSqlite *This);
    void (*finalize)(struct _TSqlite *This);
    void (*step)(struct _TSqlite *This);
    void (*bind_int)(struct _TSqlite *This,int arg);
    void (*bind_text)(struct _TSqlite *This,char *text);
    void (*bind_blob)(struct _TSqlite *This,void *data,int size);
	void (*getBlobData)(struct _TSqlite *This,void *data);
} TSqlite;

typedef struct _TSqliteData {      
    const char *file_name;         
    TSqlite *sql;                  
    int (*checkFunc)(TSqlite *sql);
}TSqliteData;                      

TSqlite * CreateLocalQuery(const char *FileName);
void LocalQueryLoad(TSqliteData *sql);

BOOL LocalQueryOpen(TSqlite *Query,char *SqlStr);
BOOL LocalQueryExec(TSqlite *Query,char *SqlStr);
char* LocalQueryOfChar(TSqlite *Query,char *FieldName,char *cBuf,int Size);
int LocalQueryOfInt(TSqlite *Query,char *FieldName);
double LocalQueryOfFloat(TSqlite *Query,char *FieldName);
//----------------------------------------------------------------------------
#endif
