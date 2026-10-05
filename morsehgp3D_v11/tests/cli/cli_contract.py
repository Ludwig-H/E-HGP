"""Porte mhgp11_cli_contract (tranche S5) : contrat de l'executable mhgp11, refus dans l'ordre du paragraphe 3 de
docs/SORTIES.md.

    python3 cli_contract.py --cli <mhgp11> --bits <18|21|24> [--preload <bibliotheque>]
            [--fault-preload <bibliotheque>]

Des temoins (le meme appel sans la faute : code 0, dossier conforme, ligne de succes published_complete) encadrent les
cas. Chaque refus exige le code exact (2 refus, 3 invariant), exactement une ligne JSON (sur la sortie standard, ou sur
la sortie d'erreur si la sortie standard est fermee, en lecture seule, designe une entree, ou echoue) avec statut,
raison, etape, output (null a l'etape des options, "full" ensuite) et l'etat du dossier : publication "none" et
manifest_sha256 null, sauf dans les doubles echecs, ou publication "published_complete" et l'empreinte du manifeste de
D, publie et complet. Ni dossier ni D.pending crees hors des doubles echecs (un dossier ou un D.pending preexistants
restent intacts), entrees inchangees. Les cas a deux fautes fixent l'ordre des etapes (options avant plan, sortie
standard avant dossier, plan avant lecture, positions repetees avant K > n). Apres la publication : sortie standard
pleine (/dev/full) ou tube sans lecteur, le dossier est retire ; tube plein, puis D.pending cree des que D apparait
(le retrait est alors refuse), sans crochet : published_complete. Limite de taille de fichier (RLIMIT_FSIZE, SIGXFSZ
par defaut) : refus a l'etape publish, sans arret par signal ni D.pending orphelin. Bibliotheques prechargees
(absentes sous sanitizer) : exception flottante demasquee (environment_selftest, code 3) ; synchronisation du parent
de D puis renommages de D refuses (io_fault_preload.cpp) : commit en double echec, retrait reussi (none) ou refuse
(published_complete).
Codes : 0 conforme, 1 ecart, 3 plancher (moins de 20 refus).
"""
import argparse
import fcntl
import os
import resource
import shutil
import signal
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cli_support as cs  # noqa: E402

mhgp11_gate = cs.mhgp11_gate
formats = cs.formats

STATUS = {'parameter_out_of_range': 'invalid_input', 'output_conflict': 'invalid_input',
          'output_unwritable': 'resource_exhausted', 'input_unreadable': 'invalid_input',
          'memory_budget': 'resource_exhausted', 'empty_input': 'invalid_input',
          'coordinate_out_of_domain': 'invalid_input', 'duplicate_point_id': 'invalid_input',
          'multiplicity_unsupported': 'unsupported_degeneracy', 'environment_selftest': 'invariant_violated'}
# Forme exacte des deux lignes de la sortie standard (cles et ordre), fixee en S5 (docs/SORTIES.md, paragraphe 3).
REFUSAL_KEYS = ['phase', 'output', 'status', 'reason', 'stage', 'coord_bits', 'publication', 'manifest_sha256']
SUCCESS_KEYS = ['phase', 'output', 'status', 'reason', 'coord_bits', 'k', 'workers', 'sites', 'stages_ns',
                'peaks_bytes', 'counts', 'publication', 'manifest_sha256']
SUCCESS_PARTS = dict(stages_ns=['cloud', 'index', 'domain', 'tree', 'attach', 'output', 'write', 'total'],
                     peaks_bytes=['cloud', 'index', 'domain', 'tree', 'output', 'write'],
                     counts=['nodes', 'births', 'edges'])


