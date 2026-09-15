-- ============================================================
-- LocalLife Platform 本地生活商户评价与消费服务平台
-- 数据库建表脚本（SQLite，兼容 MySQL 语义迁移）
-- 规范：所有表使用 IF NOT EXISTS 保证幂等；初始数据用 INSERT OR IGNORE
-- ============================================================

PRAGMA foreign_keys = ON;

-- 1. 用户表（三角色 RBAC）
CREATE TABLE IF NOT EXISTS users (
  id            INTEGER PRIMARY KEY AUTOINCREMENT,
  username      TEXT    NOT NULL UNIQUE,
  password_hash TEXT    NOT NULL,
  salt          TEXT    NOT NULL,
  role          TEXT    NOT NULL DEFAULT 'consumer'
                CHECK (role IN ('consumer', 'merchant', 'admin')),
  nickname      TEXT    DEFAULT '',
  phone         TEXT    DEFAULT '',
  avatar        TEXT    DEFAULT '',
  status        TEXT    NOT NULL DEFAULT 'active'
                CHECK (status IN ('active', 'disabled')),
  created_at    TEXT    NOT NULL
);

-- 2. 用户消费偏好
CREATE TABLE IF NOT EXISTS user_preferences (
  user_id           INTEGER PRIMARY KEY REFERENCES users (id),
  prefer_categories TEXT DEFAULT '',   -- 逗号分隔的类别 id
  price_range       TEXT DEFAULT '',   -- 如 '0-50' / '50-100' / '100+'
  area              TEXT DEFAULT ''
);

-- 3. 登录会话表（Token 会话管理）
CREATE TABLE IF NOT EXISTS sessions (
  token      TEXT PRIMARY KEY,
  user_id    INTEGER NOT NULL REFERENCES users (id),
  created_at TEXT    NOT NULL,
  expires_at TEXT    NOT NULL
);

-- 4. 商户类别
CREATE TABLE IF NOT EXISTS categories (
  id        INTEGER PRIMARY KEY AUTOINCREMENT,
  name      TEXT NOT NULL UNIQUE,
  parent_id INTEGER DEFAULT NULL,
  sort      INTEGER DEFAULT 0
);

