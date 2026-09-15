/* 巷味 · 门店选购页：#/s/{id}（消费者在此选择服务/套餐下单，下单主体为门店） */
LL.views.store = async function (el, id) {
  const user = LL.user;
  let s;
  try { s = await LL.api("GET", "/api/stores/" + id); }
  catch (e) { el.innerHTML = '<div class="container-xl">' + emptyBox(e.message) + "</div>"; return; }

  const m = s.merchant || {};
  const sum = s.summary || {};
  const star = Number(sum.avg_score) || 0;
  const canBuy = !!(user && user.role === "consumer");
  const statusDesc = { open: "营业中", rest: "休息中", closed: "已打烊" };

  // ---------- 招牌服务（本店在售） ----------
  const svcs = s.services || [];
  const svcHtml = '<div class="panel" style="margin-top:1rem"><h5><i class="bi bi-list-check"></i> 招牌服务 · 本店在售</h5>' +
    (svcs.length
      ? svcs.map(x => {
          const pic = LL.imgList(x.images)[0];
          return '<div class="good-row">' +
            (pic ? '<img src="' + LL.esc(pic) + '" data-view-src="' + LL.esc(pic) + '" ' +
              'style="width:52px;height:52px;object-fit:cover;border-radius:10px;border:1px solid var(--line);margin-right:10px;cursor:zoom-in">' : "") +
            '<div class="good-name"><b>' + LL.esc(x.name) + "</b><small>" +
            LL.esc(x.applicable_time || "适用时段不限") +
            (x.price_unit ? " · " + LL.esc(x.price_unit) : "") +
            (Number(x.stock) >= 0 ? " · 余量 " + x.stock : "") + "</small></div>" +
            '<span class="good-price">' + LL.money(x.price) + "</span>" +
            '<a class="btn btn-ghost btn-sm ms-2" href="#/talk/service/' + x.id + '">口碑 ›</a>' +
            (canBuy ? '<button class="btn btn-fire btn-sm ms-1" data-order="service" data-id="' + x.id +
              '" data-name="' + LL.esc(x.name) + '">立即下单</button>' : "") +
            "</div>";
        }).join("")
      : emptyBox("本店暂未上架服务项目"));

  // ---------- 优惠套餐（本店在售） ----------
  const pkgs = s.packages || [];
  const pkgHtml = '<div class="panel" style="margin-top:1rem"><h5><i class="bi bi-gift"></i> 优惠套餐 · 本店在售</h5>' +
    (pkgs.length
      ? pkgs.map(x => {
          const pic = LL.imgList(x.images)[0];
          return '<div class="good-row">' +
            (pic ? '<img src="' + LL.esc(pic) + '" data-view-src="' + LL.esc(pic) + '" ' +
              'style="width:52px;height:52px;object-fit:cover;border-radius:10px;border:1px solid var(--line);margin-right:10px;cursor:zoom-in">' : "") +
            '<div class="good-name"><b>' + LL.esc(x.name) + "</b><small>" +
            LL.esc(x.content || "") + (x.valid_days ? " · 有效期 " + x.valid_days + " 天" : "") +
            (Number(x.limit_count) > 0 ? " · 每人限购 " + x.limit_count + " 份" : "") + "</small></div>" +
            '<span class="good-price">' + LL.money(x.price) + "</span>" +
            '<a class="btn btn-ghost btn-sm ms-2" href="#/talk/package/' + x.id + '">口碑 ›</a>' +
            (canBuy ? '<button class="btn btn-fire btn-sm ms-1" data-order="package" data-id="' + x.id +
              '" data-name="' + LL.esc(x.name) + '">立即购买</button>' : "") +
            "</div>";
        }).join("")
      : emptyBox("本店暂未上架优惠套餐"));

  // ---------- 本店参与的优惠活动 ----------
  const cpnHtml = '<div class="panel" style="margin-top:1rem"><h5><i class="bi bi-ticket-perforated"></i> 本店优惠活动</h5>' +
    '<div class="p-1" id="storeCpn">加载中…</div></div>';

  el.innerHTML = '<div class="container-xl">' +
    '<div class="d-flex align-items-center gap-2 mb-2" style="font-size:13px">' +
      '<a href="#/m/' + m.id + '" style="color:var(--green);text-decoration:none">← 返回「' + LL.esc(m.name) + '」</a>' +
      '<span class="text-muted">/</span><span class="text-muted">门店选购</span></div>' +
    '<section class="panel">' +
      '<div class="d-flex flex-wrap gap-2 align-items-center">' +
        '<h3 style="margin:0;font-family:var(--serif)">' + LL.esc(s.name) + "</h3>" +
        '<span class="tag ' + (s.status === "open" ? "green" : "red") + '">' + (statusDesc[s.status] || s.status) + "</span>" +
        '<span class="tag gold">' + LL.emoji(m.category_id) + " " + LL.esc(LL.cat(m.category_id)) + "</span></div>" +
      '<div class="text-muted mt-2" style="font-size:13px">' +
        '<i class="bi bi-geo-alt"></i> ' + LL.esc(s.area || "本城") + (s.address ? " · " + LL.esc(s.address) : "") +
        "　<i class='bi bi-clock'></i> " + LL.esc(m.business_hours || "以店公告为准") +
        "　<i class='bi bi-telephone'></i> " + LL.esc(m.phone || "—") + "</div>" +
      '<div class="mt-2"><b style="font-family:var(--serif);font-size:1.3rem;color:var(--persimmon)">' +
        (star > 0 ? star.toFixed(1) : "新") + "</b> " + LL.starsHtml(star) +
        ' <span class="text-muted" style="font-size:13px">门店口碑 ' + (Number(sum.count) || 0) + " 条</span>" +
        '<a class="btn btn-ghost btn-sm ms-2" href="#/talk/store/' + s.id + '">门店口碑 ›</a>' +
        (canBuy ? "" : '<span class="text-muted ms-2" style="font-size:12.5px">' +
          (user ? "当前身份不支持下单" : "登录消费者账号后可下单") + "</span>") +
      "</div>" +
      (LL.imgList(s.images).length
        ? '<div class="mt-2"><div class="text-muted mb-1" style="font-size:12.5px"><i class="bi bi-images"></i> 门店实景（' +
          LL.imgList(s.images).length + "）</div>" + LL.imgThumbs(s.images, 92) + "</div>"
        : "") +
      "</section>" +
    '<div class="row g-3"><div class="col-lg-8">' + svcHtml + pkgHtml + "</div>" +
    '<div class="col-lg-4">' + cpnHtml + "</div></div></div>";

  // ---------- 下单（下单主体＝门店） ----------
  el.querySelectorAll("[data-order]").forEach(btn => btn.addEventListener("click", async () => {
    if (!LL.user) { LL.openAuth("login"); return; }
    if (LL.user.role !== "consumer") { LL.toast("请使用消费者账号下单", "info"); return; }
    const type = btn.dataset.order;
    const label = type === "service" ? "服务" : "套餐";
    if (!confirm("确认在「" + s.name + "」下单" + label + "「" + btn.dataset.name + "」？")) return;
    const raw = btn.innerHTML;
    btn.disabled = true;
    btn.innerHTML = "提交中…";
    try {
      const o = await LL.api("POST", "/api/store/" + s.id + "/order",
        { item_type: type, item_id: Number(btn.dataset.id) });
      LL.toast("下单成功！订单号 " + o.order_no + "，可在「我的-我的订单」核销或退款", "ok", 3200);
      btn.innerHTML = "已下单";
    } catch (e) {
      LL.toast(e.message, "err");
      btn.disabled = false;
      btn.innerHTML = raw;
    }
  }));

  // ---------- 本店优惠活动（领取） ----------
  const cpnBox = el.querySelector("#storeCpn");
  const paintCoupons = async () => {
    try {
      let have = new Set();
      if (LL.user && LL.user.role === "consumer") {
        const mine = await LL.api("GET", "/api/my/coupons?status=all");
        have = new Set(mine.map(x => Number(x.coupon_id)));
      }
      const rows = s.coupons || [];
      if (!rows.length) { cpnBox.innerHTML = emptyBox("本店暂无进行中的优惠活动"); return; }
      cpnBox.innerHTML = rows.map(r => {
        const val = r.type === "discount"
          ? (Number(r.discount_rate) * 10).toFixed(1) + " 折" : "¥" + LL.money(r.face_value);
        const limit = Number(r.threshold) ? "满 " + LL.money(r.threshold) + " 可用" : "无门槛";
        const act = canBuy
          ? (have.has(Number(r.id))
              ? '<span class="btn btn-ghost btn-sm disabled">已领取</span>'
              : '<button class="btn btn-fire btn-sm" data-receive="' + r.id + '">领取</button>')
          : "";
        return '<div class="d-flex align-items-center gap-2 border rounded p-2 mb-2" style="border-color:var(--line)!important">' +
          '<div class="text-center" style="flex:0 0 62px;color:#cf4a2b;font-weight:800;border-right:1px dashed #e4c4b8">' + val + "</div>" +
          '<div style="flex:1;min-width:0"><div style="font-weight:600;font-size:13.5px">' + LL.esc(r.name) + "</div>" +
          '<div class="text-muted" style="font-size:11.5px">' + limit + " · 至 " +
          LL.esc(String(r.end_time || "").slice(0, 10)) + "</div></div>" + act + "</div>";
      }).join("");
      cpnBox.querySelectorAll("[data-receive]").forEach(b => b.addEventListener("click", async () => {
        if (!LL.user) { LL.openAuth("login"); return; }
        try {
          await LL.api("POST", "/api/coupon/" + b.dataset.receive + "/receive");
          LL.toast("领取成功！可在「我的-我的卡券」查看核销码", "ok");
          await paintCoupons();
        } catch (e) { LL.toast(e.message, "err"); }
      }));
    } catch (e) { cpnBox.innerHTML = '<div class="text-muted small">优惠活动加载失败</div>'; }
  };
  await paintCoupons();
};