class Contract:
    def __init__(self, args, gate, root):
        self.args, self.gate, self.root = args, gate, root
        self.refusals = self.witnesses = 0
        self.points = [(0, 0, 0), (2, 0, 0), (4, 0, 0), (1, 3, 0), (7, 7, 7)]
        self.ids = [(1 << 32) - 1, 7, 99, 0, 12345]

    def fresh(self, points=None, ids=None):
        """Dossier de cas : entrees ecrites, D a creer ; rend (dossier, points, ids, D)."""
        folder = tempfile.mkdtemp(prefix='c', dir=self.root)
        xyz, names = cs.write_inputs(folder, self.points if points is None else points,
                                     self.ids if ids is None else ids)
        return folder, xyz, names, os.path.join(folder, 'D')

    def argv(self, xyz, names, directory, k=3, extra=()):
        return cs.cli_argv(self.args.cli, xyz, names, directory, k, None, extra)

    def witness(self, argv, directory):
        result, rows = cs.run_cli(argv, timeout=120)
        line = rows[0] if len(rows) == 1 and rows[0] else {}
        ok = result.code == 0 and line.get('status') == 'ok' and line.get('publication') == 'published_complete'
        ok = ok and list(line) == SUCCESS_KEYS and all(type(line[key]) is dict and list(line[key]) == names and
                                                       all(type(v) is int for v in line[key].values())
                                                       for key, names in SUCCESS_PARTS.items())
        try:
            report = formats.check_directory(directory, self.args.bits)
            ok = ok and line.get('manifest_sha256') == report['manifest_sha256']
        except (OSError, ValueError) as error:
            ok = self.gate.check(False, 'temoin : dossier non conforme : %s' % error)
        self.gate.check(ok, 'temoin %r : %s %r' % (argv[1:], result.describe(), (result.stderr or '')[-300:]))
        shutil.rmtree(directory, ignore_errors=True)
        self.witnesses += 1

    def refuse(self, name, argv, reason, stage, absent=(), present=(), inputs=(), code=2, on_stderr=False,
               run=None, publication='none', manifest=None):
        """Joue un refus : code exact, une ligne JSON (statut, raison, etape, output, etat du dossier), chemins
        `absent` absents (D et D.pending), chemins `present` intacts (D ou orphelin preexistants, D publie d'un double
        echec), entrees inchangees. `manifest` : empreinte attendue du manifeste publie, ou fonction qui la lit apres
        l'appel ; None pour la publication none."""
        before = [cs.file_sha(path) for path in inputs]
        result = run(argv) if run is not None else mhgp11_gate.run(argv, timeout=120)
        rows = cs.json_lines(result.stderr if on_stderr else result.stdout)
        if on_stderr:  # la sortie d'erreur porte aussi le message en francais
            rows = [row for row in rows if row is not None]
        expected = manifest() if callable(manifest) else manifest
        want = dict(phase='mhgp11', output=None if stage == 'options' else 'full', status=STATUS[reason],
                    reason=reason, stage=stage, coord_bits=self.args.bits, publication=publication,
                    manifest_sha256=expected)
        line = rows[0] if len(rows) == 1 and rows[0] else {}
        self.gate.check(publication == 'none' or expected is not None, '%s : empreinte attendue absente' % name)
        self.gate.check(result.code == code, '%s : %s, attendu code %d' % (name, result.describe(), code))
        self.gate.check({key: line.get(key) for key in want} == want, '%s : lignes %r, attendu %r' % (name, rows, want))
        self.gate.check(list(line) == REFUSAL_KEYS, '%s : cles de la ligne de refus %r' % (name, list(line)))
        for path in absent:
            self.gate.check(not os.path.lexists(path), '%s : %s present apres le refus' % (name, path))
        for path in present:
            self.gate.check(os.path.lexists(path), '%s : %s retire par le refus' % (name, path))
        self.gate.check([cs.file_sha(path) for path in inputs] == before, '%s : entree modifiee' % name)
        self.refusals += 1


