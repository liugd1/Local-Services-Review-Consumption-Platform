/* 巷味 · 消费者个人中心 */
LL.views.me = async function (el, tab) {
  const user = LL.user;
  const tabs = [
    ["overview", "bi-person-badge", "资料与偏好"],
    ["favorites", "bi-heart", "我的收藏"],
    ["follows", "bi-bell", "关注的店"],
    ["history", "bi-clock-history", "浏览足迹"],
    ["consumption", "bi-receipt", "消费记录"],
    ["reviews", "bi-chat-square-quote", "我的评价"],
    ["coupons", "bi-ticket-perforated", "我的卡券"],
    ["orders", "bi-bag-check", "套餐订单"]
  ];
  const activeTab = tabs.some(t => t[0] === tab) ? tab : "overview";
  const links = tabs.map(t =>
    '<a class="ws-link' + (t[0] === activeTab ? " active" : "") + '" href="#/me?tab=' + t[0] + '">' +
    '<i class="bi ' + t[1] + '"></i>' + t[2] + "</a>").join("") +
    '<a class="ws-link logout" onclick="LL.logout()"><i class="bi bi-box-arrow-right"></i> 退出登录</a>';

  el.innerHTML = '<div class="container-xl"><div class="page-title"><span class="tick" style="display:inline-block;width:8px;height:26px;border-radius:4px;background:linear-gradient(180deg,#cf4a2b,#d99b2b)"></span>' +
    LL.esc(user.nickname || user.username) + " · 我的巷味</div>" +
    '<div class="workbench"><aside class="side-nav"><div class="side-user d-flex align-items-center gap-2">' +
    LL.avatarHtml(user) + '<div><b>' + LL.esc(user.nickname || user.username) + '</b><div class="small opacity-75">@' + LL.esc(user.username) + " · 食客</div></div></div>" +
    links + "</aside><div class='ws-main'><div class='panel text-center p-5'>加载中…</div></div></div></div>";

  const main = el.querySelector(".ws-main");
  const renderers = {
    overview: renderOverview, favorites: renderFavorites, follows: renderFollows,
    history: renderHistory, consumption: renderConsumption,
    coupons: renderMyCoupons, orders: renderMyOrders, reviews: renderReviews
  };
  try { await renderers[activeTab](main); } catch (e) { main.innerHTML = '<div class="panel">' + LL.esc(e.message) + "</div>"; }
};

async function renderFavorites(main) {
  let type = "merchant";
  main.innerHTML = '<div class="table-card"><div class="t-head"><h5>我的收藏</h5>' +
    '<div class="cat-chips"><button class="cat-chip on" data-t="merchant">商户</button><button class="cat-chip" data-t="service">服务</button></div></div>' +
    '<div class="p-3" id="favBody">加载中…</div></div>';
  const load = async () => {
    const body = main.querySelector("#favBody");
    try {
      const rows = await LL.api("GET", "/api/my/favorites?type=" + type);
      if (!rows.length) { body.innerHTML = emptyBox("还没有收藏，遇见喜欢的店就点亮 ❤ 吧"); return; }
      if (type === "merchant") {
        body.innerHTML = '<div class="merchant-grid">' +
          rows.map(r => merchantCard({ id: r.merchant_id, name: r.merchant_name, logo: r.logo,
            category_id: r.category_id || 0, area: r.area, price_min: r.price_min, price_max: r.price_max,
            status: r.status, avg_score: 0, review_count: 0 })).join("") + "</div>";
      } else {
        body.innerHTML = '<table class="table"><tbody>' + rows.map(r =>
          "<tr><td><b>" + LL.esc(r.service_name) + "</b></td><td>" + LL.esc(r.merchant_name) +
          "</td><td class='price-min'>" + LL.money(r.price) +
          '</td><td><a class="btn btn-ghost btn-sm" href="#/m/' + r.merchant_id + '">去商户</a></td></tr>').join("") + "</tbody></table>";
      }
    } catch (e) { body.innerHTML = '<div class="empty">' + LL.esc(e.message) + "</div>"; }
  };
  main.querySelectorAll("[data-t]").forEach(b => b.addEventListener("click", () => {
    type = b.dataset.t;
    main.querySelectorAll("[data-t]").forEach(x => x.classList.remove("on"));
    b.classList.add("on");
    load();
  }));
  await load();
}

