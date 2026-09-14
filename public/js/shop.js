/* 巷味 · 商户端：入驻申请 + 经营工作台 */

/* ---- 经营台公共 ---- */
async function loadMyMerchant() {
  try { return await LL.api("GET", "/api/merchant/me"); }
  catch (e) { return null; }
}

function statusBanner(m) {
  const map = {
    pending: ["待审核", "平台正在审核你的入驻资料，通常很快就好。", "badge-pending"],
    approved: ["经营中", "你的店铺已对街坊们可见。", "badge-approved"],
    rejected: ["未通过", "很遗憾，入驻申请未通过。可在「商户资料」修改后重新提交。", "badge-rejected"]
  };
  const x = map[m.status] || [m.status, "", "badge-rest"];
  return '<div class="panel mb-3 d-flex align-items-center gap-3"><span class="badge-soft ' + x[2] + '">' + x[0] +
    "</span><div><b>" + LL.esc(m.name) + "</b><p class='mb-0 text-muted' style='font-size:13px'>" + x[1] + "</p></div></div>";
}

LL.views.shop = async function (el, tab) {
  const data = await loadMyMerchant();
  if (!data) { el.innerHTML = '<div class="container-xl">' + emptyBox("暂未入驻商户，请先提交入驻申请") + "</div>"; return; }
  const m = data.merchant;
  // 未通过审核时只开放「经营概览」（完善资料 / 查看审核状态），
  // 门店、服务、套餐、优惠活动、回复评价、统计均需审核通过后使用
  const approved = m.status === "approved";
  const allTabs = [["overview", "bi-speedometer2", "经营概览"], ["stores", "bi-shop", "门店管理"],
    ["services", "bi-list-check", "服务项目"], ["packages", "bi-gift", "优惠套餐"],
    ["coupons", "bi-ticket-perforated", "优惠活动"], ["reply", "bi-chat-left-text", "回复评价"],
    ["stats", "bi-bar-chart-line", "经营统计"]];
  const tabs = approved ? allTabs : allTabs.filter(t => t[0] === "overview");
  const active = tabs.some(t => t[0] === tab) ? tab : "overview";
  const links = tabs.map(t =>
    '<a class="ws-link' + (t[0] === active ? " active" : "") + '" href="#/shop?tab=' + t[0] + '"><i class="bi ' + t[1] + '"></i>' + t[2] + "</a>").join("") +
    '<a class="ws-link logout" onclick="LL.logout()"><i class="bi bi-box-arrow-right"></i> 退出登录</a>';
  el.innerHTML = '<div class="container-xl"><div class="page-title"><span class="tick" style="display:inline-block;width:8px;height:26px;border-radius:4px;background:linear-gradient(180deg,#cf4a2b,#d99b2b)"></span>' +
    LL.esc(m.name) + " · 掌柜工作台</div><div class='workbench'><aside class='side-nav'>" + links +
    "</aside><div class='ws-main'><div class='panel text-center p-5'>加载中…</div></div></div></div>";
  const main = el.querySelector(".ws-main");
  const renderers = { overview: shopOverview, stores: shopStores, services: shopServices, packages: shopPackages,
    coupons: shopCoupons, reply: shopReply, stats: shopStats };
  try { await renderers[active](main, data); } catch (e) { main.innerHTML = '<div class="panel">' + LL.esc(e.message) + "</div>"; }
};

