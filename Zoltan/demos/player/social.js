/*
 * Lecteur des vidéos courtes « HGP vs HDBSCAN » pour téléphone (LinkedIn), en anglais.
 *
 * Format portrait 1080 x 1350 (4:5). Deux panneaux superposés, HGP en haut, HDBSCAN en bas, mêmes points, même
 * caméra fixe ; les deux hiérarchies balaient le rayon r EN MÊME TEMPS. Seuls textes : HGP, HDBSCAN, r, et les
 * clusters (lettres des objets, coches). Fin : le meilleur nœud de chaque hiérarchie pour chaque objet (best_sites de
 * la scène, écrit par tools/duel_scene.py), encadré en vert s'il retrouve l'objet (IoU > 0,5), en rouge sinon.
 *
 * Même rejeu exact que player/duel.js (union-find des événements par plateaux) ; scènes data/duel_k<k>_en.js. Une
 * vidéo enchaîne plusieurs scènes (tools/render_social.cjs) : SocialPlayer.load(scène) puis renderAt(t).
 */
(function () {
  'use strict';

  const W = 1080, H = 1350;
  const PANELS = [{ key: 'hgp', name: 'HGP', x: 16, y: 132, w: 1048, h: 598 },
    { key: 'hdbscan', name: 'HDBSCAN', x: 16, y: 744, w: 1048, h: 598 }];
  const FONT = '"DejaVu Sans", "Helvetica Neue", Arial, sans-serif';
  const EPS = 1 + 1e-12;
  // Chronologie d'une scène (secondes)
  const T_INTRO = 2.4, T_MIX = 0.6, T_SWEEP = 9.0, T_FINAL_IN = 0.8, T_FINAL = 4.6, T_FADE = 0.35, T_PAUSE = 1.7;
  const SWEEP0 = T_INTRO + T_MIX;
  let SWEEP1 = 0, FINAL0 = 0, DURATION = 0;  // propres à la scène : le balayage s'arrête aux événements importants

  // Objets en bleu, ambre, violet : le vert et le rouge sont réservés au verdict (retrouvé / réuni ou manqué).
  const THEMES = {
    dark: {
      bg: '#071b2e', panel: '#0f2c48', frame: '#3a5773', text: '#eaf5f7', dim: '#9fb4c4',
      plate: 'rgba(15,44,72,0.90)', objects: ['#62b3ff', '#ffc53d', '#c792ff'], good: '#3ddc84', bad: '#ff5c6c',
      halo: 0.22, alone: 'rgba(159,180,196,0.32)', other: '#5d6676',
    },
    light: {
      bg: '#ffffff', panel: '#f4fafb', frame: '#b9cad4', text: '#082c4c', dim: '#4a5f71',
      plate: 'rgba(244,250,251,0.93)', objects: ['#1f5fbf', '#c27a00', '#7b3fbf'], good: '#16924a', bad: '#c8102e',
      halo: 0.16, alone: 'rgba(93,115,133,0.32)', other: '#b4b4c4',
    },
  };
  let C = THEMES.dark;
  function themeName(name) { return name === 'light' || name === 'clair' ? 'light' : 'dark'; }
  function setTheme(name) { C = THEMES[themeName(name)]; return themeName(name); }

  const lerp = (a, b, u) => a + (b - a) * u;
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
  const smooth = (u) => { u = clamp(u, 0, 1); return u * u * (3 - 2 * u); };
  function hexToRgb(h) { const v = parseInt(h.slice(1), 16); return [(v >> 16) & 255, (v >> 8) & 255, v & 255]; }
  function rgba(h, a) { const c = hexToRgb(h); return `rgba(${c[0]},${c[1]},${c[2]},${a})`; }

  // ---------------------------------------------------------------- texte
  function setFont(ctx, size, bold) { ctx.font = `${bold ? 'bold ' : ''}${size}px ${FONT}`; }
  function text(ctx, str, x, y, size, color, opt) {
    opt = opt || {};
    setFont(ctx, size, opt.bold);
    ctx.fillStyle = color; ctx.textAlign = opt.align || 'left'; ctx.textBaseline = 'alphabetic';
    ctx.fillText(str, x, y);
  }
  function measure(ctx, str, size, bold) { setFont(ctx, size, bold); return ctx.measureText(str).width; }
  function richWidth(ctx, parts, size) { return parts.reduce((s, p) => s + measure(ctx, p[0], size, p[2]), 0); }
  function rich(ctx, parts, x, y, size) {  // centré en x
    let xx = x - richWidth(ctx, parts, size) / 2;
    for (const [str, col, bold] of parts) { text(ctx, str, xx, y, size, col, { bold }); xx += measure(ctx, str, size, bold); }
  }
  function roundRect(ctx, x, y, w, h, r) {
    ctx.beginPath();
    ctx.moveTo(x + r, y); ctx.lineTo(x + w - r, y); ctx.arcTo(x + w, y, x + w, y + r, r);
    ctx.lineTo(x + w, y + h - r); ctx.arcTo(x + w, y + h, x + w - r, y + h, r);
    ctx.lineTo(x + r, y + h); ctx.arcTo(x, y + h, x, y + h - r, r);
    ctx.lineTo(x, y + r); ctx.arcTo(x, y, x + r, y, r); ctx.closePath();
  }
  function pill(ctx, parts, cx, y, size, border) {  // étiquette arrondie centrée en cx, haut en y
    const w = richWidth(ctx, parts, size) + size * 1.3, h = size * 1.55;
    ctx.fillStyle = C.plate; roundRect(ctx, cx - w / 2, y, w, h, h / 2); ctx.fill();
    ctx.strokeStyle = border; ctx.lineWidth = 3; roundRect(ctx, cx - w / 2 + 1.5, y + 1.5, w - 3, h - 3, h / 2 - 1.5); ctx.stroke();
    rich(ctx, parts, cx, y + h * 0.71, size);
    return w;
  }
  const disc = (ctx, x, y, r) => { ctx.moveTo(x + r, y); ctx.arc(x, y, r, 0, 2 * Math.PI); };

  // ---------------------------------------------------------------- scène
  let S = null;
  function load(scene) {
    if (scene.schema !== 'ehgp.zoltan.duel.v3') throw new Error('unknown scene schema');
    const n = scene.points.x.length;
    S = {
      scene, n, nobj: scene.objects.length,
      x: Float64Array.from(scene.points.x), y: Float64Array.from(scene.points.y), z: Float64Array.from(scene.points.z),
      gt: Int8Array.from(scene.gt), background: scene.gt.some((g) => g < 0),
      rmin: scene.timing.rmin, rmax: scene.timing.rmax, methods: {},
    };
    // fin du balayage : juste après le dernier événement utile (meilleurs nœuds, fusions des objets suivis)
    let last = 0;
    for (const p of PANELS) { const m = scene.methods[p.key]; for (const v of m.best_level) last = Math.max(last, v); for (const f of m.fusions) last = Math.max(last, f.r); }
    S.rend = clamp(1.12 * last, S.rmin * 1.5, S.rmax);
    for (const p of PANELS) {
      const m = scene.methods[p.key];
      S.methods[p.key] = { m, levels: Float64Array.from(m.levels), evP: Int32Array.from(m.ev_plateau), evK: Int8Array.from(m.ev_kind),
        evA: Int32Array.from(m.ev_a), evB: Int32Array.from(m.ev_b), state: null,
        bestSet: m.best_sites.map((list) => new Set(list)) };
    }
    S.cam = camera(scene.view || { az: 0, el: 30 });
    fit();
    schedule();
    return DURATION;
  }

  function camera(view) {
    let rad = 0, zmax = 0;
    for (let i = 0; i < S.n; i++) if (S.gt[i] >= 0) { rad = Math.max(rad, Math.hypot(S.x[i], S.y[i], S.z[i])); zmax = Math.max(zmax, S.z[i]); }
    const a = view.az * Math.PI / 180, e = view.el * Math.PI / 180, tg = [0, 0, zmax / 2], dist = 5.5 * Math.max(rad, 0.5);
    const pos = [tg[0] + dist * Math.cos(e) * Math.sin(a), tg[1] - dist * Math.cos(e) * Math.cos(a), tg[2] + dist * Math.sin(e)];
    const f = norm([tg[0] - pos[0], tg[1] - pos[1], tg[2] - pos[2]]);
    const right = norm(cross(f, [0, 0, 1]));
    return { pos, f, right, up: cross(right, f) };
  }
  function cross(a, b) { return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]; }
  function norm(a) { const l = Math.hypot(a[0], a[1], a[2]); return [a[0] / l, a[1] / l, a[2] / l]; }
  // Projection fixe (caméra immobile) : coordonnées écran relatives au panneau, calculées une fois.
  function fit() {
    const cam = S.cam, n = S.n, px = new Float64Array(n), py = new Float64Array(n), pz = new Float64Array(n);
    for (let i = 0; i < n; i++) {
      const dx = S.x[i] - cam.pos[0], dy = S.y[i] - cam.pos[1], dz = S.z[i] - cam.pos[2];
      const zc = dx * cam.f[0] + dy * cam.f[1] + dz * cam.f[2];
      px[i] = (dx * cam.right[0] + dy * cam.right[1] + dz * cam.right[2]) / zc;
      py[i] = -(dx * cam.up[0] + dy * cam.up[1] + dz * cam.up[2]) / zc;
      pz[i] = zc;
    }
    let x0 = Infinity, x1 = -Infinity, y0 = Infinity, y1 = -Infinity;
    for (let i = 0; i < n; i++) if (S.gt[i] >= 0) { x0 = Math.min(x0, px[i]); x1 = Math.max(x1, px[i]); y0 = Math.min(y0, py[i]); y1 = Math.max(y1, py[i]); }
    const P = PANELS[0], top = 96, bottom = 86;  // place pour le nom de la méthode et le verdict
    const scale = Math.min(0.84 * P.w / (x1 - x0), (P.h - top - bottom) / (y1 - y0)) * (S.background ? 0.94 : 1);
    const cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;
    S.sx = new Float64Array(n); S.sy = new Float64Array(n);
    for (let i = 0; i < n; i++) { S.sx[i] = P.w / 2 + scale * (px[i] - cx); S.sy[i] = top + (P.h - top - bottom) / 2 + scale * (py[i] - cy); }
    S.order = Array.from({ length: n }, (_, i) => i).sort((a, b) => pz[b] - pz[a]);
  }

  // État d'une hiérarchie au niveau r (rejeu des événements des plateaux de niveau <= r), comme player/duel.js.
  function hierarchyAt(M, r) {
    let P = 0, lo = 0, hi = M.levels.length - 1;
    while (lo <= hi) { const mid = (lo + hi) >> 1; if (M.levels[mid] <= r * EPS) { P = mid + 1; lo = mid + 1; } else hi = mid - 1; }
    let st = M.state;
    if (!st || st.P > P) {
      st = M.state = { P: 0, j: 0, dsu: new Int32Array(S.n), size: new Int32Array(S.n), entered: new Uint8Array(S.n) };
      for (let i = 0; i < S.n; i++) st.dsu[i] = i;
    }
    const find = (x) => { let root = x; while (st.dsu[root] !== root) root = st.dsu[root]; while (st.dsu[x] !== root) { const nx = st.dsu[x]; st.dsu[x] = root; x = nx; } return root; };
    while (st.j < M.evP.length && M.evP[st.j] < P) {
      const j = st.j++;
      if (M.evK[j] === 0) { st.entered[M.evA[j]] = 1; st.size[M.evA[j]] = 1; continue; }
      let a = find(M.evA[j]), b = find(M.evB[j]);
      if (a === b) continue;
      if (st.size[a] < st.size[b]) { const t = a; a = b; b = t; }
      st.dsu[b] = a; st.size[a] += st.size[b];
    }
    st.P = P;
    const root = new Int32Array(S.n), big = new Uint8Array(S.n);
    for (let i = 0; i < S.n; i++) { root[i] = find(i); big[i] = st.entered[i] && st.size[root[i]] >= 2 ? 1 : 0; }
    return { root, big };
  }
  const foundBy = (track, r) => track.some((row) => row[0] <= r * EPS && row[1] > 0.5);

  // Progression du balayage sans pauses : v (temps) -> u (fraction du log de r)
  const ease = (v) => 0.5 * v + 0.5 * smooth(v);
  function easeInv(u) { let a = 0, b = 1; for (let i = 0; i < 50; i++) { const m = (a + b) / 2; if (ease(m) < u) a = m; else b = m; } return (a + b) / 2; }
  const L = (r) => (Math.log(r) - Math.log(S.rmin)) / (Math.log(S.rend) - Math.log(S.rmin));
  // Événements importants (rôles des pauses de tools/duel_scene.py) : pour chaque méthode, « tous retrouvés et encore
  // séparés » s'il existe, sinon chaque objet retrouvé ; et les fusions d'objets dont l'un n'était pas encore retrouvé.
  function schedule() {
    const keys = (o) => S.scene.objects[o].key;
    const ev = [];
    for (const P of PANELS) {
      const m = S.scene.methods[P.key];
      const roles = [];
      for (const pz of S.scene.timing.pauses) for (const role of pz.roles) { const [kind, method, what] = role.split(':'); if (method === P.key) roles.push({ kind, what, r: pz.r }); }
      const sep = roles.find((x) => x.kind === 'sep');
      if (sep) ev.push({ r: sep.r, key: P.key, good: true, parts: () => [[sep.what.split('+').map((o) => keys(+o)).join(', '), null, true], [' found ✓', C.good, true]], objs: sep.what.split('+').map(Number) });
      else for (const x of roles) if (x.kind === 'best') ev.push({ r: x.r, key: P.key, good: true, parts: () => [[keys(+x.what), null, true], [' found ✓', C.good, true]], objs: [+x.what] });
      for (const f of m.fusions) if (!f.before.every(Boolean)) ev.push({ r: f.r, key: P.key, good: false, parts: () => [[f.objects.map(keys).join(' + '), null, true], [' merged too early ✗', C.bad, true]], objs: f.objects });
    }
    ev.sort((a, b) => a.r - b.r);
    const stops = [];
    for (const e of ev) {
      if (e.r < S.rmin || e.r > S.rend) continue;
      const last = stops[stops.length - 1];
      if (last && e.r / last.r < 1.008) last.events.push(e); else stops.push({ r: e.r, events: [e] });
    }
    let shift = 0;
    for (const s of stops) { s.t0 = SWEEP0 + T_SWEEP * easeInv(clamp(L(s.r), 0, 1)) + shift; s.t1 = s.t0 + T_PAUSE; shift += T_PAUSE; }
    S.stops = stops;
    SWEEP1 = SWEEP0 + T_SWEEP + shift; FINAL0 = SWEEP1 + T_FINAL_IN; DURATION = FINAL0 + T_FINAL;
  }
  function rAt(t) {
    let shift = 0;
    for (const s of S.stops) { if (t >= s.t1) shift += T_PAUSE; else if (t >= s.t0) return s.r; }
    const v = clamp((t - shift - SWEEP0) / T_SWEEP, 0, 1);
    return Math.exp(lerp(Math.log(S.rmin), Math.log(S.rend), ease(v)));
  }
  function stopAt(t) { return S.stops.find((s) => t >= s.t0 && t <= s.t1) || null; }

  // ---------------------------------------------------------------- dessin
  let haloCanvas = null;
  function halos(ctx, P, alpha) {
    if (!haloCanvas) { haloCanvas = document.createElement('canvas'); haloCanvas.width = P.w; haloCanvas.height = P.h; }
    const lc = haloCanvas.getContext('2d');
    for (let o = 0; o < S.nobj; o++) {
      lc.clearRect(0, 0, P.w, P.h); lc.fillStyle = C.objects[o]; lc.beginPath();
      for (let i = 0; i < S.n; i++) if (S.gt[i] === o) disc(lc, S.sx[i], S.sy[i], 13);
      lc.fill();
      ctx.globalAlpha = alpha; ctx.drawImage(haloCanvas, P.x, P.y); ctx.globalAlpha = 1;
    }
  }
  function panelFrame(ctx, P) {
    ctx.fillStyle = C.panel; roundRect(ctx, P.x, P.y, P.w, P.h, 14); ctx.fill();
    ctx.strokeStyle = C.frame; ctx.lineWidth = 1.5; roundRect(ctx, P.x + 0.75, P.y + 0.75, P.w - 1.5, P.h - 1.5, 14); ctx.stroke();
  }
  // style par point : 0 seul, 1 autre groupe, 2+o branche de l'objet o, 9 objets réunis
  function stylesAt(M, r) {
    const h = hierarchyAt(M, r);
    const seedRoot = M.m.seeds.map((s) => (h.big[s] ? h.root[s] : -1));
    const fused = new Set();
    seedRoot.forEach((x, o) => { if (x >= 0 && seedRoot.some((y, q) => q !== o && y === x)) fused.add(x); });
    const style = new Uint8Array(S.n);
    for (let i = 0; i < S.n; i++) {
      if (!h.big[i]) continue;
      let s = 1;
      for (let o = 0; o < S.nobj; o++) if (seedRoot[o] === h.root[i]) { s = fused.has(h.root[i]) ? 9 : 2 + o; break; }
      style[i] = s;
    }
    const marks = S.scene.objects.map((_, o) => (fused.has(seedRoot[o]) ? 'x' : (foundBy(M.m.tracks[o], r) ? 'v' : '')));
    return { style, marks };
  }
  function pointRadius(s, g) { return g >= 0 ? (s === 0 ? 2.6 : (s === 1 ? 3.4 : 4.8)) : (s === 0 ? 1.7 : (s === 1 ? 2.3 : 2.8)); }

  // Points d'un panneau. mode 'truth' : couleurs de la vérité ; 'sweep' : groupes de la hiérarchie au niveau r.
  function drawPoints(ctx, P, mode, st) {
    const colorOf = (s) => (s === 9 ? C.bad : C.objects[s - 2]);
    const pass = (keep) => {
      for (const i of S.order) {
        const g = S.gt[i], s = mode === 'truth' ? (g >= 0 ? 2 + g : 0) : st.style[i];
        if (!keep(s, g)) continue;
        ctx.fillStyle = s === 0 ? C.alone : (s === 1 ? C.other : colorOf(s));
        ctx.beginPath(); disc(ctx, P.x + S.sx[i], P.y + S.sy[i], pointRadius(s, g)); ctx.fill();
      }
    };
    pass((s, g) => s <= 1 && g < 0); pass((s, g) => s <= 1 && g >= 0); pass((s) => s >= 2);
  }
  // Étiquettes des objets au-dessus de leurs points : « A · bicycle » sur la vérité, puis « A », « A ✓ », « A ✗ ».
  function drawLabels(ctx, P, mode, marks) {
    const size = 34, placed = [];
    for (let o = 0; o < S.nobj; o++) {
      let mx = 0, cnt = 0, top = Infinity, bot = -Infinity;
      for (let i = 0; i < S.n; i++) if (S.gt[i] === o) { mx += S.sx[i]; cnt++; top = Math.min(top, S.sy[i]); bot = Math.max(bot, S.sy[i]); }
      if (!cnt) continue;
      const ob = S.scene.objects[o];
      const parts = [[mode === 'truth' ? `${ob.key} · ${ob.name}` : ob.key, C.objects[o], true]];
      if (marks && marks[o] === 'v') parts.push([' ✓', C.good, true]);
      if (marks && marks[o] === 'x') parts.push([' ✗', C.bad, true]);
      const w = richWidth(ctx, parts, size) + size * 1.3, h = size * 1.55;
      // position entièrement dans le panneau, hors du nom de la méthode et du bandeau du bas, sans chevauchement
      const fits = (x, y) => x - w / 2 >= P.x + 8 && x + w / 2 <= P.x + P.w - 8 && y >= P.y + 12 && y + h <= P.y + P.h - 92 &&
        !(x - w / 2 < P.x + 330 && y < P.y + 80) && !placed.some((q) => Math.abs(q.x - x) < (q.w + w) / 2 + 8 && Math.abs(q.y - y) < h + 6);
      const cx = P.x + mx / cnt, ya = P.y + top - h - 10, yb = P.y + bot + 12, cands = [];
      for (const y of [ya, ya - h - 8, ya + h + 8]) for (const dx of [0, 1, -1, 2, -2]) cands.push([cx + dx * (w * 0.75 + 12), y]);
      for (const dx of [0, 1, -1]) cands.push([cx + dx * (w * 0.75 + 12), yb]);
      const pick = cands.find(([x, y]) => fits(x, y)) || [clamp(cx, P.x + w / 2 + 10, P.x + P.w - w / 2 - 10), clamp(ya, P.y + 12, P.y + P.h - 92 - h)];
      const lb = { x: pick[0], y: pick[1], w };
      placed.push(lb);
      pill(ctx, parts, lb.x, lb.y, size, rgba(C.objects[o], 0.95));
    }
  }
  // Fin : meilleur nœud de la hiérarchie pour chaque objet, encadré en vert (IoU > 0,5) ou en rouge.
  function drawBest(ctx, P, M) {
    const ok = M.m.best.map((v) => v > 0.5);
    for (const i of S.order) {  // tout en pâle d'abord
      ctx.fillStyle = C.alone; ctx.beginPath(); disc(ctx, P.x + S.sx[i], P.y + S.sy[i], S.gt[i] >= 0 ? 2.8 : 1.7); ctx.fill();
    }
    const objs = [...Array(S.nobj).keys()].sort((a, b) => ok[a] - ok[b]);  // rouges dessous, verts dessus
    const boxes = [];
    for (const o of objs) {
      const col = ok[o] ? C.good : C.bad;
      let x0 = Infinity, x1 = -Infinity, y0 = Infinity, y1 = -Infinity;
      ctx.fillStyle = col; ctx.beginPath();
      for (const i of S.order) {
        if (!M.bestSet[o].has(i)) continue;
        const X = P.x + S.sx[i], Y = P.y + S.sy[i];
        disc(ctx, X, Y, S.gt[i] >= 0 ? 4.8 : 3.0);
        x0 = Math.min(x0, X); x1 = Math.max(x1, X); y0 = Math.min(y0, Y); y1 = Math.max(y1, Y);
      }
      ctx.fill();
      if (x0 === Infinity) continue;
      const pad = 16;
      boxes.push({ o, col, x0: Math.max(P.x + 6, x0 - pad), x1: Math.min(P.x + P.w - 6, x1 + pad), y0: Math.max(P.y + 6, y0 - pad), y1: Math.min(P.y + P.h - 6, y1 + pad) });
    }
    for (const b of boxes) {
      ctx.strokeStyle = b.col; ctx.lineWidth = 5; roundRect(ctx, b.x0, b.y0, b.x1 - b.x0, b.y1 - b.y0, 12); ctx.stroke();
    }
    const size = 36, placed = [];
    for (const b of boxes) {
      const parts = [[S.scene.objects[b.o].key, C.objects[b.o], true], [ok[b.o] ? ' ✓' : ' ✗', b.col, true]];
      const w = richWidth(ctx, parts, size) + size * 1.3, h = size * 1.55;
      // positions candidates collées au cadre : au-dessus à gauche, au-dessus à droite, dessous, dedans en haut
      const cands = [[b.x0 + w / 2, b.y0 - h - 8], [b.x1 - w / 2, b.y0 - h - 8], [b.x0 + w / 2 + 8, b.y0 + 8], [b.x1 - w / 2 - 8, b.y0 + 8],
        [b.x0 - w / 2 - 8, b.y0], [b.x1 + w / 2 + 8, b.y0], [b.x0 - w / 2 - 8, (b.y0 + b.y1 - h) / 2], [b.x1 + w / 2 + 8, (b.y0 + b.y1 - h) / 2],
        [b.x0 + w / 2, b.y1 + 8], [b.x1 - w / 2, b.y1 + 8]];
      const fits = (x, y) => y >= P.y + 12 && y + h <= P.y + P.h - 92 && !(x - w / 2 < P.x + 250 && y < P.y + 76) &&
        x - w / 2 >= P.x + 8 && x + w / 2 <= P.x + P.w - 8 &&
        !placed.some((q) => Math.abs(q.x - x) < (q.w + w) / 2 + 8 && Math.abs(q.y - y) < h + 6);
      let pick = cands.find(([x, y]) => fits(x, y)) || cands[0];
      const lb = { x: clamp(pick[0], P.x + w / 2 + 10, P.x + P.w - w / 2 - 10), y: clamp(pick[1], P.y + 70, P.y + P.h - 92 - h), w };
      placed.push(lb);
      pill(ctx, parts, lb.x, lb.y, size, b.col);
    }
    const found = ok.filter(Boolean).length, all = ok.length;
    const col = found === all ? C.good : C.bad;
    pill(ctx, [[found === all ? '✓ ' : '✗ ', col, true], [`${found} / ${all} clusters found`, C.text, true]], P.x + P.w / 2, P.y + P.h - 72, 36, col);
  }

  function renderAt(ctx, t) {
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.fillStyle = C.bg; ctx.fillRect(0, 0, W, H);
    const phase = t < T_INTRO ? 'intro' : (t < SWEEP1 ? 'sweep' : 'final');
    const r = phase === 'intro' ? S.rmin : rAt(t);
    // en-tête : r pendant le balayage
    if (phase === 'intro') text(ctx, 'The objects', W / 2, 90, 56, C.text, { bold: true, align: 'center' });
    else if (t < FINAL0) {
      const u = phase === 'final' ? 1 - smooth((t - SWEEP1) / T_FINAL_IN) : 1;
      ctx.globalAlpha = u;
      text(ctx, `ε = ${(100 * r).toFixed(1)} cm`, W / 2, 92, 60, C.text, { bold: true, align: 'center' });
      ctx.globalAlpha = 1;
    }
    if (phase === 'final') {
      ctx.globalAlpha = smooth((t - SWEEP1) / T_FINAL_IN);
      text(ctx, 'Best cluster for each object', W / 2, 90, 50, C.text, { bold: true, align: 'center' });
      ctx.globalAlpha = 1;
    }
    for (const P of PANELS) {
      const M = S.methods[P.key];
      panelFrame(ctx, P);
      ctx.save(); roundRect(ctx, P.x + 1, P.y + 1, P.w - 2, P.h - 2, 13); ctx.clip();
      const mix = phase === 'intro' ? 0 : smooth((t - T_INTRO) / T_MIX);
      halos(ctx, P, C.halo * (1.6 - 0.6 * mix));
      if (phase === 'intro' || mix < 1) {
        ctx.globalAlpha = 1 - mix; drawPoints(ctx, P, 'truth', null); ctx.globalAlpha = 1;
      }
      if (phase !== 'intro') {
        const fin = phase === 'final' ? smooth((t - SWEEP1) / T_FINAL_IN) : 0;
        if (fin < 1) {
          const st = stylesAt(M, r);
          ctx.globalAlpha = mix * (1 - fin); drawPoints(ctx, P, 'sweep', st); ctx.globalAlpha = 1;
          if (mix > 0.5) { ctx.globalAlpha = (1 - fin); drawLabels(ctx, P, 'sweep', st.marks); ctx.globalAlpha = 1; }
        }
        if (fin > 0) { ctx.globalAlpha = fin; drawBest(ctx, P, M); ctx.globalAlpha = 1; }
        const stop = phase === 'sweep' ? stopAt(t) : null;
        if (stop) {
          ctx.globalAlpha = Math.min(smooth((t - stop.t0) / 0.25), smooth((stop.t1 - t) / 0.25));
          let y = P.y + P.h - 72;
          for (const e of stop.events.filter((x) => x.key === P.key).reverse()) {
            const parts = e.parts();
            if (e.objs.length === 1) parts[0][1] = C.objects[e.objs[0]]; else parts[0][1] = C.text;
            pill(ctx, parts, P.x + P.w / 2, y, 36, e.good ? C.good : C.bad);
            y -= 66;
          }
          ctx.globalAlpha = 1;
        }
      } else drawLabels(ctx, P, 'truth', null);
      ctx.restore();
      text(ctx, P.name, P.x + 26, P.y + 60, 48, C.text, { bold: true });
    }
    // fondu d'entrée et de sortie de la scène
    const fade = Math.max(1 - smooth(t / T_FADE), 1 - smooth((DURATION - t) / T_FADE));
    if (fade > 0) { ctx.globalAlpha = fade; ctx.fillStyle = C.bg; ctx.fillRect(0, 0, W, H); ctx.globalAlpha = 1; }
  }

  window.SocialPlayer = { W, H, THEMES, setTheme, load, renderAt, stops: () => S.stops, duration: () => DURATION };
})();
