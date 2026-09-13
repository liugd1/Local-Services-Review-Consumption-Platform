# 本地生活商户评价与消费服务平台 —— 开发计划（C++ 实现）

> **版本**：v1.3（2026-09-08）
> **更新说明**：v1.2 之上完成**剩余模块**：⑥ 优惠活动（商户发券/上下线、消费者领取防超发限领一张、核销码到店核销）、
> ⑦ 套餐订单（购买→核销联动消费流水→退款状态机）、⑧ 经营/运营统计看板；前端补齐三端入口与 ECharts 数据可视化。
> 剩余模块 6-8 与可选功能方案不变（见第十三章）。

---

## 一、项目概述

本系统面向**普通消费者、商户经营者、平台管理员**三类角色，为餐饮、休闲娱乐、美容美发、运动健身、摄影、桌游、KTV 等本地生活服务提供：商户展示、组合搜索、用户评价、优惠活动、消费记录与商户运营分析。

**当前核心目标（v1.2）**：后端 8 大模块中的 1-5 已实现并通过 API 测试；前端三端（食客 / 掌柜 / 管理员）界面上线并逐页验证；**评论体系已完成对象化与任意层互动**；继续实现优惠活动、套餐订单与经营/运营统计，随后补数据可视化与个性化推荐。

---

## 二、总体进度一览

| 范围 | 内容 | 状态 |
| --- | --- | --- |
| 环境与工程 | CMake + Ninja + MinGW + SQLite 3.53.4，头部依赖已就位，任意目录可启动 | ✅ |
| 数据库 | `sql/schema.sql` 22 张业务表 + 幂等初始化 + 自动创建 admin | ✅ |
| 后端 模块1 | 用户注册/登录/资料/偏好/三角色 RBAC/Token 会话 | ✅ 已测 |
| 后端 模块2 | 商户入驻审核、门店/服务/套餐管理、图片上传 | ✅ 已测 |
| 后端 模块3 | 组合条件检索、热门/新店/分类榜单、商户详情(浏览计数) | ✅ 已测 |
| 后端 模块4 | 收藏、关注、浏览历史、消费记录 | ✅ 已测 |
| 后端 模块5 | 对象化多维评分、图文评价、点赞评论（任意层回复）、商户回复、举报→审核 | ✅ 已测 |
| 评论增强 | 店铺/门店/服务/套餐独立评论区、图片前端上传、我的评价直达定位、楼中楼 | ✅ headless 验证 |
| 前端 | 「巷味 · 本地生活志」三端界面 + 评论交互（评价 tab/口碑页） | ✅ 逐页验证 |
| 后端 模块6 | 优惠活动（券/满减/折扣/套餐，领取与核销校验） | ✅ 已测 |
| 后端 模块7 | 套餐购买订单（购买/核销/退款状态机） | ✅ 已测 |
| 后端 模块8 | 经营统计 / 平台运营统计接口 | ✅ 已测 |
| 前端 | 掌柜“优惠活动/经营统计”、食客“领券中心/我的卡券/套餐订单”、平台“数据看板” | ✅ headless 验证 |
| 可选 | 数据可视化(ECharts)、推荐、画像、可信度/假评识别、口碑趋势 | 可视化 ✅ 统计看板已上线 |

---

## 三、技术选型（已落地）

| 层次 | 选型 | 说明 / 现状 |
| --- | --- | --- |
| 语言 / 构建 | C++17，CMake 4.4.3 + Ninja 1.13.2 | `D:\cmake`、`D:\ninja`，已写入用户 PATH |
| 编译器 | MinGW-w64 GCC 16.2.0（gdb 17.2） | `D:\mingw64` |
| HTTP 服务 | cpp-httplib（master，header-only） | 路由 / 静态挂载 / multipart 上传；**注意其新 API：文件在 `req.form`** |
| JSON | nlohmann/json 3.12.0（header-only） | 统一 `{code,message,data}` 响应 |
| 数据库 | SQLite 3.53.4 amalgamation（编译进工程） | 单文件零部署；`FULLMUTEX` + 互斥锁串行化，事务 RAII |
| 密码/会话 | PicoSHA2（加盐哈希）、64 位 hex Token | Token 存 `sessions` 表，7 天过期，登录实时校验状态 |
| 前端 | 原生 HTML/CSS/JS + Bootstrap 5.3 + Bootstrap Icons（CDN） | 无构建步骤，后端直接挂载 `public/`；后续引入 ECharts |
| 视觉 | 本地生活志气质：暖纸底/墨绿/柿红/楷书+衬线 | 字体 Google Fonts（离线自动回退系统字体） |
| 代码编辑 | VS Code：C/C++、CMake Tools、CMake | 配置已写入 `.vscode/`，F5 走 gdb 调试 |

