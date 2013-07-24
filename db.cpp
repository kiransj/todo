#include <stdlib.h>
#include "db.h"

char DEFAULT_FILE_NAME[1024] = "todo.db";
const char * pr_state_tostring(PrState state)
{

    if(state >= STATE_NOT_STARTED && state < STATE_UNKNOWN)
        return PrState_string[state];
    else
        return PrState_string[0];
}

PrInfo NewPR(const char *pr_number, const char * pr_desc)
{
    PrInfo pr;
    strcpy(pr.pr_number, pr_number);
    strcpy(pr.pr_header, pr_desc);
    time(&pr.pr_date);
    pr.pr_state = STATE_NOT_STARTED;
    return pr;
}

const char * time_since(time_t old_time)
{
    double diff;
    time_t now;
    static char str[128] = "\0";
    time(&now);
    diff = difftime(now, old_time);

    memset(str, 0, 128);

    if(diff < 60)
    {
        snprintf(str, 128, "few Sec Ago");
        return str;
    }
    diff = diff/60;
    if(diff < 60)
    {
        snprintf(str, 128, "%2d Mn%s Ago", (int)diff, ((int)diff) == 1 ? " " : "s");
        return str;
    }
    diff = diff/60;
    if(diff < 24)
    {
        snprintf(str, 128, "%2d Hr%s Ago", (int)diff, ((int)diff) == 1 ? " " : "s");
        return str;
    }
    diff = diff/24;
    if(diff < 7)
    {
        snprintf(str, 128, "%2d Dy%s Ago ", (int)diff, ((int)diff) == 1 ? " " : "s");
        return str;
    }
    diff = diff/7;
    if(diff <= 4)
    {
        snprintf(str, 128, "%2d Wk%s Ago ", (int)diff, ((int)diff) == 1 ? " " : "s");
        return str;
    }

    diff = diff/4;
    if(diff < 12)
    {
        snprintf(str, 128, "%2d Mn%s Ago", (int)diff, ((int)diff) == 1 ? " " : "s");
        return str;
    }
    return str;
}


void print_pr(PrInfo pr)
{
    long diff;
    time_t now;
    char buffer[1024];
    struct tm *timeinfo;

    (long)time(&now);
    diff = (long) difftime(now, pr.pr_date);
    timeinfo = localtime(&pr.pr_date);
    strftime(buffer, 1024, "%e-%b-%y %I%p", timeinfo);
    printf("%10s %luHr %10s %s\n", pr.pr_number, diff/3600, pr_state_tostring(pr.pr_state), pr.pr_header);    
    for(int i = 0; i < pr.pr_desc.size(); i++)
    {
        printf("       %d> %s %d\n", 1+i, pr.pr_desc[i].pr_desc, pr.pr_desc[i].updated_date); 
    }
}

bool SqlDB::execute_stmt(const char *query)
{
    int rc;
    bool flag = false;    
    sqlite3_stmt *statement;        
    rc = sqlite3_prepare_v2(database, query, -1, &statement, 0);
    if(rc == SQLITE_OK)
    {
        int rc = sqlite3_step(statement);
        if(rc == SQLITE_DONE)
        {
            flag = true;
        }
        else
        {
            snprintf(last_error_msg, 1024, "Create table query '%s' failed", query);
        }
        sqlite3_finalize(statement);
    }
    else
    {
        snprintf(last_error_msg, 1024, "Create table query '%s' failed", query);
    }
    return flag;
}
bool SqlDB::connect(const char *filepath)
{
    const char *filename = (NULL == filepath) ? DEFAULT_FILE_NAME : filepath;
    const char *query_enable_foreign_keys = "PRAGMA foreign_keys = ON;";   

    const char *query_create_todo_table =   "CREATE TABLE IF NOT EXISTS TODOLIST (" 
                                            "PR_NUMBER TEXT PRIMARY KEY,"
                                            "PR_HEADER TEXT NOT NULL,"
                                            "PR_DATE INTEGER NOT NULL,"
                                            "PR_STATE INTEGER NOT NULL"
                                            ");";

    const char *query_create_desc_table =   "CREATE TABLE IF NOT EXISTS PR_DESC (" 
                                            "PR_NUMBER TEXT NOT NULL,"
                                            "DESCRIPTION TEXT NOT NULL,"
                                            "UPDATED_DATE INTEGER NOT NULL,"
                                            "FOREIGN KEY(PR_NUMBER) REFERENCES TODOLIST(PR_NUMBER)"
                                            ");";   


    status = false;
    if(sqlite3_open(filename, &database) != SQLITE_OK)
    {
        snprintf(last_error_msg, 1024, "sqlite3_open(%s) failed", filepath);
        database = NULL;
        return false;
    } 
    else
    {
        status = execute_stmt(query_enable_foreign_keys);
        if(true == status) status = execute_stmt(query_create_todo_table);
        if(true == status) status = execute_stmt(query_create_desc_table);
    }
    if(status == false)
    {
        sqlite3_close(database);
    }
    else
    {
        strcpy(sqldb, filename);
    }
    return status;
}


