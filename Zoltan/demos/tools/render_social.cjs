#!/usr/bin/env node
/*
 * Vidéos courtes « HGP vs HDBSCAN » pour téléphone (LinkedIn) : player/social.html, 1080 x 1350, anglais.
 * Enchaîne plusieurs scènes (data/duel_k<k>_en.js écrites par tools/duel_scene.py --relabel en) dans un seul MP4.
 *
 * Usage : node tools/render_social.cjs --out FICHIER.mp4 --theme clair|sombre [--fps 30] [--crf 23]
 *         [--stills t1,t2 --scene i] SCENE1.js SCENE2.js ...
 */
'use strict';
const path = require('path');
const fs = require('fs');
const { execSync, spawn } = require('child_process');

function ffmpegPath() {
  if (process.env.FFMPEG) return process.env.FFMPEG;
  return execSync('python3 -c "import imageio_ffmpeg as i; print(i.get_ffmpeg_exe())"').toString().trim();
}
function opt(args, name, dflt) { const i = args.indexOf(name); if (i < 0) return dflt; const v = args[i + 1]; args.splice(i, 2); return v; }
function sceneOf(file) { const s = fs.readFileSync(file, 'utf8'); return JSON.parse(s.slice(s.indexOf('{'), s.lastIndexOf('}') + 1)); }

async function main() {
  const args = process.argv.slice(2);
  const out = opt(args, '--out', null), theme = opt(args, '--theme', 'sombre');
  const fps = Number(opt(args, '--fps', 30)), crf = opt(args, '--crf', '23');
  const stills = opt(args, '--stills', null), only = opt(args, '--scene', null);
  if (!out || !args.length) throw new Error('usage : render_social.cjs --out F.mp4 --theme clair|sombre SCENE.js ...');
  const scenes = args.map((f) => sceneOf(path.resolve(f)));
  const { chromium } = require('playwright');
  const browser = await chromium.launch({ args: ['--allow-file-access-from-files'] });
  try {
    const page = await browser.newPage({ viewport: { width: 1080, height: 1350 }, deviceScaleFactor: 1 });
    const errors = [];
    page.on('pageerror', (e) => errors.push(e.message));
    await page.goto('file://' + path.resolve(__dirname, '..', 'player', 'social.html'));
    await page.waitForFunction(() => !!window.SocialPlayer);
    await page.evaluate((th) => { window.SocialPlayer.setTheme(th); window.ctx2d = document.getElementById('c').getContext('2d'); }, theme);
    const grab = async (t) => Buffer.from(await page.evaluate((tt) => { window.SocialPlayer.renderAt(window.ctx2d, tt); return document.getElementById('c').toDataURL('image/png').split(',')[1]; }, t), 'base64');
    const check = () => { if (errors.length) throw new Error('erreur dans la page : ' + errors.join(' | ')); };
    if (stills) {
      const list = only == null ? scenes.map((_, i) => i) : [Number(only)];
      for (const i of list) {
        await page.evaluate((sc) => window.SocialPlayer.load(sc), scenes[i]);
        for (const s of stills.split(',')) {
          const f = out.replace(/\.png$|\.mp4$/, '') + `_s${i}_${s.replace('.', '_')}.png`;
          fs.writeFileSync(f, await grab(Number(s))); console.log(f);
        }
      }
      check(); return;
    }
    const part = out.replace(/\.mp4$/, '.part.mp4');
    const ff = spawn(ffmpegPath(), ['-y', '-loglevel', 'error', '-f', 'image2pipe', '-vcodec', 'png', '-framerate', String(fps), '-i', '-',
      '-c:v', 'libx264', '-preset', 'slow', '-crf', String(crf), '-pix_fmt', 'yuv420p', '-profile:v', 'high', '-level', '4.1',
      '-r', String(fps), '-movflags', '+faststart', part], { stdio: ['pipe', 'inherit', 'inherit'] });
    const done = new Promise((res, rej) => ff.on('close', (c) => (c === 0 ? res() : rej(new Error('ffmpeg ' + c)))));
    let total = 0;
    for (const sc of scenes) {
      const dur = await page.evaluate((s) => window.SocialPlayer.load(s), sc);
      const n = Math.round(dur * fps);
      for (let f = 0; f < n; f++) {
        if (errors.length) { ff.stdin.destroy(); ff.kill(); fs.rmSync(part, { force: true }); check(); }
        const png = await grab(f / fps);
        if (!ff.stdin.write(png)) await new Promise((r) => ff.stdin.once('drain', r));
      }
      total += n;
    }
    ff.stdin.end(); await done; check();
    fs.renameSync(part, out);
    console.log(`${out} (${total} images, ${(total / fps).toFixed(1)} s)`);
  } finally { await browser.close(); }
}
main().catch((e) => { console.error(e); process.exit(1); });
