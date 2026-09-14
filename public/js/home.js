/* 巷味 · 首页 / 搜索 / 商户详情 */
LL.views = LL.views || {};

function _g(id) { return (LL.CATS && LL.CATS[id]) ? LL.CATS[id] : null; }

/* 商户卡片 */
function merchantCard(m) {
  const cid = Number(m.category_id);
  const star = Number(m.avg_score) || 0;
  return `<a class="m-card" href="#/m/${m.id}" style="animation:rise .4s both">
    <div class="m-cover" style="${LL.cover(cid)}">
      <span class="emoji">${LL.emoji(cid)}</span>
      <span class="cat-pill">${LL.esc(LL.cat(cid))}</span>
      ${star > 0 ? `<span class="score" style="position:absolute;right:10px;top:44px;color:#fff;text-shadow:0 1px 4px rgba(0,0,0,.35)">${star.toFixed(1)}</span>` : ""}
    </div>
    <div class="m-body">
      <h3>${LL.esc(m.name)}</h3>
      <div class="m-meta">
        <span><i class="bi bi-geo-alt"></i> ${LL.esc(m.area || "本城")}</span>
        <span><i class="bi bi-chat-left-text"></i> ${Number(m.review_count) || 0} 条评价</span>
      </div>
      <div class="m-foot">
        <span class="price-min">${LL.money(m.price_min)}<small>-${LL.money(m.price_max || m.price_min)}</small></span>
        <span class="stars">${LL.starsHtml(star)}</span>
      </div>
    </div></a>`;
}

/* 大行卡片（搜索列表） */
function merchantRow(m) {
  const cid = Number(m.category_id);
  const star = Number(m.avg_score) || 0;
  return `<a class="row-card" href="#/m/${m.id}">
    <div class="row-thumb" style="${LL.cover(cid)}">
      <span>${LL.emoji(cid)}</span>
    </div>
    <div class="row-info">
      <div class="d-flex align-items-center gap-2 flex-wrap">
        <h3>${LL.esc(m.name)}</h3>
        <span class="badge-soft badge-pending" style="background:#f1e8d6;color:#7a5f2a">${LL.esc(LL.cat(cid))}</span>
      </div>
      <div class="tag-line">
        <span><i class="bi bi-geo-alt"></i> ${LL.esc(m.area || "本城")}</span>
        <span><i class="bi bi-clock"></i> ${LL.esc(m.business_hours || "见店公告")}</span>
        <span><i class="bi bi-chat-left-text"></i> ${Number(m.review_count) || 0} 评价</span>
      </div>
      <p class="text-muted mb-1 mt-1" style="font-size:13px">${LL.esc((m.intro || "").slice(0, 60))}</p>
      <div class="d-flex align-items-center justify-content-between">
        <span class="price-min">${LL.money(m.price_min)}<small>-${LL.money(m.price_max || m.price_min)}</small></span>
        <span class="d-flex align-items-center gap-2">
          <span class="score-big">${star > 0 ? star.toFixed(1) : "—"}</span>
          ${LL.starsHtml(star)}
        </span>
      </div>
    </div></a>`;
}

function emptyBox(text) {
  return `<div class="empty"><span class="ic">🫧</span>${LL.esc(text || "这里还空空的")}</div>`;
}

function secHead(title, moreHtml) {
  return `<div class="sec-head"><h2><span class="tick"></span>${LL.esc(title)}</h2>${moreHtml || ""}</div>`;
}

function loadMore() { LL.toast("已到底啦", "info", 1500); }

/* 渲染商户网格/列表 */
function drawMerchants(rows, asRow) {
  if (!rows || rows.length === 0) return emptyBox("没有找到符合条件的商户，换个关键词试试？");
  return asRow ? rows.map(merchantRow).join("") : `<div class="merchant-grid">${rows.map(merchantCard).join("")}</div>`;
}

