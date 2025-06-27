#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <kernel/lib/console.h>
#include <string.h>
#include <wpa_ctrl.h>
#include <errno.h>
#include <sqlite3.h>

#define DB_NAME "/media/sda1/test.db"

// Error handling function
int handle_error(int rc, sqlite3 *db)
{
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}
	return 0;
}

// Debug output function
static void debug_callback(void *NotUsed, int iErrCode, const char *zSql) {
    printf("iErrCode: %d, SQL: %s\n", iErrCode, zSql);  // Print each executed SQL statement
}
static int sqlite_init = 0;
static int sqlite3_test_write(int argc, char **argv)
{
	sqlite3 *db;
	char *errMsg = 0;
	int rc;
	if (argc != 2) {
		printf("Usage: %s <db_path>\n", argv[0]);
		return 1;
	}

	if (sqlite_init == 0) {
		rc = sqlite3_config(SQLITE_CONFIG_LOG, debug_callback, 0);
		if (rc != SQLITE_OK) {
			printf("Failed to configure SQLite logging\n");
		}
		sqlite_init = 1;
	}
	// Open or create database
	rc = sqlite3_open(argv[1], &db);
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return 1;
	} else {
		printf("Opened database successfully\n");
	}

	// Create table
	const char *create_table_sql =
		"CREATE TABLE IF NOT EXISTS TEST (ID INTEGER PRIMARY KEY AUTOINCREMENT, NAME TEXT NOT NULL);";
	rc = sqlite3_exec(db, create_table_sql, 0, 0, &errMsg);
	if (handle_error(rc, db))
		return 1;
	printf("Table created successfully\n");

	// Insert data
	const char *insert_sql = "INSERT INTO TEST (NAME) VALUES ('Alice'), ('Bob'), ('Charlie');";
	rc = sqlite3_exec(db, insert_sql, 0, 0, &errMsg);
	if (handle_error(rc, db))
		return 1;
	printf("Data inserted successfully\n");

	// Query and print data
	const char *select_sql = "SELECT * FROM TEST;";
	sqlite3_stmt *stmt;
	rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
	if (handle_error(rc, db))
		return 1;

	printf("ID | Name\n");
	printf("------------\n");
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		const char *name = (const char *)sqlite3_column_text(stmt, 1);
		printf("%d | %s\n", id, name);
	}

	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
	}

	sqlite3_finalize(stmt);

	// Update data
	const char *update_sql = "UPDATE TEST SET NAME = 'David' WHERE NAME = 'Bob';";
	rc = sqlite3_exec(db, update_sql, 0, 0, &errMsg);
	if (handle_error(rc, db))
		return 1;
	printf("Data updated successfully\n");

	// Query and print updated data
	rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
	if (handle_error(rc, db))
		return 1;

	printf("ID | Name\n");
	printf("------------\n");
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		const char *name = (const char *)sqlite3_column_text(stmt, 1);
		printf("%d | %s\n", id, name);
	}

	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
	}

	sqlite3_finalize(stmt);

	// Delete data
	const char *delete_sql = "DELETE FROM TEST WHERE NAME = 'Charlie';";
	rc = sqlite3_exec(db, delete_sql, 0, 0, &errMsg);
	if (handle_error(rc, db))
		return 1;
	printf("Data deleted successfully\n");

	// Query and print final data
	rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
	if (handle_error(rc, db))
		return 1;

	printf("ID | Name\n");
	printf("------------\n");
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		const char *name = (const char *)sqlite3_column_text(stmt, 1);
		printf("%d | %s\n", id, name);
	}

	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
	}

	sqlite3_finalize(stmt);

	// Close database
	sqlite3_close(db);
	printf("Database closed successfully\n");

	return 0;
}


// Callback function to print query results
static int print_table_data(void *data, int argc, char **argv, char **azColName) {
    // Print each column name and value
    for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");  // Print a blank line to separate each record
    return 0;  // Continue processing
}

// Print all contents of the specified table
void print_table_contents(sqlite3 *db, const char *table_name) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT * FROM %s;", table_name);  // Construct SQL query
    
    printf("Contents of table '%s':\n", table_name);
    int rc = sqlite3_exec(db, sql, print_table_data, 0, NULL);
    if (rc != SQLITE_OK) {
        // If execution fails, print error message
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    }
    printf("\n");  // Print a blank line to separate contents of different tables
}

static int print_table_name(void *data, int argc, char **argv, char **azColName)
{
    // Callback function to print each table name
    for (int i = 0; i < argc; i++) {
        printf("Table: %s\n", argv[i]);
        // Print contents of each table
        print_table_contents((sqlite3 *)data, argv[i]);
    }
    return 0;
}

int sqlite3_test_read(int argc, char **argv)
{
    sqlite3 *db;
    char *errMsg = 0;
    int rc;
	if (argc != 2) {
		printf("Usage: %s <db_path>\n", argv[0]);
		return 1;
	}

	if (sqlite_init == 0) {
		rc = sqlite3_config(SQLITE_CONFIG_LOG, debug_callback, 0);
		if (rc != SQLITE_OK) {
			printf("Failed to configure SQLite logging\n");
		}
		sqlite_init = 1;
	}
	
    // Open database
    rc = sqlite3_open(argv[1], &db);
    if (rc) {
        // If opening database fails, print error message
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    } else {
        printf("Opened database %s successfully\n", DB_NAME);
    }

    // Query sqlite_master table to get names of all tables
    const char *sql = "SELECT name FROM sqlite_master WHERE type='table';";

    rc = sqlite3_exec(db, sql, print_table_name, (void *)db, &errMsg);

    if (rc != SQLITE_OK) {
        // If query execution fails, print error message
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);  // Free error message
        sqlite3_close(db);  // Close database
        return 1;
    }

    // Close database
    sqlite3_close(db);
    return 0;
}

static int sqlite3_test_entry(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	return 0;
}


CONSOLE_CMD(sqlite3, NULL, sqlite3_test_entry, CONSOLE_CMD_MODE_SELF, "enter sqlite3 test utilities")
CONSOLE_CMD(write, "sqlite3", sqlite3_test_write, CONSOLE_CMD_MODE_SELF, "sqlite3 test write")
CONSOLE_CMD(read, "sqlite3", sqlite3_test_read, CONSOLE_CMD_MODE_SELF, "sqlite3 test read")
