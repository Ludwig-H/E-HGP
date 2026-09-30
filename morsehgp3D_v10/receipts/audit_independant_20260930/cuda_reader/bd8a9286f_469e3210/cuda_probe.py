"""Compile et execute la sonde CUDA (cuda_probe.cu) sur la VM G4, avec le nvcc de /usr/local/cuda (hors du PATH).
Python systeme, sans dependance. Chaque tentative ecrit dans un dossier --out NEUF (absent ou vide) :
  attempt.json   toujours ecrit, quel que soit le chemin (etape atteinte, code, raisons, fin des sorties) ;
  compile.txt    commande et sorties de nvcc ;
  cuda_probe.json la sortie brute de la sonde, seulement si la sonde s'est executee ;
  probe_stdout.txt / probe_stderr.txt les sorties partielles en cas de delai ou de signal.
Un ancien resultat ne peut donc jamais etre relu comme celui de la tentative courante.

  python3 cuda_probe.py --out DIR --work DIR [--arch sm_120]
  python3 cuda_probe.py --self-test        (juge et chemins d'echec simules, sans CUDA)
Codes de la sonde, propages seulement si le juge les confirme : 0 ok, 1 ecart d'exactitude i128, 2 CUDA indisponible,
3 appel CUDA en echec, 5 duree mesuree invalide. Codes du lanceur : 4 nvcc introuvable, 6 compilation en echec, 7 sonde
tuee par un signal ou hors delai, 8 sortie refusee par le juge (schema, valeurs ou coherence avec le code), 9 refus des
arguments (dossier de sortie non neuf, options manquantes).
Le juge lit une seule ligne JSON stricte (cles dupliquees et constantes NaN/Infinity refusees), controle les champs
requis et leurs types, puis les valeurs : aucun desaccord i128 pour ok, egalites forcees presentes, nombres finis,
durees valides et debits strictement positifs pour ok, debits nuls quand les durees sont invalides.
"""
import glob
import hashlib
import json
import math
import os
import re
import subprocess
import sys

STATUS_OF_CODE = {0: 'ok', 1: 'i128_mismatch', 2: 'no_cuda', 3: 'cuda_error', 5: 'timing_invalid'}
PAIRS = 1 << 24  # paires i128 de la sonde (n = 2^24)


def _multiples(m):
    return (PAIRS - 1) // m + 1  # multiples de m dans [0, PAIRS)


# Egalites forcees par la sonde : c = b, d = a pour i multiple de 7, sauf si a (i multiple de 11) ou b (i multiple de 13)
# est ensuite remplace par une borne : 2 011 255 cas (inclusion-exclusion), exactement le compte du reçu de la session 7.
FORCED_EQUAL = _multiples(7) - _multiples(77) - _multiples(91) + _multiples(1001)
RATES = ('loop64_gops', 'loop128_gops', 'i128_over_i64', 'h2d_gbps', 'd2h_gbps', 'launch_sync_us', 'launch_async_us')
FULL_TYPES = {'status': str, 'device': str, 'cc': str, 'sms': int, 'global_mem_gib': float, 'cuda_error': str,
              'i128_pairs': int, 'i128_equal_cases': int, 'i128_mismatches': int, 'timing_ok': bool, 'loops': str}
FULL_TYPES.update({k: float for k in RATES})


class Refus(Exception):
    pass


def arg(name, default=None):
    return sys.argv[sys.argv.index(name) + 1] if name in sys.argv and sys.argv.index(name) + 1 < len(sys.argv) else default


def _pairs_no_duplicates(pairs):
    keys = [k for k, _ in pairs]
    if len(keys) != len(set(keys)):
        raise Refus('cle JSON dupliquee')
    return dict(pairs)


def _refuse_constant(name):
    raise Refus('constante JSON non standard : %s' % name)


def parse_record(stdout):
    """Exactement une ligne non vide, objet JSON strict. Leve Refus sinon."""
    lines = [line for line in stdout.splitlines() if line.strip()]
    if len(lines) != 1:
        raise Refus('%d lignes non vides au lieu d une' % len(lines))
    try:
        rec = json.loads(lines[0], object_pairs_hook=_pairs_no_duplicates, parse_constant=_refuse_constant)
    except ValueError as e:
        raise Refus('JSON illisible : %s' % e)
    if not isinstance(rec, dict):
        raise Refus('la ligne n est pas un objet JSON')
    return rec