/* ---------------- 视图：首页 / 搜索 ---------------- */
LL.views.home = async function (el, params) {
  params = params || {};
  const inp = document.getElementById("searchInput");
  if (inp) inp.value = params.kw || "";
  const isSearch = !!(params.kw || params.category);

  let cats = [];
  try { cats = await LL.api("GET", "/api/categories"); } catch (e) { cats = []; }

  const chipsHtml = () => {
    const h = ['<div class="cat-chips">'];
    const mk = (label, q) => '<button class="cat-chip' + (q ? "" : " on") + '" onclick="location.hash=\'' + q + '\'">' + label + "</button>";
    h.push(mk("全部", isSearch ? "#/s?kw=" + encodeURIComponent(params.kw || "") : "#/"));
    cats.forEach(c => {
      const on = String(params.category) === String(c.id);
      h.push('<button class="cat-chip' + (on ? " on" : "") + '" onclick="location.hash=\'#/s?kw=' +
        encodeURIComponent(params.kw || "") + "&category=" + c.id + '\'">' +
        LL.emoji(c.id) + " " + LL.esc(c.name) + "</button>");
    });
    h.push("</div>");
    return h.join("");
  };

  const selHtml = () => {
    const s = params.sort || "";
    const opt = v => '<option value="' + v + '"' + (s === v ? " selected" : "") + ">" +
      ({ "": "综合推荐", newest: "最新上架", popularity: "人气优先", score: "好评优先" }[v] || v) + "</option>";
    return '<div class="d-flex align-items-center gap-2"><label class="text-muted small mb-0">排序</label><select id="sortSel" class="form-select form-select-sm" style="width:auto">' +
      opt("") + opt("score") + opt("popularity") + opt("newest") + "</select></div>";
  };

  let html = '<div class="container-xl">';

  if (!isSearch) {
    // 首页 hero
    let total = 0;
    try { const r = await LL.api("GET", "/api/search?page=1&size=1"); total = r.total; } catch (e) {}
    html += `<section class="hero">
      <div class="eyebrow">· 巷味 LOCAL LIFE ·</div>
      <h1>拐进巷子，遇见好店</h1>
      <p class="lead">本地口碑好店都在这里 —— 美食、休闲、美发、健身、摄影、桌游与欢唱，
      把这座城市的烟火气一页页翻给你看。</p>
      <div class="stat-row">
        <div class="stat"><b>${total}</b><span>平台入驻好店</span></div>
        <div class="stat"><b>${cats.length}</b><span>生活分类</span></div>
        <div class="stat"><b>100%</b><span>商户平台审核</span></div>
        <div class="stat"><b>评分真实</b><span>多维度用户口碑</span></div>
      </div></section>`;
    html += '<div class="sec-head" style="margin-top:2rem"><h2><span class="tick"></span>按兴趣逛</h2></div>' + chipsHtml() + "</div>";
    html += '<div class="container-xl" style="margin-top:.5rem">' + secHead("🔥 人气热榜",
      '<a class="more" href="#/s?sort=popularity">查看全部 →</a>') + "</div>";
    html += '<div class="container-xl"><div id="hotGrid" class="merchant-grid">' + skeleton(4) + "</div></div>";
    html += '<div class="container-xl">' + secHead("✨ 新店登场",
      '<a class="more" href="#/s?sort=newest">查看全部 →</a>') + "</div>";
    html += '<div class="container-xl"><div id="newGrid" class="merchant-grid">' + skeleton(4) + "</div></div>";
  } else {
    // 搜索模式
    const title = params.kw ? "搜索 “" + params.kw + "”" : LL.cat(params.category) + " · 精选";
    html += secHead(title, selHtml());
    html += '<div class="mb-3">' + chipsHtml() + "</div>";
    html += '<div id="resultList" style="min-height:200px">' + skeleton(3) + "</div>";
  }
  html += "</div>";
  el.innerHTML = html;

  const sortSel = document.getElementById("sortSel");
  if (sortSel) sortSel.addEventListener("change", e => {
    location.hash = "#/s?kw=" + encodeURIComponent(params.kw || "") +
      (params.category ? "&category=" + params.category : "") + "&sort=" + e.target.value;
  });

  const doSearch = async (o) => {
    const q = new URLSearchParams();
    if (o.kw) q.set("keyword", o.kw);
    if (o.category) q.set("category", o.category);
    q.set("sort", o.sort || "");
    q.set("page", "1"); q.set("size", o.size || 12);
    try {
      const r = await LL.api("GET", "/api/search?" + q.toString());
      return r;
    } catch (e) { LL.toast(e.message, "err"); return null; }
  };

  if (!isSearch) {
    const [hot, fresh] = await Promise.all([
      doSearch({ sort: "popularity", size: 8 }),
      doSearch({ sort: "newest", size: 8 })
    ]);
    const hg = document.getElementById("hotGrid");
    if (hg) hg.innerHTML = hot ? drawMerchants(hot.list) : emptyBox("暂无数据");
    const ng = document.getElementById("newGrid");
    if (ng) ng.innerHTML = fresh ? drawMerchants(fresh.list) : emptyBox("暂无数据");
  } else {
    const r = await doSearch({ kw: params.kw, category: params.category, sort: params.sort || "", size: 30 });
    const list = document.getElementById("resultList");
    if (list) {
      if (!r) list.innerHTML = emptyBox("加载失败");
      else {
        const head = '<p class="text-muted" style="font-size:13px">共 ' + r.total + " 家店 · 第 " + r.page + " 页</p>";
        list.innerHTML = head + drawMerchants(r.list, true);
      }
    }
  }
};

