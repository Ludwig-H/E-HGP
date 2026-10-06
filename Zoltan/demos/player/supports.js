/*
 * Lecteur des vidéos « hiérarchie des supports » des exemples (Zoltan/demos/videos_hgp_hdbscan).
 *
 * L'arbre couvrant d'ordre k de Morse HGP 3D v11 (sortie supports, format MHGP11SP version 2) : ses arêtes de
 * Kruskal, naissances et fusions, portent chacune leur support S*, arête (q2), triangle (q3) ou tétraèdre (q4) ; un
 * nœud est réalisé par les supports des boules de son sous-arbre. Le niveau r (rayon de la sphère de S*) croît en
 * échelle logarithmique : chaque support apparaît au niveau de sa boule, et la hiérarchie se dessine.
 *
 * Lecture :
 * - halo coloré sous les points : la vérité terrain, un halo par objet (A, B, C) ;
 * - supports de la couleur d'un objet : ceux du nœud qui suit cet objet (sa branche) ; rouges : un nœud qui réunit
 *   les branches de deux objets ou plus ; gris : les autres nœuds ;
 * - en bas, l'IoU des sites des supports de chaque branche suivie en fonction de r (points void exclus).
 *
 * Rendu Canvas2D pur, déterministe : l'image ne dépend que du temps t, de la scène (window.SUPPORTS_SCENE, écrite par
 * tools/supports_scene.py) et du thème. Thèmes de Percolia.com, mêmes couleurs que duel.js.
 */