def _is_type(value, t):
    if t is bool:
        return isinstance(value, bool)
    if t is int:
        return isinstance(value, int) and not isinstance(value, bool)
    if t is float:
        return isinstance(value, (int, float)) and not isinstance(value, bool)
    return isinstance(value, t)


def judge(code, rec):
    """Liste des raisons de refus (vide si l'enregistrement est conforme au code du processus)."""
    why = []
    if code not in STATUS_OF_CODE:
        return ['code de processus %r hors des codes de la sonde' % code]
    status = STATUS_OF_CODE[code]
    if rec.get('status') != status:
        return ['status %r en desaccord avec le code %d' % (rec.get('status'), code)]
    if status == 'no_cuda':
        return [] if set(rec) == {'status'} else ['no_cuda porte des champs inattendus']
    if status == 'cuda_error' and set(rec) == {'status', 'call', 'error'}:
        ok = isinstance(rec['call'], str) and rec['call'] and isinstance(rec['error'], str) and rec['error']
        return [] if ok else ['cuda_error : call et error doivent etre des chaines non vides']
    missing = sorted(set(FULL_TYPES) - set(rec))
    extra = sorted(set(rec) - set(FULL_TYPES))
    if missing:
        why.append('champs manquants : %s' % ', '.join(missing))
    if extra:
        why.append('champs inattendus : %s' % ', '.join(extra))
    for k, t in FULL_TYPES.items():
        if k in rec and not _is_type(rec[k], t):
            why.append('type de %s : %r' % (k, rec[k]))
    if why:
        return why
    for k in RATES + ('global_mem_gib',):
        if not math.isfinite(rec[k]):
            why.append('%s non fini' % k)
    if rec['loops'] != 'modular_unsigned':
        why.append('boucles non modulaires : %r' % rec['loops'])
    if not re.fullmatch(r'\d+\.\d+', rec['cc']):
        why.append('cc mal forme : %r' % rec['cc'])
    if rec['sms'] <= 0 or rec['global_mem_gib'] <= 0:
        why.append('peripherique sans SM ou sans memoire')
    if rec['i128_pairs'] != PAIRS:
        why.append('i128_pairs %d au lieu de %d' % (rec['i128_pairs'], PAIRS))
    if not FORCED_EQUAL <= rec['i128_equal_cases'] <= PAIRS:
        why.append('egalites forcees absentes ou hors bornes : %d' % rec['i128_equal_cases'])
    if not 0 <= rec['i128_mismatches'] <= PAIRS:
        why.append('i128_mismatches hors bornes : %d' % rec['i128_mismatches'])
    if status == 'ok':
        if rec['i128_mismatches'] != 0:
            why.append('ok avec %d desaccords i128' % rec['i128_mismatches'])
        if rec['cuda_error'] != 'no error':
            why.append('ok avec erreur CUDA %r' % rec['cuda_error'])
        if rec['timing_ok'] is not True:
            why.append('ok avec durees invalides')
        for k in RATES:
            if not rec[k] > 0:
                why.append('ok avec %s <= 0' % k)
    elif status == 'i128_mismatch':
        if rec['i128_mismatches'] <= 0:
            why.append('i128_mismatch sans desaccord compte')
    elif status == 'timing_invalid':
        if rec['timing_ok'] is not False:
            why.append('timing_invalid avec timing_ok vrai')
        if rec['i128_mismatches'] != 0 or rec['cuda_error'] != 'no error':
            why.append('timing_invalid masque un desaccord ou une erreur CUDA')
        for k in RATES:
            if rec[k] != 0:
                why.append('timing_invalid publie %s = %r' % (k, rec[k]))
    elif status == 'cuda_error':
        if rec['cuda_error'] == 'no error':
            why.append('cuda_error sans erreur CUDA')
    return why