-- 5. 商户表（入驻申请 → 平台审核）
CREATE TABLE IF NOT EXISTS merchants (
  id             INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id        INTEGER NOT NULL REFERENCES users (id),
  name           TEXT    NOT NULL,
  category_id    INTEGER REFERENCES categories (id),
  area           TEXT    DEFAULT '',
  business_hours TEXT    DEFAULT '',
  phone          TEXT    DEFAULT '',
  intro          TEXT    DEFAULT '',
  price_min      REAL    DEFAULT 0,
  price_max      REAL    DEFAULT 0,
  logo           TEXT    DEFAULT '',
  images         TEXT    DEFAULT '',  -- 逗号分隔图片路径
  status         TEXT    NOT NULL DEFAULT 'pending'
                 CHECK (status IN ('pending', 'approved', 'rejected')),
  reject_reason  TEXT    DEFAULT '',
  view_count     INTEGER DEFAULT 0,
  created_at     TEXT    NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_merchants_category ON merchants (category_id);
CREATE INDEX IF NOT EXISTS idx_merchants_area     ON merchants (area);
CREATE INDEX IF NOT EXISTS idx_merchants_status   ON merchants (status);

-- 6. 门店表
CREATE TABLE IF NOT EXISTS stores (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  merchant_id INTEGER NOT NULL REFERENCES merchants (id),
  name        TEXT    NOT NULL,
  address     TEXT    DEFAULT '',
  area        TEXT    DEFAULT '',
  images      TEXT    DEFAULT '',   -- 门店图片（多个用英文逗号分隔）
  status      TEXT    NOT NULL DEFAULT 'open'
              CHECK (status IN ('open', 'rest', 'closed')),
  created_at  TEXT    NOT NULL
);

-- 7. 服务项目表
CREATE TABLE IF NOT EXISTS services (
  id              INTEGER PRIMARY KEY AUTOINCREMENT,
  merchant_id     INTEGER NOT NULL REFERENCES merchants (id),
  store_id        INTEGER REFERENCES stores (id),
  name            TEXT    NOT NULL,
  price           REAL    NOT NULL DEFAULT 0,
  price_unit      TEXT    DEFAULT '',
  applicable_time TEXT    DEFAULT '',
  stock           INTEGER DEFAULT -1,   -- -1 表示不限量
  limit_count     INTEGER DEFAULT 0,    -- 每人限购数量，0 表示不限
  images          TEXT    DEFAULT '',   -- 服务项目图片（多个用英文逗号分隔）
  status          TEXT    NOT NULL DEFAULT 'on'
                  CHECK (status IN ('on', 'off')),
  created_at      TEXT    NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_services_merchant ON services (merchant_id);

-- 8. 消费套餐表
CREATE TABLE IF NOT EXISTS packages (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  merchant_id INTEGER NOT NULL REFERENCES merchants (id),
  name        TEXT    NOT NULL,
  content     TEXT    DEFAULT '',
  price       REAL    NOT NULL DEFAULT 0,
  valid_days  INTEGER DEFAULT 30,
  limit_count INTEGER DEFAULT 0,
  images      TEXT    DEFAULT '',   -- 套餐图片（多个用英文逗号分隔）
  status      TEXT    NOT NULL DEFAULT 'on'
              CHECK (status IN ('on', 'off')),
  created_at  TEXT    NOT NULL
);

-- 9. 评价表（多目标：店铺/门店/服务/套餐 各自独立评论区）
CREATE TABLE IF NOT EXISTS reviews (
  id            INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id       INTEGER NOT NULL REFERENCES users (id),
  merchant_id   INTEGER NOT NULL REFERENCES merchants (id),  -- 归属商户（冗余，供统计/回复/审核）
  target_type   TEXT    NOT NULL DEFAULT 'merchant'
                CHECK (target_type IN ('merchant', 'store', 'service', 'package')),
  target_id     INTEGER NOT NULL DEFAULT 0,                  -- 对应目标对象 id
  env_score     REAL    NOT NULL CHECK (env_score BETWEEN 1 AND 5),
  service_score REAL    NOT NULL CHECK (service_score BETWEEN 1 AND 5),
  price_score   REAL    NOT NULL CHECK (price_score BETWEEN 1 AND 5),
  avg_score     REAL    NOT NULL,
  content       TEXT    DEFAULT '',
  status        TEXT    NOT NULL DEFAULT 'visible'
                CHECK (status IN ('visible', 'hidden', 'deleted')),
  created_at    TEXT    NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_reviews_merchant ON reviews (merchant_id);
CREATE INDEX IF NOT EXISTS idx_reviews_user     ON reviews (user_id);

-- 10. 评价图片
CREATE TABLE IF NOT EXISTS review_images (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  review_id  INTEGER NOT NULL REFERENCES reviews (id),
  image_path TEXT    NOT NULL
);

-- 11. 评价点赞
CREATE TABLE IF NOT EXISTS review_likes (
  review_id  INTEGER NOT NULL REFERENCES reviews (id),
  user_id    INTEGER NOT NULL REFERENCES users (id),
  created_at TEXT    NOT NULL,
  PRIMARY KEY (review_id, user_id)
);

-- 12. 评价评论（parent_id 支持“对评论追加评论”，NULL 为楼顶评论）
CREATE TABLE IF NOT EXISTS review_comments (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  review_id  INTEGER NOT NULL REFERENCES reviews (id),
  user_id    INTEGER NOT NULL REFERENCES users (id),
  parent_id  INTEGER REFERENCES review_comments (id),
  content    TEXT    NOT NULL,
  created_at TEXT    NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_comments_review ON review_comments (review_id);

-- 12a. 评论点赞（每条评论都可被“有用”）
CREATE TABLE IF NOT EXISTS comment_likes (
  comment_id INTEGER NOT NULL REFERENCES review_comments (id),
  user_id    INTEGER NOT NULL REFERENCES users (id),
  created_at TEXT    NOT NULL,
  PRIMARY KEY (comment_id, user_id)
);

-- 12b. 评论举报
CREATE TABLE IF NOT EXISTS comment_reports (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  comment_id INTEGER NOT NULL REFERENCES review_comments (id),
  user_id    INTEGER NOT NULL REFERENCES users (id),
  reason     TEXT    NOT NULL,
  status     TEXT    NOT NULL DEFAULT 'pending'
             CHECK (status IN ('pending', 'resolved', 'rejected')),
  result     TEXT    DEFAULT '',
  created_at TEXT    NOT NULL
);

-- 13. 商户回复
CREATE TABLE IF NOT EXISTS review_replies (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  review_id   INTEGER NOT NULL REFERENCES reviews (id),
  comment_id  INTEGER REFERENCES review_comments (id),
  merchant_id INTEGER NOT NULL REFERENCES merchants (id),
  content     TEXT    NOT NULL,
  created_at  TEXT    NOT NULL
);

-- 14. 评价举报（平台审核）
CREATE TABLE IF NOT EXISTS review_reports (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  review_id  INTEGER NOT NULL REFERENCES reviews (id),
  user_id    INTEGER NOT NULL REFERENCES users (id),
  reason     TEXT    NOT NULL,
  status     TEXT    NOT NULL DEFAULT 'pending'
             CHECK (status IN ('pending', 'resolved', 'rejected')),
  result     TEXT    DEFAULT '',
  created_at TEXT    NOT NULL
);

-- 15. 收藏表（商户/服务）
CREATE TABLE IF NOT EXISTS favorites (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id     INTEGER NOT NULL REFERENCES users (id),
  target_type TEXT    NOT NULL CHECK (target_type IN ('merchant', 'service')),
  target_id   INTEGER NOT NULL,
  created_at  TEXT    NOT NULL,
  UNIQUE (user_id, target_type, target_id)
);

-- 16. 关注商户
CREATE TABLE IF NOT EXISTS follows (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id     INTEGER NOT NULL REFERENCES users (id),
  merchant_id INTEGER NOT NULL REFERENCES merchants (id),
  created_at  TEXT    NOT NULL,
  UNIQUE (user_id, merchant_id)
);

-- 17. 浏览历史
CREATE TABLE IF NOT EXISTS browse_history (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id     INTEGER NOT NULL REFERENCES users (id),
  merchant_id INTEGER NOT NULL REFERENCES merchants (id),
  viewed_at   TEXT    NOT NULL
);

-- 18. 消费记录
CREATE TABLE IF NOT EXISTS consumption_records (
  id           INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id      INTEGER NOT NULL REFERENCES users (id),
  merchant_id  INTEGER NOT NULL REFERENCES merchants (id),
  store_id     INTEGER REFERENCES stores (id),   -- 发生消费的门店
  service_id   INTEGER REFERENCES services (id),
  package_id   INTEGER REFERENCES packages (id),
  amount       REAL    DEFAULT 0,
  consume_time TEXT    NOT NULL,
  created_at   TEXT    NOT NULL
);

-- 19. 优惠活动表（优惠券/满减/折扣/套餐）
CREATE TABLE IF NOT EXISTS coupons (
  id            INTEGER PRIMARY KEY AUTOINCREMENT,
  merchant_id   INTEGER NOT NULL REFERENCES merchants (id),
  type          TEXT    NOT NULL
                CHECK (type IN ('coupon', 'full_reduction', 'discount', 'package')),
  name          TEXT    NOT NULL,
  face_value    REAL    DEFAULT 0,   -- 优惠券面值
  threshold     REAL    DEFAULT 0,   -- 满减门槛
  discount_rate REAL    DEFAULT 0,   -- 折扣率，如 0.8 表示 8 折
  total         INTEGER DEFAULT 0,   -- 发放总量，0 表示不限
  received      INTEGER DEFAULT 0,   -- 已领取
  used          INTEGER DEFAULT 0,   -- 已核销
  start_time    TEXT    NOT NULL,
  end_time      TEXT    NOT NULL,
  scope         TEXT    DEFAULT '',  -- 适用范围说明
  status        TEXT    NOT NULL DEFAULT 'published'
                CHECK (status IN ('draft', 'published', 'offline')),
  created_at    TEXT    NOT NULL
);

-- 20. 优惠券领取/使用记录
CREATE TABLE IF NOT EXISTS coupon_user (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  coupon_id   INTEGER NOT NULL REFERENCES coupons (id),
  user_id     INTEGER NOT NULL REFERENCES users (id),
  code        TEXT    NOT NULL UNIQUE,
  status      TEXT    NOT NULL DEFAULT 'unused'
              CHECK (status IN ('unused', 'used', 'expired')),
  received_at TEXT    NOT NULL,
  used_time   TEXT,
  UNIQUE (coupon_id, user_id)
);

-- 21. 订单（下单主体为「门店」；支持服务项目 / 优惠套餐两类商品）
CREATE TABLE IF NOT EXISTS orders (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  order_no   TEXT    NOT NULL UNIQUE,
  user_id    INTEGER NOT NULL REFERENCES users (id),
  store_id   INTEGER REFERENCES stores (id),      -- 下单门店（消费者在哪个门店消费）
  item_type  TEXT    NOT NULL DEFAULT 'package'
             CHECK (item_type IN ('package', 'service')),
  package_id INTEGER REFERENCES packages (id),    -- item_type=package 时有效
  service_id INTEGER REFERENCES services (id),    -- item_type=service 时有效
  amount     REAL    NOT NULL,
  status     TEXT    NOT NULL DEFAULT 'purchased'
             CHECK (status IN ('purchased', 'used', 'refunded')),
  created_at TEXT    NOT NULL,
  used_time  TEXT
);

-- 21a. 门店经营项目上架关系：门店 × 服务项目（是否运营由门店决定）
CREATE TABLE IF NOT EXISTS store_services (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  store_id   INTEGER NOT NULL REFERENCES stores (id),
  service_id INTEGER NOT NULL REFERENCES services (id),
  status     TEXT    NOT NULL DEFAULT 'on'
             CHECK (status IN ('on', 'off')),
  created_at TEXT    NOT NULL,
  UNIQUE (store_id, service_id)
);

-- 21b. 门店经营项目上架关系：门店 × 优惠套餐
CREATE TABLE IF NOT EXISTS store_packages (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  store_id   INTEGER NOT NULL REFERENCES stores (id),
  package_id INTEGER NOT NULL REFERENCES packages (id),
  status     TEXT    NOT NULL DEFAULT 'on'
             CHECK (status IN ('on', 'off')),
  created_at TEXT    NOT NULL,
  UNIQUE (store_id, package_id)
);

-- 21c. 门店经营项目上架关系：门店 × 优惠活动（门店是否参与该活动）
CREATE TABLE IF NOT EXISTS store_coupons (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  store_id   INTEGER NOT NULL REFERENCES stores (id),
  coupon_id  INTEGER NOT NULL REFERENCES coupons (id),
  status     TEXT    NOT NULL DEFAULT 'on'
             CHECK (status IN ('on', 'off')),
  created_at TEXT    NOT NULL,
  UNIQUE (store_id, coupon_id)
);

CREATE INDEX IF NOT EXISTS idx_store_services_store ON store_services (store_id);
CREATE INDEX IF NOT EXISTS idx_store_packages_store ON store_packages (store_id);
CREATE INDEX IF NOT EXISTS idx_store_coupons_store  ON store_coupons (store_id);

-- 说明：orders / consumption_records 在旧版库中缺少新列，其索引统一由 Migrate.h 建立
-- （schema.sql 先于迁移执行，直接建索引会因“no such column”导致初始化失败）

-- 22. 运营统计缓存表
CREATE TABLE IF NOT EXISTS operation_stats (
  id             INTEGER PRIMARY KEY AUTOINCREMENT,
  merchant_id    INTEGER NOT NULL REFERENCES merchants (id),
  stat_date      TEXT    NOT NULL,
  view_count     INTEGER DEFAULT 0,
  favorite_count INTEGER DEFAULT 0,
  review_count   INTEGER DEFAULT 0,
  avg_score      REAL    DEFAULT 0,
  UNIQUE (merchant_id, stat_date)
);

-- ============================================================
-- 初始数据（商户类别）
-- ============================================================
INSERT OR IGNORE INTO categories (name, sort) VALUES
  ('美食餐饮', 1),
  ('休闲娱乐', 2),
  ('美容美发', 3),
  ('运动健身', 4),
  ('摄影写真', 5),
  ('桌游娱乐', 6),
  ('KTV欢唱', 7);

-- 注意：管理员账号（admin）由程序启动时自动创建（密码需加盐哈希）

