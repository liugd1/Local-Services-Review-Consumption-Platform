#pragma once
// SQLite 数据库封装：线程安全（互斥串行化）、参数绑定防注入、事务 RAII
#include <sqlite3.h>

#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class Database {
public:
    static Database& instance() {
        static Database db;
        return db;
    }

    // 打开数据库（FULLMUTEX 串行模式）；成功返回 true
    bool open(const std::string& path) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (db_) return true;
        return sqlite3_open_v2(path.c_str(), &db_,
                               SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                               nullptr) == SQLITE_OK;
    }

    void close() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    // SELECT -> 行数组 [{col: value, ...}, ...]
    nlohmann::json query(const std::string& sql, const std::vector<std::string>& params = {}) {
        std::lock_guard<std::mutex> lock(mtx_);
        nlohmann::json rows = nlohmann::json::array();
        sqlite3_stmt* st = prepare(sql, params);
        if (!st) return rows;
        while (sqlite3_step(st) == SQLITE_ROW) {
            rows.push_back(rowToJson(st));
        }
        sqlite3_finalize(st);
        return rows;
    }

    // SELECT 单行 -> 对象，无结果返回 nullptr
    nlohmann::json queryOne(const std::string& sql, const std::vector<std::string>& params = {}) {
        std::lock_guard<std::mutex> lock(mtx_);
        nlohmann::json row = nullptr;
        sqlite3_stmt* st = prepare(sql, params);
        if (!st) return nullptr;
        if (sqlite3_step(st) == SQLITE_ROW) {
            row = rowToJson(st);
        }
        sqlite3_finalize(st);
        return row;
    }

    // INSERT / UPDATE / DELETE -> 受影响行数；失败返回 -1
    long execute(const std::string& sql, const std::vector<std::string>& params = {}) {
        std::lock_guard<std::mutex> lock(mtx_);
        return executeNoLock(sql, params);
    }

    long long lastInsertId() {
        std::lock_guard<std::mutex> lock(mtx_);
        return db_ ? sqlite3_last_insert_rowid(db_) : 0;
    }

    std::string lastError() {
        std::lock_guard<std::mutex> lock(mtx_);
        return db_ ? sqlite3_errmsg(db_) : "database not opened";
    }

    // 执行多语句 SQL 脚本（如 schema.sql）
    bool execScript(const std::string& sql) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!db_) return false;
        char* errMsg = nullptr;
        bool ok = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) == SQLITE_OK;
        if (errMsg) sqlite3_free(errMsg);
        return ok;
    }

    // 事务 RAII：构造即 BEGIN（独占锁），commit 后析构释放；异常/忘记提交则自动 ROLLBACK
    class Transaction {
    public:
        explicit Transaction(Database& db) : db_(db) {
            db_.mtx_.lock();
            db_.executeNoLock("BEGIN IMMEDIATE");
        }
        ~Transaction() {
            if (!done_) db_.executeNoLock("ROLLBACK");
            db_.mtx_.unlock();
        }
        void commit() {
            db_.executeNoLock("COMMIT");
            done_ = true;
        }

    private:
        Database& db_;
        bool done_ = false;
    };

private:
    Database() = default;
    ~Database() { close(); }
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    sqlite3_stmt* prepare(const std::string& sql, const std::vector<std::string>& params) {
        if (!db_) return nullptr;
        sqlite3_stmt* st = nullptr;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) {
            return nullptr;
        }
        for (size_t i = 0; i < params.size(); ++i) {
            sqlite3_bind_text(st, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
        }
        return st;
    }

    static nlohmann::json rowToJson(sqlite3_stmt* st) {
        nlohmann::json row = nlohmann::json::object();
        int count = sqlite3_column_count(st);
        for (int i = 0; i < count; ++i) {
            const char* name = sqlite3_column_name(st, i);
            switch (sqlite3_column_type(st, i)) {
                case SQLITE_INTEGER:
                    row[name] = static_cast<long long>(sqlite3_column_int64(st, i));
                    break;
                case SQLITE_FLOAT:
                    row[name] = sqlite3_column_double(st, i);
                    break;
                case SQLITE_TEXT:
                    row[name] = reinterpret_cast<const char*>(sqlite3_column_text(st, i));
                    break;
                default:
                    row[name] = nullptr;
                    break;
            }
        }
        return row;
    }

    long executeNoLock(const std::string& sql, const std::vector<std::string>& params = {}) {
        if (!db_) return -1;
        sqlite3_stmt* st = prepare(sql, params);
        if (!st) return -1;
        int rc = sqlite3_step(st);
        sqlite3_finalize(st);
        return rc == SQLITE_DONE || rc == SQLITE_ROW ? sqlite3_changes(db_) : -1;
    }

    std::mutex mtx_;
    sqlite3* db_ = nullptr;
};
