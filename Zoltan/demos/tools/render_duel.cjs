#!/usr/bin/env node
/*
 * Vidéo « HGP contre HDBSCAN » d'un bout : capture image par image de player/duel.html, puis encodage.
 *
 * Usage : node Zoltan/demos/tools/render_duel.cjs <bout> [--k 5] [--theme clair|sombre] [--fps 30]
 *                                                 [--stills t1,t2,... --out DIR]
 *   <bout> : sous-dossier d'un bout (data/duel_k<k>.js écrit par tools/duel_scene.py) ; sans --k, le seul
 *   data/duel_k*.js présent. Sans --theme, les deux thèmes, comme Percolia.com.
 *   --stills : seulement des images fixes aux instants donnés (secondes), pour relecture.
 *
 * Sorties (un jeu par thème) : <bout>_hgp_hdbscan_k<k>_<thème>.mp4 (H.264 High, yuv420p, 1920 × 1080, 30 i/s,
 * sans son, +faststart), _instant_cle.png (la fusion trop précoce de HDBSCAN) et _bilan.png (dernière image).
 * Le MP4 n'est renommé qu'après un encodage complet. ffmpeg : variable FFMPEG, sinon celui d'imageio-ffmpeg.
 * Playwright : module du dossier courant, de NODE_PATH ou global.
 */
'use strict';
const path = require('path');
const fs = require('fs');
const { execSync, spawn } = require('child_process');

function requirePlaywright() {
  try { return require('playwright'); } catch (e) {
    return require(path.join(execSync('npm root -g').toString().trim(), 'playwright'));
  }
}
function ffmpegPath() {
  if (process.env.FFMPEG) return process.env.FFMPEG;
  return execSync('python3 -c "import imageio_ffmpeg as i; print(i.get_ffmpeg_exe())"').toString().trim();
}
function opt(args, name, dflt) { return args.includes(name) ? args[args.indexOf(name) + 1] : dflt; }

async function main() {
  const args = process.argv.slice(2);
  if (!args.length) throw new Error('usage : render_duel.cjs <bout> [--k 5] [--theme clair|sombre] [--fps 30] [--stills t,... --out DIR]');
  const bout = path.resolve(args[0]);
  let k = opt(args, '--k', null);
  if (k == null) {
    const found = fs.readdirSync(path.join(bout, 'data')).filter((f) => /^duel_k\d+\.js$/.test(f));
    if (found.length !== 1) throw new Error('préciser --k : ' + found.join(', '));
    k = found[0].match(/\d+/)[0];
  }
  const fps = Number(opt(args, '--fps', 30));
  const only = opt(args, '--theme', null);
  if (only && !['clair', 'sombre'].includes(only)) throw new Error('--theme : clair ou sombre');
  const themes = only ? [only] : ['sombre', 'clair'];
  const root = path.resolve(__dirname, '..');
  const sceneFile = path.join(bout, 'data', `duel_k${k}.js`);
  if (!fs.existsSync(sceneFile)) throw new Error('scène absente : lancer tools/duel_scene.py');
  const sceneRel = path.relative(path.join(root, 'player'), sceneFile).split(path.sep).join('/');
  const stills = opt(args, '--stills', null);
  const { chromium } = requirePlaywright();
  const browser = await chromium.launch({ args: ['--allow-file-access-from-files'] });
  try {
    for (const theme of themes) {
      await renderTheme(browser, { bout, k, fps, theme, root, sceneRel, stills, out: opt(args, '--out', null) });
    }
  } finally {
    await browser.close();
  }
}

