/* 巷味 · 公共库：API 封装 / 登录态 / 导航 / 认证弹窗 / UI 工具 */
const LL = (() => {
  const LS_TOKEN = "ll_token", LS_USER = "ll_user";
  const state = { token: localStorage.getItem(LS_TOKEN) || "", user: null };

  try { state.user = JSON.parse(localStorage.getItem(LS_USER) || "null"); } catch (e) { state.user = null; }

  /* ---------- HTTP ---------- */
  async function api(method, url, body) {
    const opt = { method, headers: {} };
    if (body !== undefined) { opt.headers["Content-Type"] = "application/json"; opt.body = JSON.stringify(body); }
    if (state.token) opt.headers["Authorization"] = "Bearer " + state.token;
    let res;
    try { res = await fetch(url, opt); } catch (e) { throw new Error("网络连接失败，请确认服务已启动"); }
    let j = null;
    try { j = await res.json(); } catch (e) { /* 非 JSON */ }
    if (!j) throw new Error("服务器返回异常（HTTP " + res.status + "）");
    if (j.code !== 0) { const e = new Error(j.message || "操作失败"); e.code = j.code; throw e; }
    return j.data;
  }

  /* ---------- 登录态 ---------- */
  function setAuth(token, user) {
    state.token = token; state.user = user;
    localStorage.setItem(LS_TOKEN, token); localStorage.setItem(LS_USER, JSON.stringify(user));
    renderChrome();
  }
  function clearAuth() {
    state.token = ""; state.user = null;
    localStorage.removeItem(LS_TOKEN); localStorage.removeItem(LS_USER);
    renderChrome();
  }
  async function logout() {
    try { await api("POST", "/api/auth/logout"); } catch (e) { /* ignore */ }
    clearAuth();
    toast("已安全退出", "info");
    location.hash = "#/";
  }

  /* ---------- 提示 ---------- */
  function toast(msg, kind = "ok", ms = 2600) {
    const box = document.getElementById("toastBox");
    if (!box) { alert(msg); return; }
    const t = document.createElement("div");
    t.className = "toast align-items-center ll-toast " + kind;
    t.innerHTML = '<div class="d-flex"><div class="toast-body">' + esc(msg) + '</div></div>';
    box.appendChild(t);
    const bs = new bootstrap.Toast(t, { delay: ms });
    bs.show();
    t.addEventListener("hidden.bs.toast", () => t.remove());
  }

  /* ---------- 转义 ---------- */
  function esc(s) {
    return String(s == null ? "" : s).replace(/[&<>"']/g, c => (
      { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
  }

  /* ---------- 分类 / 封面 ---------- */
  const CATS = {
    1: ["美食餐饮", "🍜"], 2: ["休闲娱乐", "🧩"], 3: ["美容美发", "💇"], 4: ["运动健身", "🏋️"],
    5: ["摄影写真", "📷"], 6: ["桌游娱乐", "🎲"], 7: ["KTV欢唱", "🎤"]
  };
  const GRADS = [
    ["#f6e0c4", "#e3c193"], ["#d9e8d5", "#a9c9b0"], ["#f4d7cf", "#e0a491"], ["#d7e2ef", "#a5c0d8"],
    ["#ece0f0", "#c9aad6"], ["#f6ecc8", "#ddd08a"], ["#cfe4e0", "#92bdb3"], ["#f4e3d3", "#e2b98a"]
  ];
  function cat(id) { const c = CATS[id]; return c ? c[0] : "本地生活"; }
  function emoji(id) { const c = CATS[id]; return c ? c[1] : "🏪"; }
  function cover(id) {
    const g = GRADS[Number(id) % GRADS.length];
    return "background:linear-gradient(135deg," + g[0] + "," + g[1] + ")";
  }

  /* ---------- 评分 / 金额 / 状态 ---------- */
  function starsHtml(score) {
    const s = Number(score) || 0;
    if (s <= 0) return '<span class="text-muted small">暂无评分</span>';
    let out = "";
    for (let i = 1; i <= 5; i++) out += i <= Math.round(s) ? '<i class="bi bi-star-fill"></i>' : '<i class="bi bi-star text-muted"></i>';
    return out;
  }
  function money(n) {
    n = Number(n) || 0;
    return "¥" + (Number.isInteger(n) ? n : n.toFixed(2));
  }
  const STATUS_ZH = {
    pending: "待审核", approved: "已通过", rejected: "已驳回",
    on: "上架", off: "下架", open: "营业中", rest: "休息中", closed: "已打烊",
    active: "正常", disabled: "已禁用", visible: "正常", hidden: "已隐藏", deleted: "已删除",
    used: "已核销", unused: "未使用", expired: "已过期", purchased: "已购买", refunded: "已退款",
    resolved: "已处理", published: "已发布", draft: "草稿", offline: "已下线"
  };
  function statusZh(s) { return STATUS_ZH[s] || s; }
  function statusClass(s) {
    const ok = ["approved", "on", "open", "active", "visible", "purchased", "used", "resolved", "published"];
    const no = ["rejected", "off", "deleted", "disabled", "hidden", "closed", "refunded", "expired", "offline"];
    if (ok.includes(s)) return "badge-approved";
    if (no.includes(s)) return "badge-rejected";
    if (s === "pending" || s === "unused") return "badge-pending";
    return "badge-rest";
  }
  function avatarHtml(u) {
    const name = (u && u.nickname) || (u && u.username) || "客";
    return '<span class="avatar" title="' + esc(u ? (u.username || "") : "") + '">' + esc(name[0]) + "</span>";
  }

  /* ---------- 顶栏渲染 ---------- */
  function renderChrome() {
    const user = state.user;
    const nav = document.getElementById("topNav");
    const box = document.getElementById("authBox");
    if (!nav || !box) return;
    const links = [];
    const hash = location.hash;
    const active = t => { const a = t === "#/"; return (a ? hash === "#/" || hash === "" : hash.indexOf(t) === 0) ? "active" : ""; };
    links.push('<a href="#/" class="' + active("#/") + '">🏠 首页</a>');
    if (!user) {
      links.push('<a href="#/apply" class="' + active("#/apply") + '">🏮 开店入驻</a>');
      box.innerHTML =
        '<button class="btn btn-ghost btn-sm px-3" onclick="LL.openAuth(\'login\')">登录</button>' +
        '<button class="btn btn-main btn-sm px-3" onclick="LL.openAuth(\'register\')">注册</button>';
    } else {
      if (user.role === "consumer") {
        links.push('<a href="#/apply" class="' + active("#/apply") + '">🏮 开店入驻</a>');
        links.push('<a href="#/me" class="' + active("#/me") + '">🧭 我的</a>');
      } else if (user.role === "merchant") {
        links.push('<a href="#/shop" class="' + active("#/shop") + '">🧮 经营台</a>');
      } else if (user.role === "admin") {
        links.push('<a href="#/admin" class="' + active("#/admin") + '">🛡️ 平台管理</a>');
      }
      const roleName = { consumer: "食客", merchant: "掌柜", admin: "管理员" }[user.role] || user.role;
      box.innerHTML =
        '<span class="user-chip">' + avatarHtml(user) +
        '<div class="lh-sm d-none d-md-block"><div style="font-size:13px;font-weight:700">' + esc(user.nickname || user.username) +
        '</div><div class="text-muted" style="font-size:11px">' + roleName + "</div></div>" +
        '<button class="btn btn-ghost btn-sm ms-1" title="退出登录" onclick="LL.logout()"><i class="bi bi-box-arrow-right"></i></button></span>';
    }
    nav.innerHTML = links.join("");
    window.scrollTo({ top: 0 });
  }

  /* ---------- 认证弹窗 ---------- */
  let modalInstance = null;
  function authModal() {
    if (!modalInstance) modalInstance = new bootstrap.Modal("#authModal");
    return modalInstance;
  }
  function openAuth(tab) {
    document.querySelectorAll("#authTabs .auth-tab").forEach(b => b.classList.remove("active"));
    const t = tab === "register" ? "register" : "login";
    document.querySelector('#authTabs [data-tab="' + t + '"]').classList.add("active");
    document.getElementById("loginForm").classList.toggle("show", t === "login");
    document.getElementById("registerForm").classList.toggle("show", t === "register");
    authModal().show();
  }
  function bindAuthEvents() {
    document.getElementById("authTabs").addEventListener("click", e => {
      const b = e.target.closest(".auth-tab");
      if (b) openAuth(b.dataset.tab);
    });
    document.getElementById("loginForm").addEventListener("submit", async e => {
      e.preventDefault();
      const f = e.target;
      try {
        const d = await api("POST", "/api/auth/login", { username: f.username.value.trim(), password: f.password.value });
        setAuth(d.token, d.user);
        authModal().hide();
        toast("欢迎回来，" + (d.user.nickname || d.user.username) + "！", "ok");
        LL.route();
      } catch (err) { toast(err.message, "err"); }
    });
    document.getElementById("registerForm").addEventListener("submit", async e => {
      e.preventDefault();
      const f = e.target;
      try {
        const d = await api("POST", "/api/auth/register", {
          username: f.username.value.trim(), password: f.password.value, nickname: f.nickname.value.trim()
        });
        toast("注册成功，请登录", "ok");
        openAuth("login");
        f.reset();
      } catch (err) { toast(err.message, "err"); }
    });
    document.getElementById("searchForm").addEventListener("submit", e => {
      e.preventDefault();
      const v = document.getElementById("searchInput").value.trim();
      location.hash = "#/s?kw=" + encodeURIComponent(v);
    });
  }

  /* ---------- 导出 ---------- */
  return {
    get token() { return state.token; },
    get user() { return state.user; },
    api, setAuth, clearAuth, logout,
    toast, esc, cat, emoji, cover, starsHtml, money,
    statusZh, statusClass, avatarHtml, renderChrome,
    openAuth, bindAuthEvents,
    route: () => { /* 由 app.js 覆盖 */ }
  };
})();