function skeleton(n) {
  let s = "";
  for (let i = 0; i < n; i++) s += '<div class="m-card" style="height:232px;opacity:.5">' +
    '<div class="m-cover" style="background:#e9e2d2"></div><div class="m-body">' +
    '<div style="height:14px;width:60%;background:#eee5d2;border-radius:8px"></div>' +
    '<div style="height:10px;width:90%;background:#f1ead9;border-radius:6px;margin-top:6px"></div></div></div>';
  return s;
}

/* ---------------- 视图：商户详情 ---------------- */
const detailState = { page: 1, total: 0, type: "", merchant: 0, summary: null, focus: 0 };

LL.views.merchant = async function (el, id) {
  detailState.merchant = Number(id);
  detailState.type = "";
  detailState.summary = null;
  detailState.page = 1;
  detailState.total = 0;
  const _qi = location.hash.indexOf("?");
  detailState.focus = Number(new URLSearchParams(_qi >= 0 ? location.hash.slice(_qi + 1) : "").get("focus")) || 0;
  let m;
  try { m = await LL.api("GET", "/api/merchants/" + id); }
  catch (e) { el.innerHTML = '<div class="container-xl">' + emptyBox(e.message) + "</div>"; return; }

  const user = LL.user;
  const isOwner = !!(user && Number(user.id) === Number(m.user_id));
  const images = m.images ? String(m.images).split(",").filter(x => x) : [];
  const photo = images[0];
  const star = Number(m.avg_score) || 0;
  const statusDesc = { pending: "入驻审核中", approved: "营业中", rejected: "申请被驳回" };

  const actions = [];
  if (user && user.role === "consumer" && !isOwner) {
    actions.push('<button class="btn btn-ghost" id="favBtn"><i class="bi bi-heart"></i> 收藏</button>');
    actions.push('<button class="btn btn-ghost" id="folBtn"><i class="bi bi-bell"></i> 关注</button>');
  }
  actions.push('<a class="btn btn-ghost" href="javascript:history.back()"><i class="bi bi-arrow-left"></i> 返回</a>');

  let hero = `<section class="detail-hero">
    <span class="ghost">${LL.emoji(m.category_id)}</span>
    <div class="detail-title">
      <h1>${LL.esc(m.name)}</h1>
      <span class="tag green">${LL.esc(LL.cat(m.category_id))}</span>
      ${m.status === "approved"
        ? '<span class="tag gold"><i class="bi bi-check-circle"></i> 营业中</span>'
        : '<span class="tag red"><i class="bi bi-clock-history"></i> ' + (statusDesc[m.status] || m.status) + "</span>"}
    </div>
    <div class="detail-sub">
      <span><b style="font-family:var(--serif);font-size:1.5rem;color:#f6dfa8">${star > 0 ? star.toFixed(1) : "新"}</b>
        ${LL.starsHtml(star)} · ${Number(m.review_count) || 0} 条评价</span>
      <span><i class="bi bi-geo-alt"></i> ${LL.esc(m.area || "本城")}</span>
      <span><i class="bi bi-clock"></i> ${LL.esc(m.business_hours || "以店公告为准")}</span>
      <span><i class="bi bi-telephone"></i> ${LL.esc(m.phone || "—")}</span>
      <span><i class="bi bi-eye"></i> 浏览量 ${Number(m.view_count) || 0}</span>
    </div>
    <div class="d-flex flex-wrap gap-2" style="margin-top:1.2rem">${actions.join("")}</div>
    ${photo
      ? '<div class="cover-photo" style="background-image:url(\'' + LL.esc(photo) + '\');background-size:cover;background-position:center"></div>'
      : '<div class="cover-photo" style="' + LL.cover(m.category_id) + '"><span>' + LL.emoji(m.category_id) + "</span></div>"}
  </section>`;

  const stores = (m.stores || []).map(s =>
    '<div class="info-cell"><b><span class="badge-soft badge-' + LL.statusClass(s.status).replace("badge-", "") + '">' + LL.statusZh(s.status) + '</span> ' + LL.esc(s.name) +
    '</b><span>' + LL.esc(s.address || "—") + '</span>' +
    '<a class="small fw-bold" style="color:var(--green);text-decoration:none" href="#/talk/store/' + s.id + '">门店口碑 ›</a></div>').join("");
  const infoRow = `<div class="panel">
      <h5><i class="bi bi-shop"></i> 店铺档案</h5>
      <p class="text-muted" style="font-size:13.5px">${LL.esc(m.intro || "掌柜还没有留下介绍……")}</p>
      <div class="info-row">
        ${stores || '<div class="info-cell"><b>营业点</b><span>到店咨询</span></div>'}
        <div class="info-cell"><b>人均区间</b><span>${LL.money(m.price_min)} - ${LL.money(m.price_max || m.price_min)}</span></div>
        <div class="info-cell"><b>入驻时间</b><span>${LL.esc(String(m.created_at || "").slice(0, 10))}</span></div>
      </div></div>`;

  const canBuy = !!(user && user.role === "consumer" && !isOwner);
  const svcs = m.services || [];
  const svcStores = m.service_stores || {};
  const firstStoreOf = (map, id) => {
    const arr = map[String(id)] || [];
    return arr.length ? arr[0] : 0;
  };
  const svcHtml = svcs.length
    ? '<div class="panel" style="margin-top:1.1rem"><h5><i class="bi bi-list-check"></i> 招牌服务</h5>' +
      svcs.map(s => {
        const sid = firstStoreOf(svcStores, s.id);
        return '<div class="good-row"><div class="good-name"><b>' + LL.esc(s.name) + "</b><small>" +
          LL.esc(s.applicable_time || "适用时段不限") +
          (Number(s.stock) >= 0 ? " · 余量 " + s.stock : "") + "</small></div>" +
          '<span class="good-price">' + LL.money(s.price) + '</span>' +
          '<a class="btn btn-ghost btn-sm ms-2" href="#/talk/service/' + s.id + '">口碑评价 ›</a>' +
          (sid
            ? '<a class="btn btn-fire btn-sm ms-1" href="#/s/' + sid + '">' +
              (canBuy ? "选择门店下单" : "进店选购") + "</a>"
            : '<span class="text-muted small ms-2">暂未在门店上架</span>') +
          "</div>";
      }).join("") + "</div>"
    : "";

  const pkgs = m.packages || [];
  const pkgStores = m.package_stores || {};
  const pkgHtml = pkgs.length
    ? '<div class="panel" style="margin-top:1.1rem"><h5><i class="bi bi-gift"></i> 优惠套餐</h5>' +
      pkgs.map(p => {
        const sid = firstStoreOf(pkgStores, p.id);
        return '<div class="good-row"><div class="good-name"><b>' + LL.esc(p.name) + "</b><small>" +
          LL.esc(p.content || "") + (p.valid_days ? " · 有效期 " + p.valid_days + " 天" : "") + "</small></div>" +
          '<span class="good-price">' + LL.money(p.price) + '</span>' +
          '<a class="btn btn-ghost btn-sm ms-2" href="#/talk/package/' + p.id + '">口碑评价 ›</a>' +
          (sid
            ? '<a class="btn btn-fire btn-sm ms-1" href="#/s/' + sid + '">' +
              (canBuy ? "选择门店下单" : "进店选购") + "</a>"
            : '<span class="text-muted small ms-2">暂未在门店上架</span>') +
          "</div>";
      }).join("") + "</div>"
    : "";

  // 门店列表：消费者从这里进入门店选购页（下单主体为门店）
  const storeList = m.stores || [];
  const storeHtml = '<div class="panel" style="margin-top:1.1rem" id="storePanel"><h5><i class="bi bi-shop"></i> 门店（选择门店选购）</h5>' +
    (storeList.length
      ? storeList.map(st => {
          const os = st.on_sale || {};
          const desc = { open: "营业中", rest: "休息中", closed: "已打烊" }[st.status] || st.status;
          return '<div class="good-row"><div class="good-name"><b>' + LL.esc(st.name) + "</b><small>" +
            '<span class="badge-soft badge-' + LL.statusClass(st.status).replace("badge-", "") + '">' + desc + "</span> " +
            LL.esc(st.area || "本城") + (st.address ? " · " + LL.esc(st.address) : "") +
            " · 在售 服务 " + (os.services || 0) + " / 套餐 " + (os.packages || 0) + " / 活动 " + (os.coupons || 0) +
            "</small></div>" +
            '<a class="btn btn-main btn-sm ms-2" href="#/s/' + st.id + '">' +
            (canBuy ? "进店选购 ›" : "查看门店 ›") + "</a></div>";
        }).join("")
      : emptyBox("该店铺还没有门店")) + "</div>";

  // 评价表单（消费者，非店主）
  let form = "";
  if (user && user.role === "consumer" && !isOwner) {
    const dims = [["env_score", "环境", "🛋"], ["service_score", "服务", "🙋"], ["price_score", "性价比", "💰"]];
    let rows = "";
    dims.forEach(x => {
      let opts = "";
      for (let v = 5; v >= 1; v--) opts += '<option value="' + v + '">' + "★".repeat(v) + "☆".repeat(5 - v) + "</option>";
      rows += '<div class="col-4"><label class="form-label">' + x[2] + " " + x[1] + '</label>' +
        '<select name="' + x[0] + '" class="form-select">' + opts + "</select></div>";
    });
    form = '<div class="panel"><h5><i class="bi bi-pencil-square"></i> 写条评价</h5><form id="reviewForm">' +
      '<div class="row g-3">' + rows +
      '<div class="col-12"><label class="form-label">说说体验</label>' +
      '<textarea class="form-control" name="content" rows="3" maxlength="2000" placeholder="味道、环境、服务怎么样？"></textarea></div>' +
      '<div class="col-12"><label class="form-label">晒图（最多 6 张，jpg/png/gif/webp）</label>' +
      '<div class="d-flex gap-2 flex-wrap mb-1" id="pickWrap"></div>' +
      '<label class="btn btn-ghost btn-sm"><i class="bi bi-camera"></i> 选择图片' +
      '<input type="file" id="reviewImgs" accept="image/*" multiple hidden></label>' +
      '<div class="text-muted" style="font-size:12px">上传后立即保存，评价发布时自动带上图片</div></div>' +
      "</div><button class='btn btn-fire mt-3' type='submit'><i class='bi bi-send'></i> 发布评价</button></form></div>";
  } else if (!user) {
    form = '<div class="panel text-center"><p class="mb-2 text-muted">登录后即可收藏、评价这家店</p>' +
      '<button class="btn btn-main" onclick="LL.openAuth(\'login\')">去登录</button></div>';
  }

  el.innerHTML = '<div class="container-xl">' + hero + "</div>" +
    '<div class="container-xl">' +
    '<div class="row g-3 mt-2"><div class="col-lg-7">' + infoRow + storeHtml + svcHtml + pkgHtml + "</div>" +
    '<div class="col-lg-5">' + form +
    '<div class="cat-chips mb-2" id="revTabs"></div>' +
    '<div id="revHead"></div><div id="revList"></div>' +
    '<div id="revMoreWrap" class="text-center"></div></div></div></div>';

  // 收藏 / 关注状态与操作（消费者）
  if (user && user.role === "consumer" && !isOwner) {
    const fb = document.getElementById("favBtn"), fo = document.getElementById("folBtn");
    const paint = async () => {
      try {
        const [f1, f2] = await Promise.all([
          LL.api("GET", "/api/my/favorites?type=merchant"),
          LL.api("GET", "/api/my/follows")
        ]);
        const faved = f1.some(f => Number(f.merchant_id) === Number(id));
        const foled = f2.some(f => Number(f.merchant_id) === Number(id));
        if (fb) fb.innerHTML = faved
          ? '<i class="bi bi-heart-fill" style="color:var(--persimmon)"></i> 已收藏'
          : '<i class="bi bi-heart"></i> 收藏';
        if (fo) fo.innerHTML = foled
          ? '<i class="bi bi-bell-fill" style="color:var(--gold)"></i> 已关注'
          : '<i class="bi bi-bell"></i> 关注';
        fb.dataset.on = faved ? "1" : "0";
        fo.dataset.on = foled ? "1" : "0";
      } catch (e) {}
    };
    paint();
    if (fb) fb.addEventListener("click", async () => {
      try {
        if (fb.dataset.on === "1") { await LL.api("DELETE", "/api/favorite", { target_type: "merchant", target_id: Number(id) }); LL.toast("已取消收藏", "info"); }
        else { await LL.api("POST", "/api/favorite", { target_type: "merchant", target_id: Number(id) }); LL.toast("已收藏 ❤", "ok"); }
        await paint();
      } catch (e) { LL.toast(e.message, "err"); }
    });
    if (fo) fo.addEventListener("click", async () => {
      try {
        if (fo.dataset.on === "1") { await LL.api("DELETE", "/api/follow", { merchant_id: Number(id) }); LL.toast("已取消关注", "info"); }
        else { await LL.api("POST", "/api/follow", { merchant_id: Number(id) }); LL.toast("已关注商家 🔔", "ok"); }
        await paint();
      } catch (e) { LL.toast(e.message, "err"); }
    });

    // 领券与购买均已下沉到门店页（#/s/{id}）：
    // 商户详情页只做门店引导，避免绕过「门店是否参与该活动 / 是否上架该项目」的经营设置。
  }

  // 发表评价
  const rvForm = document.getElementById("reviewForm");
  if (rvForm) rvForm.addEventListener("submit", async e => {
    e.preventDefault();
    const f = rvForm;
    const btn = rvForm.querySelector("button[type=submit]");
    btn.disabled = true;
    try {
      await LL.api("POST", "/api/review", {
        merchant_id: Number(id), target_type: "merchant", target_id: Number(id),
        env_score: Number(f.env_score.value), service_score: Number(f.service_score.value),
        price_score: Number(f.price_score.value), content: f.content.value.trim(),
        images: window.__reviewImgs ? window.__reviewImgs.slice() : []
      });
      LL.toast("评价已发布，感谢分享！", "ok");
      window.__reviewImgs = [];
      const pw = document.getElementById("pickWrap");
      if (pw) pw.innerHTML = "";
      rvForm.reset();
      await reloadReviews(Number(id), 1);
    } catch (err) {
      LL.toast(err.message, "err");
      if (String(err.message).indexOf("已评价") >= 0) rvForm.remove();
    } finally { btn.disabled = false; }
  });

  // 评价图片上传器（talk.js 提供）
  if (window.ReviewPicker) ReviewPicker.init();

  await reloadReviews(Number(id), 1);
};