async function renderTheme(browser, o) {
  const url = 'file://' + path.join(o.root, 'player', 'duel.html') + `?capture=1&theme=${o.theme}&scene=${encodeURIComponent(o.sceneRel)}`;
  const page = await browser.newPage({ viewport: { width: 1920, height: 1080 }, deviceScaleFactor: 1 });
  const errors = [];
  page.on('pageerror', (e) => errors.push(e.message));
  const check = () => { if (errors.length) throw new Error(`erreur dans la page (${o.theme}) : ${errors.join(' | ')}`); };
  await page.goto(url);
  await page.waitForFunction(() => window.sceneReady === true || window.sceneError, null, { timeout: 60000 });
  check();
  if (await page.evaluate(() => !!window.sceneError)) {
    throw new Error(`scène refusée (${o.theme}) : ${await page.evaluate(() => document.getElementById('stage').textContent.trim())}`);
  }
  const applied = await page.evaluate(() => document.documentElement.getAttribute('data-theme'));
  if (applied !== (o.theme === 'clair' ? 'light' : 'dark')) throw new Error(`thème ${o.theme} non appliqué (${applied})`);
  // le fond réellement peint doit être celui du thème
  const px = await page.evaluate(() => { window.renderAt(0); return Array.from(document.getElementById('c').getContext('2d').getImageData(2, 2, 1, 1).data).slice(0, 3).join(','); });
  const want = await page.evaluate((th) => { const h = DuelPlayer.THEMES[th].bg; const v = parseInt(h.slice(1), 16); return [(v >> 16) & 255, (v >> 8) & 255, v & 255].join(','); }, applied);
  if (px !== want) throw new Error(`fond peint ${px} ≠ fond du thème ${o.theme} (${want})`);
  const info = await page.evaluate(() => ({ duration: window.sceneDuration, timing: window.DUEL_SCENE.timing }));
  const grab = async (t) => {
    const b64 = await page.evaluate((tt) => { window.renderAt(tt); return document.getElementById('c').toDataURL('image/png').split(',')[1]; }, t);
    return Buffer.from(b64, 'base64');
  };
  const base = path.basename(o.bout);
  if (o.stills) {
    const dir = path.resolve(o.out || '.');
    fs.mkdirSync(dir, { recursive: true });
    const list = o.stills.split(',').flatMap((s) => (s === 'pauses' ? info.timing.pauses.map((p) => String((p.t0 + p.t1) / 2)) : [s]));
    for (const s of list) {
      const t = s === 'key' ? info.timing.key : (s === 'end' ? info.duration - 0.05 : Number(s));
      const file = path.join(dir, `${base}_k${o.k}_${o.theme}_${String(t.toFixed(2)).replace('.', '_')}.png`);
      fs.writeFileSync(file, await grab(t));
      console.log(file);
    }
    check();
    await page.close();
    return;
  }
  const nframes = Math.ceil(info.duration * o.fps);
  const stem = path.join(o.bout, `${base}_hgp_hdbscan_k${o.k}_${o.theme}`);
  const outMp4 = `${stem}.mp4`, partMp4 = `${stem}.part.mp4`;
  const ff = spawn(ffmpegPath(), ['-y', '-loglevel', 'error', '-f', 'image2pipe', '-vcodec', 'png', '-framerate', String(o.fps), '-i', '-',
    '-c:v', 'libx264', '-preset', 'slow', '-crf', '22', '-tune', 'animation', '-pix_fmt', 'yuv420p', '-profile:v', 'high', '-level', '4.1',
    '-r', String(o.fps), '-movflags', '+faststart', partMp4], { stdio: ['pipe', 'inherit', 'inherit'] });
  const done = new Promise((res, rej) => ff.on('close', (c) => (c === 0 ? res() : rej(new Error('ffmpeg ' + c)))));
  for (let f = 0; f < nframes; f++) {
    if (errors.length) { ff.stdin.destroy(); ff.kill(); fs.rmSync(partMp4, { force: true }); check(); }
    const png = await grab(f / o.fps);
    if (!ff.stdin.write(png)) await new Promise((r) => ff.stdin.once('drain', r));
  }
  ff.stdin.end();
  await done;
  const key = await grab(info.timing.key), summary = await grab(info.duration - 0.05);
  check();
  fs.renameSync(partMp4, outMp4);
  fs.writeFileSync(`${stem}_instant_cle.png`, key);
  fs.writeFileSync(`${stem}_bilan.png`, summary);
  await page.close();
  console.log(`${outMp4} (${nframes} images, ${info.duration.toFixed(1)} s)`);
}

main().catch((e) => { console.error(e); process.exit(1); });
