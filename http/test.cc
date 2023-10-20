#include"utils.hpp"


#define HOST "127.0.0.1"
#define PORT 3306
#define USER "root"
#define PASSWARD "#Yaz546040011213"
#define DBNAME "calculate_db"
int main()
{
    MYSQL* mysql = mysql_util::mysql_create(HOST, USER, PASSWARD, DBNAME, PORT);
    const char* sql = "insert CalculatorHistory (operand1, operand2, operator, result) values (1.0, 2.0, '*', 2.0)";
    bool ret = mysql_util::mysql_exec(mysql, sql);
    if(ret == false) {
        return -1;
    }
    mysql_util::mysql_destroy(mysql);
    return 0;
}