def check_output(code, stdout):
    """Code final : celui de la sonde si le juge l'accepte, 8 sinon (voir judge)."""
    return verdict(code, stdout)[0]


def verdict(code, stdout):
    try:
        rec = parse_record(stdout)
    except Refus as e:
        return 8, [str(e)]
    why = judge(code, rec)
    return (8, why) if why else (code, [])


def _text(value):
    if value is None:
        return ''
    return value.decode('utf-8', 'replace') if isinstance(value, bytes) else value


def _sha256(path):
    try:
        with open(path, 'rb') as f:
            return hashlib.sha256(f.read()).hexdigest()
    except OSError:
        return None


def main():
    out, work, arch = arg('--out'), arg('--work'), arg('--arch', 'sm_120')
    if not out or not work:
        print('REFUS : --out DIR --work DIR attendus')
        return 9
    if os.path.exists(out) and (not os.path.isdir(out) or os.listdir(out)):
        print('REFUS : --out doit etre un dossier neuf (absent ou vide) : un ancien resultat ne doit pas etre relu')
        return 9
    os.makedirs(out, exist_ok=True)
    os.makedirs(work, exist_ok=True)
    src = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'cuda_probe.cu')
    attempt = {'stage': 'nvcc', 'code': None, 'reasons': [], 'source_sha256': _sha256(src), 'arch': arch}

    def finish(code, stage, reasons=(), stdout='', stderr=''):
        attempt.update(code=code, stage=stage, reasons=list(reasons), stdout_tail=_text(stdout)[-4000:],
                       stderr_tail=_text(stderr)[-4000:])
        with open(os.path.join(out, 'attempt.json'), 'w') as f:
            f.write(json.dumps(attempt, indent=1, sort_keys=True) + '\n')
        return code

    cands = ['/usr/local/cuda/bin/nvcc'] + sorted(glob.glob('/usr/local/cuda-*/bin/nvcc'))
    nvcc = next((c for c in cands if os.path.exists(c)), None)
    if nvcc is None:
        print('nvcc introuvable')
        return finish(4, 'nvcc', ['nvcc introuvable'])
    exe = os.path.join(work, 'cuda_probe')
    cmd = [nvcc, '-O3', '-std=c++17', '-arch=' + arch, '-o', exe, src]
    attempt['nvcc'] = nvcc
    r = subprocess.run(cmd, capture_output=True, text=True)
    with open(os.path.join(out, 'compile.txt'), 'w') as f:
        f.write(' '.join(cmd) + '\ncode %d\n%s\n%s' % (r.returncode, r.stdout, r.stderr))
    if r.returncode != 0:
        print('compilation en echec, voir compile.txt')
        return finish(6, 'compile', ['nvcc code %d' % r.returncode], r.stdout, r.stderr)
    attempt['binary_sha256'] = _sha256(exe)
    try:
        r = subprocess.run([exe], capture_output=True, text=True, timeout=600)
    except subprocess.TimeoutExpired as e:
        for name, value in (('probe_stdout.txt', e.stdout), ('probe_stderr.txt', e.stderr)):
            with open(os.path.join(out, name), 'w') as f:
                f.write(_text(value))
        print('sonde hors delai (600 s), sorties partielles conservees')
        return finish(7, 'timeout', ['delai de 600 s depasse'], e.stdout, e.stderr)
    with open(os.path.join(out, 'cuda_probe.json'), 'w') as f:
        f.write(r.stdout)
    print(r.stdout.strip())
    if r.stderr:
        print(r.stderr[-2000:])
    if r.returncode < 0:
        for name, value in (('probe_stdout.txt', r.stdout), ('probe_stderr.txt', r.stderr)):
            with open(os.path.join(out, name), 'w') as f:
                f.write(_text(value))
        print('sonde tuee par le signal %d' % -r.returncode)
        return finish(7, 'signal', ['signal %d' % -r.returncode], r.stdout, r.stderr)
    code, why = verdict(r.returncode, r.stdout)
    if code == 8:
        print('sortie refusee par le juge (code sonde %d) : %s' % (r.returncode, '; '.join(why)))
    return finish(code, 'probe', why, r.stdout, r.stderr)


