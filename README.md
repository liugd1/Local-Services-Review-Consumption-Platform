# 巷味 · 本地生活商户评价与消费服务平台

> **C++17 RESTful 后端 + 原生单页前端** 的本地生活服务（餐饮 / 休闲娱乐 / 美容美发 / 运动健身 / 摄影 / 桌游 / KTV）评价与消费平台，
> 面向 **普通消费者 · 商户经营者 · 平台管理员** 三类角色。
>
> 更新日期：2026-09-08

---

## 1. 项目简介

平台把"逛店 → 看口碑 → 领券 → 下单 → 到店核销 → 记录消费 → 评价互动"串成完整闭环：

- **消费者**：组合检索与榜单、商户详情、公开/透明**对象化评价**（店铺 / 门店 / 服务 / 套餐各自独立评论区）、
  任意层评论回复、点赞举报、收藏关注、浏览足迹、领取优惠券、购买套餐并到店核销、消费账单与个人中心。
- **商户经营者**：提交入驻申请 → 平台审核；维护门店 / 服务项目 / 套餐（上下架、限购、库存）；
  发布优惠活动（券 / 满减 / 折扣）并核销；回复顾客评价；查看**经营统计看板**。
- **平台管理员**：商户审核与驳回、举报中心裁决（评价下架 / 评论删除）、用户禁用、
  以及**平台运营数据看板**（用户 / 商户 / 评价 / 订单 / 消费 / 券运营 / 类别分布 / 近 7 日趋势）。

工程特点：零重量级框架依赖，`third_party/` 随仓内置（cpp-httplib、nlohmann/json、SQLite3、PicoSHA2），
**一条 CMake 命令即可构建，单个 exe 启动即用**（首次启动自动建表 + 初始化分类 + 创建管理员）。

---

## 2. 能力总览

| # | 模块 | 能力 | 状态 |
| --- | --- | --- | --- |
| 1 | 用户与权限 | 注册 / 登录 / 登出 / 资料 / 偏好；三角色 RBAC；Token 会话（7 天） | ✅ |
| 2 | 商户经营 | 入驻申请 → 审核（驳回可改资料重提；审核期间仅可查看状态与完善资料）；店铺级维护门店 / 服务 / 套餐；**按门店决定是否上架**；**店铺/门店/服务/套餐均可上传多图**；图片上传 | ✅ |
| 3 | 检索与榜单 | 关键词 / 类别 / 区域 / 价格 / 评分组合检索；人气热榜、新店、分类榜；详情浏览计数 | ✅ |
| 4 | 收藏与互动 | 收藏（商户 / 服务）、关注、浏览足迹、消费记账 | ✅ |
| 5 | 评分与评价 | 对象化解构评价（一对象一评）、三维评分、图文、点赞、任意层回复、举报、掌柜回复 | ✅ |
| 6 | 优惠活动 | 券 / 满减 / 折扣 / 套餐券；创建→上下线；领取防超发限领一张；核销码校验 | ✅ |
| 7 | 门店订单 | **在门店下单**（服务项目 / 优惠套餐）→ 核销自动落消费流水 → 未使用可退款（状态机） | ✅ |
| 8 | 统计看板 | 商户经营看板 + 平台运营看板；近 7 日趋势、热门服务、类别分布（ECharts） | ✅ |

---

## 3. 快速开始

### 3.1 环境要求

| 依赖 | 版本 | 说明 |
| --- | --- | --- |
| CMake | ≥ 3.16（实测 4.4） | 构建脚本 |
| Ninja | 任意较新版本 | 生成器，也可换 Visual Studio / MinGW Makefiles |
| C++ 编译器 | 支持 C++17（实测 MinGW GCC 16.2） | Windows 下需可用的 g++ |
| Git | 任意 | 仅克隆用 |

> Linux / macOS 上同样可构建，把生成器切回 `Unix Makefiles` 或 `Ninja` 即可。

