#include <SDL3/SDL.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "database.h"
#include "helpers.h"
#include "model.h"

static sqlite3* handle;
static sqlite3_stmt* set_state_stmt;
static sqlite3_stmt* get_state_stmt;
static sqlite3_stmt* set_model_stmt;
static sqlite3_stmt* get_models_stmt;

bool database_init(
    const char* path)
{
    assert(path);
    if (sqlite3_open(path, &handle) != SQLITE_OK)
    {
        SDL_Log("Failed to open database: %s, %s", path, sqlite3_errmsg(handle));
        return false;
    }
    const char* create =
        "CREATE TABLE IF NOT EXISTS states ("
        "    id INTEGER PRIMARY KEY,"
        "    model INTEGER NOT NULL,"
        "    x REAL NOT NULL,"
        "    z REAL NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS models ("
        "    model INTEGER NOT NULL,"
        "    x INTEGER NOT NULL,"
        "    z INTEGER NOT NULL,"
        "    PRIMARY KEY (x, z)"
        ");"
        "CREATE INDEX IF NOT EXISTS states_index ON states (id);";
        "CREATE INDEX IF NOT EXISTS models_index ON models (x, z);";
    if (sqlite3_exec(handle, create, 0, NULL, NULL) != SQLITE_OK)
    {
        SDL_Log("Failed to execute SQL: %s, %s", path, sqlite3_errmsg(handle));
        database_free();
        return false;
    }
    const char* set_state = 
        "INSERT OR REPLACE INTO states (id, model, x, z) VALUES (?, ?, ?, ?);";
    const char* get_state = 
        "SELECT model, x, z FROM states WHERE id = ?;";
    const char* set_model = 
        "INSERT OR REPLACE INTO models (model, x, z) VALUES (?, ?, ?);";
    const char* get_models = 
        "SELECT model, x, z FROM models WHERE x BETWEEN ? AND ? AND z BETWEEN ? AND ?;";
    if (sqlite3_prepare_v2(handle, set_state, -1, &set_state_stmt, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to prepare set state: %s", sqlite3_errmsg(handle));
        database_free();
        return false;
    }
    if (sqlite3_prepare_v2(handle, get_state, -1, &get_state_stmt, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to prepare get state: %s", sqlite3_errmsg(handle));
        database_free();
        return false;
    }
    if (sqlite3_prepare_v2(handle, set_model, -1, &set_model_stmt, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to prepare set model: %s", sqlite3_errmsg(handle));
        database_free();
        return false;
    }
    if (sqlite3_prepare_v2(handle, get_models, -1, &get_models_stmt, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to prepare get models: %s", sqlite3_errmsg(handle));
        database_free();
        return false;
    }
    if (sqlite3_exec(handle, "BEGIN;", 0, 0, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to begin transaction: %s", sqlite3_errmsg(handle));
        database_free();
        return false;
    }
    return true;
}

void database_free()
{
    sqlite3_exec(handle, "COMMIT;", 0, 0, 0);
    sqlite3_finalize(set_state_stmt);
    sqlite3_finalize(get_state_stmt);
    sqlite3_finalize(set_model_stmt);
    sqlite3_finalize(get_models_stmt);
    sqlite3_free(handle);
    set_state_stmt = NULL;
    get_state_stmt = NULL;
    set_model_stmt = NULL;
    get_models_stmt = NULL;
    handle = NULL;
}

bool database_commit()
{
    if (sqlite3_exec(handle, "COMMIT; BEGIN;", 0, 0, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to end transaction: %s", sqlite3_errmsg(handle));
        return false;
    }
    return true;
}

void database_set_state(
    const model_t model,
    const float x,
    const float z)
{
    sqlite3_bind_int(set_state_stmt, 1, 0);
    sqlite3_bind_int(set_state_stmt, 2, model);
    sqlite3_bind_double(set_state_stmt, 3, x);
    sqlite3_bind_double(set_state_stmt, 4, z);
    if (sqlite3_step(set_state_stmt) != SQLITE_DONE)
    {
        SDL_Log("Failed to set state: %s", sqlite3_errmsg(handle));
    }
    sqlite3_reset(set_state_stmt);
}

void database_get_state(
    model_t* model,
    float* x,
    float* z)
{
    assert(model);
    assert(x);
    assert(z);
    sqlite3_bind_int(get_state_stmt, 1, 0);
    if (sqlite3_step(get_state_stmt) == SQLITE_ROW)
    {
        *model = sqlite3_column_int(get_state_stmt, 0); 
        *x = sqlite3_column_double(get_state_stmt, 1); 
        *z = sqlite3_column_double(get_state_stmt, 2); 
    }
    else
    {
        *model = 0;
        *x = 0.0f;
        *z = 0.0f;
    }
    sqlite3_reset(get_state_stmt);
}

void database_set_model(
    const model_t model,
    const int x,
    const int z)
{
    sqlite3_bind_int(set_model_stmt, 1, model);
    sqlite3_bind_int(set_model_stmt, 2, x);
    sqlite3_bind_int(set_model_stmt, 3, z);
    if (sqlite3_step(set_model_stmt) != SQLITE_DONE)
    {
        SDL_Log("Failed to set model: %s", sqlite3_errmsg(handle));
    }
    sqlite3_reset(set_model_stmt);
}

void database_get_models(
    const database_get_models_func_t func,
    const int x1,
    const int z1,
    const int x2,
    const int z2)
{
    assert(func);
    sqlite3_bind_int(get_models_stmt, 1, x1);
    sqlite3_bind_int(get_models_stmt, 2, x2);
    sqlite3_bind_int(get_models_stmt, 3, z1);
    sqlite3_bind_int(get_models_stmt, 4, z2);
    while (sqlite3_step(get_models_stmt) == SQLITE_ROW)
    {
        const model_t model = sqlite3_column_int(get_models_stmt, 0);
        const int x = sqlite3_column_int(get_models_stmt, 1);
        const int z = sqlite3_column_int(get_models_stmt, 2);
        func(model, x, z);
    }
    sqlite3_reset(get_models_stmt);
}