# --- auto-test : juge et chemins d'echec simules (aucun processus CUDA) -------------------------------------------

def _full(**over):
    rec = {'status': 'ok', 'device': 'NVIDIA RTX PRO 6000 Blackwell', 'cc': '12.0', 'sms': 188, 'global_mem_gib': 94.9,
           'cuda_error': 'no error', 'i128_pairs': PAIRS, 'i128_equal_cases': 2011255, 'i128_mismatches': 0,
           'timing_ok': True, 'loop64_gops': 2579.1, 'loop128_gops': 1111.2, 'i128_over_i64': 2.32, 'h2d_gbps': 57.1,
           'd2h_gbps': 55.0, 'launch_sync_us': 5.1, 'launch_async_us': 1.86, 'loops': 'modular_unsigned'}
    rec.update(over)
    return json.dumps(rec)


def _self_test():
    zero = {k: 0 for k in RATES}
    cases = [
        # (nom, code du processus, sortie brute, code attendu)
        ('complet_ok', 0, _full(), 0),
        ('status_seul', 0, '{"status":"ok"}', 8),
        ('timing_faux', 0, _full(timing_ok=False), 8),
        ('desaccord_positif', 0, _full(i128_mismatches=1), 8),
        ('debit_nan', 0, _full().replace('2579.1', 'NaN'), 8),
        ('debit_infini', 0, _full().replace('2579.1', 'Infinity'), 8),
        ('debit_negatif', 0, _full(loop64_gops=-1.0), 8),
        ('debit_nul', 0, _full(h2d_gbps=0.0), 8),
        ('statut_duplique', 0, '{"status":"cuda_error","status":"ok"}', 8),
        ('statut_duplique_complet', 0, _full()[:-1] + ',"status":"ok"}', 8),
        ('timing_invalide_avec_debit', 5, '{"status":"timing_invalid","rate":1000}', 8),
        ('timing_invalide_complet', 5, _full(status='timing_invalid', timing_ok=False, **zero), 5),
        ('timing_invalide_debit_restant', 5, _full(status='timing_invalid', timing_ok=False), 8),
        ('mauvais_status', 0, '{"status":"cuda_error"}', 8),
        ('mauvais_code', 3, '{"status":"ok"}', 8),
        ('deux_lignes', 0, '{"status":"ok"}\n{"status":"ok"}', 8),
        ('champ_manquant', 0, json.dumps({k: v for k, v in json.loads(_full()).items() if k != 'd2h_gbps'}), 8),
        ('champ_en_trop', 0, _full(rate=1.0), 8),
        ('type_booleen_pour_entier', 0, _full(i128_mismatches=False), 8),
        ('paires_fausses', 0, _full(i128_pairs=1000), 8),
        ('egalites_absentes', 0, _full(i128_equal_cases=0), 8),
        ('egalites_incompletes', 0, _full(i128_equal_cases=FORCED_EQUAL - 1), 8),
        ('boucles_signees', 0, _full(loops='signed'), 8),
        ('erreur_cuda_masquee', 0, _full(cuda_error='unspecified launch failure'), 8),
        ('desaccord_declare', 1, _full(status='i128_mismatch', i128_mismatches=3), 1),
        ('desaccord_sans_compte', 1, _full(status='i128_mismatch'), 8),
        ('no_cuda', 2, '{"status":"no_cuda"}', 2),
        ('no_cuda_bavard', 2, '{"status":"no_cuda","device":"x"}', 8),
        ('ck_erreur', 3, '{"status":"cuda_error","call":"cudaMalloc(&da, n * 8)","error":"out of memory"}', 3),
        ('ck_erreur_vide', 3, '{"status":"cuda_error","call":"","error":"x"}', 8),
        ('code_inconnu', 4, '{"status":"ok"}', 8),
        ('json_liste', 0, '[1]', 8),
        ('bruit', 0, 'bruit', 8),
    ]
    bad = []
    for name, code, raw, want in cases:
        got = check_output(code, raw)
        if got != want:
            bad.append((name, got, want, verdict(code, raw)[1]))
    nsim, sim_fails = _simulate_paths()
    bad.extend(sim_fails)
    if FORCED_EQUAL != 2011255:
        bad.append('egalites forcees : %d au lieu de 2011255' % FORCED_EQUAL)
    print(json.dumps({'self_test': 'ok' if not bad else 'ECHEC', 'cas_juge': len(cases), 'simulations': nsim,
                      'echecs': [str(b) for b in bad]}, ensure_ascii=False))
    return 0 if not bad else 1