/* 概览 + 资料编辑 */
async function shopOverview(main, data) {
  const m = data.merchant, cats = await LL.api("GET", "/api/categories");
  main.innerHTML = statusBanner(m) +
    (m.status !== "approved"
      ? '<div class="panel mb-3"><b><i class="bi bi-info-circle"></i> 审核通过后开放经营功能</b>' +
        '<p class="mb-0 text-muted" style="font-size:13px">' +
        (m.status === "rejected"
          ? "请在下方核对并修改店铺资料，然后点击「保存并重新提交审核」重新提交。"
          : "你可以在下方核对并修改店铺资料；平台审核通过后，即可管理门店 / 服务 / 套餐、发布优惠活动并查看经营统计。") +
        "</p></div>"
      : "") +
    '<div class="stat-cards"><div class="stat-card"><i class="bi bi-eye ic"></i><b>' + (m.view_count || 0) +
    '</b><span>总浏览量</span></div><div class="stat-card"><i class="bi bi-chat-square-text ic"></i><b>' + (data.reviews ? "—" : "—") +
    '</b><span>评价（在店铺页查看）</span></div><div class="stat-card"><i class="bi bi-list-check ic"></i><b>' + (data.services || []).length +
    '</b><span>服务项目</span></div><div class="stat-card"><i class="bi bi-gift ic"></i><b>' + (data.packages || []).length +
    '</b><span>优惠套餐</span></div></div>' +
    '<div class="panel"><h5><i class="bi bi-pencil"></i> 商户资料</h5><form id="shopInfoForm" class="row g-3">' +
    '<div class="col-md-6"><label class="form-label">店铺名称 *</label><input class="form-control" name="name" value="' + LL.esc(m.name) + '" required></div>' +
    '<div class="col-md-6"><label class="form-label">类别 *</label><select class="form-select" name="category_id">' +
    cats.map(c => '<option value="' + c.id + '"' + (Number(m.category_id) === c.id ? " selected" : "") + ">" + LL.esc(c.name) + "</option>").join("") + "</select></div>" +
    '<div class="col-md-4"><label class="form-label">所在区域</label><input class="form-control" name="area" value="' + LL.esc(m.area || "") + '" placeholder="如：朝阳"></div>' +
    '<div class="col-md-4"><label class="form-label">营业时间</label><input class="form-control" name="business_hours" value="' + LL.esc(m.business_hours || "") + '" placeholder="如 09:00-22:00"></div>' +
    '<div class="col-md-4"><label class="form-label">联系电话</label><input class="form-control" name="phone" value="' + LL.esc(m.phone || "") + '"></div>' +
    '<div class="col-md-3"><label class="form-label">人均下限</label><input class="form-control" name="price_min" type="number" value="' + (m.price_min || 0) + '"></div>' +
    '<div class="col-md-3"><label class="form-label">人均上限</label><input class="form-control" name="price_max" type="number" value="' + (m.price_max || 0) + '"></div>' +
    '<div class="col-md-6"><label class="form-label">一句话介绍</label><input class="form-control" name="intro" value="' + LL.esc(m.intro || "") + '"></div>' +
    '<div class="col-12 d-flex gap-2">' +
    (m.status === "rejected" ? '<button class="btn btn-fire" type="submit" name="reapply">保存并重新提交审核</button>' : '<button class="btn btn-main" type="submit">保存资料</button>') +
    "</div></form></div>";

  const f = main.querySelector("#shopInfoForm");
  f.addEventListener("submit", async e => {
    e.preventDefault();
    const b = e.submitter;
    try {
      await LL.api("PUT", "/api/merchant/me", {
        name: f.name.value.trim(), category_id: f.category_id.value, area: f.area.value.trim(),
        business_hours: f.business_hours.value.trim(), phone: f.phone.value.trim(),
        price_min: Number(f.price_min.value), price_max: Number(f.price_max.value), intro: f.intro.value.trim()
      });
      if (b && b.name === "reapply") {
        await LL.api("POST", "/api/merchant/apply", {
          name: f.name.value.trim(), category_id: f.category_id.value, area: f.area.value.trim(),
          business_hours: f.business_hours.value.trim(), phone: f.phone.value.trim(),
          price_min: Number(f.price_min.value), price_max: Number(f.price_max.value), intro: f.intro.value.trim()
        });
      }
      LL.toast(b && b.name === "reapply" ? "已重新提交审核" : "资料已保存", "ok");
      setTimeout(() => location.reload(), 800);
    } catch (err) { LL.toast(err.message, "err"); }
  });
}

