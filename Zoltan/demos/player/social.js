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
  const T_INTRO = 2.4, T_MIX = 0.6, T_FINAL_IN = 0.8, T_FINAL = 4.6, T_FADE = 0.35;
  const PAUSE = { found: 1.6, sep: 2.2, merge: 2.5, chute: 1.6 };  // secondes d'arrêt, selon l'événement
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
        bestSet: m.best_sites.map((list) => new Set(list)),
        // tous les objets retrouvés : la hiérarchie se fige au dernier de leurs maxima (elle a réussi)
        freeze: m.best.every((v) => v > 0.5) ? Math.max(...m.best_level) : Infinity };
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

  const L = (r) => (Math.log(r) - Math.log(S.rmin)) / (Math.log(S.rend) - Math.log(S.rmin));
  // Événements montrés (rôles des pauses de tools/duel_scene.py), pour chaque méthode :
  // - « tous retrouvés et encore séparés » s'il existe, sinon chaque objet retrouvé (maximum de son IoU, > 0,5) ;
  // - les fusions d'objets dont l'un n'avait pas encore été retrouvé (fusions parasites) ;
  // - l'absorption par le fond d'un objet déjà retrouvé, avant que la hiérarchie ne se fige (elle explique la couleur).
  function schedule() {
    const ev = [];
    for (const P of PANELS) {
      const M = S.methods[P.key], m = M.m;
      const roles = [];
      for (const pz of S.scene.timing.pauses) for (const role of pz.roles) { const [kind, method, what] = role.split(':'); if (method === P.key) roles.push({ kind, what, r: pz.r, pz }); }
      const sep = roles.find((x) => x.kind === 'sep');
      if (sep) ev.push({ r: sep.r, key: P.key, kind: 'sep', objs: sep.what.split('+').map(Number) });
      else for (const x of roles) if (x.kind === 'best') ev.push({ r: x.r, key: P.key, kind: 'found', objs: [+x.what] });
      for (const f of m.fusions) {
        if (f.before.every(Boolean)) continue;
        ev.push({ r: f.r, key: P.key, kind: 'merge', objs: f.objects, never: f.objects.filter((o) => m.best[o] <= 0.5),
          late: f.objects.filter((o, j) => m.best[o] > 0.5 && !f.before[j]) });
      }
      const done = new Set();
      for (const x of roles) {
        if (x.kind !== 'chute' || done.has(x.pz) || x.r > M.freeze * EPS) continue;
        done.add(x.pz);
        const badge = x.pz.badges[P.key].find((b) => b.border === 'fusion' && / merge/.test(b.parts.map((q) => q[0]).join('')));
        if (!badge) continue;
        const words = badge.parts.map((q) => q[0]).join('').replace(/^✗ /, '').split(' · ')[0];
        const objs = x.pz.roles.filter((q) => q.startsWith(`chute:${P.key}:`)).map((q) => +q.split(':')[2]);
        ev.push({ r: x.r, key: P.key, kind: 'chute', objs, words });
      }
    }
    ev.sort((a, b) => a.r - b.r);
    // la conclusion est acquise au dernier événement décisif : le balayage s'arrête juste après
    const decisive = ev.filter((e) => e.kind !== 'chute');
    if (decisive.length) S.rend = clamp(1.03 * decisive[decisive.length - 1].r, S.rmin * 1.5, S.rend);
    // une pause par événement ; deux méthodes dont les événements sont presque simultanés partagent la même
    const stops = [];
    for (const e of ev) {
      if (e.r < S.rmin || e.r > S.rend) continue;
      const last = stops[stops.length - 1];
      if (last && e.r / last.r0 < 1.008 && !last.events.some((x) => x.key === e.key)) { last.events.push(e); last.r = Math.max(last.r, e.r); }
      else stops.push({ r0: e.r, r: e.r, events: [e] });
    }
    for (const s of stops) s.pause = Math.max(...s.events.map((e) => PAUSE[e.kind]));
    // ralentis : la vitesse du balayage (en log de epsilon) baisse à l'approche de chaque événement et en repartant
    const us = stops.map((s) => clamp(L(s.r), 0, 1));
    const speed = (u) => {
      let dip = 0;
      for (const ue of us) dip = Math.max(dip, Math.exp(-(((u - ue) / 0.03) ** 2)));
      return Math.max(0.1, 1 - 0.85 * dip) * (0.55 + 0.45 * smooth(u / 0.12));
    };
    const N = 3000, cum = new Float64Array(N + 1);
    for (let i = 0; i < N; i++) cum[i + 1] = cum[i] + 1 / (N * speed((i + 0.5) / N));
    S.tsw = 5.0 + 0.7 * stops.length;  // durée du balayage hors pauses
    for (let i = 0; i <= N; i++) cum[i] *= S.tsw / cum[N];
    S.cum = cum;
    const tauOf = (u) => { const x = u * N, i = Math.min(N - 1, Math.floor(x)); return lerp(cum[i], cum[i + 1], x - i); };
    let shift = 0;
    for (let k = 0; k < stops.length; k++) { stops[k].t0 = SWEEP0 + tauOf(us[k]) + shift; stops[k].t1 = stops[k].t0 + stops[k].pause; shift += stops[k].pause; }
    S.stops = stops;
    SWEEP1 = SWEEP0 + S.tsw + shift; FINAL0 = SWEEP1 + T_FINAL_IN; DURATION = FINAL0 + T_FINAL;
  }
  function rAt(t) {
    let shift = 0;
    for (const s of S.stops) { if (t >= s.t1) shift += s.pause; else if (t >= s.t0) return s.r; else break; }
    const tau = clamp(t - shift - SWEEP0, 0, S.tsw), cum = S.cum, N = cum.length - 1;
    let lo = 0, hi = N;
    while (hi - lo > 1) { const mid = (lo + hi) >> 1; if (cum[mid] <= tau) lo = mid; else hi = mid; }
    const u = (lo + (tau - cum[lo]) / Math.max(1e-12, cum[hi] - cum[lo])) / N;
    return Math.exp(lerp(Math.log(S.rmin), Math.log(S.rend), clamp(u, 0, 1)));
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
    const marks = S.scene.objects.map((_, o) => (fused.has(seedRoot[o]) ? 'x'
      : (M.m.best[o] > 0.5 && r * EPS >= M.m.best_level[o] ? 'v' : '')));
    return { style, marks };
  }
  const colorOf = (s) => (s === 9 ? C.bad : C.objects[s - 2]);
  function pointRadius(s, g) { return g >= 0 ? (s === 0 ? 2.6 : (s === 1 ? 3.4 : 4.8)) : (s === 0 ? 1.7 : (s === 1 ? 2.3 : 2.8)); }

  // Points d'un panneau. mode 'truth' : couleurs de la vérité ; 'sweep' : groupes de la hiérarchie au niveau r.
  function drawPoints(ctx, P, mode, st) {
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
  function drawLabels(ctx, P, mode, marks, reserve) {
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
        !(x - w / 2 < P.x + reserve && y < P.y + 80) && !placed.some((q) => Math.abs(q.x - x) < (q.w + w) / 2 + 8 && Math.abs(q.y - y) < h + 6);
      const cx = P.x + mx / cnt, ya = P.y + top - h - 10, yb = P.y + bot + 12, cands = [];
      for (const y of [ya, ya - h - 8, ya + h + 8]) for (const dx of [0, 1, -1, 2, -2]) cands.push([cx + dx * (w * 0.75 + 12), y]);
      for (const dx of [0, 1, -1]) cands.push([cx + dx * (w * 0.75 + 12), yb]);
      const pick = cands.find(([x, y]) => fits(x, y)) || [clamp(cx, P.x + w / 2 + 10, P.x + P.w - w / 2 - 10), clamp(ya, P.y + 12, P.y + P.h - 92 - h)];
      const lb = { x: pick[0], y: pick[1], w };
      placed.push(lb);
      pill(ctx, parts, lb.x, lb.y, size, rgba(C.objects[o], 0.95));
    }
  }
  // Commentaire d'un événement, sur une ou deux lignes (formulation des vidéos longues, sans les IoU).
  function listParts(objs) {
    const out = [];
    objs.forEach((o, j) => {
      if (j) out.push([j === objs.length - 1 ? ' and ' : ', ', C.text, true]);
      out.push([S.scene.objects[o].key, C.objects[o], true]);
    });
    return out;
  }
  function captionOf(e) {
    if (e.kind === 'found') return { border: C.good, lines: [{ size: 38, parts: [...listParts(e.objs), [' recovered ✓', C.good, true]] }] };
    if (e.kind === 'sep') {
      return { border: C.good, lines: [{ size: 38, parts: [...listParts(e.objs), [' recovered ✓', C.good, true]] },
        { size: 31, parts: [['still separate', C.text, false]] }] };
    }
    if (e.kind === 'merge') {
      const who = e.never.length ? e.never : e.late;
      return { border: C.bad, lines: [{ size: 38, parts: [['✗ ', C.bad, true], ...listParts(e.objs), [' now merged', C.text, true]] },
        { size: 31, parts: [...listParts(who), [e.never.length ? ' never recovered separately' : ' not yet recovered', C.text, false]] }] };
    }
    const key = S.scene.objects[e.objs[0]].key;
    const parts = e.objs.length === 1 && e.words.startsWith(key + ' ')
      ? [[key, C.objects[e.objs[0]], true], [e.words.slice(key.length), C.text, true]] : [[e.words, C.text, true]];
    return { border: C.dim, lines: [{ size: 34, parts }] };
  }
  function card(ctx, cap, cx, bottom) {
    const pad = 24, gap = 6;
    const w = Math.max(...cap.lines.map((l) => richWidth(ctx, l.parts, l.size))) + 2 * pad;
    const h = cap.lines.reduce((s, l) => s + 1.22 * l.size, 0) + gap * (cap.lines.length - 1) + 24;
    const y0 = bottom - h;
    ctx.fillStyle = C.plate; roundRect(ctx, cx - w / 2, y0, w, h, 18); ctx.fill();
    ctx.strokeStyle = cap.border; ctx.lineWidth = 3; roundRect(ctx, cx - w / 2 + 1.5, y0 + 1.5, w - 3, h - 3, 16.5); ctx.stroke();
    let y = y0 + 12;
    for (const l of cap.lines) { rich(ctx, l.parts, cx, y + 0.93 * l.size, l.size); y += 1.22 * l.size + gap; }
    return h;
  }
  // Surbrillance d'un événement : le reste du panneau s'estompe, les clusters concernés ressortent, avec un éclair
  // bref au début de la pause puis un anneau qui pulse (vert : retrouvé ; rouge : fusion parasite).
  function highlight(ctx, P, st, mine, t, stop, alpha) {
    const ring = new Map();  // style -> couleur de l'anneau
    for (const e of mine) {
      if (e.kind === 'merge') ring.set(9, C.bad);
      else if (e.kind === 'chute' && e.objs.length > 1) ring.set(9, C.text);
      else for (const o of e.objs) ring.set(2 + o, e.kind === 'chute' ? C.objects[o] : C.good);
    }
    ctx.globalAlpha = 0.62 * alpha; ctx.fillStyle = C.panel; ctx.fillRect(P.x, P.y, P.w, P.h); ctx.globalAlpha = 1;
    for (const i of S.order) {
      const s = st.style[i];
      if (!ring.has(s)) continue;
      ctx.fillStyle = colorOf(s); ctx.beginPath(); disc(ctx, P.x + S.sx[i], P.y + S.sy[i], pointRadius(s, S.gt[i]) + 0.4); ctx.fill();
    }
    const u = t - stop.t0, pulse = (0.5 + 0.5 * Math.cos(2 * Math.PI * u / 0.9)) * alpha, flash = 1 - smooth(u / 0.4);
    ctx.lineWidth = 2.5;
    for (const [s, col] of ring) {
      ctx.strokeStyle = rgba(col, Math.min(1, 0.65 * pulse + 0.35 * flash)); ctx.beginPath();
      for (const i of S.order) if (S.gt[i] >= 0 && st.style[i] === s) disc(ctx, P.x + S.sx[i], P.y + S.sy[i], 8 + 3 * pulse + 9 * flash);
      ctx.stroke();
    }
  }

  // Fin : meilleur nœud de la hiérarchie pour chaque objet, encadré en vert (IoU > 0,5) ou en rouge.
  // aPts : opacité des points et des cadres ; aTxt : celle des étiquettes et du bilan (le texte entre après celui
  // du balayage, sans superposition).
  function drawBest(ctx, P, M, reserve, aPts, aTxt) {
    const ok = M.m.best.map((v) => v > 0.5);
    ctx.globalAlpha = aPts;
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
    ctx.globalAlpha = aTxt;
    const size = 36, placed = [];
    for (const b of boxes) {
      const parts = [[S.scene.objects[b.o].key, C.objects[b.o], true], [ok[b.o] ? ' ✓' : ' ✗', b.col, true]];
      const w = richWidth(ctx, parts, size) + size * 1.3, h = size * 1.55;
      // positions candidates collées au cadre : au-dessus à gauche, au-dessus à droite, dessous, dedans en haut
      const cands = [[b.x0 + w / 2, b.y0 - h - 8], [b.x1 - w / 2, b.y0 - h - 8], [b.x0 + w / 2 + 8, b.y0 + 8], [b.x1 - w / 2 - 8, b.y0 + 8],
        [b.x0 - w / 2 - 8, b.y0], [b.x1 + w / 2 + 8, b.y0], [b.x0 - w / 2 - 8, (b.y0 + b.y1 - h) / 2], [b.x1 + w / 2 + 8, (b.y0 + b.y1 - h) / 2],
        [b.x0 + w / 2, b.y1 + 8], [b.x1 - w / 2, b.y1 + 8]];
      const fits = (x, y) => y >= P.y + 12 && y + h <= P.y + P.h - 92 && !(x - w / 2 < P.x + reserve && y < P.y + 80) &&
        x - w / 2 >= P.x + 8 && x + w / 2 <= P.x + P.w - 8 &&
        !placed.some((q) => Math.abs(q.x - x) < (q.w + w) / 2 + 8 && Math.abs(q.y - y) < h + 6);
      let pick = cands.find(([x, y]) => fits(x, y)) || cands[0];
      const lb = { x: clamp(pick[0], P.x + w / 2 + 10, P.x + P.w - w / 2 - 10), y: clamp(pick[1], P.y + 70, P.y + P.h - 92 - h), w };
      placed.push(lb);
      pill(ctx, parts, lb.x, lb.y, size, b.col);
    }
    const found = ok.filter(Boolean).length, all = ok.length;
    const col = found === all ? C.good : C.bad;
    pill(ctx, [[found === all ? '✓ ' : '✗ ', col, true], [`${found} / ${all} objects recovered`, C.text, true]], P.x + P.w / 2, P.y + P.h - 72, 36, col);
    ctx.globalAlpha = 1;
  }

  function renderAt(ctx, t) {
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.fillStyle = C.bg; ctx.fillRect(0, 0, W, H);
    const phase = t < T_INTRO ? 'intro' : (t < SWEEP1 ? 'sweep' : 'final');
    const r = phase === 'intro' ? S.rmin : rAt(t);
    // vers la fin : les points se fondent sur toute la transition ; les textes du balayage sortent pendant la
    // première moitié, ceux de la fin entrent pendant la seconde
    const half = T_FINAL_IN / 2;
    const fo = phase === 'final' ? 1 - smooth((t - SWEEP1) / half) : 1, fi = phase === 'final' ? smooth((t - SWEEP1 - half) / half) : 0;
    // en-tête : r pendant le balayage
    if (phase === 'intro') text(ctx, 'The objects', W / 2, 90, 56, C.text, { bold: true, align: 'center' });
    else if (t < FINAL0) {
      ctx.globalAlpha = fo;
      text(ctx, `ε = ${(100 * r).toFixed(1)} cm`, W / 2, 92, 60, C.text, { bold: true, align: 'center' });
      ctx.globalAlpha = 1;
    }
    if (phase === 'final') {
      ctx.globalAlpha = fi;
      text(ctx, 'Best cluster for each object', W / 2, 90, 50, C.text, { bold: true, align: 'center' });
      ctx.globalAlpha = 1;
    }
    for (const P of PANELS) {
      const M = S.methods[P.key];
      const nameW = measure(ctx, P.name, 48, true), epsText = `ε = ${(100 * M.freeze).toFixed(1)} cm`;
      const reserveName = 26 + nameW + 24, reserve = reserveName + (M.freeze < Infinity ? 22 + measure(ctx, epsText, 34, true) : 0);
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
          const rp = Math.min(r, M.freeze);
          const st = stylesAt(M, rp);
          ctx.globalAlpha = mix * (1 - fin); drawPoints(ctx, P, 'sweep', st); ctx.globalAlpha = 1;
          const stop = phase === 'sweep' ? stopAt(t) : null;
          const mine = stop ? stop.events.filter((e) => e.key === P.key) : [];
          if (mine.length) highlight(ctx, P, st, mine, t, stop, Math.min(smooth((t - stop.t0) / 0.2), smooth((stop.t1 - t) / 0.3)));
          if (mix > 0.5) { ctx.globalAlpha = fo; drawLabels(ctx, P, 'sweep', st.marks, reserve); ctx.globalAlpha = 1; }
        }
        if (fin > 0) drawBest(ctx, P, M, reserveName, fin, fi);
        const stop = phase === 'sweep' ? stopAt(t) : null;
        if (stop) {
          ctx.globalAlpha = Math.min(smooth((t - stop.t0) / 0.25), smooth((stop.t1 - t) / 0.25));
          let bottom = P.y + P.h - 18;
          for (const e of stop.events.filter((x) => x.key === P.key).reverse()) bottom -= card(ctx, captionOf(e), P.x + P.w / 2, bottom) + 10;
          ctx.globalAlpha = 1;
        }
      } else drawLabels(ctx, P, 'truth', null, reserveName);
      ctx.restore();
      text(ctx, P.name, P.x + 26, P.y + 60, 48, C.text, { bold: true });
      if (phase === 'sweep' && r > M.freeze) {
        ctx.globalAlpha = smooth((r / M.freeze - 1) / 0.03);
        text(ctx, epsText, P.x + 26 + nameW + 22, P.y + 58, 34, C.dim, { bold: true });
        ctx.globalAlpha = 1;
      }
    }
    // fondu d'entrée et de sortie de la scène
    const fade = Math.max(1 - smooth(t / T_FADE), 1 - smooth((DURATION - t) / T_FADE));
    if (fade > 0) { ctx.globalAlpha = fade; ctx.fillStyle = C.bg; ctx.fillRect(0, 0, W, H); ctx.globalAlpha = 1; }
  }

  window.SocialPlayer = { W, H, THEMES, setTheme, load, renderAt, stops: () => S.stops, duration: () => DURATION };
})();
