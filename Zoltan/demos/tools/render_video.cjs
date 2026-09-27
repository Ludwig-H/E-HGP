#!/usr/bin/env node
/*
 * Capture image par image d'une scène du lecteur, puis encodage vidéo.
 *
 * Usage : node Zoltan/demos/tools/render_video.cjs <démo> <étiquette> [--theme clair|sombre] [--fps 30]
 *   (étiquette : hdbscan_K5, hdbscan_K10, alpine_bev… ; sans --theme, les deux thèmes)
 *
 * Ouvre player/index.html?capture&theme=<thème>&scene=../<démo>/data/scene_<étiquette>.js
 * dans Chromium sans tête (Playwright), appelle renderAt(t) pour chaque image,
 * récupère le canevas en PNG et le pousse dans ffmpeg :
 *   MP4 H.264 High, yuv420p, 1920×1080, 30 i/s, +faststart
 * (lisible par PowerPoint, Keynote, Google Slides, LibreOffice et les
 * lecteurs appelés par Beamer), plus deux images fixes : l'instant clé
 * (timing.key de la scène : par défaut la première fusion de branches suivies,
 * règle réglable par démo, voir build_scene.key_time) et le bilan final. Un jeu par thème, comme Percolia.com :
 *   <démo>_<étiquette>_sombre.mp4, _sombre_instant_cle.png, _sombre_bilan.png
 *   <démo>_<étiquette>_clair.mp4,  _clair_instant_cle.png,  _clair_bilan.png
 * ffmpeg : variable FFMPEG, sinon celui du paquet Python imageio-ffmpeg.
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

async function main() {
  const args = process.argv.slice(2);
  if (args.length < 2) throw new Error('usage: render_video.cjs <demo_dir> <tag> [--theme clair|sombre] [--fps 30]');
  const demo = path.resolve(args[0]);
  const tag = args[1];
  const fps = args.includes('--fps') ? Number(args[args.indexOf('--fps') + 1]) : 30;
  const only = args.includes('--theme') ? args[args.indexOf('--theme') + 1] : null;
  if (only && !['clair', 'sombre'].includes(only)) throw new Error('--theme : clair ou sombre');
  const themes = only ? [only] : ['sombre', 'clair'];
  const root = path.resolve(__dirname, '..');
  const sceneRel = path.relative(path.join(root, 'player'), path.join(demo, 'data', `scene_${tag}.js`)).split(path.sep).join('/');
  if (!fs.existsSync(path.join(demo, 'data', `scene_${tag}.js`))) throw new Error('scène absente : lancer build_scene.py');
  const { chromium } = requirePlaywright();
  const browser = await chromium.launch({ args: ['--allow-file-access-from-files'] });
  for (const theme of themes) await renderTheme(browser, demo, tag, fps, theme, root, sceneRel);
  await browser.close();
}

async function renderTheme(browser, demo, tag, fps, theme, root, sceneRel) {
  const url = 'file://' + path.join(root, 'player', 'index.html') + `?capture=1&theme=${theme}&scene=${encodeURIComponent(sceneRel)}`;
  const page = await browser.newPage({ viewport: { width: 1920, height: 1080 }, deviceScaleFactor: 1 });
  const errors = [];  // une erreur de page annule le rendu avant toute écriture
  page.on('pageerror', (e) => errors.push(e.message));
  const check = () => { if (errors.length) throw new Error(`erreur dans la page (${theme}) : ${errors.join(' | ')}`); };
  const refused = async () => { if (await page.evaluate(() => !!window.sceneError)) throw new Error(`scène refusée par le lecteur (${theme}) : ${await page.evaluate(() => document.getElementById('stage').textContent.trim())}`); };
  await page.goto(url);
  await page.waitForFunction(() => window.sceneReady === true || window.sceneError, null, { timeout: 60000 });
  check();
  await refused();
  // le thème est imposé par l'URL ; on vérifie l'attribut ET le fond réellement peint par le canevas
  const applied = await page.evaluate(() => document.documentElement.getAttribute('data-theme'));
  if (applied !== (theme === 'clair' ? 'light' : 'dark')) throw new Error(`thème ${theme} non appliqué (${applied})`);
  const px = await page.evaluate(() => { window.renderAt(0); return Array.from(document.getElementById('c').getContext('2d').getImageData(2, 2, 1, 1).data).slice(0, 3).join(','); });
  const want = await page.evaluate((th) => { const h = DemoPlayer.THEMES[th].bg; const v = parseInt(h.slice(1), 16); return [(v >> 16) & 255, (v >> 8) & 255, v & 255].join(','); }, applied);
  if (px !== want) throw new Error(`fond peint ${px} ≠ fond du thème ${theme} (${want})`);
  const info = await page.evaluate(() => ({ duration: window.sceneDuration, timing: window.DEMO_SCENE.timing }));
  if (info.timing.key == null) throw new Error('scène sans timing.key : relancer tools/build_scene.py');
  const nframes = Math.ceil(info.duration * fps);

  const base = path.basename(demo);
  const stem = path.join(demo, `${base}_${tag}_${theme}`);
  // anciennes sorties sans suffixe de thème : plus aucun README ne les cite
  for (const s of ['.mp4', '_instant_cle.png', '_bilan.png']) fs.rmSync(path.join(demo, `${base}_${tag}${s}`), { force: true });
  const outMp4 = `${stem}.mp4`, partMp4 = `${stem}.part.mp4`;  // renommé seulement si tout a réussi
  const ff = spawn(ffmpegPath(), ['-y', '-loglevel', 'error', '-f', 'image2pipe', '-vcodec', 'png', '-framerate', String(fps), '-i', '-',
    '-c:v', 'libx264', '-preset', 'slow', '-crf', '22', '-tune', 'animation', '-pix_fmt', 'yuv420p', '-profile:v', 'high', '-level', '4.1',
    '-r', String(fps), '-movflags', '+faststart', partMp4], { stdio: ['pipe', 'inherit', 'inherit'] });
  const done = new Promise((res, rej) => ff.on('close', (c) => (c === 0 ? res() : rej(new Error('ffmpeg ' + c)))));

  const tKey = info.timing.key;
  const tSummary = info.duration - 0.5;
  const grab = async (t) => {
    const b64 = await page.evaluate((tt) => { window.renderAt(tt); return document.getElementById('c').toDataURL('image/png').split(',')[1]; }, t);
    return Buffer.from(b64, 'base64');
  };
  for (let f = 0; f < nframes; f++) {
    if (errors.length) { ff.stdin.destroy(); ff.kill(); fs.rmSync(partMp4, { force: true }); check(); }
    const png = await grab(f / fps);
    if (!ff.stdin.write(png)) await new Promise((r) => ff.stdin.once('drain', r));
    if (f % 300 === 0) process.stdout.write(`  ${base} ${tag} ${theme} : image ${f}/${nframes}\n`);
  }
  ff.stdin.end();
  await done;
  const key = await grab(tKey), summary = await grab(tSummary);
  check();
  fs.renameSync(partMp4, outMp4);
  fs.writeFileSync(`${stem}_instant_cle.png`, key);
  fs.writeFileSync(`${stem}_bilan.png`, summary);
  await page.close();
  console.log(`${outMp4} (${nframes} images, ${info.duration.toFixed(1)} s)`);
}

main().catch((e) => { console.error(e); process.exit(1); });