/* 门店管理 */
async function shopStores(main, data) {
  const rows = data.stores || [];
  const badge = s => '<span class="badge-soft badge-' + LL.statusClass(s).replace("badge-", "") + '">' + LL.statusZh(s) + "</span>";
  main.innerHTML = '<div class="table-card"><div class="t-head"><h5>门店管理</h5>' +
    '<button class="btn btn-main btn-sm" onclick="LL.openStoreForm()"><i class="bi bi-plus-lg"></i> 新增门店</button></div>' +
    '<table class="table"><thead><tr><th>门店</th><th>地址</th><th>区域</th><th>状态</th><th style="width:230px">操作</th></tr></thead><tbody>' +
    (rows.map(s => "<tr><td><b>" + LL.esc(s.name) + "</b></td><td>" + LL.esc(s.address || "—") + "</td><td>" + LL.esc(s.area || "—") +
      "</td><td>" + badge(s.status) + '</td><td><div class="btn-group btn-group-sm">' +
      '<button class="btn btn-ghost" data-edit="' + s.id + '">编辑</button>' +
      '<button class="btn btn-ghost text-danger" data-del="' + s.id + '">删除</button></div></td></tr>').join("") ||
      '<tr><td colspan="5">' + emptyBox("还没有门店，先新增一家吧") + "</td></tr>") +
    "</tbody></table></div>" +
    '<div class="panel mt-3" id="storeFormBox" style="display:none"><h5 id="storeFormTitle">新增门店</h5>' +
    '<form id="storeForm" class="row g-3"><input type="hidden" name="id">' +
    '<div class="col-md-6"><label class="form-label">门店名称 *</label><input class="form-control" name="name"></div>' +
    '<div class="col-md-6"><label class="form-label">状态</label><select class="form-select" name="status">' +
    '<option value="open">营业中</option><option value="rest">休息中</option><option value="closed">已打烊</option></select></div>' +
    '<div class="col-md-6"><label class="form-label">地址</label><input class="form-control" name="address"></div>' +
    '<div class="col-md-6"><label class="form-label">区域</label><input class="form-control" name="area"></div>' +
    '<div class="col-12"><button class="btn btn-main">保存</button> <button type="button" class="btn btn-ghost" id="storeCancel">取消</button></div>' +
    "</form></div>";

  const box = main.querySelector("#storeFormBox");
  const open = s => {
    box.style.display = "";
    const form = main.querySelector("#storeForm");
    form.reset();
    form.id.value = s ? s.id : "";
    if (s) { form.name.value = s.name; form.address.value = s.address || ""; form.area.value = s.area || ""; form.status.value = s.status || "open"; }
    main.querySelector("#storeFormTitle").textContent = s ? "编辑门店" : "新增门店";
  };
  LL.openStoreForm = () => open(null);
  main.querySelector("#storeCancel").addEventListener("click", () => box.style.display = "none");
  main.querySelectorAll("[data-edit]").forEach(b => b.addEventListener("click", () => open(rows.find(r => r.id === Number(b.dataset.edit)))));
  main.querySelectorAll("[data-del]").forEach(b => b.addEventListener("click", async () => {
    if (!confirm("删除该门店？（其下服务将改为到店服务）")) return;
    try { await LL.api("DELETE", "/api/merchant/stores/" + b.dataset.del); LL.toast("已删除", "ok"); location.reload(); }
    catch (e) { LL.toast(e.message, "err"); }
  }));
  main.querySelector("#storeForm").addEventListener("submit", async e => {
    e.preventDefault(); const f = e.target;
    const body = { name: f.name.value.trim(), address: f.address.value.trim(), area: f.area.value.trim(), status: f.status.value };
    try {
      if (f.id.value) await LL.api("PUT", "/api/merchant/stores/" + f.id.value, body);
      else await LL.api("POST", "/api/merchant/stores", body);
      LL.toast("已保存", "ok"); location.reload();
    } catch (err) { LL.toast(err.message, "err"); }
  });
}

/* 服务项目管理 */
async function shopServices(main, data) {
  const rows = data.services || [];
  const badge = s => '<span class="badge-soft badge-' + LL.statusClass(s).replace("badge-", "") + '">' + (s === "on" ? "已上架" : "已下架") + "</span>";
  main.innerHTML = '<div class="table-card"><div class="t-head"><h5>服务项目</h5>' +
    '<button class="btn btn-main btn-sm" id="svcAdd"><i class="bi bi-plus-lg"></i> 新增服务</button></div>' +
    '<table class="table"><thead><tr><th>服务</th><th>价格</th><th>时段/门店</th><th>状态</th><th style="width:230px">操作</th></tr></thead><tbody>' +
    (rows.map(s => "<tr><td><b>" + LL.esc(s.name) + "</b></td><td class='price-min'>" + LL.money(s.price) +
      "</td><td class='text-muted' style='font-size:12px'>" + LL.esc(s.applicable_time || "不限") + (s.store_name ? " · " + LL.esc(s.store_name) : "") + "</td><td>" + badge(s.status) +
      '</td><td><div class="btn-group btn-group-sm">' +
      '<button class="btn btn-ghost" data-edit="' + s.id + '">编辑</button>' +
      '<button class="btn btn-ghost" data-status="' + s.id + '" data-cur="' + s.status + '">' + (s.status === "on" ? "下架" : "上架") + "</button></div></td></tr>").join("") ||
      '<tr><td colspan="5">' + emptyBox("还没有服务项目") + "</td></tr>") +
    "</tbody></table></div>" +
    '<div class="panel mt-3" id="svcFormBox" style="display:none"><h5 id="svcFormTitle">新增服务</h5>' +
    '<form id="svcForm" class="row g-3"><input type="hidden" name="id">' +
    '<div class="col-md-6"><label class="form-label">服务名称 *</label><input class="form-control" name="name"></div>' +
    '<div class="col-md-3"><label class="form-label">价格（元）</label><input class="form-control" name="price" type="number" step="0.01" min="0"></div>' +
    '<div class="col-md-3"><label class="form-label">计价单位</label><input class="form-control" name="price_unit" placeholder="如：次/位"></div>' +
    '<div class="col-md-6"><label class="form-label">适用时间</label><input class="form-control" name="applicable_time" placeholder="如：工作日 14:00-17:00"></div>' +
    '<div class="col-md-3"><label class="form-label">库存（-1 不限）</label><input class="form-control" name="stock" type="number" value="-1"></div>' +
    '<div class="col-md-3"><label class="form-label">每人限购</label><input class="form-control" name="limit_count" type="number" value="0"></div>' +
    '<div class="col-12"><button class="btn btn-main">保存</button> <button type="button" class="btn btn-ghost" id="svcCancel">取消</button></div></form></div>';

  const box = main.querySelector("#svcFormBox");
  const open = s => {
    box.style.display = "";
    const form = main.querySelector("#svcForm");
    form.reset();
    form.id.value = s ? s.id : "";
    if (s) {
      form.name.value = s.name; form.price.value = s.price; form.price_unit.value = s.price_unit || "";
      form.applicable_time.value = s.applicable_time || ""; form.stock.value = s.stock; form.limit_count.value = s.limit_count || 0;
    }
    main.querySelector("#svcFormTitle").textContent = s ? "编辑服务" : "新增服务";
  };
  main.querySelector("#svcAdd").addEventListener("click", () => open(null));
  main.querySelector("#svcCancel").addEventListener("click", () => box.style.display = "none");
  main.querySelectorAll("[data-edit]").forEach(b => b.addEventListener("click", () => open(rows.find(r => r.id === Number(b.dataset.edit)))));
  main.querySelectorAll("[data-status]").forEach(b => b.addEventListener("click", async () => {
    try {
      await LL.api("PUT", "/api/merchant/services/" + b.dataset.status + "/status", { status: b.dataset.cur === "on" ? "off" : "on" });
      LL.toast("已更新", "ok"); location.reload();
    } catch (e) { LL.toast(e.message, "err"); }
  }));
  main.querySelector("#svcForm").addEventListener("submit", async e => {
    e.preventDefault(); const f = e.target;
    const body = { name: f.name.value.trim(), price: Number(f.price.value), price_unit: f.price_unit.value.trim(),
      applicable_time: f.applicable_time.value.trim(), stock: Number(f.stock.value), limit_count: Number(f.limit_count.value) };
    try {
      if (f.id.value) await LL.api("PUT", "/api/merchant/services/" + f.id.value, body);
      else await LL.api("POST", "/api/merchant/services", body);
      LL.toast("已保存", "ok"); location.reload();
    } catch (err) { LL.toast(err.message, "err"); }
  });
}

