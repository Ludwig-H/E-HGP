"""Differentiel tour du verificateur r2 : base (reference, build-base) contre r2 (build-r2) et r2 instrumente
(compteurs de chemin de SiteTree, sortie INSTR sur stderr). Dump exact (sha256) et compteurs deterministes (1 fil),
plus un rejeu a 3 fils du dump seul. Code 0 si tout est identique et si le repli de SiteTree n'est jamais pris."""
import hashlib, json, os, re, subprocess, sys
V = '/tmp/mhgp10-r2/sitetree-verif'
INP = '/workspaces/E-HGP/build/v10-scale-inputs'
EXE = {'base': V + '/build-base/mhgp10_tower', 'r2': V + '/build-r2/mhgp10_tower', 'instr': V + '/instr/r2_instr/mhgp10_tower'}
CASES = [('syn_clusters_density_x1', 10, 'cover'), ('syn_shells_space_x2', 5, 'cover'),
         ('syn_uniform_density_x4', 10, 'core'), ('syn_terrain_density_x2', 10, 'cover'),
         ('syn_filaments_density_x4', 5, 'cover'), ('lidar02_quarter_x_nonneg_y_neg', 5, 'cover'),
         ('lidar01_half_x_nonneg', 10, 'core')]
VOL = ('rss_kib', 'passes_catalogue_s', 'passes_tower_s', 'threads')
def strip(o):
    if isinstance(o, dict):
        return {k: strip(v) for k, v in o.items() if k not in VOL and not k.startswith(('t_', 'sum_order_')) and not k.endswith('_s')}
    if isinstance(o, list):
        return [strip(v) for v in o]
    return o
def run(exe, src, k, entry, threads, dump):
    r = subprocess.run(['nice', '-n', '5', exe, src, '--k=%d' % k, '--threads=%d' % threads, '--entry=' + entry, '--dump=' + dump],
                       capture_output=True, text=True)
    h = None
    if r.returncode == 0 and os.path.exists(dump):
        s = hashlib.sha256(); s.update(open(dump, 'rb').read()); h = s.hexdigest()
    if os.path.exists(dump):
        os.remove(dump)
    lines = [x for x in r.stdout.splitlines() if x.startswith('{')]
    js = json.loads(lines[-1]) if lines else {}
    m = re.search(r'INSTR repli_dirige=(\d+) repli_nearest=(\d+) filtre_dirige=(\d+) filtre_nearest=(\d+)', r.stderr)
    return r.returncode, h, js, (tuple(int(x) for x in m.groups()) if m else None)
bad = 0
work = V + '/runs/diff_tour'
os.makedirs(work, exist_ok=True)
for name, k, entry in CASES:
    src = os.path.join(INP, name + '.u32le')
    res = {t: run(e, src, k, entry, 1, os.path.join(work, t + '.dump')) for t, e in EXE.items()}
    r3 = {t: run(EXE[t], src, k, entry, 3, os.path.join(work, t + '3.dump')) for t in ('base', 'r2')}
    codes = [res[t][0] for t in res] + [r3[t][0] for t in r3]
    hs = {res[t][1] for t in res} | {r3[t][1] for t in r3}
    same_dump = None not in hs and len(hs) == 1
    same_cnt = strip(res['base'][2]) == strip(res['r2'][2]) == strip(res['instr'][2])
    ins = res['instr'][3]
    no_fallback = ins is not None and ins[0] == 0 and ins[1] == 0 and ins[2] == 0
    ok = same_dump and same_cnt and all(c == 0 for c in codes) and no_fallback
    bad += not ok
    print('%-32s n=%-6s K=%-2d %-5s codes=%s dump=%s dumps(1f,3f)=%s compteurs=%s instr(repli_dir,repli_near,filtre_dir,filtre_near)=%s %s'
          % (name, res['r2'][2].get('n'), k, entry, codes, (res['base'][1] or '-')[:16], 'identiques' if same_dump else 'DIFFERENTS',
             'identiques' if same_cnt else 'DIFFERENTS', ins, 'IDENTIQUES' if ok else 'ECART'), flush=True)
print('ECARTS %d sur %d' % (bad, len(CASES)))
sys.exit(1 if bad else 0)
