#include "event_bus.h"
#include "log.h"

#include <dirent.h>
#include <pthread.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

static sqlite3      *g_db = NULL;
static char          g_snap_dir[256] = {0};
static long          g_min_free_mb = 0;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

#define MAX_SUBS 8
static event_cb_t g_subs_cb[MAX_SUBS];
static void      *g_subs_user[MAX_SUBS];
static int        g_sub_count = 0;

void event_bus_subscribe(event_cb_t cb, void *user)
{
    pthread_mutex_lock(&g_lock);
    if (g_sub_count < MAX_SUBS) {
        g_subs_cb[g_sub_count] = cb;
        g_subs_user[g_sub_count] = user;
        g_sub_count++;
    }
    pthread_mutex_unlock(&g_lock);
}

/* 删最旧 evt_*.jpg 直到剩余空间足够。复用 recorder cleanup_disk 套路 */
static void cleanup_snapshots(void)
{
    if (g_min_free_mb <= 0 || g_snap_dir[0] == 0) return;
    struct statvfs vfs;
    if (statvfs(g_snap_dir, &vfs) != 0) return;
    unsigned long free_mb = (vfs.f_bavail * (unsigned long)vfs.f_frsize) / (1024 * 1024);
    if ((long)free_mb >= g_min_free_mb) return;

    LOG_WRN("快照盘剩余 %luMB < %ldMB，清理最旧快照", free_mb, g_min_free_mb);
    struct dirent **namelist = NULL;
    int n = scandir(g_snap_dir, &namelist, NULL, alphasort);
    if (n < 0) return;
    for (int i = 0; i < n; i++) {
        const char *name = namelist[i]->d_name;
        if (name[0] == '.') { continue; }
        size_t l = strlen(name);
        if (l < 5 || strcmp(name + l - 4, ".jpg")) { continue; }
        if (statvfs(g_snap_dir, &vfs) != 0) break;
        if ((long)((vfs.f_bavail * (unsigned long)vfs.f_frsize) / (1024 * 1024)) >= g_min_free_mb) break;
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", g_snap_dir, name);
        if (unlink(path) == 0) LOG_INF("删除旧快照: %s", name);
    }
    for (int i = 0; i < n; i++) free(namelist[i]);
    free(namelist);
}

int event_bus_init(const char *db_path, const char *snapshot_dir, long min_free_mb)
{
    pthread_mutex_lock(&g_lock);
    g_min_free_mb = min_free_mb;
    if (snapshot_dir) {
        strncpy(g_snap_dir, snapshot_dir, sizeof(g_snap_dir) - 1);
        mkdir(g_snap_dir, 0755);
    }
    if (sqlite3_open(db_path, &g_db) != SQLITE_OK) {
        LOG_ERR("打开 SQLite 失败: %s (%s)", db_path, sqlite3_errmsg(g_db));
        sqlite3_close(g_db);
        g_db = NULL;
        pthread_mutex_unlock(&g_lock);
        return -1;
    }
    const char *sql =
        "CREATE TABLE IF NOT EXISTS events ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  created_at INTEGER NOT NULL,"
        "  event_type TEXT,"
        "  confidence REAL,"
        "  description TEXT,"
        "  suggested_action TEXT,"
        "  jpeg_path TEXT);"
        "CREATE INDEX IF NOT EXISTS idx_events_ts ON events(created_at DESC);";
    char *err = NULL;
    if (sqlite3_exec(g_db, sql, NULL, NULL, &err) != SQLITE_OK) {
        LOG_ERR("建表失败: %s", err ? err : "?");
        sqlite3_free(err);
        pthread_mutex_unlock(&g_lock);
        return -1;
    }
    pthread_mutex_unlock(&g_lock);
    LOG_INF("event_bus 就绪: db=%s snapshots=%s", db_path, g_snap_dir);
    return 0;
}