/* 优惠套餐管理 */
async function shopPackages(main, data) {
  const rows = data.packages || [];
  const badge = s => '<span class="badge-soft badge-' + LL.statusClass(s).replace("badge-", "") + '">' + (s === "on" ? "已上架" : "已下架") + "</span>";
  main.innerHTML = '<div class="table-card"><div class="t-head"><h5>优惠套餐</h5>' +
    '<button class="btn btn-main btn-sm" id="pkgAdd"><i class="bi bi-plus-lg"></i> 新增套餐</button></div>' +
    '<table class="table"><thead><tr><th>套餐</th><th>内容</th><th>价格</th><th>状态</th><th style="width:230px">操作</th></tr></thead><tbody>' +
    (rows.map(p => "<tr><td><b>" + LL.esc(p.name) + "</b></td><td class='text-muted' style='font-size:12px'>" + LL.esc((p.content || "").slice(0, 30)) +
      "</td><td class='price-min'>" + LL.money(p.price) + "</td><td>" + badge(p.status) +
      '</td><td><div class="btn-group btn-group-sm">' +
      '<button class="btn btn-ghost" data-edit="' + p.id + '">编辑</button>' +
      '<button class="btn btn-ghost" data-status="' + p.id + '" data-cur="' + p.status + '">' + (p.status === "on" ? "下架" : "上架") + "</button></div></td></tr>").join("") ||
      '<tr><td colspan="5">' + emptyBox("还没有套餐，加一个招牌套餐吧") + "</td></tr>") +
    "</tbody></table></div>" +
    '<div class="panel mt-3" id="pkgFormBox" style="display:none"><h5 id="pkgFormTitle">新增套餐</h5>' +
    '<form id="pkgForm" class="row g-3"><input type="hidden" name="id">' +
    '<div class="col-md-6"><label class="form-label">套餐名称 *</label><input class="form-control" name="name"></div>' +
    '<div class="col-md-3"><label class="form-label">价格（元）</label><input class="form-control" name="price" type="number" step="0.01" min="0"></div>' +
    '<div class="col-md-3"><label class="form-label">有效期（天）</label><input class="form-control" name="valid_days" type="number" value="30"></div>' +
    '<div class="col-12"><label class="form-label">套餐内容</label><textarea class="form-control" name="content" rows="2"></textarea></div>' +
    '<div class="col-12"><button class="btn btn-main">保存</button> <button type="button" class="btn btn-ghost" id="pkgCancel">取消</button></div></form></div>';

  const box = main.querySelector("#pkgFormBox");
  const open = p => {
    box.style.display = "";
    const form = main.querySelector("#pkgForm");
    form.reset();
    form.id.value = p ? p.id : "";
    if (p) { form.name.value = p.name; form.price.value = p.price; form.valid_days.value = p.valid_days; form.content.value = p.content || ""; }
    main.querySelector("#pkgFormTitle").textContent = p ? "编辑套餐" : "新增套餐";
  };
  main.querySelector("#pkgAdd").addEventListener("click", () => open(null));
  main.querySelector("#pkgCancel").addEventListener("click", () => box.style.display = "none");
  main.querySelectorAll("[data-edit]").forEach(b => b.addEventListener("click", () => open(rows.find(r => r.id === Number(b.dataset.edit)))));
  main.querySelectorAll("[data-status]").forEach(b => b.addEventListener("click", async () => {
    try {
      await LL.api("PUT", "/api/merchant/packages/" + b.dataset.status + "/status", { status: b.dataset.cur === "on" ? "off" : "on" });
      LL.toast("已更新", "ok"); location.reload();
    } catch (e) { LL.toast(e.message, "err"); }
  }));
  main.querySelector("#pkgForm").addEventListener("submit", async e => {
    e.preventDefault(); const f = e.target;
    const body = { name: f.name.value.trim(), price: Number(f.price.value), valid_days: Number(f.valid_days.value), content: f.content.value.trim() };
    try {
      if (f.id.value) await LL.api("PUT", "/api/merchant/packages/" + f.id.value, body);
      else await LL.api("POST", "/api/merchant/packages", body);
      LL.toast("已保存", "ok"); location.reload();
    } catch (err) { LL.toast(err.message, "err"); }
  });
}

