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

const browser = await chromium.launch({ headless: true });
const alice = await loginToken(browser, "alice", "123456");
const ctx = await browser.newContext();
await ctx.addInitScript(({ token, user }) => {
  localStorage.setItem("ll_token", token);
  localStorage.setItem("ll_user", JSON.stringify(user));
}, alice);
const page = await ctx.newPage();
page.on("dialog", d => d.accept());
const errs = [];
page.on("pageerror", e => errs.push(String(e)));

// 1 详情页领 88 折券
await page.goto(BASE + "/#/m/1", { waitUntil: "networkidle" });
await page.waitForTimeout(500);
const recv = page.locator("[data-receive]").first();
const rc = await recv.count();
console.log("receive buttons:", rc);
if (rc) { await recv.click(); await page.waitForTimeout(900); }
const t1 = await page.evaluate(() => document.body.innerText);
console.log("领券后仍存在领取按钮:", (await page.locator("[data-receive]").count()) === 0 || t1.includes("已领取"));

// 2 购买套餐
await page.goto(BASE + "/#/m/1", { waitUntil: "networkidle" });
await page.waitForTimeout(400);
const buy = page.locator("[data-buy-pkg]").first();
console.log("buy buttons:", await buy.count());
if (await buy.count()) {
  await buy.click();
  await page.waitForTimeout(1200);
}

// 3 我的订单 -> 核销第一个待使用订单
await page.goto(BASE + "/#/me?tab=orders", { waitUntil: "networkidle" });
await page.waitForTimeout(800);
const t3 = await page.evaluate(() => document.body.innerText);
console.log("orders含待使用:", t3.includes("已购买"));
const use = page.locator("[data-use]").first();
console.log("use buttons:", await use.count());
if (await use.count()) {
  await use.click();
  await page.waitForTimeout(1200);
  const t4 = await page.evaluate(() => document.body.innerText);
  console.log("核销后状态已使用:", t4.includes("已核销") || t4.includes("已使用"));
}
console.log("页面JS错误:", errs.length);
await browser.close();
