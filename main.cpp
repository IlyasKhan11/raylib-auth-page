#include "raylib.h"
#include "sqlite3.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_INPUT_LENGTH 50

typedef enum {
    PAGE_SIGNIN,
    PAGE_SIGNUP
} PageType;

typedef struct {
    char text[MAX_INPUT_LENGTH];
    int length;
    Rectangle bounds;
    bool isActive;
} InputField;

void InitInputField(InputField* field, Rectangle bounds) {
    field->text[0] = '\0';
    field->length = 0;
    field->bounds = bounds;
    field->isActive = false;
}

void UpdateInputField(InputField* field) {
    if (CheckCollisionPointRec(GetMousePosition(), field->bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        field->isActive = true;
    } else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        field->isActive = false;
    }

    if (field->isActive) {
        int key = GetCharPressed();
        while (key > 0) {
            if (field->length < MAX_INPUT_LENGTH - 1) {
                field->text[field->length] = (char)key;
                field->text[field->length + 1] = '\0';
                field->length++;
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && field->length > 0) {
            field->length--;
            field->text[field->length] = '\0';
        }
    }
}

bool ValidateCredentials(sqlite3* db, const char* username, const char* password) {
    sqlite3_stmt* stmt;
    const char* selectSQL = "SELECT password FROM users WHERE username = ?;";
    int rc = sqlite3_prepare_v2(db, selectSQL, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* storedPassword = (const char*)sqlite3_column_text(stmt, 0);
        bool matches = strcmp(password, storedPassword) == 0;
        sqlite3_finalize(stmt);
        return matches;
    }
    
    sqlite3_finalize(stmt);
    return false;
}

int main() {
    // Initialize window
    InitWindow(800, 450, "Login System");
    SetTargetFPS(60);

    // Initialize SQLite
    sqlite3* db;
    int rc = sqlite3_open("users.db", &db);
    if (rc) {
        printf("Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    // Create users table
    const char* createTableSQL = "CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY, password TEXT);";
    char* errMsg = 0;
    rc = sqlite3_exec(db, createTableSQL, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        printf("SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    // Initialize input fields
    InputField usernameField;
    InputField passwordField;
    InitInputField(&usernameField, (Rectangle){250, 140, 300, 40});
    InitInputField(&passwordField, (Rectangle){250, 200, 300, 40});

    PageType currentPage = PAGE_SIGNIN;
    bool actionSuccess = false;
    bool actionError = false;
    const char* message = NULL;
    bool shouldClose = false;

    while (!WindowShouldClose() && !shouldClose) {
        // Handle tab switching
        if (IsKeyPressed(KEY_TAB)) {
            currentPage = (currentPage == PAGE_SIGNIN) ? PAGE_SIGNUP : PAGE_SIGNIN;
            // Clear fields and messages when switching
            usernameField.text[0] = '\0';
            usernameField.length = 0;
            passwordField.text[0] = '\0';
            passwordField.length = 0;
            message = NULL;
            actionSuccess = false;
            actionError = false;
        }

        // Update input fields
        UpdateInputField(&usernameField);
        UpdateInputField(&passwordField);

        // Handle action button (signin/signup)
        Rectangle actionButton = {300, 280, 200, 40};
        Vector2 mousePoint = GetMousePosition();
        
        if (CheckCollisionPointRec(mousePoint, actionButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (strlen(usernameField.text) > 0 && strlen(passwordField.text) > 0) {
                if (currentPage == PAGE_SIGNUP) {
                    // Handle signup
                    sqlite3_stmt* stmt;
                    const char* insertSQL = "INSERT INTO users (username, password) VALUES (?, ?);";
                    rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, NULL);
                    
                    if (rc == SQLITE_OK) {
                        sqlite3_bind_text(stmt, 1, usernameField.text, -1, SQLITE_STATIC);
                        sqlite3_bind_text(stmt, 2, passwordField.text, -1, SQLITE_STATIC);
                        
                        rc = sqlite3_step(stmt);
                        if (rc == SQLITE_DONE) {
                            actionSuccess = true;
                            message = "Signup successful!";
                            // Clear input fields
                            usernameField.text[0] = '\0';
                            usernameField.length = 0;
                            passwordField.text[0] = '\0';
                            passwordField.length = 0;
                        } else {
                            actionError = true;
                            message = "Username already exists!";
                        }
                        sqlite3_finalize(stmt);
                    }
                } else {
                    // Handle signin
                    if (ValidateCredentials(db, usernameField.text, passwordField.text)) {
                        actionSuccess = true;
                        message = "Login successful!";
                        shouldClose = true;  // Close the window on successful login
                    } else {
                        actionError = true;
                        message = "Invalid username or password!";
                    }
                }
            } else {
                actionError = true;
                message = "Please fill all fields!";
            }
        }

        // Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw title
        const char* title = (currentPage == PAGE_SIGNIN) ? "Sign In" : "Sign Up";
        DrawText(title, 350, 50, 30, DARKGRAY);

        // Draw tab indicators
        DrawRectangle(250, 90, 150, 30, (currentPage == PAGE_SIGNIN) ? BLUE : LIGHTGRAY);
        DrawRectangle(400, 90, 150, 30, (currentPage == PAGE_SIGNUP) ? BLUE : LIGHTGRAY);
        DrawText("Sign In", 290, 95, 20, (currentPage == PAGE_SIGNIN) ? WHITE : DARKGRAY);
        DrawText("Sign Up", 440, 95, 20, (currentPage == PAGE_SIGNUP) ? WHITE : DARKGRAY);

        // Draw input fields
        DrawRectangleRec(usernameField.bounds, LIGHTGRAY);
        DrawRectangleRec(passwordField.bounds, LIGHTGRAY);
        DrawText("Username:", 250, 120, 20, DARKGRAY);
        DrawText("Password:", 250, 180, 20, DARKGRAY);
        
        // Draw input field text
        DrawText(usernameField.text, usernameField.bounds.x + 5, usernameField.bounds.y + 10, 20, DARKGRAY);
        // Draw password as asterisks
        char passwordDisplay[MAX_INPUT_LENGTH];
        memset(passwordDisplay, '*', strlen(passwordField.text));
        passwordDisplay[strlen(passwordField.text)] = '\0';
        DrawText(passwordDisplay, passwordField.bounds.x + 5, passwordField.bounds.y + 10, 20, DARKGRAY);

        // Draw active field indicator
        if (usernameField.isActive) DrawRectangleLinesEx(usernameField.bounds, 2, BLUE);
        if (passwordField.isActive) DrawRectangleLinesEx(passwordField.bounds, 2, BLUE);

        // Draw action button
        DrawRectangleRec(actionButton, CheckCollisionPointRec(mousePoint, actionButton) ? DARKBLUE : BLUE);
        DrawText(title, actionButton.x + 60, actionButton.y + 10, 20, WHITE);

        // Draw message
        if (message) {
            DrawText(message, 250, 350, 20, actionSuccess ? GREEN : RED);
        }

        // Draw tab instruction
        DrawText("Press TAB to switch between Sign In and Sign Up", 250, 400, 20, DARKGRAY);

        EndDrawing();
    }

    // Cleanup
    sqlite3_close(db);
    CloseWindow();

    return 0;
}