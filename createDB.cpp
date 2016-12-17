//
// Created by cristian on 10.01.2016.
//

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string>
#include <iostream>
#include <string.h>

using namespace std;
char *argv_password;
int count=0;

static int callback(void *data, int argc, char **argv, char **azColName){
    count = atoi(argv[0]);
    printf("\n");
    return 0;
}
string encryptDecrypt(string toEncrypt) {
    char key = 'K'; //Any char will work
    string output = toEncrypt;
    printf("Parola criptata: %s",output.c_str());
    for (int i = 0; i < toEncrypt.size(); i++)
        output[i] = toEncrypt[i] ^ key;
    printf("\nParola decriptata: %s",output.c_str());
    return output;
}
void insertData(sqlite3 *clients_db, string username, string password){
    string sql;
    int  rc;
    char *errMsg = 0;

    //Insert into table
    //sql = "INSERT INTO CLIENTS (USERNAME,PASSWORD) "  \
         "VALUES ('mircea.cretu', '12345asd'); " \
         "INSERT INTO CLIENTS (USERNAME,PASSWORD) "  \
         "VALUES ('ion', '123abc'); " \
         "INSERT INTO CLIENTS (USERNAME,PASSWORD) "  \
         "VALUES ('mihai_blegu', '12345'); " \
         "INSERT INTO CLIENTS (USERNAME,PASSWORD) "  \
         "VALUES ('eu', '12345'); ";

    //Insert into table
    sql = "INSERT INTO CLIENTS (USERNAME,PASSWORD,LOGIN_BAN,UPLOAD_BAN,DOWNLOAD_BAN,MKDIR_BAN,COPYDIR_BAN) "  \
         "VALUES ('";
    sql += username ;
    sql += "', '";
    sql += password;
    sql += "', 0, 0, 0, 0, 0 ";
    sql += "); ";
    cout << "\nSQL: " <<sql <<'\n';
    const char *cstr = sql.c_str();

    // Execute SQL statement
    rc = sqlite3_exec(clients_db, cstr, callback, 0, &errMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }else{
        fprintf(stdout, "User creat cu succes\n");
    }
}
int search_user(sqlite3 *clients_db, string password, string username){
    string sql;
    int  rc;
    char *errMsg = 0;

    //Select * from table
    sql = "SELECT count(*) FROM CLIENTS WHERE USERNAME LIKE '";
    sql += username ;
    sql += "' AND PASSWORD LIKE '";
    sql += password;
    sql += "';";
    const char* data = "Callback function called";
    printf("\nSQL: %s\n",sql.c_str());

    /* Execute SQL statement */
    rc = sqlite3_exec(clients_db, sql.c_str(), callback, (void*)data, &errMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);

    }else {
        fprintf(stdout, "Operation done successfully\n");
        printf("%d\n", count);
    }
    if(count==1){
        return 1;
    }
    else{
        return 0;
    }
}