// 对象类型文案
const OBJ_LABEL = { merchant: "店铺", store: "门店", service: "服务项目", package: "优惠套餐" };

/* 评价列表（对象维度/翻页），配套评论区对象 tab */
async function reloadReviews(merchantId, page) {
  let data;
  try {
    data = await LL.api("GET", "/api/merchants/" + merchantId + "/reviews?type=" +
      encodeURIComponent(detailState.type || "") + "&page=" + page + "&size=5");
  } catch (e) { return; }
  detailState.page = page;
  detailState.total = data.total;
  await renderReviewTabs(merchantId);
  const head = document.getElementById("revHead");
  if (head) {
    const label = detailState.type ? ({ merchant: "店铺", store: "门店", service: "服务项目", package: "优惠套餐" }[detailState.type] || "对象") : "本店";
    head.innerHTML = secHead("💬 " + label + "评价" + (data.total ? "（" + data.total + "）" : ""));
  }
  const list = document.getElementById("revList");
  const more = document.getElementById("revMoreWrap");
  if (!list) return;
  list.innerHTML = data.list.length ? data.list.map(reviewCard).join("") : emptyBox("还没有评价，来做第一个分享的人吧！");
  if (more) more.innerHTML = page * 5 < data.total
    ? '<button class="btn btn-ghost btn-sm mt-1" id="moreRevBtn">加载更多评价 ↓</button>' : "";
  const mb = document.getElementById("moreRevBtn");
  if (mb) mb.addEventListener("click", () => reloadReviews(merchantId, detailState.page + 1));
  bindReviewActs(list);
  // 定位跳转：从「我的评价」等入口直达某条评价
  if (detailState.focus) {
    const card = list.querySelector('.review-card[data-id="' + detailState.focus + '"]');
    if (card) {
      card.style.boxShadow = "0 0 0 3px var(--gold)";
      setTimeout(() => card.scrollIntoView({ behavior: "smooth", block: "center" }), 250);
    }
    detailState.focus = 0;
  }
}