### 3.2 构建与运行

```bash
git clone https://github.com/liugd1/Local-Services-Review-Consumption-Platform.git
cd Local-Services-Review-Consumption-Platform

# 1) 构建（项目根目录）
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 2) 运行（任意工作目录均可，程序自动定位项目根）
build/locallife.exe          # Windows
# ./build/locallife           # Linux / macOS
```

启动后浏览器访问 **<http://127.0.0.1:8080>**。

- 首次启动自动执行 `sql/schema.sql`（幂等）建表，并创建管理员账号。
- 旧库启动时 `src/db/Migrate.h` 会按需增量补列（如 `reviews.target_type/target_id`、`review_comments.parent_id`）。
- **重置演示数据**：停止服务 → 删除 `data/locallife.db` → 重新启动。
- **切换端口 / 数据目录**：见 `src/main.cpp`（默认 8080，数据库固定落在 `data/locallife.db`）。
- 开发：VS Code 打开项目根，Kit 选 GCC，`F5` 调试（`.vscode/` 已配置好 launch / tasks / IntelliSense）。

### 3.3 演示账号

| 角色 | 账号 / 密码 | 入口 |
| --- | --- | --- |
| 平台管理员 | `admin` / `admin123` | 顶部导航「平台管理」→ `#/admin` |
| 商户掌柜 | `bob` / `123456` | 已入驻「Bob Restaurant」，进入「掌柜工作台」`#/shop` |
| 普通消费者 | `alice` / `123456`（也可自行注册） | 首页逛店、评价、`#/me` 个人中心 |

---

## 4. 重点模块详解

### 4.1 对象化评价与互动评论（模块 5）

- **评价对象化**：一条评价绑定 `target_type + target_id`，取值 `merchant`（店铺）/ `store`（门店）/
  `service`（服务项目）/ `package`（优惠套餐），并冗余归属 `merchant_id` 便于聚合。
- 每个对象拥有**独立评论区**（口碑页 `#/talk/{type}/{id}`）与独立均分、数量；详情页评价区支持
  「全部 / 店铺 / 门店 / 服务 / 套餐」切换（带条数徽标）。
- **评分规则**：环境 / 服务 / 性价比 三维各 1–5 星，均值保留一位小数实时聚合；
  **店铺评分只统计 `target_type='merchant'` 的可见评价**，服务与套餐好评不会污染店铺分。
- **图文评价**：发布时可即时上传图片并预览（jpg / png / gif / webp，≤6 张，≤8MB）。
- **任意层评论可回复**：点击评价卡「评论 N」展开评论楼；主评论与楼中楼子评论都有「回复」按钮，
  例如 A 评服务 → B 回复 A → C 回复 B → D 回复 C 均成立；展示采用「主评论 + 平铺回复」
  （`AliceRenamed 回复 @AliceRenamed：…`），不无限缩进。
- **权限**：评论作者本人或平台管理员可删除任意评论/评价；主评论删除时级联清理其全部子孙评论与附属点赞 / 举报记录；
  点赞（有用）、举报对所有登录角色开放；管理员仅浏览与治理，不参与点赞举报。
- 其它：掌柜可对评价做整体回复（一条可修改）；举报进入平台举报中心裁决。

### 4.2 优惠活动（模块 6）

- **活动类型**：`coupon` 代金券 / `full_reduction` 满减券 / `discount` 折扣券 / `package` 套餐券，
  支持面值 `face_value`、门槛 `threshold`、折扣率 `discount_rate`、发放总量 `total`（0 = 不限）、
  有效期 `start_time` ~ `end_time`、使用范围 `scope`。
- **生命周期**：商户创建为 `draft` → 发布 `published` → 下线 `offline`；
  消费者侧只可见「已发布 且 当前时间落在有效期内」的活动。