def option_cases(c):
    """Etape 1 : refus parameter_out_of_range avant tout effet (ni lecture, ni dossier)."""
    folder, xyz, names, d = c.fresh()
    c.witness(c.argv(xyz, names, d), d)
    base = c.argv(xyz, names, d)

    def swap(prefix, value):
        return [a for a in base if not a.startswith(prefix)] + ([value] if value else [])

    cases = [
        ('sans argument', [c.args.cli]),
        ('positionnel', base + ['full']),
        ('sans --sortie', swap('--sortie=', None)),
        ('option inconnue', base + ['--sortiee=full']),
        ('option repetee', base + ['--k=3']),
        ('sans signe egal', base + ['--fils']),
        ('mcs avec supports', swap('--sortie=', '--sortie=supports') + ['--mcs=20']),
        ('z avec points', swap('--sortie=', '--sortie=points') + ['--z=2']),
        ('sortie plat', swap('--sortie=', '--sortie=plat')),
        ('sortie inconnue', swap('--sortie=', '--sortie=FULL')),
        ('mcs hors plat', base + ['--mcs=20']),
        ('z hors plat', base + ['--z=1']),
        ('selection hors plat', base + ['--selection=eom']),
        ('sans --points', swap('--points=', None)),
        ('sans --dossier', swap('--dossier=', None)),
        ('chemin vide', swap('--points=', '--points=')),
        ('k nul', swap('--k=', '--k=0')),
        ('k 13', swap('--k=', '--k=13')),
        ('k signe', swap('--k=', '--k=-1')),
        ('k texte', swap('--k=', '--k=trois')),
        ('fils nul', base + ['--fils=0']),
        ('fils 257', base + ['--fils=257']),
        ('budget nul', base + ['--budget=0']),
        ('budget deborde', base + ['--budget=18446744073709551616']),
        ('pas exposant', base + ['--pas=1e-3']),
        ('pas nul', base + ['--pas=0.000']),
        ('origine a deux axes', base + ['--origine=1,2']),
        ('origine non decimale', base + ['--origine=1,2,x']),
    ]
    for name, argv in cases:
        c.refuse(name, argv, 'parameter_out_of_range', 'options', absent=(d, d + '.pending'), inputs=(xyz, names))
    os.mkdir(d)  # deux fautes : l'option l'emporte sur le conflit de dossier
    c.refuse('option avant dossier', base + ['--fils=0'], 'parameter_out_of_range', 'options',
             absent=(d + '.pending',), present=(d,), inputs=(xyz, names))
    os.rmdir(d)


