"""Regression (29 septembre 2026, audits continu et independant du 29 septembre : frontieres d'entree des sondes). Chaque
cas est une fixture minimale gravee aux coordonnees exactes ; le code de sortie est EXACT et un signal n'est jamais un
succes (delai depasse : processus tue et recolte, echec).

  (a) Fin de fichier u32le incomplete : quatre points valides suivis de r = 1..11 octets (prefixe du point hors u18
      262144 1 2) -> refus size_mismatch (invalid_input), code 2, sans sortie ecrite, sur les quatre sondes
      (mhgp10_catalogue, mhgp10_tower, mhgp10_cluster, mhgp10_mreach_cluster). Temoins contre le vert par vacuite :
      r = 0 lu en entier (code 0, n = 4) ; r = 12 (point complet) lu puis juge par prepare_cloud : hors u18 refuse
      (coordinate_out_of_domain), admis par le temoin mreach en 21 bits (n = 5). Fichier absent, dossier :
      input_unreadable.
  (b) mhgp10_tower --no-points --dump sur trois points alignes : code 0, et le dump est exactement le dump complet
      prive de ses lignes point (noeuds, parents, niveaux exacts, verticales).
  (c) mhgp10_tower --repeat=0, -1, illisible : refus parameter_out_of_range ; temoin --repeat=2 (deux passes publiees).
  (d) mhgp10_catalogue --leaf=M avec M < K + 3 sur les huit coins du cube u18, K = 5 : refus parameter_out_of_range
      avant calcul (l'ancien binaire subdivise sans fin : delai depasse) ; temoins M = K + 3 et defaut (27 boules,
      dumps identiques).
  (e) Valeur d'option illisible (--k=abc) : refus parameter_out_of_range, jamais SIGABRT, sur les quatre sondes.

Python nu (ni numpy ni scipy) : la porte tourne aussi sur la VM G4 (label fast). Aucune dependance a assert.

  python3 test_cli_input_frontiers.py <dossier de build>   -> 0 conforme, 1 desaccord, 3 plancher non atteint
"""
import json
import os
import resource
import struct
import subprocess
import sys
import tempfile

TIMEOUT_S = 60
FOUR = [(0, 0, 0), (8, 0, 0), (0, 8, 0), (0, 0, 8)]  # audit independant, run_frontier_checks.py
OUTSIDE = struct.pack('<3I', 262144, 1, 2)            # premier mot hors u18 (audit independant, one_word / two_words)
LINE3 = [(0, 0, 0), (2, 0, 0), (5, 0, 0)]              # audit continu, pool_head (export sans attaches)
CUBE = [(x, y, z) for x in (0, 262143) for y in (0, 262143) for z in (0, 262143)]  # audit continu, leaf_stress
CHECKS_EXPECTED = 15 * 4 + 4 + 4 + 8 + 4


def pack(points):
    return b''.join(struct.pack('<3I', *p) for p in points)


def no_core():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