(function () {
  'use strict';

  const W = 1920, H = 1080;
  const COL = { x: 24, w: 1872 };
  const VIEW = { y: 118, h: 640 };
  const LANES = { y: 770, h: 236 };
  const FOOT = { y: 1016, h: 48 };
  const FONT = '"DejaVu Sans", "Helvetica Neue", Arial, sans-serif';
  const NONE = 0, FUSED = 3;  // états des lignes de suivi (tools/supports_scene.py : 1 fragment, 2 retrouvé)
  const EPS = 1 + 1e-12;
  const INTRO_ORBIT = 26, INTRO_LIFT = 10;

  // Mêmes palettes que duel.js (contrastes et daltonisme contrôlés par tools/test_duel.py).
  const THEMES = {
    dark: {
      bg: '#071b2e', panel: '#0f2c48', frame: '#3a5773', grid: '#1d4466', text: '#eaf5f7', dim: '#9fb4c4',
      plate: 'rgba(15,44,72,0.88)', fusion: '#f0606e', objects: ['#62b3ff', '#ffc53d', '#5ee8c8'],
      halo: 0.24, alone: 'rgba(159,180,196,0.30)', other: '#585864', ok: '#5ee8c8', otherLine: '#7d8ba0',
    },
    light: {
      bg: '#ffffff', panel: '#f4fafb', frame: '#b9cad4', grid: '#d3e0e6', text: '#082c4c', dim: '#4a5f71',
      plate: 'rgba(244,250,251,0.92)', fusion: '#a8102c', objects: ['#1f5fbf', '#c27a00', '#00897b'],
      halo: 0.18, alone: 'rgba(93,115,133,0.30)', other: '#b8b8c7', ok: '#00897b', otherLine: '#8f9cad',
    },
  };
  let C = THEMES.dark;
  function themeName(name) { return name === 'light' || name === 'clair' ? 'light' : 'dark'; }
  function setTheme(name) { C = THEMES[themeName(name)]; return themeName(name); }

  const lerp = (a, b, u) => a + (b - a) * u;
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
  const smooth = (u) => { u = clamp(u, 0, 1); return u * u * (3 - 2 * u); };
  const frNum = (v, d) => v.toFixed(d).replace('.', ',');
  function typo(str) {
    return String(str).replace(/'/g, '’').replace(/ ([:;%?!»])/g, ' $1').replace(/« /g, '« ');
  }
  function hexToRgb(h) { const v = parseInt(h.slice(1), 16); return [(v >> 16) & 255, (v >> 8) & 255, v & 255]; }
  function rgba(h, a) { const c = hexToRgb(h); return `rgba(${c[0]},${c[1]},${c[2]},${a})`; }
  function iouText(v) {
    let d = 2;
    while (d < 6 && v !== 0.5 && Number(v.toFixed(d)) === 0.5) d++;
    return frNum(v, d);
  }
  function cm(r) { return frNum(100 * r, 1) + ' cm'; }
  function thousands(n) { return String(n).replace(/\B(?=(\d{3})+(?!\d))/g, ' '); }

  // ---------------------------------------------------------------- texte
  function text(ctx, str, x, y, size, color, opt) {
    opt = opt || {};
    ctx.font = `${opt.bold ? 'bold ' : ''}${size}px ${FONT}`;
    ctx.fillStyle = color;
    ctx.textAlign = opt.align || 'left';
    ctx.textBaseline = opt.base || 'alphabetic';
    ctx.fillText(typo(str), x, y);
  }
  function measure(ctx, str, size, bold) {
    ctx.font = `${bold ? 'bold ' : ''}${size}px ${FONT}`;
    return ctx.measureText(typo(str)).width;
  }
  function roundRect(ctx, x, y, w, h, r) {
    ctx.beginPath();
    ctx.moveTo(x + r, y); ctx.lineTo(x + w - r, y); ctx.arcTo(x + w, y, x + w, y + r, r);
    ctx.lineTo(x + w, y + h - r); ctx.arcTo(x + w, y + h, x + w - r, y + h, r);
    ctx.lineTo(x + r, y + h); ctx.arcTo(x, y + h, x, y + h - r, r);
    ctx.lineTo(x, y + r); ctx.arcTo(x, y, x + r, y, r); ctx.closePath();
  }
  function richWidth(ctx, parts, size) { return parts.reduce((s, p) => s + measure(ctx, p[0], size, p[2]), 0); }
  function rich(ctx, parts, x, y, size, align) {
    let xx = align === 'center' ? x - richWidth(ctx, parts, size) / 2 : x;
    for (const [str, col, bold] of parts) {
      text(ctx, str, xx, y, size, col, { bold });
      xx += measure(ctx, str, size, bold);
    }
  }
  function roleColor(role) {
    if (/^obj[0-2]$/.test(role)) return C.objects[Number(role[3])];
    const c = { ok: C.ok, fusion: C.fusion, text: C.text, dim: C.dim }[role];
    if (!c) throw new Error(`rôle de couleur inconnu : ${role}`);
    return c;
  }

  // ---------------------------------------------------------------- scène
  function prepare(scene) {
    if (scene.schema !== 'ehgp.zoltan.supports.v1') throw new Error('scène inconnue : relancer tools/supports_scene.py');
    for (const p of scene.timing.pauses) for (const b of p.badges) { roleColor(b.border); b.parts.forEach((q) => roleColor(q[1])); }
    const n = scene.points.x.length, sp = scene.supports, ns = sp.level.length;
    const S = {
      scene, n, nobj: scene.objects.length,
      x: Float64Array.from(scene.points.x), y: Float64Array.from(scene.points.y), z: Float64Array.from(scene.points.z),
      gt: Int8Array.from(scene.gt), timing: scene.timing, background: scene.gt.some((g) => g < 0),
      keyT: scene.timing.schedule.map((k) => k[0]), keyR: scene.timing.schedule.map((k) => k[1]),
      rmin: scene.timing.rmin, rmax: scene.timing.rmax,
      ns, level: Float64Array.from(sp.level), post: Int32Array.from(sp.post), arity: Uint8Array.from(sp.arity),
      at: new Int32Array(ns + 1), sites: Int32Array.from(sp.sites),
      cum: [new Int32Array(ns + 1), new Int32Array(ns + 1), new Int32Array(ns + 1)],
    };
    for (let i = 0; i < ns; i++) {
      S.at[i + 1] = S.at[i] + S.arity[i];
      for (let a = 0; a < 3; a++) S.cum[a][i + 1] = S.cum[a][i] + (S.arity[i] === a + 2 ? 1 : 0);
    }
    if (S.at[ns] !== S.sites.length) throw new Error('supports : arités et sites incohérents');
    S.view = fitView(S);
    return S;
  }

  function rAt(S, t) {
    const T = S.keyT, R = S.keyR;
    if (t <= T[0]) return R[0];
    for (let i = 1; i < T.length; i++) {
      if (t <= T[i]) {
        if (R[i - 1] === R[i]) return R[i];
        const u = smooth((t - T[i - 1]) / Math.max(1e-9, T[i] - T[i - 1]));
        return Math.exp(lerp(Math.log(R[i - 1]), Math.log(R[i]), u));
      }
    }
    return R[R.length - 1];
  }
  // nombre de supports de niveau <= r (supports triés par niveau)
  function prefix(S, r) {
    let lo = 0, hi = S.ns;
    while (lo < hi) { const mid = (lo + hi) >> 1; if (S.level[mid] <= r * EPS) lo = mid + 1; else hi = mid; }
    return lo;
  }
  // nœud de la chaîne d'un objet vivant au niveau r : [niveau, postordre, taille] ou null
  function nodeAt(chain, r) {
    let cur = null;
    for (const c of chain) { if (c[0] <= r * EPS) cur = c; else break; }
    return cur;
  }
  function trackAt(track, r) {
    let best = null;
    for (const row of track) { if (row[0] <= r * EPS) best = row; else break; }
    return best;
  }
  const foundBy = (track, r) => track.some((row) => row[0] <= r * EPS && row[1] > 0.5);

  // ---------------------------------------------------------------- caméra (comme duel.js)
  function cameraOf(S, t) {
    const v = S.view, intro = S.timing.intro;
    let az = v.az, el = v.el;
    if (t < intro) {
      const u = smooth((t - S.timing.hold) / (intro - S.timing.hold));
      az = lerp(v.az - INTRO_ORBIT, v.az, u);
      el = lerp(v.el + INTRO_LIFT, v.el, u);
    }
    return basis(v, az, el);
  }
  function basis(v, az, el) {
    const a = az * Math.PI / 180, e = el * Math.PI / 180, tg = v.target;
    const pos = [tg[0] + v.dist * Math.cos(e) * Math.sin(a), tg[1] - v.dist * Math.cos(e) * Math.cos(a), tg[2] + v.dist * Math.sin(e)];
    const f = norm([tg[0] - pos[0], tg[1] - pos[1], tg[2] - pos[2]]);
    const right = norm(cross(f, [0, 0, 1]));
    const up = cross(right, f);
    return { pos, f, right, up, focal: v.focal };
  }
  function cross(a, b) { return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]; }
  function norm(a) { const l = Math.hypot(a[0], a[1], a[2]); return [a[0] / l, a[1] / l, a[2] / l]; }
  function project(cam, X, Y, Z) {
    const dx = X - cam.pos[0], dy = Y - cam.pos[1], dz = Z - cam.pos[2];
    const zc = dx * cam.f[0] + dy * cam.f[1] + dz * cam.f[2];
    return [cam.focal * (dx * cam.right[0] + dy * cam.right[1] + dz * cam.right[2]) / zc,
      -cam.focal * (dx * cam.up[0] + dy * cam.up[1] + dz * cam.up[2]) / zc, zc];
  }
  function fitView(S) {
    const fit = [];
    for (let i = 0; i < S.n; i++) if (S.gt[i] >= 0) fit.push(i);
    let rad = 0, zmax = 0;
    for (const i of fit) { rad = Math.max(rad, Math.hypot(S.x[i], S.y[i], S.z[i])); zmax = Math.max(zmax, S.z[i]); }
    const view = S.scene.view || { az: 0, el: 30 };
    const v = { target: [0, 0, zmax / 2], dist: 5.5 * Math.max(rad, 0.5), az: view.az, el: view.el, focal: 1 };
    let w = 0, h = 0, cy = 0, cnt = 0;
    for (const [az, el] of [[v.az, v.el], [v.az - INTRO_ORBIT, v.el + INTRO_LIFT], [v.az - INTRO_ORBIT / 2, v.el + INTRO_LIFT / 2]]) {
      const cam = basis(v, az, el);
      let x0 = Infinity, x1 = -Infinity, y0 = Infinity, y1 = -Infinity;
      for (const i of fit) {
        const p = project(cam, S.x[i], S.y[i], S.z[i]);
        x0 = Math.min(x0, p[0]); x1 = Math.max(x1, p[0]); y0 = Math.min(y0, p[1]); y1 = Math.max(y1, p[1]);
      }
      w = Math.max(w, 2 * Math.max(-x0, x1)); h = Math.max(h, y1 - y0); cy += (y0 + y1) / 2; cnt++;
    }
    v.focal = Math.min(0.80 * COL.w / w, (VIEW.h - 170) / h) * (S.background ? 0.86 : 1);
    v.shiftY = -v.focal * cy / cnt + 60;
    return v;
  }

  // ---------------------------------------------------------------- dessin
  function panel(ctx, x, y, w, h) {
    ctx.fillStyle = C.panel; roundRect(ctx, x, y, w, h, 10); ctx.fill();
    ctx.strokeStyle = C.frame; ctx.lineWidth = 1; roundRect(ctx, x + 0.5, y + 0.5, w - 1, h - 1, 10); ctx.stroke();
  }
  function disc(ctx, x, y, r) { ctx.moveTo(x + r, y); ctx.arc(x, y, r, 0, 2 * Math.PI); }
  let haloCanvas = null;
  function haloLayer() {
    if (!haloCanvas) { haloCanvas = document.createElement('canvas'); haloCanvas.width = COL.w; haloCanvas.height = VIEW.h; }
    return haloCanvas;
  }

  // Vue 3D : halos de la vérité terrain, points, puis les supports de niveau <= r, groupés par couleur (un seul
  // chemin par groupe et par arité : des faces qui se recouvrent ne s'assombrissent pas).
  function drawView(ctx, S, cam, r, info, mix, phase) {
    const X0 = COL.x, Y0 = VIEW.y, cx = COL.w / 2, cy = VIEW.h / 2 + S.view.shiftY;
    panel(ctx, X0, Y0, COL.w, VIEW.h);
    const n = S.n, sx = new Float64Array(n), sy = new Float64Array(n);
    for (let i = 0; i < n; i++) {
      const p = project(cam, S.x[i], S.y[i], S.z[i]);
      sx[i] = X0 + cx + p[0]; sy[i] = Y0 + cy + p[1];
    }
    const layer = haloLayer(), lc = layer.getContext('2d');
    ctx.save();
    roundRect(ctx, X0 + 1, Y0 + 1, COL.w - 2, VIEW.h - 2, 9); ctx.clip();
    for (let o = 0; o < S.nobj; o++) {
      lc.clearRect(0, 0, layer.width, layer.height);
      lc.fillStyle = C.objects[o];
      lc.beginPath();
      for (let i = 0; i < n; i++) if (S.gt[i] === o) disc(lc, sx[i] - X0, sy[i] - Y0, 12);
      lc.fill();
      ctx.globalAlpha = Math.min(1, C.halo * (1 + 0.7 * (1 - mix)));
      ctx.drawImage(layer, X0, Y0);
      ctx.globalAlpha = 1;
    }
    // points : la vérité terrain au début (objets en couleur), puis de petits points pâles sous les supports
    for (let i = 0; i < n; i++) {
      const g = S.gt[i];
      if (mix < 1) {
        ctx.globalAlpha = 1 - mix;
        ctx.fillStyle = g >= 0 ? C.objects[g] : C.alone;
        ctx.beginPath(); disc(ctx, sx[i], sy[i], g >= 0 ? 4.2 : 1.6); ctx.fill();
      }
      ctx.globalAlpha = mix;
      ctx.fillStyle = C.alone;
      ctx.beginPath(); disc(ctx, sx[i], sy[i], g >= 0 ? 2.0 : 1.4); ctx.fill();
    }
    ctx.globalAlpha = 1;
    if (info) drawSupports(ctx, S, sx, sy, r, info, mix);
    // lettres des objets
    const size = 24, labels = [];
    for (let o = 0; o < S.nobj; o++) {
      let mx = 0, cnt = 0, top = Infinity;
      for (let i = 0; i < n; i++) if (S.gt[i] === o) { mx += sx[i]; cnt++; top = Math.min(top, sy[i]); }
      if (!cnt) continue;
      const ob = S.scene.objects[o];
      const mark = info ? info.marks[o] : '';
      const parts = [[mix < 0.5 ? `${ob.key} · ${ob.name}` : ob.key, C.objects[o], true]];
      if (mark === '✓') parts.push(['  ✓', C.ok, true]);
      if (mark === '✗') parts.push(['  ✗', C.fusion, true]);
      const w = Math.max(richWidth(ctx, parts, size) + 20, mix < 0.5 ? 0 : 74);
      labels.push({ o, parts, w, x: clamp(mx / cnt, X0 + w / 2 + 8, X0 + COL.w - w / 2 - 8), y: Math.max(Y0 + 76, top - 22) });
    }
    labels.sort((a, b) => b.y - a.y);
    const placed = [];
    for (const lb of labels) {
      for (let guard = 0; guard < 6 && placed.some((p) => Math.abs(p.x - lb.x) < (p.w + lb.w) / 2 + 6 && Math.abs(p.y - lb.y) < size + 16); guard++) lb.y -= size + 18;
      placed.push(lb);
      ctx.fillStyle = C.plate; roundRect(ctx, lb.x - lb.w / 2, lb.y - size - 3, lb.w, size + 12, 6); ctx.fill();
      ctx.strokeStyle = rgba(C.objects[lb.o], 0.9); ctx.lineWidth = 1.5; roundRect(ctx, lb.x - lb.w / 2 + 0.5, lb.y - size - 2.5, lb.w - 1, size + 11, 6); ctx.stroke();
      rich(ctx, lb.parts, lb.x, lb.y + 1, size, 'center');
    }
    ctx.restore();
  }

  // Supports de niveau <= r. Chaque groupe est peint opaque dans deux calques hors écran (faces ; arêtes et supports
  // q2), puis posé avec sa transparence : des faces qui se recouvrent ne s'assombrissent pas, et aucun chemin géant
  // n'est rempli. Le calque gris reçoit tous les supports, une fois (r ne fait que croître, caméra fixe). Un groupe
  // coloré (objet suivi, ou objets réunis) suit un nœud dont le sous-arbre ne fait que grandir le long de sa chaîne :
  // on n'y ajoute que les supports qui le rejoignent ; il n'est repeint que s'il change d'identité (fusion d'objets).
  const STYLE = { other: { faces: 0.12, edges: 0.38, edge: 0.7, line: 1.3 }, obj: { faces: 0.30, edges: 0.9, edge: 1.0, line: 2.3 } };
  const GROUPS = [2, 3, 4, 9];
  let cache = null;
  function layerCanvas() { const c = document.createElement('canvas'); c.width = COL.w; c.height = VIEW.h; return c; }
  function layerPair() { return { faces: layerCanvas(), edges: layerCanvas(), node: null, who: '', P: 0 }; }
  function clearPair(L) {
    L.faces.getContext('2d').clearRect(0, 0, COL.w, VIEW.h); L.edges.getContext('2d').clearRect(0, 0, COL.w, VIEW.h);
    L.node = null; L.who = ''; L.P = 0;
  }
  function paint(S, sx, sy, L, list, color, st) {
    const X0 = COL.x, Y0 = VIEW.y, X = (s) => sx[s] - X0, Y = (s) => sy[s] - Y0;
    const fc = L.faces.getContext('2d'), ec = L.edges.getContext('2d');
    fc.fillStyle = color; ec.strokeStyle = color; ec.lineCap = 'round'; ec.lineJoin = 'round';
    for (let c0 = 0; c0 < list.length; c0 += 1500) {
      fc.beginPath();
      const edges = new Path2D(), lines = new Path2D();
      for (let j = c0; j < Math.min(list.length, c0 + 1500); j++) {
        const i = list[j], a = S.at[i], ar = S.arity[i], v0 = S.sites[a], v1 = S.sites[a + 1];
        if (ar === 2) { lines.moveTo(X(v0), Y(v0)); lines.lineTo(X(v1), Y(v1)); continue; }
        const v2 = S.sites[a + 2];
        if (ar === 3) {
          fc.moveTo(X(v0), Y(v0)); fc.lineTo(X(v1), Y(v1)); fc.lineTo(X(v2), Y(v2)); fc.closePath();
          edges.moveTo(X(v0), Y(v0)); edges.lineTo(X(v1), Y(v1)); edges.lineTo(X(v2), Y(v2)); edges.closePath();
          continue;
        }
        const v = [v0, v1, v2, S.sites[a + 3]];
        for (const [p, q, w] of [[0, 1, 2], [0, 1, 3], [0, 2, 3], [1, 2, 3]]) {
          fc.moveTo(X(v[p]), Y(v[p])); fc.lineTo(X(v[q]), Y(v[q])); fc.lineTo(X(v[w]), Y(v[w])); fc.closePath();
        }
        for (const [p, q] of [[0, 1], [0, 2], [0, 3], [1, 2], [1, 3], [2, 3]]) { edges.moveTo(X(v[p]), Y(v[p])); edges.lineTo(X(v[q]), Y(v[q])); }
      }
      fc.fill();
      ec.lineWidth = st.edge; ec.stroke(edges);
      ec.lineWidth = st.line; ec.stroke(lines);
    }
  }
  function drawSupports(ctx, S, sx, sy, r, info, mix) {
    const P = prefix(S, r);
    if (!cache || cache.theme !== C.bg || P < cache.gray.P) {
      if (!cache) cache = { gray: layerPair(), groups: new Map(GROUPS.map((g) => [g, layerPair()])) };
      clearPair(cache.gray);
      for (const L of cache.groups.values()) clearPair(L);
      cache.theme = C.bg;
    }
    if (P > cache.gray.P) {
      const list = [];
      for (let i = cache.gray.P; i < P; i++) list.push(i);
      paint(S, sx, sy, cache.gray, list, C.otherLine, STYLE.other);
      cache.gray.P = P;
    }
    // groupes colorés à r : chaque objet non réuni, et le nœud des objets réunis
    const want = new Map();
    for (let o = 0; o < S.nobj; o++) {
      const nd = info.nodes[o];
      if (!nd) continue;
      const g = info.shared[o] ? 9 : 2 + o;
      const who = info.shared[o] ? info.nodes.map((q, p) => (q && q[1] === nd[1] ? p : '')).join('') : String(o);
      want.set(g, { nd, who });
    }
    const inside = (nd, pa) => pa > nd[1] - nd[2] && pa <= nd[1];
    for (const g of GROUPS) {
      const L = cache.groups.get(g), w = want.get(g);
      if (!w) { if (L.node) clearPair(L); continue; }
      // même identité et sous-arbre qui contient l'ancien : on ajoute ; sinon, on repeint
      const grows = L.node && L.who === w.who && inside(w.nd, L.node[1]) && w.nd[1] - w.nd[2] <= L.node[1] - L.node[2];
      if (!grows) clearPair(L);
      const list = [];
      const old = L.node;
      for (let i = 0; i < P; i++) {
        const pa = S.post[i];
        if (!inside(w.nd, pa)) continue;
        if (old && i < L.P && inside(old, pa)) continue;  // déjà peint
        list.push(i);
      }
      if (list.length) paint(S, sx, sy, L, list, g === 9 ? C.fusion : C.objects[g - 2], STYLE.obj);
      L.node = w.nd; L.who = w.who; L.P = P;
    }
    const X0 = COL.x, Y0 = VIEW.y;
    const layers = [[cache.gray, STYLE.other], ...GROUPS.map((g) => [cache.groups.get(g), STYLE.obj])];
    for (const [L, st] of layers) {
      if (L !== cache.gray && !L.node) continue;
      ctx.globalAlpha = mix * st.faces; ctx.drawImage(L.faces, X0, Y0);
      ctx.globalAlpha = mix * st.edges; ctx.drawImage(L.edges, X0, Y0);
    }
    ctx.globalAlpha = 1;
  }

  function drawBadges(ctx, badges) {
    let y = VIEW.y + 16;
    for (const b of badges) {
      if (b.alpha <= 0) continue;
      const size = 23, w = richWidth(ctx, b.parts, size) + 34, x = COL.x + COL.w / 2 - w / 2;
      ctx.globalAlpha = b.alpha;
      ctx.fillStyle = C.plate; roundRect(ctx, x, y, w, 42, 21); ctx.fill();
      ctx.strokeStyle = b.border; ctx.lineWidth = 2.5; roundRect(ctx, x + 1, y + 1, w - 2, 40, 20); ctx.stroke();
      rich(ctx, b.parts, COL.x + COL.w / 2, y + 29, size, 'center');
      ctx.globalAlpha = 1;
      y += 52;
    }
  }
  function badgesAt(S, t) {
    const out = [];
    for (const p of S.timing.pauses) {
      if (t < p.t0 || t > p.t1) continue;
      const alpha = Math.min(smooth((t - p.t0) / 0.3), smooth((p.t1 - t) / 0.3));
      for (const b of p.badges) out.push({ alpha, border: roleColor(b.border), parts: b.parts.map(([s, role, bold]) => [s, roleColor(role), bold]) });
    }
    return out;
  }

  // IoU des sites des supports de chaque branche suivie en fonction de r (axe log)
  function drawLanes(ctx, S, r, phase) {
    const nobj = S.nobj;
    panel(ctx, COL.x, LANES.y, COL.w, LANES.h);
    const x0 = COL.x + 76, x1 = COL.x + COL.w - 120;
    const X = (v) => x0 + (x1 - x0) * (Math.log(v) - Math.log(S.rmin)) / (Math.log(S.rmax) - Math.log(S.rmin));
    const axisY = LANES.y + LANES.h - 38;
    const laneH = Math.min(62, (axisY - LANES.y - 40) / nobj);
    const top0 = LANES.y + 36 + (axisY - LANES.y - 40 - laneH * nobj) / 2;
    text(ctx, 'IoU des sites des supports du nœud qui suit chaque objet', COL.x + 18, LANES.y + 26, 17, C.dim);
    const rShow = phase === 'intro' ? 0 : r;
    ctx.strokeStyle = C.grid; ctx.lineWidth = 1;
    const ticks = [0.005, 0.01, 0.02, 0.03, 0.05, 0.1, 0.15, 0.2, 0.3, 0.5, 1, 2].filter((v) => v >= S.rmin * 0.999 && v <= S.rmax * 1.001);
    for (const v of ticks) {
      const xx = X(v);
      if (xx < x0 + 36) continue;  // pas de graduation sous la lettre r de l'axe
      ctx.beginPath(); ctx.moveTo(xx, top0 - 4); ctx.lineTo(xx, axisY); ctx.stroke();
      text(ctx, `${frNum(100 * v, v < 0.01 ? 1 : 0)} cm`, xx, axisY + 24, 16, C.dim, { align: 'center' });
    }
    text(ctx, 'r', x0 - 30, axisY + 24, 17, C.dim, { bold: true });
    for (let o = 0; o < nobj; o++) {
      const ty = top0 + o * laneH, by = ty + laneH - 10, hgt = laneH - 18;
      const Y = (v) => by - hgt * v;
      const color = C.objects[o];
      text(ctx, S.scene.objects[o].key, COL.x + 20, ty + laneH / 2 + 4, 24, color, { bold: true });
      ctx.strokeStyle = C.grid; ctx.lineWidth = 1;
      ctx.beginPath(); ctx.moveTo(x0, by); ctx.lineTo(x1, by); ctx.stroke();
      ctx.setLineDash([5, 5]); ctx.strokeStyle = C.dim; ctx.globalAlpha = 0.8;
      ctx.beginPath(); ctx.moveTo(x0, Y(0.5)); ctx.lineTo(x1, Y(0.5)); ctx.stroke();
      ctx.setLineDash([]); ctx.globalAlpha = 1;
      if (o === 0) text(ctx, '0,5', x0 - 8, Y(0.5) + 5, 14, C.dim, { align: 'right' });
      const tr = S.scene.tracks[o];
      const segs = [];
      for (let q = 0; q < tr.length; q++) {
        if (tr[q][0] > rShow * EPS || tr[q][2] === NONE) continue;
        const a = Math.max(tr[q][0], S.rmin), b = Math.max(a, Math.min(q + 1 < tr.length ? tr[q + 1][0] : S.rmax, rShow, S.rmax));
        segs.push([a, b, tr[q][1], tr[q][2]]);
      }
      for (const [a, b, v, st] of segs) {
        ctx.fillStyle = rgba(st === FUSED ? C.fusion : color, 0.28);
        ctx.fillRect(X(a), Y(v), Math.max(0.5, X(b) - X(a)), by - Y(v));
      }
      for (const [a, b, v, st] of segs) {
        ctx.strokeStyle = st === FUSED ? C.fusion : color; ctx.lineWidth = v > 0.5 ? 4 : 2.5;
        ctx.beginPath(); ctx.moveTo(X(a), Y(v)); ctx.lineTo(X(b), Y(v)); ctx.stroke();
      }
      ctx.lineWidth = 1.5;
      for (let q = 1; q < segs.length; q++) {
        if (segs[q][0] !== segs[q - 1][1]) continue;
        ctx.strokeStyle = segs[q][3] === FUSED ? C.fusion : color;
        ctx.beginPath(); ctx.moveTo(X(segs[q][0]), Y(segs[q - 1][2])); ctx.lineTo(X(segs[q][0]), Y(segs[q][2])); ctx.stroke();
      }
      let best = 0, bestAt = null;
      for (const s of segs) if (s[2] > best) { best = s[2]; bestAt = s[0]; }
      if (bestAt != null) {
        const xx = X(bestAt), yy = Y(best);
        ctx.fillStyle = best > 0.5 ? color : C.panel; ctx.strokeStyle = color; ctx.lineWidth = 2;
        ctx.beginPath(); ctx.moveTo(xx, yy - 8); ctx.lineTo(xx + 6, yy); ctx.lineTo(xx, yy + 8); ctx.lineTo(xx - 6, yy); ctx.closePath();
        ctx.fill(); ctx.stroke();
      }
      if (phase !== 'intro') {
        const now = trackAt(tr, r), found = best > 0.5;
        const parts = [[iouText(best), found ? C.text : C.dim, true]];
        parts.push([found ? '  ✓' : (now && now[2] === FUSED ? '  ✗' : ''), found ? C.ok : C.fusion, true]);
        rich(ctx, parts, x1 + 18, ty + laneH / 2 + 7, 20, 'left');
      }
    }
    text(ctx, 'meilleur', x1 + 18, LANES.y + 26, 15, C.dim);
    if (phase !== 'intro') {
      const xx = X(clamp(r, S.rmin, S.rmax));
      ctx.strokeStyle = C.text; ctx.lineWidth = 1.5; ctx.setLineDash([4, 4]);
      ctx.beginPath(); ctx.moveTo(xx, top0 - 8); ctx.lineTo(xx, axisY); ctx.stroke(); ctx.setLineDash([]);
    }
  }

  function drawHeader(ctx, S, r, phase) {
    const m = S.scene.meta;
    text(ctx, m.title, 24, 50, 34, C.text, { bold: true });
    text(ctx, m.subtitle, 24, 84, 20, C.dim);
    if (phase !== 'intro') {
      text(ctx, `r = ${cm(r)}`, W - 24, 56, 38, C.text, { bold: true, align: 'right' });
    } else {
      text(ctx, 'vérité terrain', W - 24, 60, 30, C.dim, { bold: true, align: 'right' });
    }
  }

  // supports dessinés jusqu'ici, par arité, dans le coin de la vue
  function drawCounts(ctx, S, r) {
    const P = prefix(S, r), x = COL.x + COL.w - 22;
    const rows = [['arêtes q2', S.cum[0][P]], ['triangles q3', S.cum[1][P]], ['tétraèdres q4', S.cum[2][P]]];
    rows.forEach(([label, v], i) => {
      const y = VIEW.y + 34 + 26 * i;
      text(ctx, thousands(v), x, y, 19, C.text, { bold: true, align: 'right' });
      text(ctx, label, x - 86, y, 17, C.dim, { align: 'right' });
    });
  }

  function drawFooter(ctx, S) {
    const y = FOOT.y + 20;
    let x = 28;
    const item = (draw, label) => { draw(x, y - 6); x += 26; text(ctx, label, x, y, 16, C.dim); x += measure(ctx, label, 16) + 24; };
    item((xx, yy) => { ctx.fillStyle = rgba(C.objects[0], C.halo * 2.2); ctx.beginPath(); disc(ctx, xx + 7, yy, 9); ctx.fill(); }, 'halo : objet (vérité terrain)');
    item((xx, yy) => { ctx.fillStyle = rgba(C.objects[0], 0.4); ctx.strokeStyle = C.objects[0]; ctx.lineWidth = 1.2; ctx.beginPath(); ctx.moveTo(xx, yy + 6); ctx.lineTo(xx + 16, yy + 6); ctx.lineTo(xx + 8, yy - 7); ctx.closePath(); ctx.fill(); ctx.stroke(); }, 'supports du nœud qui suit l\'objet');
    item((xx, yy) => { ctx.fillStyle = rgba(C.fusion, 0.4); ctx.strokeStyle = C.fusion; ctx.lineWidth = 1.2; ctx.beginPath(); ctx.moveTo(xx, yy + 6); ctx.lineTo(xx + 16, yy + 6); ctx.lineTo(xx + 8, yy - 7); ctx.closePath(); ctx.fill(); ctx.stroke(); }, 'objets réunis');
    item((xx, yy) => { ctx.fillStyle = rgba(C.otherLine, 0.25); ctx.strokeStyle = C.otherLine; ctx.lineWidth = 1; ctx.beginPath(); ctx.moveTo(xx, yy + 6); ctx.lineTo(xx + 16, yy + 6); ctx.lineTo(xx + 8, yy - 7); ctx.closePath(); ctx.fill(); ctx.stroke(); }, 'autre nœud');
    item((xx, yy) => { ctx.strokeStyle = C.dim; ctx.lineWidth = 2.3; ctx.beginPath(); ctx.moveTo(xx, yy + 5); ctx.lineTo(xx + 16, yy - 5); ctx.stroke(); }, 'arête : support q2 ; triangle : q3 ; tétraèdre : q4');
    text(ctx, 'r : rayon de la sphère de S* de chaque naissance ou fusion ; un nœud de l\'arbre couvrant d\'ordre k est réalisé par les supports de son sous-arbre · SemanticKITTI (CC BY-NC-SA)',
      28, y + 26, 15, C.dim);
  }

  function listing(keys) {
    return keys.length === 1 ? keys[0] : keys.slice(0, -1).join(', ') + ' et ' + keys[keys.length - 1];
  }
  function drawSummary(ctx, S, u) {
    const best = S.scene.best;
    const letters = S.scene.objects.map((ob) => ob.key);
    const parts = [['meilleur IoU   ', C.dim, false]];
    best.forEach((v, o) => {
      parts.push([`${letters[o]} ${iouText(v)}`, C.objects[o], true]);
      parts.push([v > 0.5 ? ' ✓' : ' ✗', v > 0.5 ? C.ok : C.fusion, true]);
      if (o + 1 < best.length) parts.push(['    ', C.dim, false]);
    });
    const c = S.scene.meta.counts;
    const never = best.map((v, o) => (v <= 0.5 ? letters[o] : null)).filter(Boolean);
    const verdict = [[`${thousands(c.nodes)} nœuds, ${thousands(c.balls)} boules, ${thousands(c.supports)} supports`, C.text, true]];
    if (never.length) verdict.push([`  ·  ${listing(never)} jamais retrouvé${never.length > 1 ? 's' : ''}`, C.fusion, true]);
    ctx.globalAlpha = u;
    const w = Math.max(richWidth(ctx, parts, 26), richWidth(ctx, verdict, 20)) + 48, x = COL.x + COL.w / 2 - w / 2, y = VIEW.y + 16;
    ctx.fillStyle = C.plate; roundRect(ctx, x, y, w, 92, 14); ctx.fill();
    ctx.strokeStyle = never.length ? C.fusion : C.ok; ctx.lineWidth = 2.5; roundRect(ctx, x + 1, y + 1, w - 2, 90, 13); ctx.stroke();
    rich(ctx, parts, COL.x + COL.w / 2, y + 38, 26, 'center');
    rich(ctx, verdict, COL.x + COL.w / 2, y + 74, 20, 'center');
    ctx.globalAlpha = 1;
  }

  function renderAt(ctx, S, t) {
    const T = S.timing;
    const phase = t < T.intro ? 'intro' : (t >= T.summary ? 'summary' : 'sweep');
    const r = phase === 'intro' ? S.rmin : rAt(S, t);
    const mix = phase === 'intro' ? 0 : smooth((t - T.intro) / (T.sweep - T.intro));
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.fillStyle = C.bg; ctx.fillRect(0, 0, W, H);
    drawHeader(ctx, S, r, phase);
    let info = null;
    if (phase !== 'intro') {
      const nodes = S.scene.chains.map((ch) => nodeAt(ch, r));
      const shared = nodes.map((nd, o) => !!nd && nodes.some((q, p) => p !== o && q && q[1] === nd[1]));
      const marks = S.scene.objects.map((_, o) => (foundBy(S.scene.tracks[o], r) ? '✓' : (shared[o] ? '✗' : '')));
      info = { nodes, shared, marks };
    }
    drawView(ctx, S, cameraOf(S, t), r, info, mix, phase);
    if (phase === 'summary') drawSummary(ctx, S, smooth((t - T.summary) / 0.6));
    else if (phase === 'sweep') drawBadges(ctx, badgesAt(S, t));
    if (phase !== 'intro') drawCounts(ctx, S, phase === 'summary' ? S.rmax : r);
    drawLanes(ctx, S, phase === 'summary' ? S.rmax : r, phase);
    drawFooter(ctx, S);
    return { r, phase };
  }

  window.SupportsPlayer = { W, H, prepare, renderAt, rAt, setTheme, themeName, THEMES };
})();
