import { createRequire } from "module";
const req = createRequire(process.env.npm_root_global + "/package.json");
const { chromium } = req("playwright");
const BASE = "http://127.0.0.1:8080";

async function loginToken(browser, u, p) {
  const ctx = await browser.newContext();
  const pg = await ctx.newPage();
  await pg.goto(BASE + "/", { waitUntil: "domcontentloaded" });
  const r = await pg.evaluate(async ({ u, p }) => {
    const res = await fetch("/api/auth/login", {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username: u, password: p })
    });
    const j = await res.json();
    if (j.code !== 0) throw new Error(j.message);
    return { token: j.data.token, user: j.data.user };
  }, { u, p });
  await ctx.close();
  return r;
}

async function visit(token, user, hash, label) {
  const ctx = await browser.newContext();
  const page = await ctx.newPage();
  const errs = [];
  page.on("console", m => { if (m.type() === "error") errs.push(m.text()); });
  page.on("pageerror", e => errs.push(String(e)));
  await ctx.addInitScript(({ token, user }) => {
    localStorage.setItem("ll_token", token);
    localStorage.setItem("ll_user", JSON.stringify(user));
  }, { token, user });
  await page.goto(BASE + "/#" + hash, { waitUntil: "networkidle" });
  await page.waitForTimeout(600);
  const text = await page.evaluate(() => document.body.innerText);
  console.log("=== " + label + " errors=" + errs.length);
  errs.slice(0, 6).forEach(e => console.log("  ERR: " + String(e).slice(0, 300)));
  console.log("  text: " + text.replace(/\s+/g, " ").slice(0, 600));
  await ctx.close();
}

const browser = await chromium.launch({ headless: true });
const alice = await loginToken(browser, "alice", "123456");
const bob = await loginToken(browser, "bob", "123456");
const admin = await loginToken(browser, "admin", "admin123");
await visit(alice.token, alice.user, "/m/1", "alice-merchant");
await visit(alice.token, alice.user, "/me?tab=coupons", "alice-my-coupons");
await visit(bob.token, bob.user, "/shop?tab=coupons", "bob-shop-coupons");
await visit(bob.token, bob.user, "/shop?tab=stats", "bob-shop-stats");
await visit(admin.token, admin.user, "/admin?tab=stats", "admin-stats");
await browser.close();