- **领取规则（防超发）**：单用户对同一活动**限领一张**；领取走
  "先原子扣减 `received`（`WHERE received < total OR total = 0`）成功后再插入领取记录"，
  插入因唯一约束失败时自动回补计数，保证高并发下不超发、重复领取返回 409。
- **核销**：每张券生成唯一核销码（`CX` + 8 位 Base32 字符，去除易混字符）；
  商户在「优惠活动」页输入券码核销，服务端校验**券归属本店 / 未核销 / 活动已发布 / 在有效期内**，
  成功后写入 `used_time` 并累加 `used`，二次核销返回 409。
- 消费者可在「我的卡券」查看全部 / 未使用 / 已使用，并出示核销码。

### 4.3 门店订单（模块 6 / 7）

> **下单主体是「门店」而不是「店铺」**：一家店铺可以有多个门店（分布在不同的区域），
> 消费者需要先进入店铺、再选择具体门店，最后在该门店选购服务项目或优惠套餐。

- **门店经营项目（上架关系）**：服务项目 / 优惠套餐 / 优惠活动统一在**店铺层级**创建与删除；
  每家门店通过 `store_services` / `store_packages` / `store_coupons` 决定**本店是否运营（上架）**某项，
  未上架的项目不会出现在该门店的选购页，也不能在该门店下单（返回 409）。
- **状态机**：`purchased`（待使用）→ `used`（已核销）｜ `purchased` → `refunded`（已退款）；
  已使用不可退款，重复核销 / 重复退款均返回 409。
- **下单**：校验门店存在且未打烊 → 店铺 `approved` → 项目属于该店铺 → **该项目在本门店已上架** →
  按 `limit_count` 做每人限购校验 → 生成唯一订单号并快照金额（订单记录 `store_id + item_type + 项目 id`）。
- **核销联动**：核销时在同一业务动作内把订单置为 `used` 并**自动写入消费流水**
  （金额、商户、门店、服务/套餐），因此「我的消费记录」与「商户经营统计」即时同步。
- 消费者在「我的 → 我的订单」中查看门店 / 项目 / 类型（服务/套餐）并一键「去使用 / 退款」；
  店铺详情页提供门店列表（含各店在售数量）作为选购入口。
- **选择门店步骤**：在店铺详情点击某服务 / 套餐的「选择门店下单」时，不会直接跳转，
  而是**弹出「选择门店」窗口，列出所有正在在售该项目的门店**（含营业状态、区域地址、本店在售数量），
  消费者选定门店后才进入该门店的选购页下单；若该项目在所有门店均未上架，则显示「暂未在门店上架」且不提供下单入口。

> 说明：为兼容历史数据，`POST /api/package/{id}/buy` 仍保留（自动选取任一在售该套餐的门店），
> 但前端已不再提供「店铺层级直接下单」入口。

### 4.4 图文描述（店铺 / 门店 / 服务 / 套餐）

- 店铺主可在掌柜台为四类对象上传**一张或多张图片**（jpg / png / gif / webp，单张 ≤8MB，多图逗号分隔存储）：
  - **店铺**：「经营概览 → 商户资料 → 店铺图片」（第一张作为店铺主页封面）；
  - **门店**：「门店管理 → 新增/编辑门店 → 门店图片」；
  - **服务项目 / 优惠套餐**：「服务项目 / 优惠套餐 → 新增/编辑 → 图片」。
- 前端统一使用 `LL.imagePicker` 组件（`public/js/uploader.js`）：选图后**即传即显**、支持多选、逐张移除、点击缩略图查看大图。
- 展示位置：店铺主页「店铺相册」+ 封面、门店列表缩略图、招牌服务 / 优惠套餐行缩略图、
  门店选购页「门店实景」与在售项目缩略图、掌柜台各列表与「门店经营项目」面板缩略图。

- 消费者在「我的 → 套餐订单」中一键「去使用 / 退款」；详情页套餐行提供「立即购买」。

### 4.4 统计看板（模块 8 + 可视化）