async function renderReviewTabs(merchantId) {
  const tabs = document.getElementById("revTabs");
  if (!tabs) return;
  if (!detailState.summary) {
    try {
      detailState.summary = await LL.api("GET", "/api/merchants/" + merchantId + "/reviews/summary");
    } catch (e) {
      detailState.summary = { total: 0, merchant: { cnt: 0 }, store: { cnt: 0 }, service: { cnt: 0 }, package: { cnt: 0 } };
    }
  }
  const s = detailState.summary || {};
  const defs = [["", "全部", s.total], ["merchant", "🏬 店铺", (s.merchant || {}).cnt],
  ["store", "🏪 门店", (s.store || {}).cnt], ["service", "🧾 服务", (s.service || {}).cnt],
  ["package", "🎁 套餐", (s.package || {}).cnt]];
  tabs.innerHTML = '<div class="cat-chips">' + defs.map(d =>
    '<button class="cat-chip' + (detailState.type === d[0] ? " on" : "") + '" data-t="' + d[0] + '">' +
    d[1] + (Number(d[2]) ? " " + d[2] : "") + "</button>").join("") + "</div>";
  tabs.querySelectorAll("[data-t]").forEach(b => b.addEventListener("click", () => {
    detailState.type = b.dataset.t;
    reloadReviews(merchantId, 1);
  }));
}