def _simulate_paths():
    """Chemins d'echec simules (unittest.mock) : dossier non neuf, nvcc absent, compilation, delai, signal, juge."""
    import contextlib
    import io
    import tempfile
    from unittest import mock
    fails = []
    exe_out = _full()
    # (nom, code attendu, fichiers exactement presents dans --out apres la tentative)
    paths = (('dossier_non_neuf', 9, {'cuda_probe.json'}),
             ('nvcc_absent', 4, {'attempt.json'}),
             ('compilation', 6, {'attempt.json', 'compile.txt'}),
             ('delai', 7, {'attempt.json', 'compile.txt', 'probe_stdout.txt', 'probe_stderr.txt'}),
             ('signal', 7, {'attempt.json', 'compile.txt', 'cuda_probe.json', 'probe_stdout.txt', 'probe_stderr.txt'}),
             ('juge_refuse', 8, {'attempt.json', 'compile.txt', 'cuda_probe.json'}),
             ('succes', 0, {'attempt.json', 'compile.txt', 'cuda_probe.json'}))
    for name, want, files_expected in paths:
        with tempfile.TemporaryDirectory() as tmp:
            out, work = os.path.join(tmp, 'out'), os.path.join(tmp, 'work')
            if name == 'dossier_non_neuf':
                os.makedirs(out)
                with open(os.path.join(out, 'cuda_probe.json'), 'w') as f:
                    f.write('{"status":"ok"}\n')
            compiled = subprocess.CompletedProcess(['nvcc'], 1 if name == 'compilation' else 0, 'c_out', 'c_err')
            if name == 'delai':
                probe = subprocess.TimeoutExpired(['probe'], 600, output=b'{"status":', stderr=b'partiel')
            elif name == 'signal':
                probe = subprocess.CompletedProcess(['probe'], -15, 'partiel', 'sig')
            elif name == 'juge_refuse':
                probe = subprocess.CompletedProcess(['probe'], 0, '{"status":"ok"}\n', '')
            else:
                probe = subprocess.CompletedProcess(['probe'], 0, exe_out + '\n', '')
            run = mock.Mock(side_effect=[compiled, probe])
            real_exists = os.path.exists

            def exists(p, _name=name):
                if p.endswith('/bin/nvcc'):
                    return _name != 'nvcc_absent'
                return real_exists(p)
            argv = ['cuda_probe.py', '--out', out, '--work', work]
            with mock.patch.object(sys, 'argv', argv), mock.patch.object(os.path, 'exists', exists), \
                    mock.patch.object(subprocess, 'run', run), contextlib.redirect_stdout(io.StringIO()):
                got = main()
            present = set(os.listdir(out)) if real_exists(out) else set()
            if got != want:
                fails.append('%s : code %r au lieu de %r' % (name, got, want))
            if present != files_expected:
                fails.append('%s : fichiers %s au lieu de %s' % (name, sorted(present), sorted(files_expected)))
            if 'attempt.json' in present:
                with open(os.path.join(out, 'attempt.json')) as f:
                    rec = json.load(f)
                if rec.get('code') != got:
                    fails.append('%s : attempt.json porte le code %r au lieu de %r' % (name, rec.get('code'), got))
            if name == 'delai' and real_exists(os.path.join(out, 'probe_stdout.txt')):
                with open(os.path.join(out, 'probe_stdout.txt')) as f:
                    if f.read() != '{"status":':
                        fails.append('delai : sortie partielle perdue')
    return len(paths), fails


if __name__ == '__main__':
    if '--self-test' in sys.argv:
        sys.exit(_self_test())
    sys.exit(main())
