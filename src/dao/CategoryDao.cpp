#include "dao/CategoryDao.h"

#include "db/Database.h"

namespace CategoryDao {

nlohmann::json listAll() {
    return Database::instance().query(
        "SELECT id, name, parent_id, sort FROM categories ORDER BY sort, id");
}

}  // namespace CategoryDao