def plan_cases(c):
    """Etape 2 : sortie standard puis dossier, avant toute lecture."""
    folder, xyz, names, d = c.fresh()
    both = (xyz, names)
    os.mkdir(d)
    c.refuse('D existe', c.argv(xyz, names, d), 'output_conflict', 'plan', absent=(d + '.pending',), present=(d,),
             inputs=both)
    os.rmdir(d)
    os.mkdir(d + '.pending')
    orphan = os.path.join(d + '.pending', 'orphelin')
    with open(orphan, 'w') as handle:
        handle.write('x')
    c.refuse('D.pending orphelin', c.argv(xyz, names, d), 'output_conflict', 'plan', absent=(d,), present=(orphan,),
             inputs=both)
    shutil.rmtree(d + '.pending')
    c.refuse('dossier sur une entree', c.argv(xyz, names, xyz), 'output_conflict', 'plan',
             absent=(xyz + '.pending', d), inputs=both)
    missing = os.path.join(folder, 'absent', 'D')
    c.refuse('parent absent', c.argv(xyz, names, missing), 'output_unwritable', 'plan', absent=(missing,),
             inputs=both)
    c.refuse('parent fichier', c.argv(xyz, names, os.path.join(xyz, 'D')), 'output_unwritable', 'plan', absent=(d,),
             inputs=both)
    c.refuse('dossier point', c.argv(xyz, names, os.path.join(folder, '.')), 'parameter_out_of_range', 'plan',
             absent=(d,), inputs=both)
    # Sortie standard ajoutee a l'entree ids : refus output_conflict, ligne sur la sortie d'erreur, ids intact ; avec
    # une option fausse en plus, refus des options, et sa ligne ne va pas non plus dans l'entree.
    with open(names, 'ab') as sink:
        c.refuse('sortie standard sur une entree', c.argv(xyz, names, d), 'output_conflict', 'plan',
                 absent=(d, d + '.pending'), inputs=both, on_stderr=True, run=lambda argv: raw_run(argv, sink))
        c.refuse('option fausse, sortie standard sur une entree', c.argv(xyz, names, d) + ['--fils=0'],
                 'parameter_out_of_range', 'options', absent=(d, d + '.pending'), inputs=both, on_stderr=True,
                 run=lambda argv: raw_run(argv, sink))
    c.refuse('sortie standard fermee', c.argv(xyz, names, d), 'output_unwritable', 'plan', absent=(d, d + '.pending'),
             inputs=both, on_stderr=True, run=lambda argv: raw_run(argv, None))
    # Deux fautes : sortie standard fermee et D existant ; la sortie standard est jugee avant le dossier.
    os.mkdir(d)
    c.refuse('sortie standard fermee et D existant', c.argv(xyz, names, d), 'output_unwritable', 'plan',
             absent=(d + '.pending',), present=(d,), inputs=both, on_stderr=True, run=lambda argv: raw_run(argv, None))
    os.rmdir(d)
    # Sortie standard ouverte en lecture seule (fichier qui n'est pas une entree) : refus avant tout calcul.
    target = os.path.join(folder, 'lecture_seule.txt')
    with open(target, 'wb') as handle:
        handle.write(b'temoin\n')
    with open(target, 'rb') as sink:
        c.refuse('sortie standard en lecture seule', c.argv(xyz, names, d), 'output_unwritable', 'plan',
                 absent=(d, d + '.pending'), inputs=both + (target,), on_stderr=True,
                 run=lambda argv: raw_run(argv, sink))
    # Sortie standard ajoutee a l'entree points, puis entree ids designee par un lien symbolique vers le fichier de la
    # sortie standard (meme inode une fois le lien suivi) : output_conflict, entrees intactes.
    with open(xyz, 'ab') as sink:
        c.refuse('sortie standard sur l\'entree points', c.argv(xyz, names, d), 'output_conflict', 'plan',
                 absent=(d, d + '.pending'), inputs=both, on_stderr=True, run=lambda argv: raw_run(argv, sink))
    link = os.path.join(folder, 'ids_lien.u32le')
    os.symlink(names, link)
    with open(names, 'ab') as sink:
        c.refuse('entree par lien symbolique vers la sortie standard', c.argv(xyz, link, d), 'output_conflict', 'plan',
                 absent=(d, d + '.pending'), inputs=both, on_stderr=True, run=lambda argv: raw_run(argv, sink))
    # Deux fautes : points illisibles et dossier existant ; le plan precede la lecture.
    c.refuse('plan avant lecture', c.argv(os.path.join(folder, 'absent'), names, xyz), 'output_conflict', 'plan',
             absent=(xyz + '.pending',), inputs=both)


def completed(returncode, stdout, stderr):
    """Issue d'un processus fini : un arret par signal n'est jamais un code."""
    out = (stdout or b'').decode('utf-8', 'backslashreplace')
    err = (stderr or b'').decode('utf-8', 'backslashreplace')
    if returncode < 0:
        return mhgp11_gate.Completed(None, -returncode, False, out, err)
    return mhgp11_gate.Completed(returncode, 0, False, out, err)


def raw_run(argv, sink):
    """Lance argv avec la sortie standard sur `sink` (fichier ouvert) ou fermee (None)."""
    if sink is None:
        argv = ['/bin/sh', '-c', 'exec "$0" "$@" >&-'] + argv
    try:
        done = subprocess.run(argv, stdout=sink, stderr=subprocess.PIPE, stdin=subprocess.DEVNULL, timeout=120,
                              check=False)
    except subprocess.TimeoutExpired:
        return mhgp11_gate.Completed(None, 0, True, '', '')
    return completed(done.returncode, b'', done.stderr)


def file_size_limited_run(argv):
    """Lance argv sous RLIMIT_FSIZE de 2 048 octets, SIGXFSZ remis a sa disposition par defaut dans l'enfant : seul le
    CLI peut l'ignorer. Le fichier full du nuage du contrat depasse la limite."""
    def limit():
        signal.signal(signal.SIGXFSZ, signal.SIG_DFL)
        _, hard = resource.getrlimit(resource.RLIMIT_FSIZE)
        resource.setrlimit(resource.RLIMIT_FSIZE, (2048, hard))
    try:
        done = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.DEVNULL,
                              timeout=120, check=False, preexec_fn=limit)
    except subprocess.TimeoutExpired:
        return mhgp11_gate.Completed(None, 0, True, '', '')
    return completed(done.returncode, done.stdout, done.stderr)


