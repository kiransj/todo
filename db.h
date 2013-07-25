#include "sqlite3.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>
using namespace std;

#define DB_NAME ".todo.db"
static const char *PrState_string[] = { "UNKOWN", 
                                        "NOT_STARTED",
                                        "WORKING",
                                        "WAITING",
                                        "CLOSED",
                                        "DELETED",
                                        "UNKNOWN"
                               };
typedef enum 
{      
    STATE_NOT_STARTED = 1,
    STATE_WORKING,
    STATE_WAITING,
    STATE_CLOSED,
    STATE_DELETED,
    STATE_UNKNOWN,
}PrState;

typedef struct 
{
    char        pr_desc[1024];
    time_t      updated_date;
}PrDesc;

typedef struct
{
    char               pr_number[16];
    char               pr_header[128];
    time_t             pr_date;
    PrState            pr_state;
    int                num_desc;
    time_t             last_update;
    vector<PrDesc>     pr_desc;
}PrInfo;


const char * pr_state_tostring(PrState state);
void print_pr(PrInfo pr);
PrInfo NewPR(const char *pr_number, const char * pr_desc);
const char * time_since(time_t time);
class SqlDB
{
    private:
        char     sqldb[256];
        sqlite3 *database;
        bool     status;
        char     last_error_msg[1024];

        bool     execute_stmt(const char *query);
        vector<PrDesc> get_pr_desc(const char *pr_number);
    public:
        SqlDB()
        {
            database = NULL;
        }
        ~SqlDB() 
        {
            if(NULL != database)
            {
                sqlite3_close(database);
            }
        }
        bool connect(const char *filepath = NULL);
        bool add_new_pr(const PrInfo pr);
        vector<PrInfo> get_all_pr(void);
        int count(void);
        char* last_error(void) { return last_error_msg; }
        bool get_pr(const char *pr_number, PrInfo *pr);
        bool change_state(const char *pr_number, PrState new_state);
        bool add_update(const char *pr_number, const char *msg);
};
