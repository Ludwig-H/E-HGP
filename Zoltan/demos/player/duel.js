/*
 * Lecteur des vidéos « HGP contre HDBSCAN » des bouts de scène (Zoltan/demos).
 *
 * Deux colonnes, mêmes points, même ordre k : à gauche la hiérarchie de points HGP de morsehgp3D_v11
 * (H^r_{k+1}), à droite l'arbre de HDBSCAN (scikit-learn, min_samples = k). Le niveau r est commun ; il croît en
 * échelle logarithmique et marque une pause à chaque événement des branches suivies : un objet retrouvé (IoU > 0,5),
 * deux objets réunis dans un même groupe.
 *
 * Rendu Canvas2D pur, sans dépendance, déterministe : l'image ne dépend que du temps t, de la scène
 * (window.DUEL_SCENE, écrite par tools/duel_scene.py) et du thème. Thèmes de Percolia.com : sombre (défaut, fond
 * marine) et clair (fond blanc), mêmes couleurs que player.js (contrastes et daltonisme contrôlés par
 * tools/test_duel.py).
 *
 * Lecture d'une colonne :
 * - halo coloré sous les points : la vérité terrain, un halo par objet (A, B, C) ;
 * - gros point de la couleur d'un objet : le groupe de la hiérarchie qui suit cet objet (sa branche) ;
 * - gros point rouge : un groupe qui réunit les branches de deux objets ou plus (fusion) ;
 * - point gris moyen : un autre groupe ; petit point pâle : point encore seul à ce niveau ;
 * - points du fond (variante sans sol : murs, végétation, sol laissé par Patchwork++) : mêmes états, en plus petit ;
 * - en bas, l'IoU de chaque branche suivie en fonction de r (seuil 0,5 de la qualité panoptique).
 * Au début, la vérité terrain reste immobile (objets en couleur et nommés, halos renforcés), puis la caméra tourne.
 */