/* 回复评价 */
async function shopReply(main, data) {
  const mid = data.merchant.id;
  let rows = [];
  try { const r = await LL.api("GET", "/api/merchants/" + mid + "/reviews?page=1&size=50"); rows = r.list; }
  catch (e) { rows = []; }
  if (!rows.length) {
    main.innerHTML = '<div class="panel">' + emptyBox("还没有收到评价，等第一位街坊的光临吧") + "</div>";
    return;
  }
  main.innerHTML = '<div class="table-card"><div class="t-head"><h5>评价与回复（' + rows.length + '）</h5></div><div class="p-3">' +
    rows.map(r => '<div class="panel mb-3"><div class="d-flex align-items-center gap-2 mb-2">' + LL.avatarHtml(r) +
      "<div><b>" + LL.esc(r.nickname || r.username) + "</b> <span class='text-muted' style='font-size:12px'>" + LL.starsHtml(r.avg_score) + " " + LL.esc(String(r.created_at || "").slice(0, 10)) + "</span></div>" +
      '<span class="score ms-auto">' + Number(r.avg_score).toFixed(1) + "</span></div>" +
      '<p class="mb-2">' + LL.esc(r.content || "") + "</p>" +
      (r.merchant_reply
        ? '<div class="reply-box mb-2"><b>掌柜已回复：</b>' + LL.esc(r.merchant_reply) + "</div>"
        : '<span class="badge-soft badge-pending">待回复</span>') +
      '<form class="row g-2 mt-2" data-reply="' + r.id + '"><div class="col"><input class="form-control" name="content" placeholder="' +
      (r.merchant_reply ? "修改你的回复…" : "回复这条评价…") + '" value="' + LL.esc(r.merchant_reply || "") + '"></div>' +
      '<div class="col-auto"><button class="btn btn-main btn-sm">' + (r.merchant_reply ? "更新回复" : "回复") + "</button></div></form></div>").join("") +
    "</div></div>";
  main.querySelectorAll("[data-reply]").forEach(form => form.addEventListener("submit", async e => {
    e.preventDefault();
    try {
      await LL.api("POST", "/api/merchant/review/" + form.dataset.reply + "/reply", { content: form.content.value.trim() });
      LL.toast("回复成功", "ok");
      location.reload();
    } catch (err) { LL.toast(err.message, "err"); }
  }));
}

/* 同步当前用户资料（入驻后角色变为 merchant，需刷新本地登录态） */
async function refreshStoredUser() {
  try {
    const p = await LL.api("GET", "/api/user/profile");
    const u = p.user;
    if (u && LL.token) LL.setAuth(LL.token, u);
  } catch (e) {}
}

