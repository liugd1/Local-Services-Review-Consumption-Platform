/* 巷味 · 平台管理台 */
LL.views.admin = async function (el, tab) {
  const tabs = [["stats", "bi-bar-chart-line", "数据看板"], ["audit", "bi-shop-check", "商户审核"], ["reports", "bi-flag", "举报中心"],
    ["users", "bi-people", "用户管理"]];
  const active = tabs.some(t => t[0] === tab) ? tab : "audit";
  const links = tabs.map(t =>
    '<a class="ws-link' + (t[0] === active ? " active" : "") + '" href="#/admin?tab=' + t[0] + '"><i class="bi ' + t[1] + '"></i>' + t[2] + "</a>").join("") +
    '<a class="ws-link logout" onclick="LL.logout()"><i class="bi bi-box-arrow-right"></i> 退出登录</a>';
  el.innerHTML = '<div class="container-xl"><div class="page-title"><span class="tick" style="display:inline-block;width:8px;height:26px;border-radius:4px;background:linear-gradient(180deg,#cf4a2b,#d99b2b)"></span>平台管理台</div>' +
    '<div class="workbench"><aside class="side-nav">' + links + "</aside>" +
    "<div class='ws-main'><div class='panel text-center p-5'>加载中…</div></div></div></div>";
  const main = el.querySelector(".ws-main");
  const renderers = { stats: renderAdminStats, audit: renderAudit, reports: renderReports, users: renderUsers };
  try { await renderers[active](main); } catch (e) { main.innerHTML = '<div class="panel">' + LL.esc(e.message) + "</div>"; }
};

/* 商户审核 */
async function renderAudit(main) {
  let status = "pending";
  const badge = s => '<span class="badge-soft badge-' + LL.statusClass(s).replace("badge-", "") + '">' + LL.statusZh(s) + "</span>";
  const tabsHtml = () => '<div class="table-card mt-3"><div class="t-head"><h5>入驻申请审核</h5><div class="cat-chips">' +
    [["pending", "待审核"], ["approved", "已通过"], ["rejected", "已驳回"]].map(x =>
      '<button class="cat-chip' + (status === x[0] ? " on" : "") + '" data-s="' + x[0] + '">' + x[1] + "</button>").join("") +
    "</div></div><div class='p-3' id='auditBody'>加载中…</div></div>";

  const load = async () => {
    main.innerHTML = '<div class="text-center p-4">加载中…</div>';
    try {
      const [r, counts] = await Promise.all([
        LL.api("GET", "/api/admin/merchants?status=" + status + "&page=1&size=100"),
        LL.api("GET", "/api/admin/merchants/counts")
      ]);
      main.innerHTML =
        '<div class="stat-cards mb-0">' +
        '<div class="stat-card"><i class="bi bi-hourglass-split ic"></i><b>' + (counts.pending || 0) + '</b><span>待审核</span></div>' +
        '<div class="stat-card"><i class="bi bi-check2-circle ic"></i><b>' + (counts.approved || 0) + '</b><span>已通过</span></div>' +
        '<div class="stat-card"><i class="bi bi-x-circle ic"></i><b>' + (counts.rejected || 0) + '</b><span>已驳回</span></div>' +
        '<div class="stat-card"><i class="bi bi-shop ic"></i><b>' + (counts.total || 0) + '</b><span>商户总数</span></div></div>' +
        tabsHtml();
      const body = main.querySelector("#auditBody");
      if (!r.list.length) { body.innerHTML = emptyBox("暂无数据"); bindAudit(); return; }
      body.innerHTML = r.list.map(m => {
        const act = m.status === "pending"
          ? '<button class="btn btn-main btn-sm" data-ok="' + m.id + '"><i class="bi bi-check-lg"></i> 通过</button>' +
            '<button class="btn btn-ghost btn-sm" data-no="' + m.id + '" style="color:#b43a1e"><i class="bi bi-x-lg"></i> 驳回</button>'
          : "";
        return '<div class="panel mb-3"><div class="d-flex flex-wrap align-items-center gap-2">' +
          '<div class="row-thumb" style="width:64px;min-height:52px;border-radius:10px;font-size:24px;' + LL.cover(m.category_id) + '">' + LL.emoji(m.category_id) + "</div>" +
          "<div class='flex-grow-1'><b style='font-size:16px'>" + LL.esc(m.name) + "</b> " + badge(m.status) +
          "<div class='text-muted' style='font-size:12.5px'>类别：" + LL.esc(m.category_name || "-") + " · 经营者 @" + LL.esc(m.owner_username || "-") + " · 电话 " + LL.esc(m.phone || "-") + "</div>" +
          "<div class='text-muted' style='font-size:12.5px'>" + LL.esc(m.area || "") + " · " + LL.esc(m.business_hours || "") + " · 人均 " + LL.money(m.price_min) + "-" + LL.money(m.price_max || m.price_min) + "</div>" +
          (m.intro ? "<p class='mt-1 mb-0 text-muted' style='font-size:13px'>" + LL.esc(m.intro) + "</p>" : "") +
          (m.status === "rejected" && m.reject_reason ? '<p class="mb-0" style="font-size:12.5px;color:#b43a1e">驳回原因：' + LL.esc(m.reject_reason) + "</p>" : "") +
          "</div><div class='d-flex gap-2'>" + act + "</div></div></div>";
      }).join("");
      bindAudit();
    } catch (e) { main.innerHTML = '<div class="panel">' + LL.esc(e.message) + "</div>"; }
  };
  const bindAudit = () => {
    main.querySelectorAll("[data-s]").forEach(x => x.addEventListener("click", () => {
      status = x.dataset.s;
      main.querySelectorAll("[data-s]").forEach(y => y.classList.remove("on"));
      x.classList.add("on");
      load();
    }));
    main.querySelectorAll("[data-ok]").forEach(x => x.addEventListener("click", async () => {
      try { await LL.api("PUT", "/api/admin/merchants/" + x.dataset.ok + "/audit", { status: "approved" }); LL.toast("已通过该商户入驻", "ok"); load(); }
      catch (e) { LL.toast(e.message, "err"); }
    }));
    main.querySelectorAll("[data-no]").forEach(x => x.addEventListener("click", async () => {
      const reason = window.prompt("请输入驳回原因：");
      if (!reason) return;
      try { await LL.api("PUT", "/api/admin/merchants/" + x.dataset.no + "/audit", { status: "rejected", reason: reason.trim() }); LL.toast("已驳回", "info"); load(); }
      catch (e) { LL.toast(e.message, "err"); }
    }));
  };
  await load();
}