def full_pipe_run(argv, directory):
    """Double echec apres publication, sans crochet : la sortie standard est un tube PLEIN, la ligne de succes bloque ;
    des que D apparait, D.pending est cree (le retrait, renommage de D en D.pending sans remplacement, sera refuse),
    puis le lecteur est ferme : l'ecriture rend EPIPE (SIGPIPE ignore par le CLI)."""
    reader, writer = os.pipe()
    flags = fcntl.fcntl(writer, fcntl.F_GETFL)
    fcntl.fcntl(writer, fcntl.F_SETFL, flags | os.O_NONBLOCK)
    try:
        while True:
            os.write(writer, b'x' * 4096)
    except BlockingIOError:
        pass
    fcntl.fcntl(writer, fcntl.F_SETFL, flags)  # le CLI ecrit en mode bloquant
    try:
        child = subprocess.Popen(argv, stdout=writer, stderr=subprocess.PIPE, stdin=subprocess.DEVNULL)
    finally:
        os.close(writer)
    deadline = time.monotonic() + 120
    while not os.path.isdir(directory) and child.poll() is None and time.monotonic() < deadline:
        time.sleep(0.002)
    if os.path.isdir(directory):
        os.mkdir(directory + '.pending')
    os.close(reader)
    try:
        _, err = child.communicate(timeout=120)
    except subprocess.TimeoutExpired:
        child.kill()
        child.communicate()
        return mhgp11_gate.Completed(None, 0, True, '', '')
    return completed(child.returncode, b'', err)