- **商户经营看板** `GET /api/merchant/stats`：浏览量、收藏数、关注数、店铺均分与评价数、
  订单数与订单金额、券领取 / 核销数、热门服务 Top5（按消费次数）、近 7 日消费额与消费笔数趋势。
- **平台运营看板** `GET /api/admin/stats`：用户总数与角色分布、今日新增、商户审批状态分布、
  公开评价数、待处理举报数、订单量与金额、消费流水汇总、券发放 / 核销、**类别分布**、
  近 7 日新增用户 / 订单量 / 消费额趋势。
- 前端用 **ECharts 5** 渲染：掌柜端「近 7 日经营趋势」双轴折线、运营端柱线混合趋势 + 类别饼图 + 7 日明细表。

---

## 5. 页面与截图

| 页面 | 路由 | 截图 |
| --- | --- | --- |
| 首页（Hero + 榜单） | `#/` | `docs/screens/1_home.png` |
| 商户详情（含评论 tab / 门店入口） | `#/m/{id}` | `docs/screens/2_detail.png`、`10_detail_tabs.png`、`11_review_uploader.png` |
| 对象口碑页（独立评论区） | `#/talk/{type}/{id}` | `docs/screens/9_talk_service.png` |
| **门店选购页（下单入口）** | `#/s/{id}` | 门店信息 + 本店在售服务/套餐 + 本店优惠活动（由「选择门店」弹窗进入） |
| 搜索与筛选 | `#/s?kw=&category=&sort=` | `docs/screens/7_search.png` |
| 我的（资料 / 收藏 / 消费 / 评价） | `#/me?tab=…` | `docs/screens/3_me.png` |
| 掌柜工作台（概览 / 服务） | `#/shop?tab=…` | `docs/screens/5_shop.png`、`8_shop_services.png` |
| 平台管理台 | `#/admin?tab=…` | `docs/screens/4_admin.png` |
| 评论权限与删除治理 | 评论区操作组 | `14_admin_delete_comment.png`、`15_comment_permissions.png` |
| 任意层评论回复 | 评价卡点「评论 N」展开评论楼 | `12_nested_reply.png`、`13_reply_B_in_ui.png` |
| **领券中心 + 套餐购买** | `#/m/{id}` | `docs/screens/modules678/1_merchant_coupon_band.png` |
| **我的卡券 / 我的套餐订单** | `#/me?tab=coupons|orders` | `modules678/2_me_coupons.png`、`modules678/3_me_orders.png` |
| **优惠活动管理 + 核销** | `#/shop?tab=coupons` | `docs/screens/modules678/4_shop_coupons.png` |
| **经营统计（商户）** | `#/shop?tab=stats` | `docs/screens/modules678/5_shop_stats.png` |
| **数据看板（平台）** | `#/admin?tab=stats` | `docs/screens/modules678/6_admin_stats.png` |

---

## 6. 技术栈

| 层 | 选型 |
| --- | --- |
| 后端 | C++17；cpp-httplib（RESTful 路由，`:param` 风格）；nlohmann/json（请求/响应 JSON） |
| 存储 | SQLite 3.53.4（单文件，随仓内置 amalgamation）；`sql/schema.sql` 27 张业务表 |
| 安全 | 口令 PicoSHA2 加盐哈希；Bearer Token 会话（7 天）；参数绑定防 SQL 注入 |
| 并发 | 单连接 + FULLMUTEX + 互斥锁；写操作 `BEGIN IMMEDIATE`；领取等场景用原子 UPDATE 防超发 |
| 前端 | 原生 HTML / CSS / JS（无框架）SPA + hash 路由；Bootstrap 5.3 + Bootstrap Icons（CDN）；ECharts 5.5 |
| 构建 | CMake / Ninja / MinGW GCC（Windows）或 Make/Ninja（*nix） |
| 测试 | curl 接口回归 + Playwright（headless Chromium）端到端页面验证与截图 |


