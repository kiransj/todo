#include "sqlite.h"

const char * pr_state_tostring(PrState state)
{
    static const char *PrState_string[] = { "UNKOWN", 
                                            "NOT_STATED",
                                            "WORKING   ",
                                            "WAITING   ",
                                            "CLOSED    ",
                                            "UNKNOWN   "
                                };
    if(state >= STATE_NOT_STARTED && state <= STATE_CLOSED)
        return PrState_string[state];
    else
        return PrState_string[0];
}

PrInfo NewPR(const char *pr_number, const char * pr_desc)
{
    PrInfo pr;
    strcpy(pr.pr_number, pr_number);
    strcpy(pr.pr_desc, pr_desc);
    time(&pr.pr_date);
    pr.pr_state = STATE_WORKING;
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
        snprintf(str, 128, "few Min Ago");
        return str;
    }
    diff = diff/60;
    if(diff < 24)
    {
        snprintf(str, 128, "Few Hrs Ago", (int)diff);
        return str;
    }
    diff = diff/24;
    if(diff < 7)
    {
        snprintf(str, 128, "%d Days Ago ", (int)diff);
        return str;
    }
    diff = diff/7;
    if(diff <= 4)
    {
        snprintf(str, 128, "%d Week Ago ", (int)diff);
        return str;
    }

    diff = diff/4;
    if(diff < 12)
    {
        snprintf(str, 128, "%d Months Ago", (int)diff);
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
    diff = difftime(now, pr.pr_date);
    timeinfo = localtime(&pr.pr_date);
    strftime(buffer, 1024, "%e-%b-%y %I%p", timeinfo);
    printf("%10s %luHr %10s %s\n", pr.pr_number, diff/3600, pr_state_tostring(pr.pr_state), pr.pr_desc);
}

bool SqlDB::connect(const char *filepath)
{
    const char *query_create_todo_table =  "CREATE TABLE IF NOT EXISTS ToDoList (" 
        "PR_NUMBER TEXT PRIMARY KEY,"
        "PR_DESC TEXT NOT NULL,"
        "PR_DATE INTEGER NOT NULL,"
        "PR_STATE INTEGER NOT NULL"
        ");";

    status = false;
    if(sqlite3_open(filepath, &database) != SQLITE_OK)
    {
        snprintf(last_error_msg, 1024, "sqlite3_open(%s) failed", filepath);
        database = NULL;
        return false;
    } 
    else
    {

        int rc;
        sqlite3_stmt *statement;        
        rc = sqlite3_prepare_v2(database, query_create_todo_table, -1, &statement, 0);
        if(rc == SQLITE_OK)
        {
            int rc = sqlite3_step(statement);
            if(rc == SQLITE_DONE)
            {
                status = true;
            }
            else
            {
                snprintf(last_error_msg, 1024, "Create table query '%s' failed", query_create_todo_table);
            }
            sqlite3_finalize(statement);

        }
    }
    if(status == false)
    {
        sqlite3_close(database);
    }
    strcpy(sqldb, filepath);
    return status;
}


bool SqlDB::add_new_pr(const PrInfo pr)
{
    bool flag = false;
    char query_insert_newpr[1024];
    snprintf(query_insert_newpr, 1024,  "INSERT INTO ToDoList (PR_NUMBER, PR_DESC, PR_DATE, PR_STATE)"
                                     "VALUES('%s', '%s', %lu, %u);",
                                     pr.pr_number,
                                     pr.pr_desc,
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
                snprintf(last_error_msg, 1024, "Pr %s could be already present in the list", pr.pr_number);
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
vector<PrInfo> SqlDB::get_all_pr(void)
{
    vector<PrInfo> prlist;

    const char *query_all = "SELECT PR_NUMBER, PR_DESC, PR_STATE, PR_DATE FROM ToDoList ORDER By PR_DATE;";
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
                strcpy(pr.pr_desc, (char*)sqlite3_column_text(statement, 1));
                pr.pr_state = (PrState)sqlite3_column_int(statement, 2);
                pr.pr_date = (time_t)sqlite3_column_int(statement, 3);
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



int main2(int argc, char *argv[])
{
    SqlDB todo;   
    PrInfo pr4 = NewPR("1341", "avi files not playing in GPVR");
    PrInfo pr1 = NewPR("1342", "optimise MP4 seeks");
    PrInfo pr2 = NewPR("1343", "implement AVI file");
    PrInfo pr3 = NewPR("1344", "read the docs");
    todo.connect("1.db");
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
        print_pr(pr[i]);
    }
    return 0;
}