function reviewCard(r) {
  const isMine = LL.user && Number(LL.user.id) === Number(r.user_id);
  const dims = "环境 " + Number(r.env_score).toFixed(0) + "★ · 服务 " + Number(r.service_score).toFixed(0) +
    "★ · 性价比 " + Number(r.price_score).toFixed(0) + "★";
  const objLine = r.target_type ? '<div class="mb-1 d-flex align-items-center gap-2 flex-wrap" style="font-size:12.5px;color:var(--muted)">' +
    '<span class="badge-soft badge-pending" style="background:#efe6d2;color:#7a5f2a">' + (OBJ_LABEL[r.target_type] || "评价") + "</span>" +
    "<span>评价对象：<b>" + LL.esc(r.target_name || r.merchant_name || "") + "</b></span>" +
    '<a class="fw-bold" style="color:var(--green);text-decoration:none" href="#/talk/' + r.target_type + "/" + r.target_id + "?focus=" + r.id + '">进入评论区 ›</a></div>' : "";
  return '<div class="review-card" data-id="' + r.id + '">' +
    objLine +
    '<div class="rv-head">' + LL.avatarHtml({ nickname: r.nickname || r.username }) +
    '<div class="rv-author"><b>' + LL.esc(r.nickname || r.username) + "</b><span>@" + LL.esc(r.username) +
    " · " + LL.esc(String(r.created_at || "").slice(0, 16)) + "</span></div>" +
    '<div class="ms-auto text-end"><div class="score">' + Number(r.avg_score).toFixed(1) +
    '</div><div class="stars" style="font-size:12px">' + LL.starsHtml(r.avg_score) + "</div></div></div>" +
    '<div class="rate-tags mt-2"><span class="rate-pill" style="width:100%">' + dims + "</span></div>" +
    (r.content ? '<p class="rv-content">' + LL.esc(r.content) + "</p>" : "") +
    ((r.images || []).length ? '<div class="rv-imgs">' + r.images.map(i => '<img src="' + LL.esc(i) + '" loading="lazy" alt="">').join("") + "</div>" : "") +
    (r.merchant_reply ? '<div class="reply-box"><b>掌柜回复：</b>' + LL.esc(r.merchant_reply) + "</div>" : "") +
    '<div class="rv-foot">' +
    '<span class="act act-like" data-id="' + r.id + '"><i class="bi bi-hand-thumbs-up"></i> 有用 <b>' + Number(r.like_count) + "</b></span>" +
    '<span class="act act-cmt" data-id="' + r.id + '"><i class="bi bi-chat"></i> 评论 <b>' + Number(r.comment_count) + "</b></span>" +
    '<span class="act act-report" data-id="' + r.id + '"><i class="bi bi-flag"></i> 举报</span>' +
    (isMine || (LL.user && LL.user.role === "admin") ? '<span class="act act-del ms-auto" data-id="' + r.id + '"><i class="bi bi-trash"></i> 删除</span>' : "") +
    "</div></div>";
}