(function () {
  'use strict';

  const W = 1920, H = 1080;
  const COLS = [{ x: 24, w: 924, key: 'hgp' }, { x: 972, w: 924, key: 'hdbscan' }];
  const VIEW = { y: 158, h: 566 };
  const LANES = { y: 736, h: 268 };
  const FOOT = { y: 1016, h: 48 };
  const FONT = '"DejaVu Sans", "Helvetica Neue", Arial, sans-serif';
  const NONE = 0, FUSED = 3;  // états des lignes de suivi (tools/duel_scene.py : 1 fragment, 2 retrouvé)
  const EPS = 1 + 1e-12;
  const INTRO_ORBIT = 26, INTRO_LIFT = 10;  // degrés : petite orbite de l'introduction

  // Palettes de Percolia.com, comme player.js : sombre = fond #071b2e, surface #0f2c48, texte #eaf5f7, atténué
  // #9fb4c4 ; clair = fond #fff, panneaux #f4fafb, texte #082c4c, atténué #4a5f71. Objets A bleu, B ambre,
  // C vert d'eau ; fusion rouge. « Autre groupe » : le gris le plus contrasté qui reste à ΔE00 >= 20 de A, B, C et
  // de la fusion en vision normale, deutéranope et protanope (tools/test_duel.py) ; « point seul » plus pâle et plus petit.
  const THEMES = {
    dark: {
      bg: '#071b2e', panel: '#0f2c48', frame: '#3a5773', grid: '#1d4466', text: '#eaf5f7', dim: '#9fb4c4',
      plate: 'rgba(15,44,72,0.88)', fusion: '#f0606e', objects: ['#62b3ff', '#ffc53d', '#5ee8c8'],
      halo: 0.24, alone: 'rgba(159,180,196,0.30)', other: '#585864', ok: '#5ee8c8',
    },
    light: {
      bg: '#ffffff', panel: '#f4fafb', frame: '#b9cad4', grid: '#d3e0e6', text: '#082c4c', dim: '#4a5f71',
      plate: 'rgba(244,250,251,0.92)', fusion: '#a8102c', objects: ['#1f5fbf', '#c27a00', '#00897b'],
      halo: 0.18, alone: 'rgba(93,115,133,0.30)', other: '#b8b8c7', ok: '#00897b',
    },
  };
  let C = THEMES.dark;
  function themeName(name) { return name === 'light' || name === 'clair' ? 'light' : 'dark'; }
  function setTheme(name) { C = THEMES[themeName(name)]; return themeName(name); }

  const lerp = (a, b, u) => a + (b - a) * u;
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
  const smooth = (u) => { u = clamp(u, 0, 1); return u * u * (3 - 2 * u); };
  const frNum = (v, d) => v.toFixed(d).replace('.', ',');
  function typo(str) {  // apostrophe courbe, espaces insécables de la typographie française
    return String(str).replace(/'/g, '’').replace(/ ([:;%?!»])/g, ' $1').replace(/« /g, '« ');
  }
  function hexToRgb(h) { const v = parseInt(h.slice(1), 16); return [(v >> 16) & 255, (v >> 8) & 255, v & 255]; }
  function rgba(h, a) { const c = hexToRgb(h); return `rgba(${c[0]},${c[1]},${c[2]},${a})`; }
  // IoU : deux décimales, davantage près du seuil (0,502 > 0,5 ne doit pas se lire « 0,50 ») ; même règle que
  // iou_text de tools/duel_scene.py
  function iouText(v) {
    let d = 2;
    while (d < 6 && v !== 0.5 && Number(v.toFixed(d)) === 0.5) d++;
    return frNum(v, d);
  }
  // une décimale partout : des événements HGP se suivent à quelques millimètres (16,0 ; 16,2 ; 16,4 cm)
  function cm(r) { return frNum(100 * r, 1) + ' cm'; }

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
  // Texte en morceaux colorés sur une ligne : [[texte, couleur, gras], ...], centré en x.
  function richWidth(ctx, parts, size) { return parts.reduce((s, p) => s + measure(ctx, p[0], size, p[2]), 0); }
  function rich(ctx, parts, x, y, size, align) {
    let xx = align === 'center' ? x - richWidth(ctx, parts, size) / 2 : x;
    for (const [str, col, bold] of parts) {
      text(ctx, str, xx, y, size, col, { bold });
      xx += measure(ctx, str, size, bold);
    }
  }

  // ---------------------------------------------------------------- scène
  function prepare(scene) {
    if (scene.schema !== 'ehgp.zoltan.duel.v2') throw new Error('scène inconnue : relancer tools/duel_scene.py');
    for (const p of scene.timing.pauses) for (const key of ['hgp', 'hdbscan']) for (const b of p.badges[key]) { roleColor(b.border); b.parts.forEach((q) => roleColor(q[1])); }
    const n = scene.points.x.length;
    const S = {
      scene, n, nobj: scene.objects.length,
      x: Float64Array.from(scene.points.x), y: Float64Array.from(scene.points.y), z: Float64Array.from(scene.points.z),
      gt: Int8Array.from(scene.gt), timing: scene.timing, background: scene.gt.some((g) => g < 0),
      keyT: scene.timing.schedule.map((k) => k[0]), keyR: scene.timing.schedule.map((k) => k[1]),
      rmin: scene.timing.rmin, rmax: scene.timing.rmax,
      methods: {},
    };
    for (const col of COLS) {
      const m = scene.methods[col.key];
      S.methods[col.key] = {
        m, levels: Float64Array.from(m.levels), evP: Int32Array.from(m.ev_plateau), evK: Int8Array.from(m.ev_kind),
        evA: Int32Array.from(m.ev_a), evB: Int32Array.from(m.ev_b),
        state: null,  // rejeu incrémental : union-find des sites
      };
    }
    S.view = fitView(S);
    return S;
  }

  // Niveau au temps t : log-linéaire entre les jalons, ralenti à l'approche des pauses, exact pendant les pauses.
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
  // Avancement du balayage (0 à 1), figé pendant les pauses : il pilote le lent panoramique de la caméra.
  function sweepProgress(S, r) {
    return clamp(Math.log(r / S.rmin) / Math.log(S.rmax / S.rmin), 0, 1);
  }

  // État d'une hiérarchie au niveau r : rejoue les événements des plateaux de niveau <= r (union-find).
  function hierarchyAt(S, M, r) {
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

  // Ligne de suivi (niveau, IoU, état, masque, taille) en vigueur au niveau r.
  function trackAt(track, r) {
    let best = null;
    for (const row of track) { if (row[0] <= r * EPS) best = row; else break; }
    return best;
  }
  const foundBy = (track, r) => track.some((row) => row[0] <= r * EPS && row[1] > 0.5);

  // ---------------------------------------------------------------- caméra
  function cameraOf(S, t, r) {
    const v = S.view, intro = S.timing.intro;
    let az = v.az + v.pan * (sweepProgress(S, r) - 0.5), el = v.el;
    if (t < intro) {  // vérité terrain immobile (timing.hold), puis orbite jusqu'à la pose du balayage
      const u = smooth((t - S.timing.hold) / (intro - S.timing.hold));
      az = lerp(v.az - v.pan / 2 - INTRO_ORBIT, v.az - v.pan / 2, u);
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
  // Cadrage : la focale qui fait tenir tous les points, pour tout le panoramique et l'orbite de l'introduction, dans
  // 92 % de la largeur de la vue et sa hauteur moins 190 px (place pour les bandeaux et les lettres des objets).
  function fitView(S) {
    // cadrage sur les seuls objets suivis : le fond (variante sans sol) déborde et sort du cadre
    const fit = [];
    for (let i = 0; i < S.n; i++) if (S.gt[i] >= 0) fit.push(i);
    let rad = 0, zmax = 0;
    for (const i of fit) { rad = Math.max(rad, Math.hypot(S.x[i], S.y[i], S.z[i])); zmax = Math.max(zmax, S.z[i]); }
    const view = S.scene.view || { az: 0, el: 30 };  // choisie par tools/duel_scene.py (objets le moins superposés)
    // caméra immobile pendant le balayage (pan = 0) : seule l'introduction tourne ; des images qui ne changent que par
    // la couleur des points se compressent dix fois mieux qu'un panoramique sur des milliers de points
    const v = { target: [0, 0, zmax / 2], dist: 5.5 * Math.max(rad, 0.5), az: view.az, el: view.el, pan: 0, focal: 1 };
    let w = 0, h = 0, cy = 0, cnt = 0;
    const poses = [[v.az - v.pan / 2, v.el], [v.az, v.el], [v.az + v.pan / 2, v.el],
      [v.az - v.pan / 2 - INTRO_ORBIT, v.el + INTRO_LIFT], [v.az - v.pan / 2 - INTRO_ORBIT / 2, v.el + INTRO_LIFT / 2]];
    for (const [az, el] of poses) {
      const cam = basis(v, az, el);
      let x0 = Infinity, x1 = -Infinity, y0 = Infinity, y1 = -Infinity;
      for (const i of fit) {
        const p = project(cam, S.x[i], S.y[i], S.z[i]);
        x0 = Math.min(x0, p[0]); x1 = Math.max(x1, p[0]); y0 = Math.min(y0, p[1]); y1 = Math.max(y1, p[1]);
      }
      w = Math.max(w, 2 * Math.max(-x0, x1)); h = Math.max(h, y1 - y0); cy += (y0 + y1) / 2; cnt++;
    }
    // place libre en haut pour les bandeaux d'événement et les lettres des objets
    v.focal = Math.min(0.92 * COLS[0].w / w, (VIEW.h - 190) / h) * (S.background ? 0.86 : 1);  // un peu de contexte
    v.shiftY = -v.focal * cy / cnt + 70;
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
    if (!haloCanvas) { haloCanvas = document.createElement('canvas'); haloCanvas.width = COLS[0].w; haloCanvas.height = VIEW.h; }
    return haloCanvas;
  }

  // Vue 3D d'une colonne. info : état de la hiérarchie au niveau courant (null pendant l'introduction) ; mix : fondu
  // de la vérité vers la hiérarchie.
  function drawView(ctx, S, col, cam, info, mix) {
    const X0 = col.x, Y0 = VIEW.y, cx = col.w / 2, cy = VIEW.h / 2 + S.view.shiftY;
    panel(ctx, X0, Y0, col.w, VIEW.h);
    const n = S.n, sx = new Float64Array(n), sy = new Float64Array(n), sz = new Float64Array(n);
    for (let i = 0; i < n; i++) {
      const p = project(cam, S.x[i], S.y[i], S.z[i]);
      sx[i] = cx + p[0]; sy[i] = cy + p[1]; sz[i] = p[2];
    }
    // halos de la vérité terrain : un calque par objet, opaque, puis posé avec une seule transparence
    const layer = haloLayer(), lc = layer.getContext('2d');
    ctx.save();
    roundRect(ctx, X0 + 1, Y0 + 1, col.w - 2, VIEW.h - 2, 9); ctx.clip();
    for (let o = 0; o < S.nobj; o++) {
      lc.clearRect(0, 0, layer.width, layer.height);
      lc.fillStyle = C.objects[o];
      lc.beginPath();
      for (let i = 0; i < n; i++) if (S.gt[i] === o) disc(lc, sx[i], sy[i], 11);
      lc.fill();
      ctx.globalAlpha = Math.min(1, C.halo * (1 + 0.7 * (1 - mix)));  // plus marqués pendant la vérité terrain
      ctx.drawImage(layer, X0, Y0);
      ctx.globalAlpha = 1;
    }
    // points : d'abord seuls et autres groupes, puis branches suivies, du plus loin au plus proche
    const order = Array.from({ length: n }, (_, i) => i).sort((a, b) => sz[b] - sz[a]);
    const style = new Uint8Array(n);  // 0 seul, 1 autre groupe, 2+o branche de l'objet o, 9 fusion
    if (info) {
      for (let i = 0; i < n; i++) {
        if (!info.big[i]) { style[i] = 0; continue; }
        const rt = info.root[i];
        let s = 1;
        for (let o = 0; o < S.nobj; o++) {
          if (info.seedRoot[o] === rt) { s = info.fusedRoot.has(rt) ? 9 : 2 + o; break; }
        }
        style[i] = s;
      }
    }
    const colorOf = (s) => (s === 9 ? C.fusion : C.objects[s - 2]);
    // tailles : un point d'objet reste plus gros qu'un point du fond, quel que soit son état
    const radiusOf = (s, g) => (g >= 0 ? (s === 0 ? 2.2 : (s === 1 ? 3.0 : 4.3)) : (s === 0 ? 1.5 : (s === 1 ? 2.1 : 2.5)));
    const drawSet = (keep) => {
      for (const i of order) {
        const s = style[i], g = S.gt[i];
        if (!keep(s, g)) continue;
        const X = X0 + sx[i], Y = Y0 + sy[i];
        if (mix < 1) {  // fondu depuis la vérité : objets en couleur et gros, fond petit et pâle
          ctx.globalAlpha = 1 - mix;
          ctx.fillStyle = g >= 0 ? C.objects[g] : C.alone;
          ctx.beginPath(); disc(ctx, X, Y, g >= 0 ? 4.2 : 1.6); ctx.fill();
          ctx.globalAlpha = mix;
          if (mix <= 0) { ctx.globalAlpha = 1; continue; }
        }
        ctx.fillStyle = s === 0 ? C.alone : (s === 1 ? C.other : colorOf(s));
        ctx.beginPath(); disc(ctx, X, Y, radiusOf(s, g)); ctx.fill();
        ctx.globalAlpha = 1;
      }
    };
    drawSet((s, g) => s <= 1 && g < 0);  // fond d'abord, puis objets : un mur devant un vélo ne le cache pas
    drawSet((s, g) => s <= 1 && g >= 0);
    // anneau de la fusion : un liseré clair autour des points rouges, qui pulse pendant la pause de l'événement
    if (info && info.pulse > 0) {
      ctx.strokeStyle = rgba(C.fusion, 0.55 * info.pulse); ctx.lineWidth = 2;
      ctx.beginPath();
      for (const i of order) if (style[i] === 9) disc(ctx, X0 + sx[i], Y0 + sy[i], 7.5 + 2.5 * info.pulse);
      ctx.stroke();
    }
    drawSet((s) => s >= 2);
    // lettres des objets, au-dessus de leur point le plus haut à l'écran ; une étiquette qui en chevauche une
    // autre monte d'un cran (placement indépendant du temps hors de l'introduction : positions de la caméra figée)
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
      labels.push({ o, parts, w, x: clamp(X0 + mx / cnt, X0 + w / 2 + 8, X0 + col.w - w / 2 - 8), y: Math.max(Y0 + 76, Y0 + top - 22) });
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

  // Bandeau d'événement en haut d'une vue : [{parts, border, alpha}]
  function drawBadges(ctx, col, badges) {
    let y = VIEW.y + 16;
    for (const b of badges) {
      if (b.alpha <= 0) continue;
      const size = 23, w = richWidth(ctx, b.parts, size) + 34, x = col.x + col.w / 2 - w / 2;
      ctx.globalAlpha = b.alpha;
      ctx.fillStyle = C.plate; roundRect(ctx, x, y, w, 42, 21); ctx.fill();
      ctx.strokeStyle = b.border; ctx.lineWidth = 2.5; roundRect(ctx, x + 1, y + 1, w - 2, 40, 20); ctx.stroke();
      rich(ctx, b.parts, col.x + col.w / 2, y + 29, size, 'center');
      ctx.globalAlpha = 1;
      y += 52;
    }
  }

  // IoU de chaque branche suivie en fonction de r (axe log commun aux deux colonnes).
  function drawLanes(ctx, S, col, r, phase) {
    const M = S.methods[col.key].m, nobj = S.nobj;
    panel(ctx, col.x, LANES.y, col.w, LANES.h);
    const x0 = col.x + 76, x1 = col.x + col.w - 120;
    const X = (v) => x0 + (x1 - x0) * (Math.log(v) - Math.log(S.rmin)) / (Math.log(S.rmax) - Math.log(S.rmin));
    const axisY = LANES.y + LANES.h - 38;
    const laneH = Math.min(70, (axisY - LANES.y - 40) / nobj);
    const top0 = LANES.y + 36 + (axisY - LANES.y - 40 - laneH * nobj) / 2;
    text(ctx, 'IoU du groupe qui suit chaque objet', col.x + 18, LANES.y + 26, 17, C.dim);
    const rShow = phase === 'intro' ? 0 : r;
    // graduations en centimètres
    ctx.strokeStyle = C.grid; ctx.lineWidth = 1;
    const ticks = [0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2].filter((v) => v >= S.rmin * 0.999 && v <= S.rmax * 1.001);
    for (const v of ticks) {
      const xx = X(v);
      ctx.beginPath(); ctx.moveTo(xx, top0 - 4); ctx.lineTo(xx, axisY); ctx.stroke();
      text(ctx, `${frNum(100 * v, v < 0.01 ? 1 : 0)} cm`, xx, axisY + 24, 16, C.dim, { align: 'center' });
    }
    text(ctx, 'r', x0 - 30, axisY + 24, 17, C.dim, { bold: true });
    for (let o = 0; o < nobj; o++) {
      const ty = top0 + o * laneH, by = ty + laneH - 10, hgt = laneH - 18;
      const Y = (v) => by - hgt * v;
      const color = C.objects[o];
      text(ctx, S.scene.objects[o].key, col.x + 20, ty + laneH / 2 + 4, 24, color, { bold: true });
      ctx.strokeStyle = C.grid; ctx.lineWidth = 1;
      ctx.beginPath(); ctx.moveTo(x0, by); ctx.lineTo(x1, by); ctx.stroke();
      ctx.setLineDash([5, 5]); ctx.strokeStyle = C.dim; ctx.globalAlpha = 0.8;
      ctx.beginPath(); ctx.moveTo(x0, Y(0.5)); ctx.lineTo(x1, Y(0.5)); ctx.stroke();
      ctx.setLineDash([]); ctx.globalAlpha = 1;
      if (o === 0) text(ctx, '0,5', x0 - 8, Y(0.5) + 5, 14, C.dim, { align: 'right' });
      // courbe en escalier jusqu'au niveau courant
      const tr = M.tracks[o];
      const segs = [];
      for (let q = 0; q < tr.length; q++) {
        if (tr[q][0] > rShow * EPS || tr[q][2] === NONE) continue;  // marche du niveau courant comprise
        const a = Math.max(tr[q][0], S.rmin), b = Math.max(a, Math.min(q + 1 < tr.length ? tr[q + 1][0] : S.rmax, rShow, S.rmax));
        segs.push([a, b, tr[q][1], tr[q][2]]);
      }
      for (const [a, b, v, st] of segs) {
        const c = st === FUSED ? C.fusion : color;
        ctx.fillStyle = rgba(c, st === FUSED ? 0.30 : 0.28);
        ctx.fillRect(X(a), Y(v), Math.max(0.5, X(b) - X(a)), by - Y(v));
      }
      ctx.lineJoin = 'round';
      for (const [a, b, v, st] of segs) {
        ctx.strokeStyle = st === FUSED ? C.fusion : color;
        ctx.lineWidth = v > 0.5 ? 4 : 2.5;
        ctx.beginPath(); ctx.moveTo(X(a), Y(v)); ctx.lineTo(X(b), Y(v)); ctx.stroke();
      }
      // marches verticales
      ctx.lineWidth = 1.5;
      for (let q = 1; q < segs.length; q++) {
        if (segs[q][0] !== segs[q - 1][1]) continue;
        ctx.strokeStyle = segs[q][3] === FUSED ? C.fusion : color;
        ctx.beginPath(); ctx.moveTo(X(segs[q][0]), Y(segs[q - 1][2])); ctx.lineTo(X(segs[q][0]), Y(segs[q][2])); ctx.stroke();
      }
      // meilleur IoU atteint jusqu'ici : losange, et valeur à droite
      let best = 0, bestAt = null;
      for (const s of segs) if (s[2] > best) { best = s[2]; bestAt = s[0]; }
      if (bestAt != null) {
        const xx = X(bestAt), yy = Y(best);
        ctx.fillStyle = best > 0.5 ? color : C.panel; ctx.strokeStyle = color; ctx.lineWidth = 2;
        ctx.beginPath(); ctx.moveTo(xx, yy - 8); ctx.lineTo(xx + 6, yy); ctx.lineTo(xx, yy + 8); ctx.lineTo(xx - 6, yy); ctx.closePath();
        ctx.fill(); ctx.stroke();
      }
      const now = phase === 'intro' ? null : trackAt(tr, r);
      if (phase !== 'intro') {
        const found = best > 0.5;
        const parts = [[iouText(best), found ? C.text : C.dim, true]];
        parts.push([found ? '  ✓' : (now && now[2] === FUSED ? '  ✗' : ''), found ? C.ok : C.fusion, true]);
        rich(ctx, parts, x1 + 18, ty + laneH / 2 + 7, 20, 'left');
      }
    }
    text(ctx, 'meilleur', x1 + 18, LANES.y + 26, 15, C.dim);
    // curseur
    if (phase !== 'intro') {
      const xx = X(clamp(r, S.rmin, S.rmax));
      ctx.strokeStyle = C.text; ctx.lineWidth = 1.5; ctx.setLineDash([4, 4]);
      ctx.beginPath(); ctx.moveTo(xx, top0 - 8); ctx.lineTo(xx, axisY); ctx.stroke(); ctx.setLineDash([]);
    }
  }

  function drawHeader(ctx, S, r, phase) {
    const m = S.scene.meta;
    text(ctx, m.title, 24, 52, 34, C.text, { bold: true });
    text(ctx, m.subtitle, 24, 86, 20, C.dim);
    if (phase !== 'intro') {
      const lab = `r = ${cm(r)}`;
      text(ctx, lab, W - 24, 62, 38, C.text, { bold: true, align: 'right' });
    } else {
      text(ctx, 'vérité terrain', W - 24, 62, 30, C.dim, { bold: true, align: 'right' });
    }
    for (const col of COLS) {
      const isH = col.key === 'hgp';
      const name = isH ? 'HGP' : 'HDBSCAN';
      const sub = isH ? `Morse HGP 3D v11 · hiérarchie de points Hʳₖ₊₁ · k = ${m.k}` : `scikit-learn 1.7.2 · min_samples = ${m.k}`;
      text(ctx, name, col.x + 4, 140, 32, C.text, { bold: true });
      text(ctx, sub, col.x + 14 + measure(ctx, name, 32, true), 139, 19, C.dim);
    }
  }

  // Pied sur deux lignes : la légende, puis la convention de niveau et la source.
  function drawFooter(ctx, S) {
    const y = FOOT.y + 20;
    let x = 28;
    const item = (draw, label) => { draw(x, y - 6); x += 22; text(ctx, label, x, y, 16, C.dim); x += measure(ctx, label, 16) + 26; };
    item((xx, yy) => { ctx.fillStyle = rgba(C.objects[0], C.halo * 2.2); ctx.beginPath(); disc(ctx, xx + 6, yy, 9); ctx.fill(); }, 'halo : objet (vérité terrain)');
    item((xx, yy) => { ctx.fillStyle = C.objects[0]; ctx.beginPath(); disc(ctx, xx + 6, yy, 5); ctx.fill(); }, 'groupe qui suit l\'objet');
    item((xx, yy) => { ctx.fillStyle = C.fusion; ctx.beginPath(); disc(ctx, xx + 6, yy, 5); ctx.fill(); }, 'objets réunis');
    item((xx, yy) => { ctx.fillStyle = C.other; ctx.beginPath(); disc(ctx, xx + 6, yy, 3.6); ctx.fill(); }, 'autre groupe');
    item((xx, yy) => { ctx.fillStyle = C.alone; ctx.beginPath(); disc(ctx, xx + 6, yy, 2.4); ctx.fill(); }, 'point seul');
    if (S.background) item((xx, yy) => { ctx.fillStyle = C.other; ctx.beginPath(); disc(ctx, xx + 6, yy, 2.1); ctx.fill(); }, 'petits points : le fond (hors objets)');
    text(ctx, 'r : rayon des boules d\'ordre k ; pour HDBSCAN, distance d\'atteignabilité mutuelle / 2 · SemanticKITTI (CC BY-NC-SA)',
      28, y + 26, 15, C.dim);
  }

  function listing(keys) {
    return keys.length === 1 ? keys[0] : keys.slice(0, -1).join(', ') + ' et ' + keys[keys.length - 1];
  }
  // rôle de couleur écrit par tools/duel_scene.py -> couleur du thème courant
  function roleColor(role) {
    if (/^obj[0-2]$/.test(role)) return C.objects[Number(role[3])];
    const c = { ok: C.ok, fusion: C.fusion, text: C.text, dim: C.dim }[role];
    if (!c) throw new Error(`rôle de couleur inconnu : ${role}`);
    return c;
  }

  // Bandeaux des pauses, écrits par tools/duel_scene.py (objet retrouvé ; objets retrouvés et encore séparés ;
  // fusion, rouge si un objet réuni n'avait pas encore été retrouvé ; ce que montre l'autre colonne au même r).
  function badgesAt(S, t) {
    const out = { hgp: [], hdbscan: [] };
    for (const p of S.timing.pauses) {
      if (t < p.t0 || t > p.t1) continue;
      const alpha = Math.min(smooth((t - p.t0) / 0.3), smooth((p.t1 - t) / 0.3));
      for (const key of ['hgp', 'hdbscan']) {
        for (const b of p.badges[key]) {
          out[key].push({ alpha, border: roleColor(b.border), parts: b.parts.map(([str, role, bold]) => [str, roleColor(role), bold]) });
        }
      }
    }
    return out;
  }

  // Résumé final : meilleur IoU de chaque objet dans chaque hiérarchie.
  function drawSummary(ctx, S, col, u) {
    const M = S.methods[col.key].m;
    const letters = S.scene.objects.map((ob) => ob.key);
    const parts = [['meilleur IoU   ', C.dim, false]];
    M.best.forEach((v, o) => {
      parts.push([`${letters[o]} ${iouText(v)}`, C.objects[o], true]);
      parts.push([v > 0.5 ? ' ✓' : ' ✗', v > 0.5 ? C.ok : C.fusion, true]);
      if (o + 1 < M.best.length) parts.push(['    ', C.dim, false]);
    });
    const never = M.best.map((v, o) => (v <= 0.5 ? letters[o] : null)).filter(Boolean);
    const plural = never.length > 1;
    const verdict = never.length ? [[`${listing(never)} jamais retrouvé${plural ? 's' : ''} : aucun groupe ne ${plural ? 'les' : 'le'} recouvre à plus de 50 %`, C.fusion, true]]
      : [['chaque objet retrouvé par un groupe de la hiérarchie', C.ok, true]];
    ctx.globalAlpha = u;
    const w = Math.max(richWidth(ctx, parts, 26), richWidth(ctx, verdict, 20)) + 48, x = col.x + col.w / 2 - w / 2, y = VIEW.y + 16;
    ctx.fillStyle = C.plate; roundRect(ctx, x, y, w, 92, 14); ctx.fill();
    ctx.strokeStyle = never.length ? C.fusion : C.ok; ctx.lineWidth = 2.5; roundRect(ctx, x + 1, y + 1, w - 2, 90, 13); ctx.stroke();
    rich(ctx, parts, col.x + col.w / 2, y + 38, 26, 'center');
    rich(ctx, verdict, col.x + col.w / 2, y + 74, 20, 'center');
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
    const cam = cameraOf(S, t, r);
    // états des deux hiérarchies au niveau r
    const states = {};
    for (const col of COLS) {
      const Mq = S.methods[col.key];
      const h = hierarchyAt(S, Mq, phase === 'intro' ? 0 : r);
      const seedRoot = Mq.m.seeds.map((s) => (h.big[s] ? h.root[s] : -1));
      const fusedRoot = new Set();
      seedRoot.forEach((x, o) => { if (x >= 0 && seedRoot.some((y, q) => q !== o && y === x)) fusedRoot.add(x); });
      const marks = S.scene.objects.map((_, o) => {
        if (phase === 'intro') return '';
        if (foundBy(Mq.m.tracks[o], r)) return '✓';
        return fusedRoot.has(seedRoot[o]) ? '✗' : '';
      });
      // pulsation des points réunis pendant la pause d'une fusion de cette colonne
      let pulse = 0;
      for (const p of T.pauses) {
        if (t >= p.t0 && t <= p.t1 && p.roles.some((q) => q.startsWith('fusion:' + col.key))) {
          pulse = 0.5 + 0.5 * Math.cos(2 * Math.PI * (t - p.t0) / 1.2);
          pulse *= Math.min(smooth((t - p.t0) / 0.3), smooth((p.t1 - t) / 0.3));
        }
      }
      states[col.key] = { root: h.root, big: h.big, seedRoot, fusedRoot, marks, pulse };
    }
    const badges = phase === 'sweep' ? badgesAt(S, t) : { hgp: [], hdbscan: [] };
    for (const col of COLS) {
      drawView(ctx, S, col, cam, phase === 'intro' ? null : states[col.key], mix);
      if (phase === 'summary') drawSummary(ctx, S, col, smooth((t - T.summary) / 0.6));
      else drawBadges(ctx, col, badges[col.key]);
      drawLanes(ctx, S, col, phase === 'summary' ? S.rmax : r, phase);
    }
    drawFooter(ctx, S);
    return { r, phase };
  }

  // hierarchyAt est exposé pour tools/test_duel.py (rejeu JavaScript contre rejeu Python)
  window.DuelPlayer = { W, H, prepare, renderAt, rAt, hierarchyAt, setTheme, themeName, THEMES };
})();