def read_and_compute_cases(c):
    """Etapes 3 a 7 : budget, lecture, nuage, positions repetees, K > n, calcul."""
    bits = c.args.bits
    folder, xyz, names, d = c.fresh()
    gone = (d, d + '.pending')
    c.refuse('budget a la lecture', c.argv(xyz, names, d, extra=['--budget=1']), 'memory_budget', 'read', absent=gone,
             inputs=(xyz, names))
    c.refuse('points absents', c.argv(os.path.join(folder, 'absent'), names, d), 'input_unreadable', 'read',
             absent=gone)
    c.refuse('points dossier', c.argv(folder, names, d), 'input_unreadable', 'read', absent=gone)
    truncated = os.path.join(folder, 'tronque.u32le')
    with open(xyz, 'rb') as source, open(truncated, 'wb') as target:
        target.write(source.read()[:-1])
    c.refuse('points tronques', c.argv(truncated, names, d), 'input_unreadable', 'read', absent=gone,
             inputs=(truncated, names))
    # Deux fautes : tailles incoherentes et budget trop petit ; les tailles sont jugees avant l'admission des 16 octets
    # par point (paragraphe 3, etape 4).
    c.refuse('tailles incoherentes avant le budget', c.argv(truncated, names, d, extra=['--budget=1']),
             'input_unreadable', 'read', absent=gone, inputs=(truncated, names))
    c.refuse('nombres differents', c.argv(xyz, truncated, d), 'input_unreadable', 'read', absent=gone,
             inputs=(xyz, truncated))
    empty = os.path.join(folder, 'vide.u32le')
    open(empty, 'wb').close()
    c.refuse('nuage vide', c.argv(empty, empty, d), 'empty_input', 'compute', absent=gone)
    grid = [(i % 7, i // 7 % 7, i // 49) for i in range(60)]
    cases = [('hors domaine', [(1 << bits, 0, 0)] + c.points[1:], c.ids, 3, [], 'coordinate_out_of_domain'),
             ('identifiant double', c.points, [7, 7, 99, 0, 1], 3, [], 'duplicate_point_id'),
             ('positions repetees', [(0, 0, 0), (0, 0, 0)] + c.points[2:], c.ids, 3, [], 'multiplicity_unsupported'),
             ('repetees et K > sites', [(0, 0, 0), (0, 0, 0), (0, 0, 0), (4, 0, 0), (2, 0, 0)], c.ids, 4, [],
              'multiplicity_unsupported'),
             ('K > n', c.points, c.ids, len(c.points) + 1, [], 'parameter_out_of_range'),
             ('budget au calcul', grid, list(range(60)), 3, ['--budget=%d' % (16 * 60 + 64)], 'memory_budget')]
    for name, points, ids, k, extra, reason in cases:
        _, p, q, target = c.fresh(points, ids)
        c.refuse(name, c.argv(p, q, target, k, extra), reason, 'compute', absent=(target, target + '.pending'),
                 inputs=(p, q))


def session_cases(c, preload):
    """Etape 3 : exception flottante demasquee avant main (bibliotheque prechargee) : environment_selftest, code 3."""
    if not preload:
        return
    folder, xyz, names, d = c.fresh()
    env = dict(os.environ, LD_PRELOAD=preload)
    c.refuse('exception flottante demasquee', c.argv(xyz, names, d), 'environment_selftest', 'session',
             absent=(d, d + '.pending'), inputs=(xyz, names), code=3,
             run=lambda argv: mhgp11_gate.run(argv, timeout=120, env=env))


def provenance_witness(c):
    """Temoin : --pas, --origine et --budget recopies tels quels dans le manifeste (paragraphe 6.6), --fils dans la
    seule ligne standard."""
    folder, xyz, names, d = c.fresh()
    argv = c.argv(xyz, names, d, extra=['--pas=0.001', '--origine=-79.602,79.917,0', '--budget=4000000000',
                                        '--fils=3'])
    result, rows = cs.run_cli(argv, timeout=120)
    try:
        manifest = formats.check_directory(d, c.args.bits)['manifest']
    except (OSError, ValueError) as error:
        manifest = None
        c.gate.check(False, 'temoin de provenance : dossier non conforme : %s' % error)
    line = rows[0] if len(rows) == 1 and rows[0] else {}
    c.gate.check(result.code == 0 and line.get('workers') == 3, 'temoin de provenance : %s %r' % (result.describe(),
                                                                                                   line))
    c.gate.check(manifest is not None and manifest['parameters'] ==
                 dict(budget_bytes=4000000000, grid_step='0.001', origin=['-79.602', '79.917', '0']),
                 'temoin de provenance : parametres %r' % (manifest or {}).get('parameters'))
    c.witnesses += 1


def after_commit_cases(c):
    """Etape 9 et limite de taille : la ligne de la sortie standard echoue apres la publication, le dossier publie est
    retire (none) ; si le retrait echoue aussi, D reste publie et complet (published_complete et l'empreinte de son
    manifeste). Un refus d'options sur une sortie standard pleine va sur la sortie d'erreur. RLIMIT_FSIZE : refus a
    l'ecriture (SIGXFSZ ignore par le CLI), ni D ni D.pending."""
    folder, xyz, names, d = c.fresh()
    c.witness(c.argv(xyz, names, d), d)
    gone = (d, d + '.pending')
    with open('/dev/full', 'wb') as full:
        c.refuse('sortie standard pleine', c.argv(xyz, names, d), 'output_unwritable', 'report', absent=gone,
                 inputs=(xyz, names), on_stderr=True, run=lambda argv: raw_run(argv, full))
        c.refuse('option fausse, sortie standard pleine', c.argv(xyz, names, d) + ['--fils=0'],
                 'parameter_out_of_range', 'options', absent=gone, inputs=(xyz, names), on_stderr=True,
                 run=lambda argv: raw_run(argv, full))
    reader, writer = os.pipe()
    os.close(reader)  # aucun lecteur des le depart : l'ecriture rend EPIPE (SIGPIPE ignore par le CLI)
    with os.fdopen(writer, 'wb') as pipe:
        c.refuse('tube sans lecteur', c.argv(xyz, names, d), 'output_unwritable', 'report', absent=gone,
                 inputs=(xyz, names), on_stderr=True, run=lambda argv: raw_run(argv, pipe))
    manifest = os.path.join(d, formats.MANIFEST)
    c.refuse('tube plein puis D.pending', c.argv(xyz, names, d), 'output_unwritable', 'report',
             present=(d, d + '.pending'), inputs=(xyz, names), on_stderr=True, publication='published_complete',
             manifest=lambda: cs.file_sha(manifest) if os.path.exists(manifest) else None,
             run=lambda argv: full_pipe_run(argv, d))
    if os.path.isdir(d + '.pending'):
        os.rmdir(d + '.pending')  # cree par la porte, vide : le CLI ne retire jamais un D.pending qu'il n'a pas cree
    try:
        formats.check_directory(d, c.args.bits)
    except (OSError, ValueError) as error:
        c.gate.check(False, 'tube plein puis D.pending : D publie non conforme : %s' % error)
    shutil.rmtree(d, ignore_errors=True)
    c.refuse('limite de taille de fichier', c.argv(xyz, names, d), 'output_unwritable', 'publish', absent=gone,
             inputs=(xyz, names), run=file_size_limited_run)


INVALID_HARNESS = 'mhgp11_io_fault_preload_invalide'


def fault_run(c, name, argv, env):
    """Appel sous io_fault_preload : la bibliotheque ne decode que SYS_renameat2 (cinq arguments types) ; tout autre
    numero de syscall rend la porte invalide (jeton sur la sortie d'erreur, arret du processus), nomme ici."""
    result = mhgp11_gate.run(argv, timeout=120, env=env)
    c.gate.check(INVALID_HARNESS not in (result.stderr or ''),
                 '%s : porte INVALIDE, io_fault_preload a recu un syscall qu\'il ne sait pas decoder : %s'
                 % (name, (result.stderr or '').strip()[-200:]))
    return result


def publish_fault_cases(c, preload):
    """Etape 8, commit en double echec (bibliotheque prechargee io_fault_preload) : la synchronisation du parent de D
    echoue apres le renommage, puis le retour de D en D.pending est refuse ; le retrait de l'api le tente encore.
    Retrait reussi : none, ni D ni D.pending. Retrait refuse : published_complete, D publie, complet et conforme, et
    l'empreinte de son manifeste sur la ligne de refus. Un syscall autre que renameat2 invalide la porte (fault_run)."""
    if not preload:
        return
    for name, refused_renames in (('double echec du commit, retrait reussi', 1),
                                  ('double echec du commit, retrait refuse', 2)):
        folder, xyz, names, d = c.fresh()
        env = dict(os.environ, LD_PRELOAD=preload, MHGP11_FAULT_PARENT=folder, MHGP11_FAULT_NAME='D',
                   MHGP11_FAULT_RENAME_BACK=str(refused_renames))
        kept = refused_renames >= 2
        manifest = os.path.join(d, formats.MANIFEST)
        c.refuse(name, c.argv(xyz, names, d), 'output_unwritable', 'publish',
                 absent=(d + '.pending',) if kept else (d, d + '.pending'), present=(d,) if kept else (),
                 inputs=(xyz, names), publication='published_complete' if kept else 'none',
                 manifest=(lambda: cs.file_sha(manifest) if os.path.exists(manifest) else None) if kept else None,
                 run=lambda argv, env=env, name=name: fault_run(c, name, argv, env))
        if kept:
            try:
                formats.check_directory(d, c.args.bits)
            except (OSError, ValueError) as error:
                c.gate.check(False, '%s : D publie non conforme : %s' % (name, error))
            shutil.rmtree(d, ignore_errors=True)


def main():
    parser = argparse.ArgumentParser(description='Contrat de l\'executable mhgp11 (refus, ordre, transaction).')
    parser.add_argument('--cli', required=True)
    parser.add_argument('--bits', type=int, choices=(18, 21, 24), required=True)
    parser.add_argument('--preload', default='')
    parser.add_argument('--fault-preload', default='')
    args = parser.parse_args()
    gate = mhgp11_gate.Gate('cli_contract')
    with tempfile.TemporaryDirectory(prefix='mhgp11-cli-contract-') as root:
        c = Contract(args, gate, root)
        option_cases(c)
        plan_cases(c)
        session_cases(c, args.preload)
        read_and_compute_cases(c)
        provenance_witness(c)
        after_commit_cases(c)
        publish_fault_cases(c, args.fault_preload)
    if gate.failures == 0 and c.refusals >= 20:
        print('cli_contract_verdict conforme refus%d temoins%d' % (c.refusals, c.witnesses))
    return gate.finish(floor=20)


if __name__ == '__main__':
    sys.exit(main())