/* 举报中心（评价举报 + 评论举报） */
async function renderReports(main) {
  main.innerHTML =
    '<div class="table-card mb-3"><div class="t-head"><h5>评价举报</h5><span class="badge-soft badge-pending">待处理</span></div><div id="rptBody" class="p-3">加载中…</div></div>' +
    '<div class="table-card"><div class="t-head"><h5>评论举报</h5><span class="badge-soft badge-pending">待处理</span></div><div id="crBody" class="p-3">加载中…</div></div>';

  // ---- 评价举报 ----
  const body = main.querySelector("#rptBody");
  try {
    const r = await LL.api("GET", "/api/admin/reports?status=pending&page=1&size=100");
    if (!r.list.length) { body.innerHTML = emptyBox("没有待处理的评价举报 🎉"); }
    else {
      body.innerHTML = r.list.map(p => '<div class="panel mb-3">' +
        '<div class="d-flex align-items-center gap-2 mb-1"><span class="badge-soft badge-pending">举报 #' + p.id + "</span>" +
        '<span class="text-muted" style="font-size:13px">' + LL.esc(p.reporter_username) + " 举报了 " + LL.esc(p.review_author) + " 对「" + LL.esc(p.merchant_name) + '」的评价</span></div>' +
        '<div class="text-muted" style="font-size:13px">评价内容：' + LL.esc(p.review_content) + "</div>" +
        '<div class="mt-1" style="font-size:13px">举报理由：<b>' + LL.esc(p.reason) + "</b>（评分 " + Number(p.avg_score).toFixed(1) + "）</div>" +
        '<div class="d-flex gap-2 mt-2">' +
        '<button class="btn btn-fire btn-sm" data-hide="' + p.id + '"><i class="bi bi-eye-slash"></i> 下架该评价</button>' +
        '<button class="btn btn-ghost btn-sm" data-rej="' + p.id + '">驳回举报</button></div></div>').join("");
      body.querySelectorAll("[data-hide]").forEach(b => b.addEventListener("click", async () => {
        try { await LL.api("PUT", "/api/admin/reports/" + b.dataset.hide, { action: "hide" }); LL.toast("已下架该评价", "ok"); b.closest(".panel").remove(); }
        catch (e) { LL.toast(e.message, "err"); }
      }));
      body.querySelectorAll("[data-rej]").forEach(b => b.addEventListener("click", async () => {
        const reason = window.prompt("驳回原因：");
        if (!reason) return;
        try { await LL.api("PUT", "/api/admin/reports/" + b.dataset.rej, { action: "reject", reason: reason.trim() }); LL.toast("已驳回该举报", "ok"); b.closest(".panel").remove(); }
        catch (e) { LL.toast(e.message, "err"); }
      }));
    }
  } catch (e) { body.innerHTML = '<div class="empty">' + LL.esc(e.message) + "</div>"; }

  // ---- 评论举报 ----
  const cbody = main.querySelector("#crBody");
  try {
    const c = await LL.api("GET", "/api/admin/comment-reports?status=pending&page=1&size=100");
    if (!c.list.length) { cbody.innerHTML = emptyBox("没有待处理的评论举报 🎉"); return; }
    cbody.innerHTML = c.list.map(p => '<div class="panel mb-3">' +
      '<div class="d-flex align-items-center gap-2 mb-1"><span class="badge-soft badge-pending">评论举报 #' + p.id + "</span>" +
      '<span class="text-muted" style="font-size:13px">' + LL.esc(p.reporter_username) + " 举报了 " + LL.esc(p.comment_author) + " 在「" + LL.esc(p.merchant_name) + '」下的评论</span></div>' +
      '<div class="text-muted" style="font-size:13px">评论内容：' + LL.esc(p.comment_content) + "</div>" +
      '<div class="mt-1" style="font-size:13px">举报理由：<b>' + LL.esc(p.reason) + "</b></div>" +
      '<div class="d-flex gap-2 mt-2">' +
      '<button class="btn btn-fire btn-sm" data-cdel="' + p.id + '"><i class="bi bi-trash"></i> 删除该评论（含回复）</button>' +
      '<button class="btn btn-ghost btn-sm" data-crej="' + p.id + '">驳回举报</button></div></div>').join("");
    cbody.querySelectorAll("[data-cdel]").forEach(b => b.addEventListener("click", async () => {
      try {
        await LL.api("PUT", "/api/admin/comment-reports/" + b.dataset.cdel, { action: "delete" });
        LL.toast("已删除该评论", "ok");
        b.closest(".panel").remove();
      } catch (e) { LL.toast(e.message, "err"); }
    }));
    cbody.querySelectorAll("[data-crej]").forEach(b => b.addEventListener("click", async () => {
      const reason = window.prompt("驳回原因：");
      if (!reason) return;
      try {
        await LL.api("PUT", "/api/admin/comment-reports/" + b.dataset.crej, { action: "reject", reason: reason.trim() });
        LL.toast("已驳回该举报", "ok");
        b.closest(".panel").remove();
      } catch (e) { LL.toast(e.message, "err"); }
    }));
  } catch (e) { cbody.innerHTML = '<div class="empty">' + LL.esc(e.message) + "</div>"; }
}

