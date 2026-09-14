/* 巷味 · 路由与启动 */
(function () {
  function parse() {
    const h = location.hash || "#/";
    let raw = h[0] === "#" ? h.slice(1) : h;  // 形如 '/admin?tab=users'
    const qi = raw.indexOf("?");
    let p = (qi >= 0 ? raw.slice(0, qi) : raw) || "/";
    if (p !== "/" && p[0] === "/") p = p.slice(1);  // 归一为 'admin'
    const qs = new URLSearchParams(qi >= 0 ? raw.slice(qi + 1) : "");
    return { path: p, qs };
  }

  async function route() {
    const app = document.getElementById("app");
    LL.renderChrome();
    const { path, qs } = parse();
    const need = ["me", "shop", "admin", "apply", "me", "shop"].includes(path);
    const goAuth = () => { LL.openAuth("login"); };

    try {
      if (path === "" || path === "/") {
        await LL.views.home(app, {});
      } else if (path === "s") {
        await LL.views.home(app, {
          kw: qs.get("kw") || "", category: qs.get("category") || "", sort: qs.get("sort") || ""
        });
      } else if (/^m\/\d+$/.test(path)) {
        await LL.views.merchant(app, path.split("/")[1]);
      } else if (/^s\/\d+$/.test(path)) {
        await LL.views.store(app, path.split("/")[1]);   // 门店选购页（下单主体＝门店）
      } else if (/^talk\/(merchant|store|service|package)\/\d+$/.test(path)) {
        const ps = path.split("/");
        await LL.views.talk(app, [ps[1], ps[2]]);
      } else if (path === "me") {
        if (!LL.user) { app.innerHTML = ""; goAuth(); return; }
        await LL.views.me(app, qs.get("tab") || "overview");
      } else if (path === "apply") {
        if (!LL.user) { app.innerHTML = ""; goAuth(); return; }
        await LL.views.apply(app);
      } else if (path === "shop") {
        if (!LL.user) { app.innerHTML = ""; goAuth(); return; }
        if (LL.user.role !== "merchant") { LL.toast("该功能面向已入驻商户", "info"); location.hash = LL.user.role === "admin" ? "#/admin" : "#/apply"; return; }
        await LL.views.shop(app, qs.get("tab") || "overview");
      } else if (path === "admin") {
        if (!LL.user) { app.innerHTML = ""; goAuth(); return; }
        if (LL.user.role !== "admin") { LL.toast("仅平台管理员可访问", "err"); location.hash = "#/"; return; }
        await LL.views.admin(app, qs.get("tab") || "audit");
      } else {
        await LL.views.home(app, {});
      }
    } catch (e) {
      console.error(e);
      app.innerHTML = '<div class="container-xl"><div class="empty"><span class="ic">😵</span>' +
        LL.esc(e.message || "页面加载出错") + "</div></div>";
    }
    if (need) LL.renderChrome();
  }

  window.addEventListener("hashchange", route);
  window.addEventListener("DOMContentLoaded", () => {
    LL.bindAuthEvents();
    LL.renderChrome();
    route();
  });
  LL.route = route;   // 登录成功等场合调用
  if (document.readyState !== "loading") {
    LL.bindAuthEvents();
    LL.renderChrome();
    route();
  }
})();