> 说明：所有第三方为 header-only 或单一 C 源，随仓库 `third_party/` 管理；数据库表结构语义可平滑迁移 MySQL（DAO 层隔离）。

---

## 四、系统架构与工程目录（已落地）

```
浏览器（HTML/CSS/JS + Bootstrap；SPA hash 路由 #/…）
   │  HTTP/JSON、Authorization: Bearer <token>
   ▼
src/
├── main.cpp                入口：定位项目根(Paths)、初始化库、挂载 public、注册路由
├── server/Api.{h,cpp}      鉴权中间件 authenticate()、统一响应 sendOk/sendErr、
│                           业务异常守卫 guard()、全部 Controller 注册
├── controller/             Auth / Admin / Common / Merchant / Upload /
│                           Search / Interaction / Review（每模块一个）
├── service/                User / Merchant / Search / Interaction / ReviewService
│                           （业务规则、权限与状态校验集中在 service 层）
├── dao/                    User / Category / Merchant / Search / Interaction / ReviewDao
│                           （SQL 参数绑定，防注入）
├── db/Database.h           SQLite 封装：query/execute、事务 RAII、脚本执行
├── model/Models.h          实体(User)与 json 取值/拼串工具
├── util/                   Json.h(响应) BizError.h(业务异常) Crypto.h(密码/Token)
│                           Time.h Paths.h(以 exe 锚定项目根)
├── public/                 —— 前端（见第七章）
│   ├── index.html  css/style.css  js/{common,home,user,shop,admin,app}.js
│   └── uploads/            图片上传目录（URL /uploads/xxx）
├── sql/schema.sql          22 张表 + 7 类初始数据（幂等）
├── third_party/            httplib / json / picosha2 / sqlite3（随仓管理）
└── docs/screens/           关键页面截图
```

**分层约定**
- controller 只做参数解析与 JSON 装配；service 承载业务规则（越权/状态/评分计算/校验）；dao 单表/联查 SQL。
- 统一：业务码即 HTTP 状态码（400/401/403/404/409/500）；`code=0` 成功。
- 跨目录启动安全：`Paths.h` 以可执行文件为锚向上找含 `CMakeLists.txt` 的目录，
  使 `public/`、`data/`、`sql/` 无论从根目录或 `build/` 启动都解析一致。

---

## 五、需求拆解与实现状态

### 5.1 用户 / 权限 / 会话（✅ 模块1）
- 接口：注册/登录/登出、`GET|PUT /api/user/profile`、`PUT /api/user/preferences`、管理端用户列表与启用/禁用。
- 规则：加盐哈希；注册默认 consumer，商户入驻后升级 merchant；`sessions` Token 会话，过期自动清理；
  禁用即时失效（鉴权每次实时 join users）；RBAC 拦截 401/403。
- 已验证：409 重复注册、错密码 401、封禁后请求 401、登出后失效、偏好数组→`1,3`。

### 5.2 商户 / 门店 / 服务 / 套餐 + 入驻审核（✅ 模块2）
- 入驻申请→`pending`→管理员 `approved/rejected`（驳回须填原因，可改资料重提）；
  **未过审不能维护经营数据（403）**。
- 门店（营业/休息/打烊）、服务与套餐（上下架、限购、库存、有效期）。
- `POST /api/upload`：白名单 jpg/png/gif/webp，≤8MB，唯一文件名，存 `public/uploads`。
- 已验证：入驻→审核→上架全流程、越权 403、图片上传与静态回读。

### 5.3 检索 / 榜单 / 详情（✅ 模块3）
- `GET /api/search`：keyword/category/area/价格区间(min<=p_max, p_max>=p_min)/min_score/排序(score|popularity|newest)/分页，仅展示 approved。
- 评分聚合子查询（visible 评价实时 avg/review_count）与商户同表返回。
- 榜单：`/api/rank/hot|new|category/{id}`；详情 `GET /api/merchants/{id}`：仅 approved 公开，
  所有者/管理员可见任意状态；访问 +1 浏览量，登录用户自动记浏览历史。