---

## 7. 工程目录

```
Local-Services-Review-Consumption-Platform/
├── CMakeLists.txt                  构建脚本（C++17、随仓依赖、静态资源)
├── sql/schema.sql                  27 张业务表（幂等 DDL）+ 分类初始数据
├── src/
│   ├── main.cpp                    程序入口：初始化 DB → 迁移 → 启动 HTTP 服务
│   ├── server/Api.{h,cpp}          统一 JSON 响应、鉴权中间件、路由注册
│   ├── controller/                 11 个控制器：Auth / Admin / Common / Coupon / Interaction /
│   │                               Merchant / Order / Review / Search / Stat / Upload
│   ├── service/                    8 个业务服务：规则校验、事务编排（User / Merchant / Review /
│   │                               Interaction / Search / Coupon / Order / Stat）
│   ├── dao/                        9 个数据访问：全部参数化 SQL（User / Category / Merchant /
│   │                               Review / Interaction / Search / Coupon / Order / Stat）
│   ├── db/Database.h               单连接 + FULLMUTEX + RAII 事务 + query/queryOne/execute
│   ├── db/Migrate.h                老库启动增量补列与数据回填
│   ├── model/Models.h              实体与 JSON 取值助手（jsonStr / jsonNum / jsonInt）
│   └── util/                       BizError（业务异常）/ Json / Crypto / Time / Paths
├── public/                         前端静态资源（HTTP 根）
│   ├── index.html                  单页入口（hash 路由 + CDN 资源 + ?v= 缓存版本号）
│   ├── css/style.css               品牌样式（"巷味 · 本地生活志"视觉体系）
│   ├── js/                         common / home / store / talk / user / shop / admin / app
│   └── uploads/                    用户上传图片（运行期生成，不入库）
├── third_party/                    随仓依赖：httplib / json / picosha2 / sqlite3
├── docs/screens/                   页面截图（含 modules678/ 新模块验证图）
└── README.md / LICENSE             项目说明与开源协议
```

---

## 8. 数据库设计（`sql/schema.sql`，24 张表）

| 分组 | 表 | 说明 |
| --- | --- | --- |
| 账户 | `users`、`user_preferences`、`sessions` | 三角色账号、偏好分类、Token 会话 |
| 商户经营 | `merchants`、`stores`、`services`、`packages` | 商户主体 / 门店 / 服务项目 / 优惠套餐（含上下架、库存、限购、有效期） |
| 评价体系 | `reviews`、`review_images`、`review_likes`、`review_replies`、`review_reports` | 对象化评价、晒图、点赞、掌柜回复、举报 |
| 评论互动 | `review_comments`（自引用 `parent_id` 支持任意层）、`comment_likes`、`comment_reports` | 评论楼、评论点赞、评论举报 |
| 用户行为 | `favorites`、`follows`、`browse_history`、`consumption_records` | 收藏、关注、浏览足迹、消费流水 |
| 营销与交易 | `coupons`、`coupon_user`、`orders`、`store_services`、`store_packages`、`store_coupons` | 优惠活动、领券记录（含核销码 / 状态）、**门店订单**（`store_id + item_type + 项目 id`）、门店级上架关系 |
| 平台 | `operation_stats` | 平台运营指标留存 |

**一致性约定**

- 冗余 `merchant_id` 于评价、评论、消费、券、订单，避免多级 JOIN，同时便于按商户聚合。
- 唯一约束保障幂等：`UNIQUE(coupon_id,user_id)`（一人一券）、收藏/关注/点赞唯一键等。
- 可空外键一律显式写入 `NULL`；删除父级（评价/评论）时级联清理图片、点赞、举报、子评论。
- 对外只放行可展示状态：商户 `approved`、服务与套餐 `on`、评价 `visible`。
- 评分实时聚合：商户均分 = 该商户 `target_type='merchant'` 且 `visible` 的评价均值；
  评价下架/删除后统计自动回落。