/* ---- 入驻申请 ---- */
LL.views.apply = async function (el) {
  const user = LL.user;
  if (!user) { el.innerHTML = ""; LL.openAuth("login"); return; }
  if (user.role === "admin") {
    el.innerHTML = '<div class="container-xl">' + emptyBox("平台管理员账号不能申请入驻，请使用普通用户身份提交入驻申请") +
      '<div class="text-center mt-3"><a class="btn btn-main" href="#/admin">返回平台管理</a></div></div>';
    return;
  }
  let existed = null, msg = "";
  if (user.role === "merchant") {
    try {
      const d = await loadMyMerchant();
      if (d) {
        existed = d.merchant;
        if (existed.status === "approved") {
          el.innerHTML = '<div class="container-xl"><div class="panel text-center p-5"><span class="ic" style="font-size:46px">✅</span><h3 style="font-family:var(--kai)">你已经是「' +
            LL.esc(existed.name) + '」的掌柜啦</h3><p class="text-muted">去经营台管理你的店铺吧</p>' +
            '<a class="btn btn-main" href="#/shop">进入经营台</a></div></div>';
          return;
        }
        if (existed.status === "pending") {
          el.innerHTML = '<div class="container-xl"><div class="panel text-center p-5"><span class="ic" style="font-size:46px">⏳</span><h3 style="font-family:var(--kai)">入驻申请审核中</h3>' +
            '<p class="text-muted">' + LL.esc(existed.name) + " 正在等待平台审核，通过后即可开店经营。</p>" +
            '<a class="btn btn-ghost" href="#/shop">返回经营台</a></div></div>';
          return;
        }
        if (existed.status === "rejected") msg = existed.reject_reason || "资料未通过审核";
      }
    } catch (e) {}
  }
  const cats = await LL.api("GET", "/api/categories");
  const pre = existed || {};
  el.innerHTML = '<div class="container-xl" style="max-width:880px">' +
    '<div class="page-title"><span class="tick" style="display:inline-block;width:8px;height:26px;border-radius:4px;background:linear-gradient(180deg,#cf4a2b,#d99b2b)"></span>开店入驻</div>' +
    (existed && existed.status === "rejected"
      ? '<div class="panel mb-3" style="border-color:#e8b4a2"><b style="color:#b43a1e">上次申请被驳回</b><p class="mb-0 text-muted" style="font-size:13px">原因：' + LL.esc(msg) + "，请修改后重新提交</p></div>" : "") +
    '<div class="panel"><p class="text-muted" style="font-size:13px">填写你的店铺信息，提交后由平台审核，一般当天即可通过。<b>请在商户类别、电话、价格区间中填写真实信息。</b></p>' +
    '<form id="applyForm" class="row g-3">' +
    '<div class="col-md-8"><label class="form-label">店铺名称 *</label><input class="form-control" name="name" required value="' + LL.esc(pre.name || "") + '"></div>' +
    '<div class="col-md-4"><label class="form-label">所属类别 *</label><select class="form-select" name="category_id">' +
    cats.map(c => '<option value="' + c.id + '"' + (Number(pre.category_id) === c.id ? " selected" : "") + ">" + LL.emoji(c.id) + " " + LL.esc(c.name) + "</option>").join("") + "</select></div>" +
    '<div class="col-md-4"><label class="form-label">所在区域</label><input class="form-control" name="area" value="' + LL.esc(pre.area || "") + '" placeholder="如：朝阳"></div>' +
    '<div class="col-md-4"><label class="form-label">营业时间</label><input class="form-control" name="business_hours" value="' + LL.esc(pre.business_hours || "") + '" placeholder="如 09:00-22:00"></div>' +
    '<div class="col-md-4"><label class="form-label">联系电话 *</label><input class="form-control" name="phone" required value="' + LL.esc(pre.phone || "") + '"></div>' +
    '<div class="col-md-6"><label class="form-label">人均价格区间 *</label><div class="input-group"><input class="form-control" name="price_min" type="number" min="0" value="' + (pre.price_min || 50) + '">' +
    '<span class="input-group-text">至</span><input class="form-control" name="price_max" type="number" min="0" value="' + (pre.price_max || 200) + '"></div></div>' +
    '<div class="col-12"><label class="form-label">门店介绍</label><textarea class="form-control" name="intro" rows="3">' + LL.esc(pre.intro || "") + "</textarea></div>" +
    '<div class="col-12"><button class="btn btn-fire btn-lg px-5" type="submit" id="applySubmit">' + (existed ? "重新提交入驻申请" : "提交入驻申请") + "</button>" +
    " <span class='text-muted small ms-2'>提交即代表同意平台商户规范</span></div></form></div></div>";

  document.getElementById("applyForm").addEventListener("submit", async e => {
    e.preventDefault();
    const f = e.target;
    const body = {
      name: f.name.value.trim(), category_id: f.category_id.value, area: f.area.value.trim(),
      business_hours: f.business_hours.value.trim(), phone: f.phone.value.trim(),
      price_min: Number(f.price_min.value), price_max: Number(f.price_max.value), intro: f.intro.value.trim()
    };
    // 提交前本地校验（与后端规则一致，避免无谓往返）
    if (!body.name) { LL.toast("请填写店铺名称", "err"); f.name.focus(); return; }
    if (!body.category_id) { LL.toast("请选择所属类别", "err"); return; }
    if (!body.phone) { LL.toast("请填写联系电话", "err"); f.phone.focus(); return; }
    if (body.price_min < 0 || (body.price_max > 0 && body.price_min > body.price_max)) {
      LL.toast("价格区间设置不正确：人均下限不能高于上限", "err"); f.price_min.focus(); return;
    }
    const btn = f.querySelector("#applySubmit") || f.querySelector("button[type=submit]") || f.querySelector("button");
    const raw = btn ? btn.innerHTML : "";
    if (btn) { btn.disabled = true; btn.innerHTML = "提交中…"; }
    try {
      await LL.api("POST", "/api/merchant/apply", body);
      LL.toast("入驻申请已提交，等待平台审核 🎉", "ok");
      await refreshStoredUser();          // 角色已升级为 merchant
      location.hash = "#/shop";           // 进入工作台查看审核状态
    } catch (err) {
      LL.toast(err.message, "err");
      if (btn) { btn.disabled = false; btn.innerHTML = raw; }
    }
  });
};