- 已验证：组合过滤命中/落空、榜单、计数自增、隐藏评价后 avg 实时回落。

### 5.4 收藏 / 关注 / 历史 / 消费（✅ 模块4）
- 收藏（商户/服务）关注均可逆、UNIQUE 幂等；我的收藏/关注/足迹/账单分页。
- `POST /api/consume` 手动记账（可选关联服务/套餐，自动校验归属）；套餐核销后续自动落单。
- 已验证：增删、联表名称、服务/套餐外键空值四分支、列表隔离。

### 5.5 评分评价与评论互动（✅ 模块5 + 增强）
- **评价对象化**：评价可针对 店铺/门店/服务/套餐（`target_type + target_id`，冗余归属商户 `merchant_id`）；
  各对象拥有**独立评论区**（talk 页）与各自汇总均分；详情页评价区支持“全部/店铺/门店/服务/套餐”tab 过滤。
- 一对象一评；环境/服务/性价比三维 1-5，均值一位小数并实时聚合（**店铺评分仅统计店铺维度评价**）。
- 图文评价：后端 `review_images`；前端晒图控件即时上传/预览/移除（≤6 张）。
- **评论任意层可回复**：`review_comments.parent_id` 精确指向被回复评论；界面以“主评论 + 平铺回复”呈现，
  每条（含子评论）都有「回复」按钮，子回复标注 `回复 @某人`。
- 举报→平台审核（hide 下架 / reject 驳回）；商户对评价整体回复（一条，可改）。
- **我的评价**卡片带对象类型与「进入评论区 ›」直达（`#/talk/{type}/{id}?focus=评价id`，自动定位高亮）。
- 已验证：对象评价隔离与聚合、任意层回复链 A→B→C→D（parent 精确）、
  浏览器内对 B 行真实点击「回复」提交成功、图片上传缩略图、评价下架后统计归零、越权 403。

> 备注：评价聚合的统计一致性依赖「只统计 visible」子查询；店铺均分仅统计 `target_type='merchant'`，
> 服务/套餐评价计入各自对象评论区，互不污染。老库启动自动迁移补列并回填 target。

---

## 六、前端界面（已上线，SPA + hash 路由）

品牌「巷味 · 本地生活志」：暖纸底 × 墨绿 × 柿红，楷书/衬线标题，编辑刊物气质。

| 视图 | 路由 | 说明 |
| --- | --- | --- |
| 首页 | `#/` | Hero+统计、分类徽章、人气热榜、新店登场 |
| 搜索 | `#/s?kw=&category=&sort=` | 组合筛选 + 排序下拉 + 列表卡片 |
| 商户详情 | `#/m/{id}` | 品牌页头、资料/服务/套餐、评价（发评/点赞/举报/删除/掌柜回复） |
| 登录/注册 | 模态框 | 顶部导航登录态切换 |
| 我的 | `#/me?tab=…` | 资料与偏好 / 收藏 / 关注 / 足迹 / 消费 / 我的评价 |
| 入驻申请 | `#/apply` | 状态感知表单（通过/审核中/驳回可重提） |
| 商户工作台 | `#/shop?tab=…` | 概览 / 门店 / 服务 / 套餐 / 回复评价 |
| 平台管理 | `#/admin?tab=…` | 商户审核 / 举报中心 / 用户管理 |
| 对象口碑页 | `#/talk/{type}/{id}` | 店铺/门店/服务/套餐各自评论区：对象头+均分、发评(晒图)、评论楼展开与任意层回复、返回店铺 |

- 文件：`public/index.html`、`public/css/style.css`、`public/js/{common,home,talk,user,shop,admin,app}.js`；
  静态资源加载带 `?v=N` 版本号（更新 JS 后递增即可绕过浏览器缓存）。
- 交互验证（headless Chromium + Playwright）：各角色页面渲染、登录态注入、路由切换、评论展开与任意层回复、0 控制台错误。
- 截图存于 `docs/screens/`（home/detail/me/search/admin/shop…）。
- 规划：模块 8 完成后引入 **ECharts**（CDN）做统计可视化图表页。

---

## 七、已实现 API 一览

> 统一前缀 `/api`，`POST` body 为 JSON；鉴权头 `Authorization: Bearer <token>`。