class Gate:
    def __init__(self, build, tmp):
        self.build, self.tmp = build, tmp
        self.checks = self.failures = 0

    def run(self, argv):
        """(code, derniere ligne JSON de stdout, stderr) ; code None si le delai est depasse (processus tue)."""
        try:
            r = subprocess.run(argv, capture_output=True, text=True, timeout=TIMEOUT_S, preexec_fn=no_core)
        except subprocess.TimeoutExpired:
            return None, {}, 'delai depasse'
        js = {}
        for line in r.stdout.splitlines():
            if line.startswith('{'):
                try:
                    js = json.loads(line)
                except ValueError:
                    js = {}
        return r.returncode, js, r.stderr

    def record(self, name, ok, detail):
        self.checks += 1
        self.failures += not ok
        print('%-58s %s %s' % (name, 'ok' if ok else 'ECHEC', detail), flush=True)

    def write(self, name, data):
        path = os.path.join(self.tmp, name)
        with open(path, 'wb') as f:
            f.write(data)
        return path

    def probe(self, tool, src, extra):
        """Sonde sur src : (code, json, stderr, taille de la sortie i32 ou None)."""
        exe = os.path.join(self.build, 'mhgp10_' + tool)
        out = os.path.join(self.tmp, 'labels_' + tool + '.i32le')
        if os.path.exists(out):
            os.remove(out)
        argv = [exe, src] + ([out] if tool in ('cluster', 'mreach_cluster') else []) + extra
        code, js, err = self.run(argv)
        size = os.path.getsize(out) if os.path.exists(out) else None
        return code, js, err, size

    def refused(self, tool, code, js, err, size, reason):
        """Refus attendu avant calcul : code 2, raison publiee (ligne JSON de statut invalid_input, ou « refus <raison> »
        sur la sortie d'erreur du temoin mreach), aucune sortie ecrite."""
        if tool == 'mreach_cluster':
            said = ('refus %s' % reason) in err.splitlines()
        else:
            said = js.get('reason') == reason and js.get('status') == 'invalid_input'
        return code == 2 and said and size is None

    def truncated_tails(self):
        for tool in ('catalogue', 'tower', 'cluster', 'mreach_cluster'):
            extra = ['--k=2', '--threads=1'] + (['--mcs=2'] if tool in ('cluster', 'mreach_cluster') else [])
            for r in range(0, 13):
                src = self.write('tail%d.u32le' % r, pack(FOUR) + OUTSIDE[:r])
                code, js, err, size = self.probe(tool, src, extra)
                if r == 0:
                    ok = code == 0 and (size == 16 if tool == 'mreach_cluster' else js.get('n') == 4 and
                                        js.get('status') == 'ok')
                elif r == 12 and tool == 'mreach_cluster':
                    ok = code == 0 and size == 20  # 21 bits : le cinquieme point est lu et admis
                elif r == 12:
                    ok = self.refused(tool, code, js, err, size, 'coordinate_out_of_domain')
                else:
                    ok = self.refused(tool, code, js, err, size, 'size_mismatch')
                self.record('%s reste %d octets' % (tool, r), ok, 'code=%s raison=%s' % (code, js.get('reason', '-')))
            for label, src in (('fichier absent', os.path.join(self.tmp, 'absent.u32le')), ('dossier', self.tmp)):
                code, js, err, size = self.probe(tool, src, extra)
                self.record('%s %s' % (tool, label), self.refused(tool, code, js, err, size, 'input_unreadable'),
                            'code=%s raison=%s' % (code, js.get('reason', '-')))

    def no_points_dump(self):
        exe = os.path.join(self.build, 'mhgp10_tower')
        src = self.write('line3.u32le', pack(LINE3))
        full, bare = os.path.join(self.tmp, 'full.dump'), os.path.join(self.tmp, 'bare.dump')
        code_f, js_f, _ = self.run([exe, src, '--k=2', '--threads=1', '--dump=' + full])
        code_b, js_b, _ = self.run([exe, src, '--k=2', '--threads=1', '--no-points', '--dump=' + bare])
        self.record('tower dump complet (temoin)', code_f == 0 and js_f.get('points') is True, 'code=%s' % code_f)
        self.record('tower --no-points --dump', code_b == 0 and js_b.get('points') is False, 'code=%s' % code_b)
        lf = open(full).read().splitlines() if code_f == 0 and os.path.exists(full) else []
        lb = open(bare).read().splitlines() if code_b == 0 and os.path.exists(bare) else []
        kept = [x for x in lf if not x.startswith('point ')]
        # K = 2 sur trois points alignes : ordre 1, cinq noeuds ; ordre 2, trois noeuds ; trois attaches par ordre
        self.record('dump complet : 2 ordres, 8 noeuds, 6 attaches',
                    sum(x.startswith('order ') for x in lf) == 2 and sum(x.startswith('node ') for x in lf) == 8 and
                    sum(x.startswith('point ') for x in lf) == 6, 'lignes=%d' % len(lf))
        self.record('dump sans attaches == dump complet sans lignes point', bool(lb) and lb == kept,
                    'lignes=%d/%d' % (len(lb), len(kept)))

    def repeat(self):
        exe = os.path.join(self.build, 'mhgp10_tower')
        src = self.write('four.u32le', pack(FOUR))
        for value in ('0', '-1', 'x'):
            code, js, _ = self.run([exe, src, '--k=2', '--threads=1', '--repeat=' + value])
            self.record('tower --repeat=%s' % value, code == 2 and js.get('reason') == 'parameter_out_of_range',
                        'code=%s raison=%s' % (code, js.get('reason', '-')))
        code, js, _ = self.run([exe, src, '--k=2', '--threads=1', '--repeat=2'])
        self.record('tower --repeat=2 (temoin)', code == 0 and len(js.get('passes_catalogue_s', [])) == 2 and
                    len(js.get('passes_tower_s', [])) == 2, 'code=%s' % code)

    def leaf(self):
        exe = os.path.join(self.build, 'mhgp10_catalogue')
        src = self.write('cube.u32le', pack(CUBE))
        for leaf in ('2', '4', '5', '7', 'abc'):  # 2 : leaf_stress de l'audit ; K - 1, K, K + 2 ; illisible
            code, js, _ = self.run([exe, src, '--k=5', '--threads=1', '--leaf=' + leaf])
            self.record('catalogue K=5 --leaf=%s' % leaf, code == 2 and js.get('reason') == 'parameter_out_of_range' and
                        js.get('status') == 'invalid_input', 'code=%s raison=%s' % (code, js.get('reason', '-')))
        dumps = []
        for extra in (['--leaf=8'], []):
            dump = os.path.join(self.tmp, 'cube%d.dump' % len(dumps))
            code, js, _ = self.run([exe, src, '--k=5', '--threads=1', '--dump=' + dump] + extra)
            dumps.append(open(dump).read() if code == 0 and os.path.exists(dump) else None)
            self.record('catalogue K=5 %s (temoin)' % (extra[0] if extra else 'feuille par defaut'),
                        code == 0 and js.get('balls') == 27, 'code=%s boules=%s' % (code, js.get('balls', '-')))
        # le catalogue ne depend pas de M ; sur un arbre non trivial, l'egalite est jugee en C++ (mhgp10_unit)
        self.record('catalogue K=5 : dump --leaf=8 == dump par defaut', dumps[0] is not None and dumps[0] == dumps[1],
                    'octets=%s' % (len(dumps[0]) if dumps[0] else '-'))

    def unreadable_options(self):
        src = self.write('four.u32le', pack(FOUR))
        for tool in ('catalogue', 'tower', 'cluster', 'mreach_cluster'):
            code, js, err, size = self.probe(tool, src, ['--k=abc', '--mcs=2'] if tool in ('cluster', 'mreach_cluster')
                                             else ['--k=abc'])
            self.record('%s --k=abc' % tool, self.refused(tool, code, js, err, size, 'parameter_out_of_range'),
                        'code=%s' % code)


def main():
    build = sys.argv[1]
    # les quatre sondes doivent etre construites (sur la VM G4 : build_targets du plan) ; jamais de vert par absence
    missing = [t for t in ('catalogue', 'tower', 'cluster', 'mreach_cluster')
               if not os.access(os.path.join(build, 'mhgp10_' + t), os.X_OK)]
    if missing:
        print('PLANCHER : sonde(s) absente(s) du build : %s' % ' '.join('mhgp10_' + t for t in missing))
        return 3
    with tempfile.TemporaryDirectory() as tmp:
        gate = Gate(build, tmp)
        gate.truncated_tails()
        gate.no_points_dump()
        gate.repeat()
        gate.leaf()
        gate.unreadable_options()
    if gate.checks != CHECKS_EXPECTED:
        print('PLANCHER : %d controles au lieu de %d' % (gate.checks, CHECKS_EXPECTED))
        return 3
    if gate.failures:
        print('ECHECS %d sur %d' % (gate.failures, gate.checks))
        return 1
    print('cli_input_frontiers_ok %d' % gate.checks)
    return 0


if __name__ == '__main__':
    sys.exit(main())
