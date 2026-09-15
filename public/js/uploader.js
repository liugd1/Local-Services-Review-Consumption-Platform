/* 巷味 · 通用图片上传组件
   用法：const picker = LL.imagePicker(容器元素, { max: 6, urls: [...], hint: "..." });
        picker.getUrls() / picker.setUrls([]) / picker.reset()
   依赖：POST /api/upload（multipart，字段名 file） */
LL.imagePicker = function (container, opts) {
  const o = Object.assign({ max: 6, urls: [], hint: "支持 jpg / png / gif / webp，单张不超过 8MB" }, opts || {});
  let urls = (o.urls || []).filter(Boolean).slice();
  const inputId = "ipk_" + Math.random().toString(36).slice(2, 9);

  // 上传实现：优先复用 talk.js 的 LL.upload，缺失时兜底
  const upload = LL.upload || (async fd => {
    const opt = { method: "POST", headers: {}, body: fd };
    if (LL.token) opt.headers["Authorization"] = "Bearer " + LL.token;
    const res = await fetch("/api/upload", opt);
    const j = await res.json();
    if (j.code !== 0) throw new Error(j.message || "上传失败");
    return j.data;
  });

  container.innerHTML =
    '<div class="d-flex gap-2 flex-wrap align-items-center" data-ip-list></div>' +
    '<div class="mt-1"><label class="btn btn-ghost btn-sm mb-0" for="' + inputId + '">' +
    '<i class="bi bi-camera"></i> 添加图片</label> ' +
    '<span class="text-muted" style="font-size:12px">' + LL.esc(o.hint) + "（最多 " + o.max + " 张）</span></div>" +
    '<input id="' + inputId + '" type="file" accept="image/*" multiple hidden>';

  const list = container.querySelector("[data-ip-list]");
  const input = container.querySelector("#" + inputId);

  function render() {
    if (!urls.length) { list.innerHTML = ""; return; }
    list.innerHTML = urls.map((u, i) =>
      '<span class="position-relative d-inline-block">' +
      '<img src="' + LL.esc(u) + '" title="点击查看大图" data-view="' + i + '" ' +
      'style="width:68px;height:68px;object-fit:cover;border-radius:10px;border:1px solid var(--line);cursor:zoom-in">' +
      '<button type="button" class="btn-close position-absolute top-0 end-0 p-1" data-rm="' + i + '" ' +
      'style="font-size:10px;background:rgba(0,0,0,.55);border-radius:50%" title="移除"></button></span>').join("");
    list.querySelectorAll("[data-rm]").forEach(b => b.addEventListener("click", () => {
      urls.splice(Number(b.dataset.rm), 1);
      render();
    }));
    list.querySelectorAll("[data-view]").forEach(img => img.addEventListener("click", () => {
      window.open(urls[Number(img.dataset.view)], "_blank");
    }));
  }

  input.addEventListener("change", async () => {
    const files = [...input.files];
    for (const f of files) {
      if (urls.length >= o.max) { LL.toast("最多上传 " + o.max + " 张图片", "err"); break; }
      if (!/^image\//.test(f.type)) continue;
      const fd = new FormData();
      fd.append("file", f);
      try {
        const r = await upload(fd);
        urls.push(r.url);
        render();
      } catch (e) { LL.toast(e.message, "err"); }
    }
    input.value = "";
  });

  render();
  return {
    getUrls: () => urls.slice(),
    setUrls: a => { urls = (a || []).filter(Boolean).slice(); render(); },
    reset: () => { urls = []; render(); }
  };
};

/* 把 "a.png,b.png" 形式的图片串转成数组 */
LL.imgList = function (csv) {
  if (!csv) return [];
  if (Array.isArray(csv)) return csv.filter(Boolean);
  return String(csv).split(",").map(x => x.trim()).filter(Boolean);
};

/* 只读缩略图组（详情页/列表展示用） */
LL.imgThumbs = function (csv, size) {
  const arr = LL.imgList(csv);
  if (!arr.length) return "";
  const s = size || 60;
  return '<div class="d-flex gap-1 flex-wrap">' + arr.map(u =>
    '<img src="' + LL.esc(u) + '" data-view-src="' + LL.esc(u) + '" title="点击查看大图" ' +
    'style="width:' + s + 'px;height:' + s + 'px;object-fit:cover;border-radius:8px;border:1px solid var(--line);cursor:zoom-in">'
  ).join("") + "</div>";
};

/* 缩略图点击放大（事件委托，全局一次） */
document.addEventListener("click", e => {
  const img = e.target.closest("[data-view-src]");
  if (img) window.open(img.dataset.viewSrc, "_blank");
});