---

## 9. API 速查

鉴权：请求头 `Authorization: Bearer <token>`；响应统一为 `{ code, message, data }`（`code = 0` 表示成功，
非 0 返回业务错误码与中文提示）。

### 9.1 账户与个人中心（模块 1）

| 说明 | 方法与路径 |
| --- | --- |
| 注册 / 登录 / 登出 | `POST /api/auth/register`、`/api/auth/login`、`/api/auth/logout` |
| 我的资料 / 修改 | `GET` / `PUT /api/user/profile` |
| 兴趣偏好 | `PUT /api/user/preferences` |
| 我的收藏 / 关注 / 足迹 / 消费 | `GET /api/my/favorites?type=`、`/api/my/follows`、`/api/my/history`、`/api/my/consumption` |
| 收藏 / 关注 / 记账 | `POST|DELETE /api/favorite`、`POST|DELETE /api/follow`、`POST /api/consume` |

### 9.2 商户与经营（模块 2）

| 说明 | 方法与路径 |
| --- | --- |
| 入驻申请 / 我的商户 | `POST /api/merchant/apply`、`GET|PUT /api/merchant/me` |
| 门店 / 服务 / 套餐 | `/api/merchant/stores`、`/api/merchant/services`、`/api/merchant/packages`（增删改查 + 状态） |
| 门店经营项目（上架） | `GET /api/merchant/stores/{id}/offerings`、`PUT .../offerings`（单项）、`PUT .../offerings/bulk`（批量） |
| 掌柜回复评价 | `POST /api/merchant/review/{id}/reply` |
| 优惠活动管理 | `GET|POST /api/merchant/coupons`、`PUT /api/merchant/coupons/{id}/status` |
| 优惠券核销 | `POST /api/merchant/coupon/verify` |
| 经营统计 | `GET /api/merchant/stats` |
| 图片上传 | `POST /api/upload`（multipart，白名单 jpg/png/gif/webp，≤8MB） |

### 9.3 检索 / 详情（模块 3，公开）

| 说明 | 方法与路径 |
| --- | --- |
| 组合搜索 | `GET /api/search?keyword=&category=&area=&price_min=&price_max=&min_score=&sort=&page=` |
| 榜单 | `GET /api/hot`、`/api/new`、`/api/categories`、`/api/categories/{id}/merchants` |
| 商户详情（含浏览计数） | `GET /api/merchants/{id}` |
| **门店详情（选购入口）** | `GET /api/stores/{id}`（门店信息 + 本店在售服务/套餐/活动 + 门店口碑汇总） |
| 某商户可领券列表 | `GET /api/coupons?merchant_id=` |

### 9.4 评价与评论（模块 5）

| 说明 | 方法与路径 |
| --- | --- |
| 发表评价（对象化） | `POST /api/review`（body：`merchant_id`、`target_type`、`target_id`、三维评分、图文） |
| 我的评价 / 店内评价 | `GET /api/my/reviews`、`GET /api/merchants/{id}/reviews?type=`、`/reviews/summary` |
| 对象评论区 | `GET /api/targets/{type}/{id}/reviews`、`/api/targets/{type}/{id}/info` |
| 评论（任意层回复） | `POST /api/review/{id}/comment`（带 `parent_id`）、`GET /api/review/{id}/comments` |
| 评论互动 | `POST|DELETE /api/comments/{id}/like`、`POST /api/comments/{id}/report`、`DELETE /api/comments/{id}` |
| 评价互动 | `POST|DELETE /api/review/{id}/like`、`POST /api/review/{id}/report`、`DELETE /api/review/{id}` |

### 9.5 优惠券与门店订单（模块 6 / 7）