/* 经营统计（ECharts 趋势） */
async function shopStats(main) {
  main.innerHTML = '<div class="text-center p-5">加载中…</div>';
  let s;
  try { s = await LL.api("GET", "/api/merchant/stats"); }
  catch (e) { main.innerHTML = '<div class="panel">' + LL.esc(e.message) + "</div>"; return; }
  const fmt = n => LL.money(n);
  main.innerHTML =
    '<div class="stat-cards">' +
    '<div class="stat-card"><i class="bi bi-eye ic"></i><b>' + (s.view_count || 0) + '</b><span>浏览量</span></div>' +
    '<div class="stat-card"><i class="bi bi-heart ic"></i><b>' + (s.favorite_count || 0) + '</b><span>收藏数</span></div>' +
    '<div class="stat-card"><i class="bi bi-bell ic"></i><b>' + (s.follower_count || 0) + '</b><span>关注数</span></div>' +
    '<div class="stat-card"><i class="bi bi-star ic"></i><b>' + (s.avg_score || 0) + '</b><span>均分(' + (s.review_count || 0) + '评)</span></div>' +
    '<div class="stat-card"><i class="bi bi-bag ic"></i><b>' + (s.order_count || 0) + '</b><span>订单 ' + fmt(s.order_amount) + '</span></div>' +
    '<div class="stat-card"><i class="bi bi-ticket ic"></i><b>' + ((s.coupons || {}).received || 0) + '</b><span>券领取/核销 ' + ((s.coupons || {}).used || 0) + '</span></div></div>' +
    '<div class="panel"><h5><i class="bi bi-graph-up"></i> 近 7 日经营趋势（消费额/笔数）</h5><div id="mTrend" style="height:280px"></div></div>' +
    '<div class="panel mt-3"><h5><i class="bi bi-fire"></i> 热门服务</h5><div id="hotSvc"></div></div>';

  if (window.echarts) {
    const t = (s.trend || []).slice().reverse();
    const ch = echarts.init(document.getElementById("mTrend"));
    ch.setOption({
      tooltip: { trigger: "axis" },
      legend: { data: ["消费额", "消费笔数"] },
      grid: { left: 40, right: 20, top: 30, bottom: 24 },
      xAxis: { type: "category", data: t.map(d => d.date) },
      yAxis: [{ type: "value", name: "金额" }, { type: "value", name: "笔数" }],
      series: [
        { name: "消费额", type: "line", smooth: true, data: t.map(d => Number(d.amount || 0)),
          itemStyle: { color: "#cf4a2b" }, areaStyle: { opacity: .12 } },
        { name: "消费笔数", type: "line", smooth: true, yAxisIndex: 1,
          data: t.map(d => Number(d.consumption || 0)), itemStyle: { color: "#1f4d3a" } }
      ]
    });
  }
  const hot = s.hot_services || [];
  const h = document.getElementById("hotSvc");
  h.innerHTML = hot.length
    ? hot.map(x => '<div class="good-row"><div class="good-name"><b>' + LL.esc(x.name) + '</b><small>被消费 ' + (x.times || 0) + ' 次</small></div>' +
      '<span class="price-min">' + fmt(x.amount) + "</span></div>").join("")
    : emptyBox("暂无服务消费数据");
}