/* 用户管理 */
async function renderUsers(main) {
  main.innerHTML = '<div class="table-card"><div class="t-head"><h5>用户管理</h5></div><div id="usrBody"></div></div>';
  const body = main.querySelector("#usrBody");
  try {
    const r = await LL.api("GET", "/api/admin/users?page=1&size=100");
    if (!r.total) { body.innerHTML = '<div class="p-3">' + emptyBox("暂无用户") + "</div>"; return; }
    const roleName = { consumer: "食客", merchant: "掌柜", admin: "管理员" };
    body.innerHTML = '<table class="table"><thead><tr><th>ID</th><th>用户</th><th>角色</th><th>手机号</th><th>注册时间</th><th>状态</th><th>操作</th></tr></thead><tbody>' +
      r.list.map(u => "<tr><td>" + u.id + "</td><td>" + LL.avatarHtml(u) + " <b>" + LL.esc(u.nickname || u.username) +
        "</b> <span class='text-muted' style='font-size:12px'>@" + LL.esc(u.username) + "</span></td><td>" + (roleName[u.role] || u.role) +
        "</td><td>" + LL.esc(u.phone || "—") + "</td><td>" + LL.esc(String(u.created_at || "").slice(0, 10)) +
        "</td><td><span class='badge-soft badge-" + (u.status === "active" ? "approved" : "rejected") + "'>" + (u.status === "active" ? "正常" : "已禁用") + "</span></td>" +
        '<td>' + (u.role !== "admin"
          ? '<button class="btn btn-sm ' + (u.status === "active" ? "btn-ghost" : "btn-main") + '" data-t="' + u.id + '" data-c="' + u.status + '">' +
            (u.status === "active" ? '<i class="bi bi-slash-circle"></i> 禁用' : '<i class="bi bi-check-circle"></i> 启用') + "</button>"
          : '<span class="text-muted small">管理员</span>') + "</td></tr>").join("") +
      "</tbody></table>";
    body.querySelectorAll("[data-t]").forEach(b => b.addEventListener("click", async () => {
      try {
        await LL.api("PUT", "/api/admin/users/" + b.dataset.t + "/status", { status: b.dataset.c === "active" ? "disabled" : "active" });
        LL.toast(b.dataset.c === "active" ? "已禁用该用户" : "已恢复该用户", "ok");
        renderUsers(main);
      } catch (e) { LL.toast(e.message, "err"); }
    }));
  } catch (e) { body.innerHTML = '<div class="p-3">' + LL.esc(e.message) + "</div>"; }
}
/* 平台数据看板 */
async function renderAdminStats(main) {
  main.innerHTML = '<div class="text-center p-5">加载中…</div>';
  let s;
  try { s = await LL.api("GET", "/api/admin/stats"); }
  catch (e) { main.innerHTML = '<div class="panel">' + LL.esc(e.message) + "</div>"; return; }
  const fmt = n => LL.money(n);
  const t = (s.trend || []).slice().reverse();
  main.innerHTML =
    '<div class="stat-cards">' +
    '<div class="stat-card"><i class="bi bi-people ic"></i><b>' + (s.users.total || 0) + '</b><span>用户（今日+ ' + (s.users.today_new || 0) + '）</span></div>' +
    '<div class="stat-card"><i class="bi bi-shop ic"></i><b>' + (s.merchants.approved || 0) + '</b><span>在营商户/共 ' + (s.merchants.total || 0) + '</span></div>' +
    '<div class="stat-card"><i class="bi bi-chat-square-quote ic"></i><b>' + (s.reviews.visible || 0) + '</b><span>公开评价</span></div>' +
    '<div class="stat-card"><i class="bi bi-bag ic"></i><b>' + (s.orders.total || 0) + '</b><span>订单 ' + fmt(s.orders.amount) + '</span></div>' +
    '<div class="stat-card"><i class="bi bi-receipt ic"></i><b>' + (s.consumption.total || 0) + '</b><span>消费流水 ' + fmt(s.consumption.amount) + '</span></div>' +
    '<div class="stat-card"><i class="bi bi-flag ic"></i><b>' + (s.reviews.pending_reports || 0) + '</b><span>待处理举报</span></div></div>' +
    '<div class="panel"><h5><i class="bi bi-graph-up"></i> 近 7 日平台趋势（新增用户 / 订单量 / 消费额）</h5><div id="adTrend" style="height:300px"></div></div>' +
    '<div class="row g-3 mt-1"><div class="col-lg-5"><div class="panel"><h5><i class="bi bi-pie-chart"></i> 在营商户 · 类别分布</h5><div id="adPie" style="height:270px"></div></div></div>' +
    '<div class="col-lg-7"><div class="panel"><h5><i class="bi bi-ticket"></i> 优惠券运营</h5>' +
    '<div class="d-flex gap-4 p-2 flex-wrap"><div><div class="text-muted small">累计发放券</div><div style="font-size:26px;font-weight:800">' + ((s.coupons || {}).claims || 0) + '</div></div>' +
    '<div><div class="text-muted small">已核销券</div><div style="font-size:26px;font-weight:800;color:#1f4d3a">' + ((s.coupons || {}).used || 0) + "</div></div>" +
    '<div class="align-self-end ms-auto"><span class="text-muted small">商户在审 ' + (s.merchants.pending || 0) + " · 驳回 " + (s.merchants.rejected || 0) + '</span></div></div>' +
    '<div class="table-card mt-3"><div class="t-head"><h5>近 7 日明细</h5></div><div class="p-2 small" style="overflow-x:auto">' +
    '<table class="table mb-0"><tr><th>日期</th><th>新增用户</th><th>订单量</th><th>订单额</th><th>核销消费额</th></tr>' +
    (t.length ? t.map(d => '<tr><td>' + LL.esc(d.date) + "</td><td>" + (d.new_users || 0) + "</td><td>" + (d.orders || 0) + "</td><td>" + fmt(d.amount) + "</td><td>" + fmt(d.consume) + "</td></tr>").join("") : "") +
    "</table></div></div></div></div>";

  if (window.echarts) {
    const line = echarts.init(document.getElementById("adTrend"));
    line.setOption({
      tooltip: { trigger: "axis" },
      legend: { data: ["新增用户", "订单量", "消费额"] },
      grid: { left: 46, right: 20, top: 30, bottom: 24 },
      xAxis: { type: "category", data: t.map(d => d.date) },
      yAxis: [{ type: "value", name: "量" }, { type: "value", name: "金额" }],
      series: [
        { name: "新增用户", type: "bar", data: t.map(d => d.new_users || 0), itemStyle: { color: "#cf4a2b" }, barWidth: 12 },
        { name: "订单量", type: "line", data: t.map(d => d.orders || 0), itemStyle: { color: "#1f4d3a" } },
        { name: "消费额", type: "line", yAxisIndex: 1, data: t.map(d => Number(d.consume || 0)), itemStyle: { color: "#d9a02b" } }
      ]
    });
    const pie = echarts.init(document.getElementById("adPie"));
    pie.setOption({
      tooltip: { trigger: "item", formatter: "{b}: {c} 家 ({d}%)" },
      series: [{
        type: "pie", radius: ["42%", "70%"],
        data: (s.category_dist || []).filter(x => x.cnt).map(x => ({ name: x.name, value: x.cnt })),
        itemStyle: { borderRadius: 6, borderColor: "#fff", borderWidth: 2 }
      }]
    });
  }
}