| 说明 | 方法与路径 |
| --- | --- |
| 领取优惠券 | `POST /api/coupon/{id}/receive` |
| 我的卡券 | `GET /api/my/coupons?status=unused|used|all` |
| **门店下单（服务 / 套餐）** | `POST /api/store/{id}/order`（body：`item_type=service\|package`、`item_id`） |
| 购买套餐（兼容旧接口） | `POST /api/package/{id}/buy`（自动选取在售该套餐的门店） |
| 我的订单 | `GET /api/my/orders?status=purchased|used|refunded|all&page=&size=` |
| 核销套餐 | `POST /api/order/{id}/use` |
| 退款套餐 | `POST /api/order/{id}/refund` |

### 9.6 平台管理（模块 8）

| 说明 | 方法与路径 |
| --- | --- |
| 商户审核 | `GET /api/admin/merchants?status=pending`、`PUT /api/admin/merchants/{id}` |
| 举报中心 | `GET /api/admin/reports`、`PUT /api/admin/reports/{id}` |
| 用户管理 | `GET /api/admin/users`、`PUT /api/admin/users/{id}/status` |
| 平台运营看板 | `GET /api/admin/stats` |

---

## 10. 验证情况

**接口回归（curl）**：注册/登录/越权 401·403、入驻→审核→上架全流程、组合检索与分页、
收藏关注幂等、评价聚合一致性、评论链 A→B→C→D、举报→裁决、图片上传与静态回读；
模块 6-8 覆盖：发券→发布→公开列表→领取发码→重复领取 409→核销 200→二次核销 409，
购买→核销（消费流水联动）→重复核销 409→已用退款 409→未用退款 200，商户与平台统计接口均 200。

**端到端页面验证（Playwright headless Chromium）**：

- 六个新页面渲染正常且 **控制台 0 错误**；
- 页面内完成「领券 → 详情购买套餐 → 我的订单出现待使用 → 点击核销 → 状态变为已使用」完整闭环；
- 截图见 `docs/screens/modules678/`。

**门店下单与门店级上架（本轮新增，Playwright headless 22/22 通过）**：

- 场景一（消费者）：进入店铺 → 从「门店（选择门店选购）」进入门店选购页 → 选购在售服务 / 套餐并下单 → 「我的订单」显示门店、项目类型与名称；
- 场景二（店铺主）：在掌柜台「服务项目」创建项目（店铺级）→ 进入「门店管理 → 经营项目」按门店上架 / 下架（支持批量）→
  下架后消费者侧不可见、直接调用下单接口返回 409，重新上架后恢复可见可下单；
- 后端回归：订单归属门店、老库升级（历史订单自动回填门店、门店上架关系回填）、越权（消费者调用掌柜接口 403、跨店操作 404）均通过。

**已知边界**

- 评论楼中楼按「主评论 + 平铺回复」呈现（数据层 `parent_id` 记录完整链路，展示保持两层以适配窄屏）。
- 前端字体与 Bootstrap / ECharts 走 CDN，离线时回退系统字体、图表不渲染（功能不受影响）；
  更新 JS 后请递增 `index.html` 中的 `?v=` 版本号以规避浏览器缓存。
- 支付环节为教学演示：下单即视为支付成功，无第三方支付对接。

---

## 11. 后续可扩展方向

1. 个性化推荐（基于偏好标签 + 浏览/消费行为）与用户画像；
2. 虚假评价识别（可信度评分：消费凭证、文本特征、行为异常）；
3. 口碑趋势分析与商户对比报表；优惠券叠加规则与满减计算引擎；
4. 部署增强：Nginx 反代 + HTTPS、SQLite 升级为 PostgreSQL / MySQL 的 DAO 适配层。

---

## 12. 许可证

本项目为个人学习作品，采用 **MIT License** 发布，可自由学习与参考；使用到的第三方库
（cpp-httplib、nlohmann/json、SQLite3、PicoSHA2、Bootstrap、ECharts）遵循其各自开源协议。

---

> 如果这个项目对你有帮助，欢迎 Star ⭐ 支持。

