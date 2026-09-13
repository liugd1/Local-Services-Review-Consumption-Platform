/* 巷味 · 评论区增强组件 + 对象评论区页（#/talk/{type}/{id}）
   依赖 home.js 全局：reviewCard / bindReviewActs / secHead / emptyBox / LL */
(function () {
  /* multipart 图片上传（登录） */
  LL.upload = async function (fd) {
    const opt = { method: "POST", headers: {}, body: fd };
    if (LL.token) opt.headers["Authorization"] = "Bearer " + LL.token;
    const res = await fetch("/api/upload", opt);
    const j = await res.json();
    if (j.code !== 0) throw new Error(j.message || "上传失败");
    return j.data;
  };

  window.__reviewImgs = [];
  const picRender = () => {
    const wrap = document.getElementById("pickWrap");
    if (!wrap) return;
    const arr = window.__reviewImgs || [];
    if (!arr.length) { wrap.innerHTML = ""; return; }
    wrap.innerHTML = arr.map((u, i) =>
      '<span class="position-relative"><img src="' + LL.esc(u) + '" style="width:58px;height:58px;object-fit:cover;border-radius:10px;border:1px solid var(--line)">' +
      '<button type="button" class="btn-close position-absolute top-0 end-0 p-1" style="font-size:10px;background:rgba(0,0,0,.55);border-radius:50%" onclick="ReviewPicker.remove(' + i + ')"></button></span>').join("");
  };

  window.ReviewPicker = {
    init: function () {
      const input = document.getElementById("reviewImgs");
      if (!input || input.dataset.bound) return;
      input.dataset.bound = "1";
      input.addEventListener("change", async function () {
        const files = [...input.files];
        for (const f of files) {
          if ((window.__reviewImgs || []).length >= 6) { LL.toast("最多上传 6 张图片", "err"); break; }
          if (!/^image\//.test(f.type)) continue;
          const fd = new FormData();
          fd.append("file", f);
          try {
            const r = await LL.upload(fd);
            window.__reviewImgs.push(r.url);
            picRender();
          } catch (e) { LL.toast(e.message, "err"); }
        }
        input.value = "";
      });
      picRender();
    },
    remove: function (i) {
      window.__reviewImgs.splice(i, 1);
      picRender();
    },
    reset: function () { window.__reviewImgs = []; picRender(); }
  };

  const esc2 = s => LL.esc(s);

  /* 评论行：每条都可 回复/点赞/举报/删除；子评论平铺并标注“回复 @某人” */
  function commentRow(c) {
    const isAuthor = LL.user && Number(LL.user.id) === Number(c.user_id);
    const isAdmin = LL.user && LL.user.role === "admin";
    const liked = Number(c.liked) === 1;
    const likeBtn = '<button type="button" class="btn btn-link btn-sm p-0 me-2" style="font-size:12px;color:' +
      (liked ? "var(--persimmon)" : "var(--muted)") + '" data-clike="' + c.id + '" data-liked="' + (liked ? "1" : "0") + '"><i class="bi bi-hand-thumbs-up' + (liked ? "-fill" : "") + '"></i> 有用 <b>' + (Number(c.like_count) || 0) + "</b></button>";
    const reportBtn = '<button type="button" class="btn btn-link btn-sm p-0 me-2" style="font-size:12px;color:var(--muted)" data-creport="' + c.id + '"><i class="bi bi-flag"></i> 举报</button>';
    const replyBtn = '<button type="button" class="btn btn-link btn-sm p-0 me-2" style="font-size:12px;color:var(--muted)" data-reply="' + c.id + '" data-name="' + esc2(c.nickname || c.username) + '"><i class="bi bi-reply"></i> 回复</button>';
    const delBtn = (isAuthor || isAdmin)
      ? '<button type="button" class="btn btn-link btn-sm p-0" style="font-size:12px;color:var(--persimmon-deep)" data-delc="' + c.id + '"><i class="bi bi-trash"></i> 删除</button>' : "";
    return '<div class="cmt">' +
      '<div class="d-flex gap-2">' + LL.avatarHtml({ nickname: c.nickname || c.username }) +
      '<div style="flex:1;min-width:0"><div style="font-size:12.5px"><b>' + esc2(c.nickname || c.username) + "</b>" +
      (c.parent_id ? ' 回复 <b style="color:var(--muted)">@' + esc2(c.reply_nickname || c.reply_username) + "</b>" : "") +
      '<span class="text-muted" style="font-size:11.5px"> · ' + esc2(String(c.created_at || "").slice(5, 16)) + "</span></div>" +
      '<div style="font-size:13px;line-height:1.7">' + esc2(c.content) + "</div>" +
      '<span class="d-flex flex-wrap align-items-center">' + likeBtn + reportBtn + replyBtn + delBtn + "</span>" +
      "</div></div></div>";
  }

  // 把任意层回复归到其“楼顶评论”下，保持主区 + 平铺回复
  function groupByRoot(list) {
    const byId = {};
    list.forEach(c => { byId[c.id] = c; });
    const seen = new Set();
    const order = [];
    const roots = [];
    list.forEach(c => { if (!c.parent_id) roots.push(c.id); });
    const walk = (id) => {
      if (seen.has(id) || !byId[id]) return;
      seen.add(id);
      order.push(byId[id]);
      list.forEach(k => { if (Number(k.parent_id) === Number(id)) walk(k.id); });
    };
    roots.forEach(walk);
    list.forEach(c => { if (!seen.has(c.id)) { seen.add(c.id); order.push(c); } });

    const map = new Map();
    const groups = [];
    order.forEach(c => {
      let p = c, n = 0;
      while (p.parent_id && byId[p.parent_id] && n++ < 20) p = byId[p.parent_id];
      const rid = p.parent_id ? c.id : p.id;  // 无有效父时自成一“根”
      if (!map.has(rid)) {
        const arr = [];
        map.set(rid, arr);
        groups.push({ root: byId[rid] || { id: rid, nickname: "已删除用户", username: "", content: "" }, items: arr });
      }
      if (c.id !== rid) map.get(rid).push(c);
    });
    return groups;
  }

  function commentFormHtml(reviewId) {
    if (!LL.user) {
      return '<div class="text-center mt-1"><button class="btn btn-ghost btn-sm" onclick="LL.openAuth(\'login\')">登录后参与评论</button></div>';
    }
    if (LL.user.role === "admin") {
      return '<div class="text-muted mt-1" style="font-size:12.5px">当前为平台管理员身份，可浏览并管理（删除）评论。</div>';
    }
    // 消费者与商户经营者均可发表评论 / 回复评论
    const ph = LL.user.role === "merchant" ? "作为商家参与评论…" : "发表你的看法…";
    return '<form class="d-flex gap-2 mt-1" data-com="' + reviewId + '">' +
      '<input class="form-control form-control-sm" name="content" maxlength="500" placeholder="' + ph + '" required>' +
      '<button class="btn btn-main btn-sm flex-shrink-0">评论</button></form>';
  }

  async function loadComments(reviewId, box, card) {
    try {
      const list = await LL.api("GET", "/api/review/" + reviewId + "/comments");
      const groups = groupByRoot(list);
      let html = "";
      groups.forEach(g => {
        html += '<div class="cmt-root">' + commentRow(g.root);
        if (g.items.length) {
          html += '<div style="border-left:2px solid var(--line);margin:4px 0 2px 26px;padding-left:12px">' +
            g.items.map(c => commentRow(c)).join("") + "</div>";
        }
        html += "</div>";
      });
      box.innerHTML = (html || '<div class="text-muted mb-1" style="font-size:12.5px">还没有评论，来抢沙发</div>') +
        commentFormHtml(reviewId);

      // 评论“有用”（每条评论）
      box.querySelectorAll("[data-clike]").forEach(b => b.addEventListener("click", async () => {
        if (!LL.user) { LL.toast("请先登录", "err"); LL.openAuth("login"); return; }
        if (LL.user.role === "admin") { LL.toast("管理员身份仅浏览，不支持点赞", "info"); return; }
        try {
          const likedNow = b.dataset.liked === "1";
          const r = await LL.api(likedNow ? "DELETE" : "POST", "/api/comments/" + b.dataset.clike + "/like");
          b.dataset.liked = r.liked ? "1" : "0";
          const icon = b.querySelector("i");
          icon.className = r.liked ? "bi bi-hand-thumbs-up-fill" : "bi bi-hand-thumbs-up";
          b.style.color = r.liked ? "var(--persimmon)" : "var(--muted)";
          const cnt = b.querySelector("b");
          if (cnt) cnt.textContent = Number(r.like_count || 0);
        } catch (e) { LL.toast(e.message, "err"); }
      }));

      // 举报某条评论（每条评论）
      box.querySelectorAll("[data-creport]").forEach(b => b.addEventListener("click", async () => {
        if (!LL.user) { LL.toast("请先登录", "err"); LL.openAuth("login"); return; }
        if (LL.user.role === "admin") { LL.toast("管理员身份仅浏览，不支持举报", "info"); return; }
        const reason = window.prompt("举报这条评论，请填写原因：");
        if (!reason || !reason.trim()) return;
        try {
          await LL.api("POST", "/api/comments/" + b.dataset.creport + "/report", { reason: reason.trim() });
          LL.toast("举报已提交，平台将尽快处理", "ok");
        } catch (e) { LL.toast(e.message, "err"); }
      }));

      // 删除评论（作者本人 / 平台管理员）；主评论会级联删除其全部子评论
      box.querySelectorAll("[data-delc]").forEach(b => b.addEventListener("click", async () => {
        if (!confirm("删除这条评论？若它下面还有回复，将一并删除。")) return;
        try {
          const r = await LL.api("DELETE", "/api/comments/" + b.dataset.delc);
          LL.toast(r.deleted > 1 ? "已删除，含 " + r.deleted + " 条（含回复）" : "评论已删除", "ok");
          const cnt = card.querySelector(".act-cmt b");
          if (cnt) cnt.textContent = Math.max(0, Number(cnt.textContent) - Number(r.deleted || 1));
          await loadComments(reviewId, box, card);
        } catch (e) { LL.toast(e.message, "err"); }
      }));

      box.querySelectorAll("[data-reply]").forEach(b => b.addEventListener("click", () => {
        const parent = b.closest(".cmt");
        const old = parent.querySelector(".reply-box");
        if (old) { old.remove(); return; }
        const ip = document.createElement("form");
        ip.className = "reply-box d-flex gap-2 mt-1";
        ip.style.cssText = "border-left:3px solid var(--gold);background:#fdf7e8;border-radius:0 8px 8px 0;padding:.4rem .6rem";
        ip.innerHTML = '<input class="form-control form-control-sm" maxlength="500" placeholder="回复 @' + b.dataset.name + '…" required>' +
          '<button class="btn btn-ghost btn-sm flex-shrink-0">发送</button>';
        parent.appendChild(ip);
        ip.querySelector("input").focus();
        ip.addEventListener("submit", async ev => {
          ev.preventDefault();
          const v = ip.querySelector("input").value.trim();
          if (!v) return;
          try {
            // 回复任意层评论：parent 传被回复评论 id，后端与前端都按其楼顶评论归组展示
            await LL.api("POST", "/api/review/" + reviewId + "/comment", { content: v, parent_id: Number(b.dataset.reply) });
            LL.toast("已追加评论", "ok");
            await loadComments(reviewId, box, card);
          } catch (e) { LL.toast(e.message, "err"); }
        });
      }));

      const mainForm = box.querySelector("[data-com]");
      if (mainForm) mainForm.addEventListener("submit", async ev => {
        ev.preventDefault();
        const v = mainForm.querySelector("input").value.trim();
        if (!v) return;
        try {
          await LL.api("POST", "/api/review/" + reviewId + "/comment", { content: v });
          LL.toast("评论成功", "ok");
          const cnt = card.querySelector(".act-cmt b");
          if (cnt) cnt.textContent = Number(cnt.textContent) + 1;
          await loadComments(reviewId, box, card);
        } catch (e) { LL.toast(e.message, "err"); }
      });
    } catch (e) { box.innerHTML = '<div class="text-muted">' + LL.esc(e.message) + "</div>"; }
  }

  window.ReviewPanel = {
    toggle: function (card, reviewId) {
      let box = card.querySelector(".rv-comments");
      if (box) { box.remove(); return; }
      box = document.createElement("div");
      box.className = "rv-comments mt-2";
      box.style.cssText = "background:#faf6ea;border:1px dashed var(--line);border-radius:12px;padding:.7rem .8rem";
      box.innerHTML = '<div class="text-muted py-1" style="font-size:13px">加载评论…</div>';
      card.appendChild(box);
      loadComments(reviewId, box, card);
    }
  };

  /* ---------------- 对象评论区页 #/talk/{type}/{id} ---------------- */
  const TYPE_META = {
    merchant: ["🏬", "店铺"], store: ["🏪", "门店"],
    service: ["🧾", "服务项目"], package: ["🎁", "优惠套餐"]
  };

  LL.views.talk = async function (el, parts) {
    const type = parts[0], id = Number(parts[1]);
    if (!TYPE_META[type] || !id) { el.innerHTML = '<div class="container-xl">' + emptyBox("参数有误") + "</div>"; return; }

    let info;
    try { info = await LL.api("GET", "/api/targets/" + type + "/" + id + "/info"); }
    catch (e) { el.innerHTML = '<div class="container-xl">' + emptyBox(e.message) + "</div>"; return; }

    const sum = info.summary || {};
    const star = Number(sum.avg_score) || 0;
    const meta = TYPE_META[type];
    const user = LL.user;

    let formHtml = "";
    if (user && user.role === "consumer") {
      const sel = () => { let s = ""; for (let v = 5; v >= 1; v--) s += '<option value="' + v + '">' + "★".repeat(v) + "☆".repeat(5 - v) + "</option>"; return s; };
      formHtml = '<div class="panel mb-3"><h5><i class="bi bi-pencil-square"></i> 写条评价</h5><form id="reviewForm">' +
        '<div class="row g-3"><div class="col-4"><label class="form-label">环境</label><select name="env_score" class="form-select">' + sel() + "</select></div>" +
        '<div class="col-4"><label class="form-label">服务</label><select name="service_score" class="form-select">' + sel() + "</select></div>" +
        '<div class="col-4"><label class="form-label">性价比</label><select name="price_score" class="form-select">' + sel() + "</select></div>" +
        '<div class="col-12"><label class="form-label">说说体验</label>' +
        '<textarea class="form-control" name="content" rows="3" maxlength="2000" placeholder="这次体验怎么样？"></textarea></div>' +
        '<div class="col-12"><label class="form-label">晒图（最多 6 张）</label>' +
        '<div class="d-flex gap-2 flex-wrap mb-1" id="pickWrap"></div>' +
        '<label class="btn btn-ghost btn-sm"><i class="bi bi-camera"></i> 选择图片' +
        '<input type="file" id="reviewImgs" accept="image/*" multiple hidden></label></div>' +
        "</div><button class='btn btn-fire mt-3'><i class='bi bi-send'></i> 发布评价</button></form></div>";
    } else if (!user) {
      formHtml = '<div class="panel text-center mb-3"><p class="mb-2 text-muted">登录后即可评价与互动</p>' +
        '<button class="btn btn-main" onclick="LL.openAuth(\'login\')">去登录</button></div>';
    } else {
      const who = user.role === "admin" ? "平台管理员" : "商户经营者";
      formHtml = '<div class="panel text-center mb-3 text-muted" style="font-size:13px">当前为' + who +
        "身份，可浏览本条目的评价；发表评价请使用消费者账号。</div>";
    }

    el.innerHTML = '<div class="container-xl" style="max-width:920px">' +
      '<div class="panel" style="background:linear-gradient(135deg,#fbf5e6,#f4ead2)">' +
      '<div class="d-flex align-items-center gap-3 flex-wrap">' +
      '<div class="row-thumb" style="width:70px;min-height:60px;border-radius:14px;font-size:30px;' + LL.cover(info.category_id || 0) + '">' + meta[0] + "</div>" +
      '<div style="flex:1;min-width:200px"><div style="font-size:12px;letter-spacing:.2em;color:var(--muted)">' + meta[1] + " · 评论区</div>" +
      '<h3 class="mb-0">' + LL.esc(info.name) + "</h3>" +
      '<div class="text-muted" style="font-size:13px">所属 <a href="#/m/' + info.merchant_id + '" style="color:var(--green)">' + LL.esc(info.merchant_name || "") + "</a></div>" +
      "</div><div class='text-end'>" +
      '<div class="score" style="font-size:1.6rem">' + (star ? star.toFixed(1) : "新") + "</div>" +
      '<div class="stars">' + LL.starsHtml(star) + '</div>' +
      '<div class="text-muted" style="font-size:12px">' + (sum.cnt || 0) + " 条评价</div></div></div>" +
      '<a class="btn btn-ghost btn-sm mt-2" href="#/m/' + info.merchant_id + '"><i class="bi bi-arrow-left"></i> 返回店铺</a>' +
      "</div>" +
      formHtml +
      '<div class="panel"><h5><i class="bi bi-chat-square-dots"></i> ' + meta[1] + "口碑</h5><div id='talkBody'></div></div>" +
      '<div id="talkMore" class="text-center mt-1"></div></div>';

    ReviewPicker.init();

    const body = el.querySelector("#talkBody");
    let page = 0;

    const loadList = async (reset) => {
      if (reset) page = 0;
      try {
        const d = await LL.api("GET", "/api/targets/" + type + "/" + id + "/reviews?page=" +
          (page + 1) + "&size=6");
        const moreEl = el.querySelector("#talkMore");
        if (!d.list.length) {
          if (reset) body.innerHTML = emptyBox("还没有人评价过，来做第一个吧！");
          moreEl.innerHTML = "";
          return;
        }
        body.insertAdjacentHTML("beforeend", d.list.map(reviewCard).join(""));
        page = d.page;
        moreEl.innerHTML = page * 6 < d.total
          ? '<button class="btn btn-ghost btn-sm" id="talkMoreBtn">加载更多 ↓</button>' : "";
        const mb = el.querySelector("#talkMoreBtn");
        if (mb) mb.addEventListener("click", () => loadList(false));
        bindReviewActs(body);
        if (reset) {
          const fq = Number(new URLSearchParams((location.hash.split("?")[1]) || "").get("focus")) || 0;
          if (fq) {
            const card = body.querySelector('.review-card[data-id="' + fq + '"]');
            if (card) {
              card.style.boxShadow = "0 0 0 3px var(--gold)";
              setTimeout(() => card.scrollIntoView({ behavior: "smooth", block: "center" }), 250);
            }
          }
        }
      } catch (e) { if (reset) body.innerHTML = '<div class="empty">' + LL.esc(e.message) + "</div>"; }
    };

    const form = el.querySelector("#reviewForm");
    if (form) form.addEventListener("submit", async ev => {
      ev.preventDefault();
      const f = form;
      try {
        await LL.api("POST", "/api/review", {
          target_type: type, target_id: id, merchant_id: Number(info.merchant_id),
          env_score: Number(f.env_score.value), service_score: Number(f.service_score.value),
          price_score: Number(f.price_score.value), content: f.content.value.trim(),
          images: window.__reviewImgs ? window.__reviewImgs.slice() : []
        });
        LL.toast("评价已发布", "ok");
        window.__reviewImgs = [];
        const pw = el.querySelector("#pickWrap");
        if (pw) pw.innerHTML = "";
        await LL.views.talk(el, parts);  // 刷新统计与列表
      } catch (e) {
        LL.toast(e.message, "err");
        if (String(e.message).indexOf("已评价") >= 0) form.remove();
      }
    });

    await loadList(true);
  };
})();