long event_bus_publish(event_t *ev, const uint8_t *jpeg, int jpeg_size)
{
    if (!g_db || !ev) return -1;
    ev->id = -1;
    ev->created_at = (int64_t)time(NULL);
    if (ev->jpeg_path[0] == 0) ev->jpeg_path[0] = 0;

    pthread_mutex_lock(&g_lock);
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT INTO events(created_at,event_type,confidence,description,suggested_action,jpeg_path)"
        " VALUES(?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERR("INSERT prepare 失败: %s", sqlite3_errmsg(g_db));
        pthread_mutex_unlock(&g_lock);
        return -1;
    }
    sqlite3_bind_int64(stmt, 1, ev->created_at);
    sqlite3_bind_text(stmt, 2, ev->event_type, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, (double)ev->confidence);
    sqlite3_bind_text(stmt, 4, ev->description, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, ev->suggested_action, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, ev->jpeg_path, -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        LOG_ERR("INSERT 失败: %s", sqlite3_errmsg(g_db));
        pthread_mutex_unlock(&g_lock);
        return -1;
    }
    long id = (long)sqlite3_last_insert_rowid(g_db);
    ev->id = id;

    /* 存快照（id 已知后命名） */
    if (jpeg && jpeg_size > 0 && g_snap_dir[0]) {
        char path[512];
        snprintf(path, sizeof(path), "%s/evt_%ld.jpg", g_snap_dir, id);
        FILE *fp = fopen(path, "wb");
        if (fp) {
            if (fwrite(jpeg, 1, jpeg_size, fp) == (size_t)jpeg_size) {
                strncpy(ev->jpeg_path, path, sizeof(ev->jpeg_path) - 1);
                ev->jpeg_path[sizeof(ev->jpeg_path) - 1] = 0;
                /* 回填 jpeg_path */
                sqlite3_stmt *u = NULL;
                if (sqlite3_prepare_v2(g_db, "UPDATE events SET jpeg_path=? WHERE id=?;", -1, &u, NULL) == SQLITE_OK) {
                    sqlite3_bind_text(u, 1, ev->jpeg_path, -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int64(u, 2, id);
                    sqlite3_step(u);
                    sqlite3_finalize(u);
                }
            } else {
                LOG_WRN("快照写入不完整: %s", path);
            }
            fclose(fp);
        }
        cleanup_snapshots();
    }

    /* 通知订阅者（在锁内回调要求回调不阻塞、不再调 event_bus_*） */
    event_cb_t cbs[MAX_SUBS];
    void      *users[MAX_SUBS];
    int nsub = g_sub_count;
    for (int i = 0; i < nsub; i++) { cbs[i] = g_subs_cb[i]; users[i] = g_subs_user[i]; }
    pthread_mutex_unlock(&g_lock);

    for (int i = 0; i < nsub; i++) {
        if (cbs[i]) cbs[i](ev, users[i]);
    }
    return id;
}

static void row_to_event(sqlite3_stmt *stmt, event_t *e)
{
    memset(e, 0, sizeof(*e));
    e->id = (long)sqlite3_column_int64(stmt, 0);
    e->created_at = sqlite3_column_int64(stmt, 1);
    const char *s;
    if ((s = (const char *)sqlite3_column_text(stmt, 2))) strncpy(e->event_type, s, sizeof(e->event_type) - 1);
    e->confidence = (float)sqlite3_column_double(stmt, 3);
    if ((s = (const char *)sqlite3_column_text(stmt, 4))) strncpy(e->description, s, sizeof(e->description) - 1);
    if ((s = (const char *)sqlite3_column_text(stmt, 5))) strncpy(e->suggested_action, s, sizeof(e->suggested_action) - 1);
    if ((s = (const char *)sqlite3_column_text(stmt, 6))) strncpy(e->jpeg_path, s, sizeof(e->jpeg_path) - 1);
}

int event_bus_query(event_t *out, int max_count)
{
    return event_bus_query_filtered(out, max_count, NULL, 0);
}

long event_bus_count(const char *type_filter)
{
    if (!g_db) return 0;
    long n = 0;
    pthread_mutex_lock(&g_lock);
    sqlite3_stmt *stmt = NULL;
    if (type_filter && type_filter[0]) {
        if (sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM events WHERE event_type=?;", -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, type_filter, -1, SQLITE_TRANSIENT);
        }
    } else {
        sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM events;", -1, &stmt, NULL);
    }
    if (stmt && sqlite3_step(stmt) == SQLITE_ROW) {
        n = (long)sqlite3_column_int64(stmt, 0);
    }
    if (stmt) sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_lock);
    return n;
}

int event_bus_query_filtered(event_t *out, int max_count, const char *type_filter, int offset)
{
    if (!g_db || max_count <= 0) return 0;
    pthread_mutex_lock(&g_lock);
    sqlite3_stmt *stmt = NULL;
    const char *sql = NULL;
    if (type_filter && type_filter[0]) {
        sql = "SELECT id,created_at,event_type,confidence,description,suggested_action,jpeg_path"
              " FROM events WHERE event_type=? ORDER BY created_at DESC,id DESC LIMIT ? OFFSET ?;";
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            pthread_mutex_unlock(&g_lock);
            return 0;
        }
        sqlite3_bind_text(stmt, 1, type_filter, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, max_count);
        sqlite3_bind_int(stmt, 3, offset);
    } else {
        sql = "SELECT id,created_at,event_type,confidence,description,suggested_action,jpeg_path"
              " FROM events ORDER BY created_at DESC,id DESC LIMIT ? OFFSET ?;";
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            pthread_mutex_unlock(&g_lock);
            return 0;
        }
        sqlite3_bind_int(stmt, 1, max_count);
        sqlite3_bind_int(stmt, 2, offset);
    }
    int n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && n < max_count) {
        row_to_event(stmt, &out[n]);
        n++;
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_lock);
    return n;
}

int event_bus_get(long id, event_t *out)
{
    if (!g_db || !out) return -1;
    pthread_mutex_lock(&g_lock);
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_db, "SELECT id,created_at,event_type,confidence,description,suggested_action,jpeg_path"
                                 " FROM events WHERE id=?;", -1, &stmt, NULL) != SQLITE_OK) {
        pthread_mutex_unlock(&g_lock);
        return -1;
    }
    sqlite3_bind_int64(stmt, 1, id);
    int rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        row_to_event(stmt, out);
        rc = 0;
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_lock);
    return rc;
}

void event_bus_deinit(void)
{
    pthread_mutex_lock(&g_lock);
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
    g_sub_count = 0;
    pthread_mutex_unlock(&g_lock);
    LOG_INF("event_bus 已关闭");
}