/* 优惠活动（创建 / 上下线 / 核销） */
async function shopCoupons(main, data) {
  main.innerHTML =
    '<div class="panel mb-3"><h5><i class="bi bi-ticket-perforated"></i> 发布新活动</h5>' +
    '<form id="cpnForm" class="row g-3">' +
    '<div class="col-md-3"><label class="form-label">类型</label><select name="type" class="form-select">' +
    '<option value="coupon">代金券</option><option value="full_reduction">满减券</option><option value="discount">折扣券</option></select></div>' +
    '<div class="col-md-5"><label class="form-label">活动名称 *</label><input class="form-control" name="name" required></div>' +
    '<div class="col-md-2"><label class="form-label">面值</label><input class="form-control" name="face_value" type="number" step="0.01" min="0" value="5"></div>' +
    '<div class="col-md-2"><label class="form-label">门槛(0=无)</label><input class="form-control" name="threshold" type="number" step="0.01" min="0" value="0"></div>' +
    '<div class="col-md-2"><label class="form-label">总量(0=不限)</label><input class="form-control" name="total" type="number" min="0" value="0"></div>' +
    '<div class="col-md-4"><label class="form-label">折扣率(选折扣时)</label><input class="form-control" name="discount_rate" type="number" step="0.01" min="0" max="1" value="0.9"></div>' +
    '<div class="col-md-6"><label class="form-label">有效期至</label><input class="form-control" name="end_time" type="datetime-local" required></div>' +
    '<div class="col-md-4"><label class="form-label">使用范围</label><input class="form-control" name="scope" placeholder="全场通用"></div>' +
    '<div class="col-12"><button class="btn btn-main" type="submit" name="save">创建草稿</button>' +
    '<button class="btn btn-fire ms-2" type="submit" name="pub">创建并立即发布</button></div></form></div>' +
    '<div class="table-card"><div class="t-head"><h5>我的活动</h5></div><div class="p-3" id="cpnList">加载中…</div></div>' +
    '<div class="panel mt-3"><h5><i class="bi bi-qr-code-scan"></i> 核销（顾客出示核销码）</h5>' +
    '<form id="verifyForm" class="d-flex gap-2"><input class="form-control" name="code" placeholder="输入核销码" required style="max-width:340px">' +
    '<button class="btn btn-fire">核销</button></form></div>';

  const listEl = main.querySelector("#cpnList");
  const load = async () => {
    try {
      const rows = await LL.api("GET", "/api/merchant/coupons");
      if (!rows.length) { listEl.innerHTML = emptyBox("还没有创建优惠活动"); return; }
      listEl.innerHTML = '<div class="row g-3">' + rows.map(c => {
        const valueDesc = c.type === "discount"
          ? (Number(c.discount_rate) * 10).toFixed(1) + " 折" : "¥" + LL.money(c.face_value);
        return '<div class="col-md-6"><div class="panel mb-0">' +
          '<div class="d-flex align-items-center gap-2 flex-wrap"><b>' + LL.esc(c.name) + "</b>" +
          '<span class="badge-soft badge-' + LL.statusClass(c.status).replace("badge-", "") + '">' + LL.statusZh(c.status) + "</span>" +
          '<span class="text-muted small">领取 ' + (c.received || 0) + " / 核销 " + (c.used || 0) + "</span></div>" +
          '<div class="text-muted" style="font-size:12.5px">' + valueDesc + (Number(c.threshold) ? " · 满 " + LL.money(c.threshold) + " 可用" : "") +
          " · 有效期至 " + LL.esc(String(c.end_time || "").slice(0, 16)) + "</div>" +
          '<div class="mt-2 d-flex gap-2">' +
          (c.status !== "published" ? '<button class="btn btn-main btn-sm" data-pub="' + c.id + '">发布</button>' : "") +
          (c.status === "published" ? '<button class="btn btn-ghost btn-sm" data-off="' + c.id + '">下线</button>' : "") +
          "</div></div></div>";
      }).join("") + "</div>";
      listEl.querySelectorAll("[data-pub]").forEach(b => b.addEventListener("click", async () => {
        try { await LL.api("PUT", "/api/merchant/coupons/" + b.dataset.pub + "/status", { status: "published" }); LL.toast("已发布", "ok"); await load(); }
        catch (e) { LL.toast(e.message, "err"); }
      }));
      listEl.querySelectorAll("[data-off]").forEach(b => b.addEventListener("click", async () => {
        try { await LL.api("PUT", "/api/merchant/coupons/" + b.dataset.off + "/status", { status: "offline" }); LL.toast("已下线", "ok"); await load(); }
        catch (e) { LL.toast(e.message, "err"); }
      }));
    } catch (e) { listEl.innerHTML = '<div class="empty">' + LL.esc(e.message) + "</div>"; }
  };
  main.querySelector("#cpnForm").addEventListener("submit", async ev => {
    ev.preventDefault();
    const f = ev.target;
    const pub = ev.submitter && ev.submitter.name === "pub";
    const body = {
      type: f.type.value, name: f.name.value.trim(),
      face_value: Number(f.face_value.value), threshold: Number(f.threshold.value),
      discount_rate: Number(f.discount_rate.value), total: Number(f.total.value),
      scope: f.scope.value.trim(), end_time: f.end_time.value
    };
    try {
      await LL.api("POST", "/api/merchant/coupons", body);
      let tip = "已创建草稿";
      if (pub) {
        const rows = await LL.api("GET", "/api/merchant/coupons");
        if (rows.length) { await LL.api("PUT", "/api/merchant/coupons/" + rows[0].id + "/status", { status: "published" }); tip = "已创建并发布"; }
      }
      LL.toast(tip, "ok");
      f.reset();
      await load();
    } catch (e) { LL.toast(e.message, "err"); }
  });

  main.querySelector("#verifyForm").addEventListener("submit", async ev => {
    ev.preventDefault();
    const code = main.querySelector("#verifyForm input").value.trim();
    if (!code) return;
    try {
      const r = await LL.api("POST", "/api/merchant/coupon/verify", { code: code });
      LL.toast("核销成功：" + r.coupon + "（顾客 " + r.claim_user + "）", "ok");
      ev.target.reset();
      await load();
    } catch (e) { LL.toast(e.message, "err"); }
  });

  await load();
}