bool SqlDB::add_new_pr(const PrInfo pr)
{
    bool flag = false;
    char query_insert_newpr[1024];
    snprintf(query_insert_newpr, 1024,  "INSERT INTO ToDoList (PR_NUMBER, PR_HEADER, PR_DATE, PR_STATE)"
                                     "VALUES('%s', '%s', %lu, %u);",
                                     pr.pr_number,
                                     pr.pr_header,
                                     pr.pr_date,
                                     pr.pr_state);
    if(status == false)
    {
        snprintf(last_error_msg, 1024, "First open the database before any query");
        return flag;    
    }
    else
    {
        int rc;
        sqlite3_stmt *statement;     
        rc = sqlite3_prepare_v2(database, query_insert_newpr, -1, &statement, 0);
        if(rc == SQLITE_OK)
        {
            int rc = sqlite3_step(statement);
            if(rc == SQLITE_DONE)
            {
                status = true;
                flag = true;
            }
            else if(rc == SQLITE_CONSTRAINT)
            {
                if(get_pr(pr.pr_number, NULL) == true)
                    snprintf(last_error_msg, 1024, "Pr %s already exists", pr.pr_number);
                else
                    snprintf(last_error_msg, 1024, "SQLITE_CONSTRAINT error");
            }
            else
            {
                snprintf(last_error_msg, 1024, "query '%s' failed", query_insert_newpr);
            }
            sqlite3_finalize(statement);
        }
        else
        {
            printf("Error in stat  %d\n", rc);
        }
    }
    return flag;
}


int SqlDB::count(void)
{
    int count = -1;
    const char *query_count = "SELECT COUNT(*) from ToDoList;";
    if(status == false)
    {
        snprintf(last_error_msg, 1024, "First open the database before any query");
        return count;
    }    
    else
    {
        int rc;
        sqlite3_stmt *statement;     
        rc = sqlite3_prepare_v2(database, query_count, -1, &statement, 0);
        if(rc == SQLITE_OK)
        {
            int rc = sqlite3_step(statement);
            if(rc == SQLITE_ROW)
            {
                count = (unsigned long)sqlite3_column_int(statement, 0);
            }
            else
            {
                snprintf(last_error_msg, 1024, "query '%s' failed", query_count);
            }
            sqlite3_finalize(statement);
        }
        else
        {
            snprintf(last_error_msg, 1024, "query '%s' failed", query_count);
        }
    }
    return count;
}

vector<PrDesc> SqlDB::get_pr_desc(const char *pr_number)
{
    vector<PrDesc> desc;

    char query_pr_desc[1024];
        
        
    snprintf(query_pr_desc, 1024, "SELECT DESCRIPTION, UPDATED_DATE FROM PR_DESC WHERE PR_NUMBER = '%s' ORDER BY UPDATED_DATE DESC;", pr_number);
    if(status == false)
    {
        snprintf(last_error_msg, 1024, "First open the database before any query");
        return desc;
    }
    else
    {
        int rc;
        sqlite3_stmt *statement;        
        rc = sqlite3_prepare_v2(database, query_pr_desc, -1, &statement, 0);
        if(rc == SQLITE_OK)
        {
            int cols = sqlite3_column_count(statement);
            int rc;
            while((rc = sqlite3_step(statement)) == SQLITE_ROW)
            {
                PrDesc pr_desc;
                strcpy(pr_desc.pr_desc, (char*)sqlite3_column_text(statement, 0));
                pr_desc.updated_date = (time_t)sqlite3_column_int(statement, 1);
                desc.push_back(pr_desc);
            }
            sqlite3_finalize(statement);
        }
        else
        {
            snprintf(last_error_msg, 1024, "Query '%s' failed", query_pr_desc);
        }
    }

    return desc;
}

