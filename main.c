#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <concord/discord.h>

#define DB_NAME "attendance.db"

// Bot configuration
const char *BOT_TOKEN = "#"; // Replace with your bot token
const char *COMMAND_PREFIX = "!"; // Command prefix
const char *OWNER_ID = "#"; // Replace with your Discord user ID
const char *GUILD_ID = "#"; // Replace with your server (guild) ID

// Function to open and set up the database if it doesn't exist
void setupDatabase() {
    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    rc = sqlite3_open(DB_NAME, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    // Create attendance table if it doesn't exist
    const char *sql = "CREATE TABLE IF NOT EXISTS attendance ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "username TEXT NOT NULL,"
                      "date TEXT NOT NULL);";
    
    rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    sqlite3_close(db);
}

// Function to save attendance to the database
void saveAttendance(const char *username) {
    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    rc = sqlite3_open(DB_NAME, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    // Insert attendance record
    char sql[256];
    snprintf(sql, sizeof(sql), "INSERT INTO attendance (username, date) VALUES ('%s', DATE('now'));", username);
    
    rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    } else {
        printf("Attendance saved for user: %s\n", username);
    }

    sqlite3_close(db);
}

// Function to load and display attendance records from the database
void showAttendance(struct discord *client, const struct discord_message *msg) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_open(DB_NAME, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    const char *sql = "SELECT username, date FROM attendance WHERE strftime('%Y-%m', date) = strftime('%Y-%m', 'now');";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    char response[1024] = 
    " Attendance History\n"
    "=======================\n"
    "|**|Username |-| Date |**|\n"
    "=======================\n"
    ;
    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *username = (const char *)sqlite3_column_text(stmt, 0);
            const char *date = (const char *)sqlite3_column_text(stmt, 1);
            
            strcat(response,"|{ ");
            strcat(response, username);
            strcat(response, " } - { ");
            strcat(response, date);
            strcat(response, " }|");
            strcat(response, "\n");
            strcat(response, "=======================\n");

        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // Use discord_create_message with the required callback and return handling
    discord_create_message(client, msg->channel_id, &(struct discord_create_message){
        .content = response
    }, NULL); // NULL as the callback
}

// Callback function when a message is created
void on_message(struct discord *client, const struct discord_message *msg) {
    if (msg->author->bot) return; // Ignore bot messages

    // Use buffers to concatenate the command prefix with the actual command strings
    char absen_command[64];
    char riwayat_command[64];

    // Concatenate command prefix with "absen" and "riwayat"
    snprintf(absen_command, sizeof(absen_command), "%s%s", COMMAND_PREFIX, "absen");
    snprintf(riwayat_command, sizeof(riwayat_command), "%s%s", COMMAND_PREFIX, "riwayat");

    // Now use the full command string in strstr
    if (strstr(msg->content, absen_command) != NULL) {
        saveAttendance(msg->author->username);
        discord_create_message(client, msg->channel_id, &(struct discord_create_message){
            .content = "Attendance recorded!"
        }, NULL); // NULL as the callback
    } else if (strstr(msg->content, riwayat_command) != NULL) {
        showAttendance(client, msg);
    }
}


int main() {
    // Set up the SQLite database
    setupDatabase();

    // Create and configure the Discord client
    struct discord *client = discord_init(BOT_TOKEN);
    discord_set_on_message_create(client, &on_message);

    // Start the Discord bot
    discord_run(client);

    // Cleanup
    discord_cleanup(client);
    return 0;
}
