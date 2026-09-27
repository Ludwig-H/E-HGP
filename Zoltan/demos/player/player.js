/*
 * Lecteur des démos « hiérarchie d'un clustering concurrent » (Zoltan/demos).
 *
 * Rendu Canvas2D pur, sans dépendance, déterministe : l'image ne dépend que
 * du temps t (secondes) et de la scène (window.DEMO_SCENE, écrite par
 * tools/build_scene.py). Le même code sert au lecteur interactif
 * (index.html) et à la capture image par image (tools/render_video.cjs).
 *
 * Géométrie : la scène est déjà tournée pour que les objets suivis soient
 * alignés sur les axes (x le long de la rangée, capteur du côté y < 0).
 * Les boîtes affichées sont donc des boîtes alignées sur les axes (AABB)
 * dans ce repère.
 *
 * Deux thèmes, comme Percolia.com : sombre (défaut, fond marine) et clair
 * (fond blanc). La scène ne porte que des rôles de couleur ('obj0'..'obj2',
 * 'fusion', 'text') ; le thème courant les résout (setTheme).
 */
(function () {
  'use strict';

  const W = 1920, H = 1080;
  const PANEL3D = { x: 24, y: 112, w: 1224, h: 780 };
  const BEV = { x: 1272, y: 112, w: 624, h: 404 };
  const DENDRO = { x: 1272, y: 532, w: 624, h: 360 };
  const FOOT = { x: 24, y: 908, w: 1872, h: 152 };

  // Palettes de Percolia.com : sombre = --color-bg #071b2e, panneaux sur la surface
  // --color-surface #0f2c48, texte #eaf5f7, atténué #9fb4c4 ; clair = fond #fff, texte
  // #082c4c. En clair, les panneaux sont #f4fafb plutôt que la surface #eaf5f7 (l'atténué
  // #5d7385 de Percolia y tomberait à 4,44:1) ; l'atténué des vidéos est de toute façon foncé
  // à #4a5f71 (6,29:1 sur #f4fafb) pour la projection.
  // Couleurs des objets suivis (A bleu, B ambre, C vert d'eau) et de la fusion (rouge) :
  // contraste >= 3:1 sur le panneau, et CIEDE2000 >= 20 entre A, B, C, fusion et « autre
  // cluster » en vision normale, deutéranope et protanope (Machado 2009) ; contrôlé par
  // tools/test_scene_contract.py. La couleur ne porte jamais seule le sens : un point
  // fusionné est cerné (anneau rouge séparé par un vide), un point de fond pris par une
  // branche est un carré creux, une boîte fusionnée a des tirets espacés.
  const THEMES = {
    dark: {
      bg: '#071b2e', panel: '#0f2c48', summary: '#0f2c48', frame: '#3a5773', grid: '#1d4466',
      text: '#eaf5f7', dim: '#9fb4c4', plate: 'rgba(15,44,72,0.88)', band: 'rgba(234,245,247,0.14)',
      alive: 'rgba(159,180,196,0.40)', unborn: 'rgba(159,180,196,0.22)', hollow: 0.75, horspq: 'rgba(159,180,196,0.70)',
      fusion: '#f0606e', objects: ['#62b3ff', '#ffc53d', '#5ee8c8'],
    },
    light: {
      bg: '#ffffff', panel: '#f4fafb', summary: '#f4fafb', frame: '#b9cad4', grid: '#d3e0e6',
      text: '#082c4c', dim: '#4a5f71', plate: 'rgba(244,250,251,0.92)', band: 'rgba(8,44,76,0.10)',
      alive: 'rgba(93,115,133,0.40)', unborn: 'rgba(93,115,133,0.22)', hollow: 0.8, horspq: 'rgba(93,115,133,0.70)',
      fusion: '#a8102c', objects: ['#1f5fbf', '#c27a00', '#00897b'],
    },
  };
  let C = THEMES.dark;
  // 'clair' / 'sombre' (noms des fichiers vidéo) ou 'light' / 'dark' (data-theme de Percolia)
  function themeName(name) {
    return name === 'light' || name === 'clair' ? 'light' : 'dark';
  }
  function setTheme(name) { C = THEMES[themeName(name)]; return themeName(name); }
  const objColor = (o) => C.objects[o];
  // rôle de couleur écrit par build_scene.py -> couleur du thème courant
  const ROLES = /^(obj[0-2]|fusion|text)$/;
  function roleColor(role) {
    if (role === 'text') return C.text;
    if (role === 'fusion') return C.fusion;
    if (ROLES.test(role)) return C.objects[Number(role[3])];
    throw new Error(`rôle de couleur inconnu : ${role}`);
  }
  const FONT = '"DejaVu Sans", "Helvetica Neue", Arial, sans-serif';
  const MATCHED = 2, FUSED = 3;
  const EV = { r: 0, box: 1, rec: 7, prec: 8, size: 9, mask: 10, state: 11 };
  const EPS = 1 + 1e-9;

  const lerp = (a, b, u) => a + (b - a) * u;
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
  const smooth = (u) => { u = clamp(u, 0, 1); return u * u * (3 - 2 * u); };
  const frNum = (v, d) => v.toFixed(d).replace('.', ',');

  // Typographie française : apostrophe courbe, espaces insécables avant : ; % ? ! » et après «.
  function typo(str) {
    return String(str)
      .replace(/'/g, '’')
      .replace(/ ([:;%?!»])/g, ' $1')
      .replace(/« /g, '« ');
  }
  function hexToRgb(h) {
    const v = parseInt(h.slice(1), 16);
    return [(v >> 16) & 255, (v >> 8) & 255, v & 255];
  }
  function rgba(h, a) { const c = hexToRgb(h); return `rgba(${c[0]},${c[1]},${c[2]},${a})`; }

  // ---------------------------------------------------------------- scène
  function prepare(scene) {
    // une scène antérieure aux thèmes porte des couleurs hexadécimales : on la refuse
    const bad = scene.captions.filter((c) => !ROLES.test(c.color));
    if (bad.length || scene.objects.length > THEMES.dark.objects.length || scene.objects.some((o) => 'color' in o)) {
      throw new Error('scène antérieure aux thèmes clair/sombre (ou plus de 3 objets) : relancer tools/build_scene.py');
    }
    const S = {
      scene, n: scene.points.x.length,
      x: Float32Array.from(scene.points.x), y: Float32Array.from(scene.points.y), z: Float32Array.from(scene.points.z),
      // niveaux en double précision : ils sont comparés exactement au niveau des pauses
      birth: Float64Array.from(scene.birth), gt: Int8Array.from(scene.gt),
      join: scene.join.map((a) => Float64Array.from(a)),
      objs: scene.objects,
      keyT: scene.schedule.map((k) => k[0]), keyR: scene.schedule.map((k) => Math.log(k[1])),
      keyRaw: scene.schedule.map((k) => k[1]),
      duration: scene.timing.duration,
    };
    S.rows = dendroRows(scene);
    return S;
  }

  // Niveau affiché au temps t : log-linéaire entre les jalons, valeur exacte pendant les pauses.
  function rAt(S, t) {
    const T = S.keyT;
    if (t <= T[0]) return S.keyRaw[0];
    for (let i = 1; i < T.length; i++) {
      if (t <= T[i]) {
        if (S.keyRaw[i - 1] === S.keyRaw[i]) return S.keyRaw[i];
        const u = (t - T[i - 1]) / Math.max(1e-9, T[i] - T[i - 1]);
        return Math.exp(lerp(S.keyR[i - 1], S.keyR[i], u));
      }
    }
    return S.keyRaw[S.keyRaw.length - 1];
  }

  // Dernier événement d'une branche suivie au niveau r (événements triés par niveau).
  function trackState(track, r) {
    const ev = track.events;
    let lo = 0, hi = ev.length - 1, best = -1;
    while (lo <= hi) {
      const mid = (lo + hi) >> 1;
      if (ev[mid][0] <= r * EPS) { best = mid; lo = mid + 1; } else hi = mid - 1;
    }
    return best < 0 ? null : ev[best];
  }

  // Structure du dendrogramme restreint : à chaque fusion, la ligne du plus petit
  // indice continue, les autres parties s'y raccordent et s'arrêtent.
  function dendroRows(scene) {
    const k = scene.objects.length;
    const group = Array.from({ length: k }, (_, o) => [o]);
    const end = new Array(k).fill(Infinity);
    const links = [];
    for (const m of [...scene.branch_merges].sort((a, b) => a.r - b.r)) {
      const parts = [];
      for (const o of m.objects) if (!parts.some((p) => p.includes(o))) parts.push(group[o]);
      const all = [...new Set(parts.flat())].sort((a, b) => a - b);
      const keep = all[0];
      for (const p of parts) {
        const head = Math.min(...p);
        if (head !== keep) { end[head] = Math.min(end[head], m.r); links.push({ r: m.r, from: head, to: keep }); }
      }
      for (const o of all) group[o] = all;
    }
    return { end, links };
  }

  // --------------------------------------------------------------- caméra
  function camera(S, t) {
    const v = S.scene.view;
    let az = v.azimuth, el = v.elevation;
    const intro = S.scene.timing.intro;
    if (t < intro) {
      const u = smooth(t / intro);
      az = lerp(v.azimuth + v.intro_orbit, v.azimuth, u);
      el = lerp(v.elevation + 12, v.elevation, u);
    }
    const a = az * Math.PI / 180, e = el * Math.PI / 180;
    const tg = v.target;
    const pos = [tg[0] + v.distance * Math.cos(e) * Math.sin(a),
      tg[1] - v.distance * Math.cos(e) * Math.cos(a),
      tg[2] + v.distance * Math.sin(e)];
    const f = norm([tg[0] - pos[0], tg[1] - pos[1], tg[2] - pos[2]]);
    const right = norm(cross(f, [0, 0, 1]));
    const up = cross(right, f);
    const focal = 0.5 * PANEL3D.h / Math.tan(v.fov * Math.PI / 360);
    return { pos, f, right, up, focal, cx: PANEL3D.x + PANEL3D.w / 2, cy: PANEL3D.y + PANEL3D.h / 2 + v.shift_y };
  }
  function cross(a, b) { return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]; }
  function norm(a) { const l = Math.hypot(a[0], a[1], a[2]); return [a[0] / l, a[1] / l, a[2] / l]; }
  function project(cam, X, Y, Z) {
    const dx = X - cam.pos[0], dy = Y - cam.pos[1], dz = Z - cam.pos[2];
    const zc = dx * cam.f[0] + dy * cam.f[1] + dz * cam.f[2];
    const xc = dx * cam.right[0] + dy * cam.right[1] + dz * cam.right[2];
    const yc = dx * cam.up[0] + dy * cam.up[1] + dz * cam.up[2];
    return [cam.cx + cam.focal * xc / zc, cam.cy - cam.focal * yc / zc, zc];
  }

  // ------------------------------------------------------- style des points
  // 0 non né · 1 vivant, hors branches suivies · 2+o point de l'objet o dans sa propre
  // branche · 5+o idem, branche fusionnée (cerné de rouge) · 8 point de fond absorbé par une
  // branche fusionnée · 9+o point de fond dans la branche o non fusionnée · 12 point void
  // (gt = -2 : hors PQ, il ne compte ni dans la précision ni dans l'IoU) pris par une branche.
  function pointStyles(S, r, phase, fused) {
    const st = new Uint8Array(S.n), k = S.join.length;
    for (let i = 0; i < S.n; i++) {
      const g = S.gt[i];
      if (phase === 'intro') { st[i] = g >= 0 ? 2 + g : 1; continue; }
      if (S.birth[i] > r * EPS) { st[i] = 0; continue; }
      if (g >= 0 && S.join[g][i] <= r * EPS) { st[i] = fused[g] ? 5 + g : 2 + g; continue; }
      let s = 1;
      for (let o = 0; o < k; o++) {
        // point d'un objet suivi pris dans la branche d'un autre : sa couleur, cerné de rouge
        if (S.join[o][i] <= r * EPS) { s = g >= 0 ? 5 + g : (g === -2 ? 12 : (fused[o] ? 8 : 9 + o)); break; }
      }
      st[i] = s;
    }
    return st;
  }
  function styleColor(S, s) {
    if (s === 0) return C.unborn;
    if (s === 1) return C.alive;
    if (s === 8) return rgba(C.fusion, C.hollow);
    if (s === 12) return C.horspq;
    if (s >= 9) return rgba(objColor(s - 9), C.hollow);
    if (s >= 5) return objColor(s - 5);
    return objColor(s - 2);
  }
  const isObject = (s) => s >= 2 && s <= 7;
  const hasRim = (s) => s >= 5 && s <= 7;
  const isHollow = (s) => s >= 8;  // point de fond pris par une branche : carré creux
  // Un point : plein (objet, autre cluster, non né) ou creux (fond pris par une branche).
  // Trait fin et taille à peine au-dessus d'un point plein : une façade entière prise par une
  // branche reste une trame légère, pas une nappe qui écrase les objets.
  function dot(ctx, s, x, y, d, small) {
    if (isHollow(s)) {
      const e = Math.max(d, small ? 3.0 : 3.4);  // en dessous, le creux se lirait plein
      ctx.strokeStyle = styleColor(null, s); ctx.lineWidth = small ? 0.9 : 1.1;
      ctx.strokeRect(x - e / 2, y - e / 2, e, e);
    } else {
      ctx.fillStyle = styleColor(null, s);
      ctx.fillRect(x - d / 2, y - d / 2, d, d);
    }
  }
  // Anneau d'un point fusionné, en deux couches dessinées avant tous les cœurs :
  // l'anneau rouge (ring) puis le vide couleur panneau (gap) qui le sépare du cœur.
  function rimLayer(ctx, pts, pad) {
    for (const [x, y, d] of pts) ctx.fillRect(x - (d + pad) / 2, y - (d + pad) / 2, d + pad, d + pad);
  }

  // --------------------------------------------------------------- dessin
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
  function wrap(ctx, str, width, size, bold) {
    const words = typo(str).split(' '), out = [];
    ctx.font = `${bold ? 'bold ' : ''}${size}px ${FONT}`;
    let cur = '';
    for (const w of words) {
      const cand = cur ? cur + ' ' + w : w;
      if (ctx.measureText(cand).width > width && cur) { out.push(cur); cur = w; } else cur = cand;
    }
    if (cur) out.push(cur);
    return out;
  }
  function titlePlate(ctx, str, x, y) {
    const w = measure(ctx, str, 17) + 16;
    ctx.fillStyle = C.plate; ctx.fillRect(x - 8, y - 20, w, 28);
    text(ctx, str, x, y, 17, C.dim);
  }
  function panel(ctx, P) {
    ctx.fillStyle = C.panel;
    ctx.fillRect(P.x, P.y, P.w, P.h);
    ctx.strokeStyle = C.frame; ctx.lineWidth = 1;
    ctx.strokeRect(P.x + 0.5, P.y + 0.5, P.w - 1, P.h - 1);
  }
  function boxEdges(b) {
    const [x0, y0, z0, x1, y1, z1] = b;
    const v = [[x0, y0, z0], [x1, y0, z0], [x1, y1, z0], [x0, y1, z0], [x0, y0, z1], [x1, y0, z1], [x1, y1, z1], [x0, y1, z1]];
    const e = [[0, 1], [1, 2], [2, 3], [3, 0], [4, 5], [5, 6], [6, 7], [7, 4], [0, 4], [1, 5], [2, 6], [3, 7]];
    return { v, e };
  }
  // Trace un chemin avec un motif de tirets alternant plusieurs couleurs.
  function strokeMulti(ctx, pathFn, colors, width, dash) {
    ctx.lineWidth = width;
    if (colors.length === 1) {
      ctx.setLineDash(dash ? [dash, dash * 0.8] : []); ctx.strokeStyle = colors[0]; pathFn(); ctx.stroke();
      ctx.setLineDash([]); return;
    }
    const seg = dash || 14;
    for (let q = 0; q < colors.length; q++) {
      // un vide de 5 px après chaque tiret : la boîte reste pointillée même si ses couleurs se confondent
      ctx.setLineDash([seg - 5, seg * (colors.length - 1) + 5]);
      ctx.lineDashOffset = -q * seg;
      ctx.strokeStyle = colors[q];
      pathFn(); ctx.stroke();
    }
    ctx.setLineDash([]); ctx.lineDashOffset = 0;
  }
  function labelTag(ctx, str, x, y, color, size, placed, P) {
    const w = measure(ctx, str, size, true) + 16;
    x = clamp(x, P.x + w / 2 + 6, P.x + P.w - w / 2 - 6);
    y = clamp(y, P.y + size + 40, P.y + P.h - 50);
    const hit = (yy) => placed.some((p) => Math.abs(p[0] - x) < (p[2] + w) / 2 + 6 && Math.abs(p[1] - yy) < size + 14);
    let guard = 0;
    while (hit(y) && guard++ < 12) y = y - (size + 16) < P.y + size + 40 ? y + 3 * (size + 16) : y - (size + 16);
    placed.push([x, y, w]);
    ctx.fillStyle = C.plate;
    ctx.fillRect(x - w / 2, y - size - 4, w, size + 11);
    ctx.strokeStyle = color; ctx.lineWidth = 1.5; ctx.setLineDash([]);
    ctx.strokeRect(x - w / 2 + 0.5, y - size - 3.5, w - 1, size + 10);
    text(ctx, str, x, y + 1, size, color, { bold: true, align: 'center' });
  }

  function drawGrid3D(ctx, S, cam) {
    const g = S.scene.grid;
    ctx.strokeStyle = C.grid; ctx.lineWidth = 1;
    ctx.beginPath();
    for (let x = g.x0; x <= g.x1 + 1e-6; x += g.step) {
      const a = project(cam, x, g.y0, g.z), b = project(cam, x, g.y1, g.z);
      if (a[2] > 0.1 && b[2] > 0.1) { ctx.moveTo(a[0], a[1]); ctx.lineTo(b[0], b[1]); }
    }
    for (let y = g.y0; y <= g.y1 + 1e-6; y += g.step) {
      const a = project(cam, g.x0, y, g.z), b = project(cam, g.x1, y, g.z);
      if (a[2] > 0.1 && b[2] > 0.1) { ctx.moveTo(a[0], a[1]); ctx.lineTo(b[0], b[1]); }
    }
    ctx.stroke();
  }

  function draw3D(ctx, S, t, phase, styles, boxes) {
    const placed = [];
    panel(ctx, PANEL3D);
    ctx.save();
    ctx.beginPath(); ctx.rect(PANEL3D.x + 1, PANEL3D.y + 1, PANEL3D.w - 2, PANEL3D.h - 2); ctx.clip();
    const cam = camera(S, t);
    drawGrid3D(ctx, S, cam);
    const n = S.n, sx = new Float32Array(n), sy = new Float32Array(n), sz = new Float32Array(n);
    const order = new Uint32Array(n);
    for (let i = 0; i < n; i++) {
      const p = project(cam, S.x[i], S.y[i], S.z[i]);
      sx[i] = p[0]; sy[i] = p[1]; sz[i] = p[2]; order[i] = i;
    }
    order.sort((a, b) => sz[b] - sz[a]);
    const ref = S.scene.view.distance, base = S.scene.view.point_size;
    const size = (i, s) => clamp(base * (isObject(s) ? 1.3 : (s === 1 ? 0.75 : (s >= 8 ? 0.9 : 0.6))) * ref / sz[i], 1.2, 8);
    // trois passes : anneaux rouges, vides, puis tous les cœurs ; en une seule passe,
    // l'anneau d'un point recouvrirait le cœur de son voisin et un objet fusionné dense virerait au rouge uni
    const rims = [];
    for (let q = 0; q < n; q++) {
      const i = order[q], s = styles[i];
      if (sz[i] > 0.2 && hasRim(s)) rims.push([sx[i], sy[i], size(i, s)]);
    }
    ctx.fillStyle = C.fusion; rimLayer(ctx, rims, 6);
    ctx.fillStyle = C.panel; rimLayer(ctx, rims, 2);
    for (let q = 0; q < n; q++) {
      const i = order[q];
      if (sz[i] <= 0.2) continue;
      const s = styles[i];
      dot(ctx, s, sx[i], sy[i], size(i, s));
    }
    for (const bx of boxes) {
      if (bx.hidden) continue;
      const { v, e } = boxEdges(bx.box);
      const pv = v.map((p) => project(cam, p[0], p[1], p[2]));
      if (pv.some((p) => p[2] <= 0.2)) continue;
      const path = () => { ctx.beginPath(); for (const [a, b] of e) { ctx.moveTo(pv[a][0], pv[a][1]); ctx.lineTo(pv[b][0], pv[b][1]); } };
      ctx.globalAlpha = bx.alpha == null ? 1 : bx.alpha;
      strokeMulti(ctx, path, bx.colors, bx.width, bx.dash);
      ctx.globalAlpha = 1;
    }
    for (const bx of boxes) {
      if (!bx.label) continue;
      const pv = boxEdges(bx.anchor || bx.box).v.map((p) => project(cam, p[0], p[1], p[2]));
      if (pv.some((p) => p[2] <= 0.2)) continue;
      const b = bx.anchor || bx.box, c = project(cam, (b[0] + b[3]) / 2, (b[1] + b[4]) / 2, b[5]);
      const top = Math.min(...boxEdges(b).v.map((p) => project(cam, p[0], p[1], p[2])[1]));
      labelTag(ctx, bx.label, c[0], top - 10, bx.labelColor || bx.colors[0], bx.labelSize || 20, placed, PANEL3D);
    }
    ctx.restore();
    titlePlate(ctx, phase === 'intro' ? 'Vue 3D — vérité terrain' : `Vue 3D — clusters de la hiérarchie au niveau ${S.scene.meta.level}`,
      PANEL3D.x + 14, PANEL3D.y + 28);
  }

  function drawBEV(ctx, S, styles, boxes, phase) {
    panel(ctx, BEV);
    const b = S.scene.bev;
    const sc = Math.min((BEV.w - 30) / (b.x1 - b.x0), (BEV.h - 60) / (b.y1 - b.y0));
    const ox = BEV.x + BEV.w / 2 - sc * (b.x0 + b.x1) / 2;
    const oy = BEV.y + 26 + (BEV.h - 26) / 2 + sc * (b.y0 + b.y1) / 2;
    const P = (x, y) => [ox + sc * x, oy - sc * y];
    ctx.save();
    ctx.beginPath(); ctx.rect(BEV.x + 1, BEV.y + 1, BEV.w - 2, BEV.h - 2); ctx.clip();
    ctx.strokeStyle = C.grid; ctx.lineWidth = 1; ctx.beginPath();
    const g = S.scene.grid;
    for (let x = g.x0; x <= g.x1 + 1e-6; x += g.step) { const a = P(x, g.y0), c = P(x, g.y1); ctx.moveTo(a[0], a[1]); ctx.lineTo(c[0], c[1]); }
    for (let y = g.y0; y <= g.y1 + 1e-6; y += g.step) { const a = P(g.x0, y), c = P(g.x1, y); ctx.moveTo(a[0], a[1]); ctx.lineTo(c[0], c[1]); }
    ctx.stroke();
    const idx = S.zOrder || (S.zOrder = Array.from({ length: S.n }, (_, i) => i).sort((a, c) => S.z[a] - S.z[c]));
    const d = S.scene.view.bev_point_size;
    const rims = [];  // anneaux d'abord, comme en 3D
    for (const i of idx) if (hasRim(styles[i])) { const q = P(S.x[i], S.y[i]); rims.push([q[0], q[1], d * 1.3]); }
    ctx.fillStyle = C.fusion; rimLayer(ctx, rims, 5);
    ctx.fillStyle = C.panel; rimLayer(ctx, rims, 2);
    for (const i of idx) {
      const s = styles[i];
      const q = P(S.x[i], S.y[i]);
      dot(ctx, s, q[0], q[1], isObject(s) ? d * 1.3 : d * 0.75, true);
    }
    for (const bx of boxes) {
      if (bx.hidden) continue;
      const a = P(bx.box[0], bx.box[1]), c = P(bx.box[3], bx.box[4]);
      const path = () => { ctx.beginPath(); ctx.rect(a[0], c[1], c[0] - a[0], a[1] - c[1]); };
      ctx.globalAlpha = bx.alpha == null ? 1 : bx.alpha;
      strokeMulti(ctx, path, bx.colors, Math.max(1.5, bx.width * 0.7), bx.dash ? Math.max(5, bx.dash * 0.6) : 0);
      ctx.globalAlpha = 1;
    }
    const sen = S.scene.sensor_xy;
    if (sen) {
      const q = P(sen[0], sen[1]);
      ctx.fillStyle = C.text; ctx.beginPath(); ctx.arc(q[0], q[1], 5, 0, 7); ctx.fill();
    }
    ctx.restore();
    // échelle 5 m sur plaque
    const meters = [10, 5, 2, 1].find((m) => m * sc <= 0.3 * BEV.w) || 1;
    const L = meters * sc, x1 = BEV.x + BEV.w - 22, y1 = BEV.y + BEV.h - 16;
    ctx.fillStyle = C.plate; ctx.fillRect(x1 - L - 10, y1 - 26, L + 20, 34);
    ctx.strokeStyle = C.dim; ctx.lineWidth = 2; ctx.beginPath(); ctx.moveTo(x1 - L, y1); ctx.lineTo(x1, y1); ctx.stroke();
    text(ctx, `${meters} m`, x1 - L / 2, y1 - 8, 15, C.dim, { align: 'center' });
    titlePlate(ctx, phase === 'intro' ? 'Vue de dessus — vérité terrain' : 'Vue de dessus', BEV.x + 14, BEV.y + 28);
    if (!sen) {
      const w = measure(ctx, `capteur ${S.scene.sensor_note}, hors cadre (en bas)`, 15) + 14;
      ctx.fillStyle = C.plate; ctx.fillRect(BEV.x + 8, BEV.y + BEV.h - 32, w, 24);
      text(ctx, `capteur ${S.scene.sensor_note}, hors cadre (en bas)`, BEV.x + 15, BEV.y + BEV.h - 14, 15, C.dim);
    }
  }

  // Dendrogramme des trois branches suivies : axe horizontal = log du niveau.
  function drawDendro(ctx, S, r, phase, cur) {
    panel(ctx, DENDRO);
    const sc = S.scene, sym = sc.meta.level;
    const rShow = phase === 'intro' ? 0 : (phase === 'summary' ? Infinity : r * EPS);
    const lx0 = DENDRO.x + 64, lx1 = DENDRO.x + DENDRO.w - 26;
    const lr0 = Math.log(sc.levels.rmin), lr1 = Math.log(sc.levels.rmax);
    const X = (v) => lx0 + (lx1 - lx0) * (Math.log(v) - lr0) / (lr1 - lr0);
    const rowY = (o) => DENDRO.y + 104 + o * 58;
    const axisY = DENDRO.y + DENDRO.h - 76;
    ctx.strokeStyle = C.frame; ctx.lineWidth = 1; ctx.beginPath(); ctx.moveTo(lx0, axisY); ctx.lineTo(lx1, axisY); ctx.stroke();
    let lastTick = -1e9;
    const unitEnd = DENDRO.x + 16 + measure(ctx, `${sym} (m)`, 17) + 8;
    for (const v of sc.levels.ticks) {
      const xx = X(v);
      if (xx - lastTick < 56 || xx - measure(ctx, frNum(v, 2), 17) / 2 < unitEnd) continue;
      lastTick = xx;
      ctx.beginPath(); ctx.moveTo(xx, axisY); ctx.lineTo(xx, axisY + 6); ctx.stroke();
      text(ctx, frNum(v, 2), xx, axisY + 25, 17, C.dim, { align: 'center' });
    }
    text(ctx, `${sym} (m)`, DENDRO.x + 16, axisY + 25, 17, C.dim);
    const nobj = sc.objects.length;
    const diamonds = [];
    for (let o = 0; o < nobj; o++) {
      const ob = sc.objects[o], y = rowY(o), stop = Math.min(S.rows.end[o], rShow, sc.levels.rmax);
      text(ctx, ob.key, DENDRO.x + 20, y + 8, 22, objColor(o), { bold: true });
      for (const sg of sc.tracks[o].segments) {
        const a = Math.max(sg[0], sc.levels.rmin), b = Math.min(sg[1], stop);
        if (b <= a) continue;
        if (sg[2] === 1) { ctx.setLineDash([7, 6]); ctx.strokeStyle = objColor(o); ctx.lineWidth = 3; }
        else if (sg[2] === MATCHED) { ctx.setLineDash([]); ctx.strokeStyle = objColor(o); ctx.lineWidth = 10; }
        else if (sg[2] === FUSED) {  // double trait : l'état se lit sans la couleur
          ctx.setLineDash([]); ctx.strokeStyle = C.fusion; ctx.lineWidth = 2.5;
          ctx.beginPath(); ctx.moveTo(X(a), y - 3.5); ctx.lineTo(X(b), y - 3.5); ctx.moveTo(X(a), y + 3.5); ctx.lineTo(X(b), y + 3.5); ctx.stroke();
          continue;
        } else continue;
        ctx.beginPath(); ctx.moveTo(X(a), y); ctx.lineTo(X(b), y); ctx.stroke(); ctx.setLineDash([]);
      }
      const bst = sc.tracks[o].best;
      let row = o;  // la ligne qui porte le meilleur nœud : on suit les raccords après la fin de la ligne o
      if (bst) {
        for (let guard = 0; guard < 4 && bst[0] > S.rows.end[row]; guard++) {
          const l = S.rows.links.find((q) => q.from === row && q.r === S.rows.end[row]);
          if (!l) break;
          row = l.to;
        }
      }
      if (bst && bst[0] <= rShow && bst[0] >= sc.levels.rmin && bst[0] <= sc.levels.rmax) diamonds.push([X(bst[0]), rowY(row), objColor(o)]);
    }
    for (const l of S.rows.links) {
      if (l.r > rShow || l.r < sc.levels.rmin || l.r > sc.levels.rmax) continue;
      const xx = X(l.r);
      ctx.strokeStyle = C.fusion; ctx.lineWidth = 3;
      ctx.beginPath(); ctx.moveTo(xx, rowY(l.from)); ctx.lineTo(xx, rowY(l.to)); ctx.stroke();
      ctx.fillStyle = C.fusion;
      for (const yy of [rowY(l.from), rowY(l.to)]) { ctx.beginPath(); ctx.arc(xx, yy, 5, 0, 7); ctx.fill(); }
    }
    // losanges des meilleurs nœuds, au-dessus des raccords ; décalés s'ils se superposent
    const placedD = [];
    for (const [x0, y, col] of diamonds) {
      let xx = x0;
      while (placedD.some((q) => Math.abs(q[0] - xx) < 12 && q[1] === y)) xx += 12;
      placedD.push([xx, y]);
      ctx.fillStyle = col; ctx.strokeStyle = C.text; ctx.lineWidth = 1.5;
      ctx.beginPath(); ctx.moveTo(xx, y - 9); ctx.lineTo(xx + 7, y); ctx.lineTo(xx, y + 9); ctx.lineTo(xx - 7, y); ctx.closePath();
      ctx.fill(); ctx.stroke();
    }
    if (phase === 'sweep') {
      const xx = X(clamp(r, sc.levels.rmin, sc.levels.rmax));
      ctx.strokeStyle = C.text; ctx.lineWidth = 1.5; ctx.setLineDash([4, 4]);
      ctx.beginPath(); ctx.moveTo(xx, DENDRO.y + 76); ctx.lineTo(xx, axisY); ctx.stroke(); ctx.setLineDash([]);
      const exact = (sc.level_labels || []).find((q) => q[0] === r);
      const lab = `${sym} = ${exact ? exact[1] : frNum(r, r < 1 ? 3 : 2)} m`;
      const w = measure(ctx, lab, 17, true) + 16, lxx = clamp(xx, lx0 + w / 2, lx1 - w / 2 + 20);
      ctx.fillStyle = C.plate; ctx.fillRect(lxx - w / 2, DENDRO.y + 46, w, 28);
      text(ctx, lab, lxx, DENDRO.y + 67, 17, C.text, { align: 'center', bold: true });
      for (let o = 0; o < nobj; o++) {
        const e = cur[o];
        if (!e || S.rows.end[o] <= r * EPS) continue;
        const st = e[EV.state];
        if (st === FUSED) {  // ✗ rouge cerné de la couleur du texte
          const y0 = rowY(o), k9 = 8;
          const cross = () => { ctx.beginPath(); ctx.moveTo(xx - k9, y0 - k9); ctx.lineTo(xx + k9, y0 + k9); ctx.moveTo(xx + k9, y0 - k9); ctx.lineTo(xx - k9, y0 + k9); };
          ctx.lineCap = 'round';
          ctx.strokeStyle = C.text; ctx.lineWidth = 7; cross(); ctx.stroke();
          ctx.strokeStyle = C.fusion; ctx.lineWidth = 3.5; cross(); ctx.stroke();
          ctx.lineCap = 'butt';
          continue;
        }
        ctx.beginPath(); ctx.arc(xx, rowY(o), 9, 0, 7);
        if (st === MATCHED) { ctx.fillStyle = objColor(o); ctx.fill(); ctx.strokeStyle = C.text; ctx.lineWidth = 2.5; ctx.stroke(); }
        else { ctx.fillStyle = C.panel; ctx.fill(); ctx.strokeStyle = objColor(o); ctx.lineWidth = 3; ctx.stroke(); }
      }
    }
    (sc.levels.marks || []).forEach((mk, q) => {
      const xx = X(mk.v);
      ctx.strokeStyle = C.dim; ctx.lineWidth = 1.5; ctx.setLineDash([4, 4]);
      ctx.beginPath(); ctx.moveTo(xx, DENDRO.y + 80); ctx.lineTo(xx, axisY); ctx.stroke(); ctx.setLineDash([]);
      const w = measure(ctx, mk.label, 15) + 8, yy = axisY - 8 - (q % 2) * 22;
      ctx.fillStyle = C.plate; ctx.fillRect(clamp(xx, lx0 + w / 2, lx1 - w / 2 + 20) - w / 2, yy - 16, w, 21);
      text(ctx, mk.label, clamp(xx, lx0 + w / 2, lx1 - w / 2 + 20), yy, 15, C.dim, { align: 'center' });
    });
    titlePlate(ctx, 'Hiérarchie restreinte aux trois branches suivies', DENDRO.x + 14, DENDRO.y + 28);
    // légende
    const ly = DENDRO.y + DENDRO.h - 16;
    let lx = DENDRO.x + 16;
    const item = (draw, label) => {
      draw(lx, ly - 5);
      text(ctx, label, lx + 40, ly, 16, C.dim);
      lx += 40 + measure(ctx, label, 16) + 16;
    };
    const seg = (w, col, dash) => (x, y) => {
      ctx.lineWidth = w; ctx.strokeStyle = col; ctx.setLineDash(dash || []);
      ctx.beginPath(); ctx.moveTo(x, y); ctx.lineTo(x + 32, y); ctx.stroke(); ctx.setLineDash([]);
    };
    item(seg(3, C.dim, [6, 5]), 'fragmenté');
    item(seg(10, C.dim), 'apparié');
    item((x, y) => {
      ctx.lineWidth = 2.5; ctx.strokeStyle = C.fusion; ctx.beginPath();
      ctx.moveTo(x, y - 3.5); ctx.lineTo(x + 32, y - 3.5); ctx.moveTo(x, y + 3.5); ctx.lineTo(x + 32, y + 3.5); ctx.stroke();
    }, 'fusionné');
    item((x, y) => {
      ctx.fillStyle = C.dim; ctx.beginPath(); ctx.moveTo(x + 16, y - 8); ctx.lineTo(x + 23, y); ctx.lineTo(x + 16, y + 8); ctx.lineTo(x + 9, y);
      ctx.closePath(); ctx.fill();
    }, 'meilleur nœud');
  }

  function drawHeader(ctx, S) {
    const m = S.scene.meta;
    const room = W - 72 - Math.max(measure(ctx, m.badge, 18), 0);
    let size = 34;
    while (size > 24 && measure(ctx, m.title, size, true) > room) size -= 1;
    text(ctx, m.title, 24, 50, size, C.text, { bold: true });
    text(ctx, m.subtitle, 24, 88, 20, C.dim);
    text(ctx, m.badge, W - 24, 50, 18, C.dim, { align: 'right' });
    text(ctx, m.badge2, W - 24, 88, 18, C.dim, { align: 'right' });
  }

  function drawFooter(ctx, S, t, phase) {
    ctx.fillStyle = C.panel; ctx.fillRect(FOOT.x, FOOT.y, FOOT.w, FOOT.h);
    ctx.strokeStyle = C.frame; ctx.lineWidth = 1; ctx.strokeRect(FOOT.x + 0.5, FOOT.y + 0.5, FOOT.w - 1, FOOT.h - 1);
    if (phase === 'summary') {
      const sm = S.scene.summary;
      const a = smooth((t - S.scene.timing.summary) / 0.6);
      ctx.globalAlpha = a;
      text(ctx, 'Conclusion', FOOT.x + 22, FOOT.y + 44, 22, C.dim, { bold: true });
      wrap(ctx, sm.conclusion, FOOT.w - 44, 28, true).slice(0, 2)
        .forEach((ln, q) => text(ctx, ln, FOOT.x + 22, FOOT.y + 88 + q * 38, 28, C.text, { bold: true }));
      ctx.globalAlpha = 1;
      return;
    }
    let cap = null;
    for (const c of S.scene.captions) if (t >= c.t0 && t < c.t1) cap = c;
    if (!cap) return;
    const a = Math.min(smooth((t - cap.t0) / 0.3), smooth((cap.t1 - t) / 0.3));
    ctx.globalAlpha = a;
    ctx.fillStyle = roleColor(cap.color); ctx.fillRect(FOOT.x + 1, FOOT.y + 1, 6, FOOT.h - 2);
    const [first, ...rest] = cap.text.split('\n');
    const l1 = wrap(ctx, first, FOOT.w - 44, 28, true);
    l1.forEach((ln, q) => text(ctx, ln, FOOT.x + 22, FOOT.y + 46 + q * 36, 28, roleColor(cap.color), { bold: true }));
    let yy = FOOT.y + 46 + l1.length * 36 + 2;
    for (const para of rest) {
      for (const ln of wrap(ctx, para, FOOT.w - 44, 23)) { if (yy < FOOT.y + FOOT.h - 6) text(ctx, ln, FOOT.x + 22, yy, 23, C.text); yy += 31; }
    }
    ctx.globalAlpha = 1;
  }

  function legend3D(ctx, S, phase) {
    const y = PANEL3D.y + PANEL3D.h - 18;
    ctx.fillStyle = C.plate;
    ctx.fillRect(PANEL3D.x + 1, y - 26, PANEL3D.w - 2, 43);
    const sw = (col) => (x) => { ctx.fillStyle = col; ctx.fillRect(x, y - 12, 14, 14); };
    const ring = (x) => {  // même dessin qu'un point fusionné : anneau rouge, vide, cœur
      ctx.fillStyle = C.fusion; ctx.fillRect(x - 3, y - 15, 20, 20);
      ctx.fillStyle = C.panel; ctx.fillRect(x - 1, y - 13, 16, 16);
      ctx.fillStyle = C.dim; ctx.fillRect(x + 2, y - 10, 10, 10);
    };
    const hollow = (x) => {  // fond pris par une branche : carré creux, rouge si la branche est fusionnée
      ctx.lineWidth = 1.4;
      ctx.strokeStyle = rgba(objColor(0), C.hollow); ctx.strokeRect(x - 1, y - 8, 8, 8);
      ctx.strokeStyle = rgba(C.fusion, C.hollow); ctx.strokeRect(x + 8, y - 8, 8, 8);
    };
    const items = phase === 'intro'
      ? S.scene.objects.map((ob, o) => [sw(objColor(o)), `${ob.key} · ${ob.name}`]).concat([[sw(C.alive), 'autres points']])
      : S.scene.objects.map((ob, o) => [sw(objColor(o)), `branche ${ob.key}`])
        .concat([[ring, 'objet fusionné (cerné)'], [hollow, 'fond pris (creux)'],
          [(x) => { ctx.lineWidth = 1.4; ctx.strokeStyle = C.horspq; ctx.strokeRect(x + 2, y - 10, 10, 10); }, 'void, hors PQ'],
          [sw(C.alive), 'autre cluster']])
        .concat(S.scene.meta.method === 'hdbscan' && S.scene.meta.K > 1
          ? [[(x) => { ctx.fillStyle = C.unborn; ctx.fillRect(x + 3, y - 9, 8, 8); }, 'pas encore né']] : []);
    let xx = PANEL3D.x + 16;
    for (const [draw, lab] of items) {
      draw(xx);
      text(ctx, lab, xx + 22, y, 16, C.dim);
      xx += 22 + measure(ctx, lab, 16) + 20;
    }
  }

  function drawSummary(ctx, S, t) {
    const sc = S.scene, sm = sc.summary, ch = sm.chart;
    const u = smooth((t - sc.timing.summary) / 0.6);
    const P = { x: 24, y: 112, w: 1872, h: 780 };
    ctx.fillStyle = C.summary; ctx.fillRect(P.x, P.y, P.w, P.h);
    ctx.globalAlpha = u;
    ctx.strokeStyle = C.frame; ctx.lineWidth = 1.5; ctx.strokeRect(P.x + 0.5, P.y + 0.5, P.w - 1, P.h - 1);
    text(ctx, sm.title, P.x + 40, P.y + 62, 32, C.text, { bold: true });
    let yy = P.y + 132;
    sm.lines.forEach((ln, q) => {
      const alert = sm.alert && sm.alert.includes(q);
      const m = /^([A-Z]) · ([^:]+?) :/.exec(ln);
      let first = true;
      for (const piece of wrap(ctx, ln, 1000, 24)) {
        if (first && m) {
          const oi = sc.objects.findIndex((o) => o.key === m[1]);
          const pre = `${m[1]} · ${m[2]}`;
          text(ctx, pre, P.x + 40, yy, 24, oi >= 0 ? objColor(oi) : C.text, { bold: true });
          text(ctx, piece.slice(typo(pre).length), P.x + 40 + measure(ctx, pre, 24, true), yy, 24, alert ? C.fusion : C.text);
        } else text(ctx, piece, P.x + 40, yy, 24, alert ? C.fusion : C.text);
        first = false;
        yy += 34;
      }
      yy += 12;
    });
    // meilleur IoU de chaque hiérarchie : HDBSCAN K = 1..10, puis ALPINE
    const G = { x: P.x + 1110, y: P.y + 150, w: 690, h: 500 };
    ctx.strokeStyle = C.frame; ctx.lineWidth = 1; ctx.strokeRect(G.x, G.y, G.w, G.h);
    text(ctx, 'Meilleur IoU d\'un nœud de chaque hiérarchie', G.x, G.y - 16, 20, C.text, { bold: true });
    const nx = ch.x_labels.length, gap = ch.gap_before_last ? 1.2 : 0;
    const gx = (i) => G.x + 62 + (G.w - 100) * (i + (i === nx - 1 ? gap : 0)) / (nx - 1 + gap);
    const gy = (v) => G.y + G.h - 56 - (G.h - 96) * v;
    ctx.beginPath();
    for (const v of [0, 0.25, 0.5, 0.75, 1]) { ctx.moveTo(G.x + 56, gy(v)); ctx.lineTo(G.x + G.w - 18, gy(v)); }
    ctx.strokeStyle = C.grid; ctx.stroke();
    for (const v of [0, 0.5, 1]) text(ctx, frNum(v, 1), G.x + 48, gy(v) + 6, 17, C.dim, { align: 'right' });
    ch.x_labels.forEach((lab, i) => text(ctx, lab, gx(i), G.y + G.h - 26, 17, i === ch.highlight ? C.text : C.dim, { align: 'center', bold: i === ch.highlight }));
    text(ctx, ch.x_title, G.x + G.w / 2, G.y + G.h + 30, 17, C.dim, { align: 'center' });
    ctx.setLineDash([6, 6]); ctx.strokeStyle = C.text; ctx.lineWidth = 1.5; ctx.beginPath();
    ctx.moveTo(G.x + 56, gy(ch.iou_ok)); ctx.lineTo(G.x + G.w - 18, gy(ch.iou_ok)); ctx.stroke(); ctx.setLineDash([]);
    const hx = gx(ch.highlight);
    ctx.fillStyle = C.band; ctx.fillRect(hx - 18, G.y + 8, 36, G.h - 52);
    text(ctx, 'cette vidéo', Math.min(hx, G.x + G.w - 50), G.y + G.h - 3, 16, C.text, { align: 'center' });
    ch.series.forEach((serie, o) => {
      const col = objColor(o);
      ctx.strokeStyle = col; ctx.lineWidth = 3.5; ctx.beginPath();
      for (let i = 0; i < nx - (gap ? 1 : 0); i++) { const px = gx(i), py = gy(serie[i]); if (i) ctx.lineTo(px, py); else ctx.moveTo(px, py); }
      ctx.stroke();
      serie.forEach((v, i) => {
        const last = gap > 0 && i === nx - 1, px = gx(i) + (last ? (o - 1) * 14 : 0);
        marker(ctx, o, px, gy(v), last ? 7.5 : 5.5, col);
      });
    });
    // lettre de chaque courbe au bout de la série HDBSCAN, écartée si deux courbes finissent ensemble
    const nK = nx - (gap ? 1 : 0), ends = ch.series.map((serie, o) => ({ o, y: gy(serie[nK - 1]) })).sort((a, b) => a.y - b.y);
    for (let q = 1; q < ends.length; q++) ends[q].y = Math.max(ends[q].y, ends[q - 1].y + 19);
    // 19 px gras : « grand texte » au sens WCAG, 3:1 suffit (B clair #c27a00 : 3,3:1)
    for (const e of ends) text(ctx, sc.objects[e.o].key, gx(nK - 1) + 13, e.y + 7, 19, objColor(e.o), { bold: true });
    let lg = G.x;  // légende sous le graphique, hors de la zone des courbes ; texte en couleur du texte
    sc.objects.forEach((ob, o) => {
      const lab = `${ob.key} · ${ob.name}`;
      marker(ctx, o, lg + 8, G.y + G.h + 56, 7, objColor(o));
      text(ctx, lab, lg + 24, G.y + G.h + 62, 17, C.text, { bold: true });
      lg += 24 + measure(ctx, lab, 17, true) + 24;
    });
    // le seuil est légendé sous le graphique : dans la zone des courbes, il
    // serait masqué par les points proches de 0,5
    const sy = G.y + G.h + 94;
    ctx.setLineDash([6, 6]); ctx.strokeStyle = C.text; ctx.lineWidth = 1.5; ctx.beginPath();
    ctx.moveTo(G.x, sy - 6); ctx.lineTo(G.x + 40, sy - 6); ctx.stroke(); ctx.setLineDash([]);
    text(ctx, 'seuil d\'appariement de la PQ : IoU > 0,5', G.x + 52, sy, 17, C.text);
    ctx.globalAlpha = 1;
  }

  // marqueur propre à chaque objet (disque A, carré B, triangle C) : les séries se lisent sans la couleur
  function marker(ctx, o, x, y, r, col) {
    ctx.fillStyle = col; ctx.beginPath();
    if (o === 1) ctx.rect(x - r * 0.9, y - r * 0.9, r * 1.8, r * 1.8);
    else if (o === 2) { ctx.moveTo(x, y - r * 1.15); ctx.lineTo(x + r, y + r * 0.75); ctx.lineTo(x - r, y + r * 0.75); ctx.closePath(); }
    else ctx.arc(x, y, r, 0, 7);
    ctx.fill();
  }

  function renderAt(ctx, S, t) {
    const sc = S.scene;
    const phase = t < sc.timing.intro ? 'intro' : (t >= sc.timing.summary ? 'summary' : 'sweep');
    const r = phase === 'intro' ? sc.levels.rmin : rAt(S, t);
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.fillStyle = C.bg; ctx.fillRect(0, 0, W, H);
    drawHeader(ctx, S);
    const cur = sc.tracks.map((tr) => trackState(tr, r));
    const fused = cur.map((e) => !!e && e[EV.state] === FUSED);
    const styles = pointStyles(S, r, phase, fused);
    const boxes = sc.objects.map((ob, o) => ({
      box: ob.gt_box, colors: [objColor(o)], width: phase === 'intro' ? 2.5 : 2, dash: 7,
      alpha: phase === 'intro' ? 1 : 0.9, label: phase === 'intro' ? `${ob.key} · ${ob.name}` : null,
    }));
    if (phase !== 'intro') {
      const seen = new Set();
      cur.forEach((e, o) => {
        if (!e) return;
        const members = [];
        for (let q = 0; q < sc.objects.length; q++) if (q === o || (e[EV.mask] & (1 << q))) members.push(q);
        const key = members.join('+');
        if (seen.has(key)) return;
        seen.add(key);
        const isFused = e[EV.state] === FUSED || members.length > 1;
        const colors = isFused ? members.map((q) => objColor(q)).concat([C.fusion]) : [objColor(o)];
        const markOf = (q) => { const eq = cur[q]; return !eq ? '' : (eq[EV.state] === MATCHED ? ' ✓' : (eq[EV.state] === FUSED ? ' ✗' : '')); };
        const lab = members.length > 1 ? members.map((q) => sc.objects[q].key + markOf(q)).join(' + ') : sc.objects[o].key + markOf(o);
        const gts = members.map((q) => sc.objects[q].gt_box);
        const anchor = [0, 1, 2].map((j) => Math.min(...gts.map((g) => g[j]))).concat([3, 4, 5].map((j) => Math.max(...gts.map((g) => g[j]))));
        const raw = e.slice(EV.box, EV.box + 6), cl = sc.clip;
        const box = raw.map((v, j) => (j < 3 ? Math.max(v, cl[j]) : Math.min(v, cl[j])));
        const over = raw.some((v, j) => (j < 3 ? v < cl[j] - 1e-6 : v > cl[j] + 1e-6));
        const huge = over && ((box[3] - box[0]) > 0.6 * (cl[3] - cl[0]) || (box[4] - box[1]) > 0.6 * (cl[4] - cl[1]));
        boxes.push({
          box, colors, width: 4, dash: isFused ? 16 : 0, hidden: huge,
          label: lab + (huge ? ' (cluster plus grand que le cadre)' : (over ? ' (déborde)' : '')), anchor,
          labelColor: isFused ? C.fusion : objColor(o),
        });
      });
    }
    draw3D(ctx, S, t, phase, styles, boxes);
    legend3D(ctx, S, phase);
    drawBEV(ctx, S, styles, boxes, phase);
    drawDendro(ctx, S, r, phase, cur);
    drawFooter(ctx, S, t, phase);
    if (phase === 'summary') drawSummary(ctx, S, t);
    return { r, phase };
  }

  window.DemoPlayer = { W, H, prepare, renderAt, rAt, setTheme, themeName, THEMES };
})();