| 模块 | 方法与路径 | 说明 | 权限 |
| --- | --- | --- | --- |
| 认证 | POST `/auth/register` `/auth/login` `/auth/logout` | 注册/登录/登出 | 公开 / 登录 |
| 用户 | GET·PUT `/user/profile`；PUT `/user/preferences` | 资料、消费偏好 | consumer+ |
| 用户 | GET `/admin/users`；PUT `/admin/users/{id}/status` | 列表、启停用 | admin |
| 公共 | GET `/categories` | 商户类别 | 公开 |
| 商户 | POST `/merchant/apply`；GET·PUT `/merchant/me` | 入驻/我的商户 | 登录/merchant |
| 商户 | CRUD `/merchant/stores`、`/merchant/services`、`/merchant/packages`(+`/status`) | 经营数据维护 | merchant |
| 审核 | GET `/admin/merchants` `/counts`；PUT `/admin/merchants/{id}/audit` | 商户审核 | admin |
| 上传 | POST `/upload` | multipart 图片 | 登录 |
| 检索 | GET `/search` | 组合搜索 | 公开 |
| 详情 | GET `/merchants/{id}` | 商户详情+浏览 | 公开(登录记足迹) |
| 榜单 | GET `/rank/hot` `/rank/new` `/rank/category/{id}` | 热门/新店/分类 | 公开 |
| 评价 | POST `/review`；GET `/my/reviews`；DELETE `/review/{id}` | 发表（对象化）/我的/删除 | consumer |
| 评价 | GET `/merchants/{id}/reviews`(+`type=`)；GET `/merchants/{id}/reviews/summary` | 店内评价/对象 tab 汇总 | 公开 |
| 口碑页 | GET `/targets/{type}/{id}/reviews`；GET `/targets/{type}/{id}/info` | 对象评论区 + 头部信息/汇总 | 公开 |
| 互动 | POST·DELETE `/review/{id}/like`；POST `/review/{id}/comment`(可带 `parent_id`)；GET `/review/{id}/comments`；POST `/review/{id}/report` | 点赞/评论(任意层回复)/评论列表/举报 | consumer |
| 回复 | POST `/merchant/review/{id}/reply` | 商户回复评价 | merchant |
| 审核 | GET `/admin/reports`；PUT `/admin/reports/{id}` | 举报中心与处理 | admin |
| 收藏 | GET `/my/favorites`；POST·DELETE `/favorite` | 商户/服务收藏 | consumer |
| 关注 | GET `/my/follows`；POST·DELETE `/follow` | 关注商户 | consumer |
| 足迹 | GET `/my/history` | 浏览历史 | consumer |
| 消费 | POST `/consume`；GET `/my/consumptions` | 记一笔/账单 | consumer |

> 待模块 6-8 新增：优惠活动、套餐订单、统计看板相关接口（见第九章方案）。

---

## 八、里程碑与开发时间安排（进度已更新）

| 阶段 | 内容 | 状态 |
| --- | --- | --- |
| 0 环境骨架 | 工具链/依赖/schema/启动 | ✅ 2026-09-06 |
| 1 用户权限 | 注册登录/资料偏好/RBAC/会话 | ✅ 已测 |
| 2 商户经营 | 入驻审核/门店/服务/套餐/上传 | ✅ 已测 |
| 3 检索详情 | 组合搜索/榜单/详情/浏览计数 | ✅ 已测 |
| 4 互动消费 | 收藏/关注/历史/消费 | ✅ 已测 |
| 5 评分评价 | 对象化评分/图文/任意层评论回复/举报审核 | ✅ 已测 |
| 5b 评论增强 | 对象独立评论区、talk 口碑页、图片上传、我的评价直达定位 | ✅ headless 验证 |
| 6 前端三端 | 食客/掌柜/管理 SPA + 设计系统 + 评论交互 | ✅ headless 验证 |
| **7 优惠活动** | coupons 创建/领取/核销（事务与规则校验） | ⏳ 下一步 |
| **8 套餐订单** | orders 购买/使用/退款状态机 + 消费联动 | ⏳ 待做 |
| **9 经营统计** | 商户维度统计 + 平台运营统计接口 | ⏳ 待做 |
| **10 可视化/可选** | ECharts 看板 + 推荐/画像/假评识别（加分） | ⏳ 视进度 |
| **11 文档与论文** | 需求分析/系统设计/E-R/测试报告/截图/关键代码 | 📌 随开发沉淀 |