async function renderFollows(main) {
  main.innerHTML = '<div class="panel"><h5><i class="bi bi-bell"></i> 关注的商户</h5><div id="folBody">加载中…</div></div>';
  const body = main.querySelector("#folBody");
  try {
    const rows = await LL.api("GET", "/api/my/follows");
    if (!rows.length) { body.innerHTML = emptyBox("还没有关注任何商户"); return; }
    body.innerHTML = '<div class="merchant-grid">' + rows.map(r => merchantCard({
      id: r.merchant_id, name: r.merchant_name, logo: r.logo, category_id: r.category_id || 0,
      area: r.area, price_min: r.price_min, price_max: r.price_max, status: r.status, avg_score: 0, review_count: 0
    })).join("") + "</div>";
  } catch (e) { body.innerHTML = '<div class="empty">' + LL.esc(e.message) + "</div>"; }
}

async function renderHistory(main) {
  main.innerHTML = '<div class="table-card"><div class="t-head"><h5>浏览足迹</h5></div><div class="p-3" id="hisBody">加载中…</div></div>';
  const body = main.querySelector("#hisBody");
  try {
    const rows = await LL.api("GET", "/api/my/history");
    if (!rows.length) { body.innerHTML = emptyBox("还没有浏览记录，去首页逛逛吧"); return; }
    body.innerHTML = rows.slice(0, 30).map(h => merchantRow({
      id: h.merchant_id, name: h.merchant_name, category_id: h.category_id || 0, area: h.area,
      avg_score: 0, review_count: 0, price_min: 0, price_max: 0,
      intro: "最近浏览 " + LL.esc(String(h.viewed_at || "").slice(0, 16)), business_hours: ""
    })).join("");
  } catch (e) { body.innerHTML = '<div class="empty">' + LL.esc(e.message) + "</div>"; }
}

async function renderOverview(main) {
  const [profile, cats] = await Promise.all([
    LL.api("GET", "/api/user/profile"), LL.api("GET", "/api/categories")
  ]);
  const u = profile.user, p = profile.preferences || {};
  const chosen = String(p.prefer_categories || "").split(",").filter(Boolean);
  const prices = [["", "不限"], ["0-50", "50 元以内"], ["50-100", "50-100 元"], ["100-200", "100-200 元"], ["200-500", "200-500 元"], ["500+", "500 元以上"]];
  main.innerHTML =
    '<div class="panel mb-3"><h5><i class="bi bi-person-badge"></i> 基本资料</h5><form id="profileForm" class="row g-3">' +
    '<div class="col-md-6"><label class="form-label">昵称</label><input class="form-control" name="nickname" value="' + LL.esc(u.nickname || "") + '"></div>' +
    '<div class="col-md-6"><label class="form-label">手机号</label><input class="form-control" name="phone" value="' + LL.esc(u.phone || "") + '"></div>' +
    '<div class="col-12 d-flex"><button class="btn btn-main ms-auto">保存资料</button></div></form></div>' +
    '<div class="panel"><h5><i class="bi bi-sliders"></i> 我的消费偏好</h5><form id="prefForm">' +
    '<div class="mb-3"><label class="form-label">偏好类别</label><div class="cat-chips" id="prefChips">' +
    cats.map(c => '<button type="button" class="cat-chip' + (chosen.includes(String(c.id)) ? " on" : "") + '" data-id="' + c.id + '">' + LL.emoji(c.id) + " " + LL.esc(c.name) + "</button>").join("") +
    '</div></div><div class="row g-3"><div class="col-md-6"><label class="form-label">单次预算</label><select name="price_range" class="form-select">' +
    prices.map(o => '<option value="' + o[0] + '"' + (String(p.price_range || "") === o[0] ? " selected" : "") + ">" + o[1] + "</option>").join("") +
    '</select></div><div class="col-md-6"><label class="form-label">常逛区域</label>' +
    '<input class="form-control" name="area" value="' + LL.esc(p.area || "") + '" placeholder="如：朝阳"></div>' +
    '<div class="col-12 d-flex"><button class="btn btn-fire ms-auto">保存偏好</button></div></div></form></div>';

  main.querySelector("#prefChips").addEventListener("click", e => {
    const b = e.target.closest(".cat-chip");
    if (b) b.classList.toggle("on");
  });
  main.querySelector("#profileForm").addEventListener("submit", async e => {
    e.preventDefault(); const f = e.target;
    try { await LL.api("PUT", "/api/user/profile", { nickname: f.nickname.value.trim(), phone: f.phone.value.trim() }); LL.toast("资料已更新", "ok"); } catch (err) { LL.toast(err.message, "err"); }
  });
  main.querySelector("#prefForm").addEventListener("submit", async e => {
    e.preventDefault(); const f = e.target;
    const ids = [...main.querySelectorAll("#prefChips .cat-chip.on")].map(b => Number(b.dataset.id));
    try {
      await LL.api("PUT", "/api/user/preferences", { prefer_categories: ids, price_range: f.price_range.value, area: f.area.value.trim() });
      LL.toast("偏好已保存", "ok");
    } catch (err) { LL.toast(err.message, "err"); }
  });
}