bool SqlDB::get_pr(const char *pr_number, PrInfo *pr)
{
    char query_pr[128];
    snprintf(query_pr, 128, "SELECT PR_NUMBER, PR_HEADER, PR_STATE, PR_DATE FROM ToDoList where PR_NUMBER = '%s'", pr_number);
    if(status == false)
    {
        snprintf(last_error_msg, 1024, "First open the database before any query");
        return false;
    }
    else
    {
        int rc;
        sqlite3_stmt *statement;        
        rc = sqlite3_prepare_v2(database, query_pr, -1, &statement, 0);
        if(rc == SQLITE_OK)
        {
            int cols = sqlite3_column_count(statement);
            int rc;
            if((rc = sqlite3_step(statement)) == SQLITE_ROW)
            {
                if(NULL != pr)
                {
                    strcpy(pr->pr_number, (char*)sqlite3_column_text(statement, 0));
                    strcpy(pr->pr_header, (char*)sqlite3_column_text(statement, 1));
                    pr->pr_state = (PrState)sqlite3_column_int(statement, 2);
                    pr->pr_date = (time_t)sqlite3_column_int(statement, 3);
                    pr->pr_desc = get_pr_desc(pr->pr_number); 
                }
                sqlite3_finalize(statement);
                return true;
            }
            sqlite3_finalize(statement);
            snprintf(last_error_msg, 1024, "Pr '%s' not found", pr_number);
        }
        else
        {
            snprintf(last_error_msg, 1024, "Query '%s' failed", query_pr);
        }
    }

    return false;
}
vector<PrInfo> SqlDB::get_all_pr(void)
{
    vector<PrInfo> prlist;

    const char *query_all = "SELECT P.PR_NUMBER, P.PR_HEADER, P.PR_STATE, P.PR_DATE, "
                            "(SELECT COUNT(*) FROM PR_DESC WHERE PR_NUMBER = P.PR_NUMBER) AS COUNT "
                            "FROM ToDoList as P "
                            "ORDER By PR_STATE ASC, PR_DATE DESC;";
    if(status == false)
    {
        snprintf(last_error_msg, 1024, "First open the database before any query");
        return prlist;
    }
    else
    {
        int rc;
        sqlite3_stmt *statement;        
        rc = sqlite3_prepare_v2(database, query_all, -1, &statement, 0);
        if(rc == SQLITE_OK)
        {
            int cols = sqlite3_column_count(statement);
            int rc;
            while((rc = sqlite3_step(statement)) == SQLITE_ROW)
            {
                PrInfo pr;
                strcpy(pr.pr_number, (char*)sqlite3_column_text(statement, 0));
                strcpy(pr.pr_header, (char*)sqlite3_column_text(statement, 1));
                pr.pr_state = (PrState)sqlite3_column_int(statement, 2);
                pr.pr_date = (time_t)sqlite3_column_int(statement, 3);
                pr.num_desc = (int)sqlite3_column_int(statement, 4);
                prlist.push_back(pr);
            }
            sqlite3_finalize(statement);
        }
        else
        {
            snprintf(last_error_msg, 1024, "Query '%s' failed", query_all);
        }
    }

    return prlist;
}

bool SqlDB::add_update(const char *pr_number, const char *msg)
{
    char query_insert_update[1024];
    time_t ti;
    time(&ti);    
    snprintf(query_insert_update, 1024,  "INSERT INTO PR_DESC (PR_NUMBER, DESCRIPTION, UPDATED_DATE)"
                                         "VALUES('%s', '%s', %lu);",
                                         pr_number, msg, ti);
    if(status == false)
    {
        snprintf(last_error_msg, 1024, "First open the database before any query");
        return false;
    }
    return execute_stmt(query_insert_update);
}

bool SqlDB::change_state(const char *pr_number, PrState new_state)
{
    PrInfo pr;
    PrState old_state;
    bool flag = false;
    char query_state_change[512];   
    snprintf(query_state_change, 512, "UPDATE TODOLIST SET PR_STATE=%d WHERE PR_NUMBER='%s';", new_state, pr_number);        
    if(status == false)
    {
        snprintf(last_error_msg, 1024, "First open the database before any query");
        return false;
    }
    
    if(get_pr(pr_number, &pr) == false)
    {
        snprintf(last_error_msg, 1024, "Pr '%s' does not exists", pr_number);
        return false;
    }

    old_state = pr.pr_state;
    if(old_state == new_state)
    {
        snprintf(last_error_msg, 1024, "new and old state are the same");
        return false;   
    }       

    flag = execute_stmt(query_state_change);
    if(flag == true)
    {
        char msg[1024];
        snprintf(msg, 1024, "change state from %s to %s", PrState_string[old_state], PrState_string[new_state]);
        flag = add_update(pr_number, msg);
    }
    return flag;
}

int start_ui(void);
int main(int argc, char *argv[])
{
    snprintf(DEFAULT_FILE_NAME, 1024, "%s/%s", getenv("HOME"), DB_NAME);
    start_ui();
    return 0;
}


int main2(int argc, char *argv[])
{
    SqlDB todo;   
    PrInfo pr4 = NewPR("1341", "avi files not playing in GPVR");
    PrInfo pr1 = NewPR("1342", "optimise MP4 seeks");
    PrInfo pr2 = NewPR("1343", "implement AVI file");
    PrInfo pr3 = NewPR("1344", "read the docs");

    if(false == todo.connect("1.db"))
    {
        printf("%s\n", todo.last_error());
        return 0l;
    }
    todo.add_new_pr(pr4);
    todo.add_new_pr(pr1);
    todo.add_new_pr(pr2);
    todo.add_new_pr(pr3);
    printf("Number of entries %d \n", todo.count());
    vector<PrInfo> pr = todo.get_all_pr();
    if(pr.size() == 0)
    {
        printf("%s\n", todo.last_error());
    } 

    for(int i = 0; i < pr.size(); i++)
    {
        PrInfo pr1;
        todo.get_pr(pr[i].pr_number, &pr1);
        print_pr(pr1);;
    }
    return 0;
}