> 建议节奏：优惠活动 2~3 天 → 订单状态机 2~3 天 → 统计接口 2 天 → 可视化 2 天 → 剩余时间做推荐/画像等加分项与论文整理。

---

## 九、数据库设计说明

`sql/schema.sql`（幂等，IF NOT EXISTS）现有 22 张表：
用户偏好/会话/类别/商户/门店/服务/套餐/评价/评价图/点赞/评论/回复/举报/收藏/关注/浏览历史/消费记录/**优惠活动/领取使用/订单/运营统计**。

关键约定与注意：
- `PRAGMA foreign_keys=ON`；可空外键（service_id/package_id 等）插入时必须显式 `NULL`（绑定空串会触发 FK 失败 —— 已在消费记录修复为分支 SQL）。
- 审核/上下架/评价可见性均以状态列驱动（pending/approved/rejected、on/off、visible/hidden/deleted），对外查询只放行可展示状态。
- 评分统计不落冗余字段（实时子查询聚合），避免一致性难题；`operation_stats` 表预留给日粒度报表/趋势。
- 新表设计沿用现有风格：状态列 + created_at，业务唯一约束用 UNIQUE / UNIQUE 索引保证幂等。

---

## 十、测试记录（已完成部分）

后端测试方式：临时脚本 + curl 接口回归；前端：Node 语法校验 + headless Chromium 渲染与路由。

已覆盖的正确性要点：
1. 角色越权 401/403（consumer 访问 merchant/admin 接口被拒）；
2. 状态机：待审商户不可经营、下架服务不出现在详情、评价 hidden 后 avg/review_count 归零；
3. 评分计算与实时聚合；一店一评冲突 409；
4. 外键空值四分支（仅服务/仅套餐/两者/均无）；
5. 收藏/关注幂等与删除；会话过期与封禁即时失效；
6. 前端路由（me/shop/admin/detail/search/talk）逐页渲染与 0 控制台错误；
7. 对象化评价：同一对象不可重复评、服务评价不污染店铺均分、店内对象 tab 计数正确；
8. 评论任意层回复链（curl：A→B→C→D parent 精确）与浏览器真实按钮路径对 B 回复成功；
9. 业务 404 文案不再被错误处理器吞掉（评价不存在/对象不存在等能正确提示）。

**待补充测试（模块 6-8）**：并发领取不超发、核销一次性、有效期/门槛边界、订单状态迁移合法性、统计数据与明细一致、退款幂等。

演示账号：`admin/admin123`（管理员）、`bob/123456`（掌柜）、`alice/123456`（食客）。

---

## 十一、构建与运行指南

```bash
# 构建（项目根）
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# 运行（任意目录均可，Paths 自动定位项目根）
build\locallife.exe        # 默认 http://127.0.0.1:8080
```

- 数据库：`bysj/data/locallife.db`（首次启动自动建表并创建 admin）。如需重置演示数据：停止服务后删除该文件重启即可。
- 前端：静态资源由后端挂载 `public/`，浏览器直接访问根路径。
- 开发：VS Code 打开项目根 → 底部 Kit 选 GCC → F5 调试；`.vscode` 已预置 cpptools/cmake-tools/launch/tasks。
- 环境：MinGW `D:\mingw64`、CMake `D:\cmake`、Ninja `D:\ninja`（已入 PATH），无需管理员安装。

---

## 十二、风险与对策（实战修正记录）

| 风险 / 问题 | 处理 |
| --- | --- |
| 从 build 目录启动时静态资源与数据库路径漂移 | `Paths.h` 以 exe 为锚向上定位项目根，public/data/uploads 统一解析（已修复并验证） |
| cpp-httplib 版本 API 差异（multipart、404 错误文案误导） | 文件上传按新版 `req.form.has_file/get_file`；错误处理器保留业务 JSON，静态 404 与接口 404 文案按场景区分 |
| SQLite 可空外键绑空串触发 FK 失败 | 空引用一律显式 `NULL`（分支 SQL 已落地） |
| 并发写 | 单连接 FULLMUTEX + 互斥锁串行化 + `BEGIN IMMEDIATE` 事务 RAII，满足演示规模 |
| 前端 hash 路由路径解析带前导 `/` 导致全部落到首页 | parse 归一化（已修复，逐路由验证通过） |
| 运行时先取 DOM 节点再整体重建 innerHTML | 渲染顺序约束：先写入容器再查询填充（renderAudit 已修复） |
| 论文需截图/图表 | 页面截图入 `docs/screens/`；模块 8 统计页面用 ECharts 出图后亦可直接作为文档配图 |
| 业务 404 文案被全局错误处理器覆盖（显示成“接口不存在”） | 404 兜底仅在响应体为空时生效，真实业务消息正常透出（已修复） |
| 前端静态 JS 缓存导致演示旧逻辑 | `index.html` 引入带版本号的 `/js/*.js?v=N`，更新时递增 N 即可强制取新文件 |

---

## 十三、剩余模块开发方案（待办细化）

### 13.1 模块 6：优惠活动
- 商户创建：类型 `coupon(券)/full_reduction(满减)/discount(折扣)/package(套餐礼券)`；含面值/门槛/折扣率、总量、有效期起止、适用范围、状态（draft/published/offline）。
- 规则校验：published 且 start<=now<=end、总量 `received<total`、每人限领（`UNIQUE(coupon_id,user_id)` 幂等返回提示）。
- 领取：事务内 `received+1`（`UPDATE … WHERE received<total` 防超发）+ 生成唯一核销码 `code`。
- 核销：校验码有效、状态 unused、未过期、满足门槛；事务改 `used` 并 `coupons.used+1`，同一 code 二次核销拒绝。
- 接口草案：`POST /api/merchant/coupons`、`GET /api/coupons?merchant_id=`、`POST /api/coupon/{id}/receive`、`POST /api/coupon/verify`(商户扫核销码)、`GET /api/my/coupons`。

### 13.2 模块 7：套餐订单
- 购买：`POST /api/package/{id}/buy`（校验套餐 on、限购数量 limit_count），生成唯一 order_no，状态 `purchased`，金额取套餐价。
- 使用/核销：`POST /api/order/{id}/use` —— 仅消费者本人、purchased 可转 used，写入使用时间并自动生成消费记录（consume）。
- 退款：`POST /api/order/{id}/refund` —— purchased 才可退（used 后不可退），幂等。
- 列表：`GET /api/my/orders`（可按状态过滤）。
- 状态迁移校验做非法迁移拒绝（purchased→purchased 幂等、refunded 不可再 use 等）。

### 13.3 模块 8：经营与运营统计
- 商户维度（`GET /api/merchant/stats`）：浏览量/收藏数/关注数/评价数/平均分/优惠领取与核销数/订单数/热门服务 TopN/近 7 日趋势。
- 平台维度（`GET /api/admin/stats`）：商户总数与按类别分布、用户活跃（注册/登录/点评数）、消费趋势（按月/日金额）、平均评分分布。
- 实现：明细表实时聚合 + 可选 `operation_stats` 日缓存（约定口径：以明细为准）。
- 前端：`#/shop?tab=stats` 与 `#/admin?tab=stats` 两个 ECharts 图表页（引入 echarts CDN）。

### 13.4 可选 / 加分
- 个性化推荐：登录用户按偏好类别 + 高评分 + 人气加权的冷启动推荐，首页「猜你喜欢」区块（基于 user_preferences + 足迹/收藏类别向量）。
- 用户画像：聚合浏览/收藏/评价类别生成标签（可在「我的」展示）。
- 评价可信度/假评识别：规则打分（长度/附图/历史点评数/评分偏离商户均值/同 IP 聚集简化），hidden 阈值标记并推送审核队列。
- 口碑趋势：按周聚合 avg_score 与评论量折线（支撑 `operation_stats`）。

### 13.5 论文文档素材（随开发同步积累）
需求与业务分析 → 系统结构/业务流程图（draw.io 出图）→ 数据库 E-R（22 表）→ 界面截图（docs/screens）→ 关键代码片段 → 测试用例与结果。

---

## 十四、下一步（优先级）

1. **模块 6 优惠活动**：新增 CouponService/Dao/Controller + 商户工作台与食客领券核销页面。
2. **模块 7 订单状态机** + 与消费记录联动。
3. **模块 8 统计接口** → 两个 ECharts 看板页。
4. 回归：补充边界/并发测试（curl 脚本化放 `tests/`）。
5. 可选功能择优实现 + 开始排版论文。

> 若中途有验收侧重（如优先保演示完整度），模块 6→7 顺序可保持，模块 8 统计至少保证 merchant/admin 两个汇总接口与一页图表，其余作为加分收尾。