/* 评价行操作绑定（容器内点击委托） */
function bindReviewActs(list) {
  if (!list) return;
  list.addEventListener("click", async e => {
    const el = e.target.closest(".act");
    if (!el) return;
    const id = el.dataset.id;
    const card = el.closest(".review-card");
    if (el.classList.contains("act-like")) {
      if (!LL.user) { LL.toast("请先登录", "err"); LL.openAuth("login"); return; }
      if (LL.user.role === "admin") { LL.toast("管理员身份仅浏览，不支持点赞", "info"); return; }
      try {
        const r = await LL.api("POST", "/api/review/" + id + "/like");
        el.innerHTML = (r.liked ? '<i class="bi bi-hand-thumbs-up-fill"></i>' : '<i class="bi bi-hand-thumbs-up"></i>') +
          ' 有用 <b>' + el.querySelector("b").textContent + "</b>";
        const b = el.querySelector("b");
        b.textContent = Number(b.textContent) + (r.liked ? 1 : -1);
      } catch (err) { LL.toast(err.message, "err"); }
    } else if (el.classList.contains("act-cmt")) {
      if (window.ReviewPanel) ReviewPanel.toggle(card, id);
      else LL.toast("评论区组件未加载，请刷新页面", "info");
    } else if (el.classList.contains("act-report")) {
      if (!LL.user) { LL.toast("请先登录", "err"); LL.openAuth("login"); return; }
      if (LL.user.role === "admin") { LL.toast("管理员身份仅浏览，不支持举报", "info"); return; }
      const reason = window.prompt("请填写举报原因：");
      if (!reason || !reason.trim()) return;
      try {
        await LL.api("POST", "/api/review/" + id + "/report", { reason: reason.trim() });
        LL.toast("已提交举报，平台将尽快处理", "ok");
      } catch (err) { LL.toast(err.message, "err"); }
    } else if (el.classList.contains("act-del")) {
      if (!confirm("确定删除这条评价吗？")) return;
      try {
        await LL.api("DELETE", "/api/review/" + id);
        LL.toast("已删除", "ok");
        if (card) card.remove();
      } catch (err) { LL.toast(err.message, "err"); }
    }
  });
}