async function renderConsumption(main) {
  main.innerHTML = '<div class="panel mb-3"><h5><i class="bi bi-receipt-cutoff"></i> 记一笔到店消费</h5>' +
    '<form id="consForm" class="row g-3 align-items-end">' +
    '<div class="col-md-5"><label class="form-label">商户</label><select name="merchant_id" class="form-select" id="consMerchant"></select></div>' +
    '<div class="col-md-3"><label class="form-label">金额（元）</label><input class="form-control" name="amount" type="number" min="1" step="0.01" required></div>' +
    '<div class="col-md-3"><label class="form-label">消费时间</label><input class="form-control" name="consume_time" type="date"></div>' +
    '<div class="col-md-1"><button class="btn btn-main w-100"><i class="bi bi-plus-lg"></i></button></div></form></div>' +
    '<div class="table-card"><div class="t-head"><h5>消费账单</h5></div><div id="consBody" class="p-3">加载中…</div></div>';
  // 商户下拉（已上架商户）
  try {
    const m = await LL.api("GET", "/api/search?page=1&size=100");
    const sel = main.querySelector("#consMerchant");
    sel.innerHTML = m.list.map(x => '<option value="' + x.id + '">' + LL.esc(x.name) + "</option>").join("") || '<option value="">暂无商户</option>';
  } catch (e) {}

  const body = main.querySelector("#consBody");
  const load = async () => {
    try {
      const r = await LL.api("GET", "/api/my/consumptions?page=1&size=50");
      if (!r.total) { body.innerHTML = emptyBox("还没有消费记录，去店里消费后记一笔吧"); return; }
      body.innerHTML = '<table class="table"><thead><tr><th>商户</th><th>项目</th><th>金额</th><th>消费时间</th></tr></thead><tbody>' +
        r.list.map(c => "<tr><td>" + LL.esc(c.merchant_name) + "</td><td>" +
          LL.esc(c.service_name || c.package_name || "—") + "</td><td class='price-min'>" + LL.money(c.amount) +
          "</td><td>" + LL.esc(String(c.consume_time || "").slice(0, 16)) + "</td></tr>").join("") + "</tbody></table>";
    } catch (e) { body.innerHTML = '<div class="empty">' + LL.esc(e.message) + "</div>"; }
  };
  main.querySelector("#consForm").addEventListener("submit", async e => {
    e.preventDefault(); const f = e.target;
    try {
      await LL.api("POST", "/api/consume", {
        merchant_id: Number(f.merchant_id.value), amount: Number(f.amount.value),
        consume_time: f.consume_time.value ? f.consume_time.value + " 12:00:00" : ""
      });
      LL.toast("记账成功", "ok");
      f.reset();
      await load();
    } catch (err) { LL.toast(err.message, "err"); }
  });
  await load();
}

async function renderReviews(main) {
  main.innerHTML = '<div class="panel"><h5><i class="bi bi-chat-square-quote"></i> 我的评价</h5><div id="myRv">加载中…</div></div>';
  const body = main.querySelector("#myRv");
  try {
    const r = await LL.api("GET", "/api/my/reviews?page=1&size=20");
    if (!r.total) { body.innerHTML = emptyBox("还没有发表过评价"); return; }
    body.innerHTML = r.list.map(reviewCard).join("");
    bindReviewActs(body);
  } catch (e) { body.innerHTML = '<div class="empty">' + LL.esc(e.message) + "</div>"; }
}

