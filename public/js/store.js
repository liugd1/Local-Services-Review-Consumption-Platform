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
            (x.items_text ? "含：" + LL.esc(x.items_text) + " · " : "") +
            LL.esc(x.content || "") + (x.valid_days ? " · 有效期 " + x.valid_days + " 天" : "") +
            (Number(x.limit_count) > 0 ? " · 每人限购 " + x.limit_count + " 份" : "") + "</small></div>" +
            '<span class="good-price">' + LL.money(x.price) +
            (Number(x.origin_price) > 0 && Number(x.origin_price) > Number(x.price)
              ? '<div class="text-muted" style="font-size:11px;text-decoration:line-through">¥' + LL.money(x.origin_price) + "</div>" : "") + "</span>" +
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

  // ---------- 选择优惠券（下单前，仅列出适用于该项目的券） ----------
  const pickCoupon = (itemName, price, list) => new Promise(resolve => {
    const old = document.getElementById("couponPickerModal");
    if (old) old.remove();
    const modalEl = document.createElement("div");
    modalEl.id = "couponPickerModal";
    modalEl.className = "modal fade";
    modalEl.tabIndex = -1;
    modalEl.innerHTML =
      '<div class="modal-dialog modal-dialog-centered modal-dialog-scrollable"><div class="modal-content">' +
      '<div class="modal-header"><div><h5 class="modal-title" style="font-family:var(--serif)">选择优惠券</h5>' +
      '<div class="text-muted" style="font-size:12.5px">' + LL.esc(itemName) + " · 原价 ¥" + LL.money(price) +
      "　共 " + list.length + " 张可用</div></div>" +
      '<button type="button" class="btn-close" data-bs-dismiss="modal"></button></div>' +
      '<div class="modal-body p-2">' +
      list.map(c => {
        const val = c.type === "discount"
          ? (Number(c.discount_rate) * 10).toFixed(1) + " 折" : "¥" + LL.money(c.face_value);
        return '<div class="d-flex align-items-center justify-content-between border rounded p-2 mb-2" style="border-color:var(--line)!important">' +
          '<div style="min-width:0"><div class="d-flex align-items-center gap-2 flex-wrap"><b>' + LL.esc(c.name) + "</b>" +
          '<span class="badge-soft badge-approved">-' + LL.money(c.discount) + "</span></div>" +
          '<div class="text-muted" style="font-size:11.5px">' + LL.esc(c.scope_desc || "全场通用") +
          (Number(c.threshold) ? " · 满 " + LL.money(c.threshold) : "") +
          " · 至 " + LL.esc(String(c.end_time || "").slice(0, 10)) + "</div></div>" +
          '<div class="text-end" style="white-space:nowrap"><div class="text-muted" style="font-size:11.5px">实付</div>' +
          '<b style="color:var(--persimmon)">¥' + LL.money(c.payable) + "</b> " +
          '<button class="btn btn-fire btn-sm ms-1" data-use-coupon="' + c.claim_id + '" data-discount="' + c.discount + '">使用</button></div></div>';
      }).join("") +
      "</div>" +
      '<div class="modal-footer"><span class="text-muted me-auto" style="font-size:12.5px">不使用优惠券则按原价下单</span>' +
      '<button class="btn btn-ghost btn-sm" data-no-coupon>不使用优惠券</button>' +
      '<button class="btn btn-ghost btn-sm" data-bs-dismiss="modal">取消</button></div>' +
      "</div></div>";
    document.body.appendChild(modalEl);
    const modal = new bootstrap.Modal(modalEl);
    let done = false;
    modalEl.addEventListener("click", e => {
      const useBtn = e.target.closest("[data-use-coupon]");
      if (useBtn) {
        done = true;
        modal.hide();
        resolve({ claimId: Number(useBtn.dataset.useCoupon), discount: Number(useBtn.dataset.discount) });
        return;
      }
      if (e.target.closest("[data-no-coupon]")) {
        done = true;
        modal.hide();
        resolve({ claimId: 0, discount: 0 });
      }
    });
    modalEl.addEventListener("hidden.bs.modal", () => {
      modalEl.remove();
      if (!done) resolve(null);   // 关闭 = 放弃下单
    });
    modal.show();
  });

  // ---------- 下单（下单主体＝门店） ----------
  el.querySelectorAll("[data-order]").forEach(btn => btn.addEventListener("click", async () => {
    if (!LL.user) { LL.openAuth("login"); return; }
    if (LL.user.role !== "consumer") { LL.toast("请使用消费者账号下单", "info"); return; }
    const type = btn.dataset.order;
    const id = Number(btn.dataset.id);
    const label = type === "service" ? "服务" : "套餐";
    const raw = btn.innerHTML;
    btn.disabled = true;
    btn.innerHTML = "请求中…";
    try {
      // 该项目在本门店可用的优惠券（含抵扣计算）
      let usable = [];
      try { usable = await LL.api("GET", "/api/store/" + s.id + "/coupons/usable?item_type=" + type + "&item_id=" + id); }
      catch (e) { usable = []; }
      let claimId = 0, discount = 0;
      if (usable.length) {
        const pick = await pickCoupon(btn.dataset.name, usable[0].payable + usable[0].discount, usable);
        if (!pick) { btn.disabled = false; btn.innerHTML = raw; return; }  // 放弃
        claimId = pick.claimId;
        discount = pick.discount;
        btn.innerHTML = "提交中…";
      }
      const o = await LL.api("POST", "/api/store/" + s.id + "/order",
        { item_type: type, item_id: id, coupon_claim_id: claimId });
      LL.toast("下单成功！" + (Number(o.discount) > 0
        ? "已抵扣 ¥" + LL.money(o.discount) + "，实付 ¥" + LL.money(o.amount) + "；"
        : "订单号 " + o.order_no + "；") + "可在「我的-我的订单」核销或退款", "ok", 3400);
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
          '<div class="text-muted" style="font-size:11.5px">' + limit +
          " · 适用：" + LL.esc(r.scope_desc || "全场通用") + "</div>" +
          '<div class="text-muted" style="font-size:11.5px">至 ' +
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