/* 我的卡券 */
async function renderMyCoupons(main) {
  main.innerHTML = '<div class="table-card"><div class="t-head"><h5>我的卡券</h5>' +
    '<div class="cat-chips"><button class="cat-chip on" data-s="">全部</button>' +
    '<button class="cat-chip" data-s="unused">未使用</button>' +
    '<button class="cat-chip" data-s="used">已使用</button></div></div><div class="p-3" id="cpnBody"></div></div>';
  const body = main.querySelector("#cpnBody");
  const load = async () => {
    body.innerHTML = "加载中…";
    try {
      const rows = await LL.api("GET", "/api/my/coupons?status=all");
      if (!rows.length) { body.innerHTML = emptyBox("还没有领取过优惠券，去商户详情页领一张吧"); return; }
      const show = rows.filter(r => !filter || r.claim_status === filter);
      if (!show.length) { body.innerHTML = emptyBox("没有符合条件的卡券"); return; }
      body.innerHTML = show.map(r => {
        const cs = r.claim_status;
        const valueDesc = r.type === "discount"
          ? (Number(r.discount_rate) * 10).toFixed(1) + " 折"
          : "¥" + LL.money(r.face_value);
        const limitDesc = Number(r.threshold) ? "满 " + LL.money(r.threshold) + " 可用" : "无门槛";
        return '<div class="panel mb-3"><div class="d-flex align-items-center gap-3 flex-wrap">' +
          '<div style="width:90px;height:96px;flex:0 0 auto;border-radius:14px;background:linear-gradient(160deg,#b43a1e,#cf4a2b);color:#fff;display:flex;flex-direction:column;align-items:center;justify-content:center">' +
          '<div style="font-size:17px;font-weight:800">' + valueDesc + "</div>" +
          '<div style="font-size:11px;opacity:.85">' + (r.type === "discount" ? "折扣券" : "优惠券") + "</div></div>" +
          '<div style="flex:1;min-width:220px"><div class="d-flex align-items-center gap-2 flex-wrap"><b>' + LL.esc(r.name) +
          '</b><span class="badge-soft badge-' + LL.statusClass(cs).replace("badge-", "") + '">' + LL.statusZh(cs) + "</span></div>" +
          '<div class="text-muted" style="font-size:12.5px">' + LL.esc(r.merchant_name) + " · " + limitDesc +
          " · 有效期至 " + LL.esc(String(r.end_time || "").slice(0, 10)) + "</div>" +
          '<div class="mt-1"><code style="font-size:13px;letter-spacing:2px">' + LL.esc(r.code) + "</code>" +
          '<span class="text-muted small ms-2">到店向商家出示核销码</span></div></div>' +
          (cs === "used" ? '<div class="text-muted" style="font-size:12px;white-space:nowrap">已核销 ' +
            LL.esc(String(r.used_time || "").slice(0, 16)) + "</div>" : "") +
          "</div></div>";
      }).join("");
    } catch (e) { body.innerHTML = '<div class="empty">' + LL.esc(e.message) + "</div>"; }
  };
  let filter = "";
  main.querySelectorAll("[data-s]").forEach(b => b.addEventListener("click", () => {
    filter = b.dataset.s;
    main.querySelectorAll("[data-s]").forEach(x => x.classList.remove("on"));
    b.classList.add("on");
    load();
  }));
  await load();
}

/* 套餐订单 */
async function renderMyOrders(main) {
  main.innerHTML = '<div class="table-card"><div class="t-head"><h5>套餐订单</h5></div><div class="p-3" id="odBody"></div></div>';
  const body = main.querySelector("#odBody");
  const load = async () => {
    body.innerHTML = "加载中…";
    try {
      const d = await LL.api("GET", "/api/my/orders?page=1&size=50");
      if (!d.total) { body.innerHTML = emptyBox("还没有购买过套餐"); return; }
      body.innerHTML = '<table class="table"><thead><tr><th>订单号</th><th>套餐</th><th>商户</th><th>金额</th><th>下单时间</th><th>状态</th><th style="width:190px">操作</th></tr></thead><tbody>' +
        d.list.map(o => {
          const acts = o.status === "purchased"
            ? '<button class="btn btn-main btn-sm" data-use="' + o.id + '">去使用</button> ' +
              '<button class="btn btn-ghost btn-sm" data-ref="' + o.id + '">退款</button>'
            : '<span class="text-muted small">—</span>';
          return '<tr><td class="text-muted small">' + LL.esc(o.order_no) + '</td><td><b>' + LL.esc(o.package_name) + '</b></td><td>' + LL.esc(o.merchant_name) +
            '</td><td class="price-min">' + LL.money(o.amount) + '</td><td class="text-muted" style="font-size:12px">' + LL.esc(String(o.created_at || "").slice(0, 16)) +
            '</td><td><span class="badge-soft badge-' + LL.statusClass(o.status).replace("badge-", "") + '">' + LL.statusZh(o.status) + "</span></td><td>" + acts + "</td></tr>";
        }).join("") + "</tbody></table>";
      body.querySelectorAll("[data-use]").forEach(b => b.addEventListener("click", async () => {
        if (!confirm("确认到店核销该套餐？将自动记录一条消费。")) return;
        try { await LL.api("POST", "/api/order/" + b.dataset.use + "/use"); LL.toast("已核销，消费记录已同步", "ok"); await load(); }
        catch (e) { LL.toast(e.message, "err"); }
      }));
      body.querySelectorAll("[data-ref]").forEach(b => b.addEventListener("click", async () => {
        if (!confirm("确认申请退款？")) return;
        try { await LL.api("POST", "/api/order/" + b.dataset.ref + "/refund"); LL.toast("已退款", "ok"); await load(); }
        catch (e) { LL.toast(e.message, "err"); }
      }));
    } catch (e) { body.innerHTML = '<div class="empty">' + LL.esc(e.message) + "</div>"; }
  };
  await load();
}

