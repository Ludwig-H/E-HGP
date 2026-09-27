#!/usr/bin/env python3
# Explicit port of b_q34_resident_g4_readback_20260927 at 03decc16c.
"""Publish/replay closed S2 evidence, without cloud calls or secret exports.

LIVE authority: private source snapshot AND original session artifacts remain
required. Failed sessions remain failed; this is never a FULL validator.
"""
import argparse
from datetime import datetime
import hashlib
import ipaddress
import json
from pathlib import Path
import re
import sys
import statistics

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PROTOCOL = HERE.parent / 'b_q34_survivors_session_20260927'
sys.path.insert(0, str(PROTOCOL))
import common as c
import package
import session
import worker

SCHEMA = 'mhgp9_survivors_g4_publication_v1'
VM_BASE = {'receipt.json', 'guard_evidence.json', 'sources_before.json',
           'sources_after.json', 'compile_before.json', 'compile_after.json'}
SUFFIXES = ('.command.json', '.intent.json', '.stdout', '.stderr')
HOST_COMMANDS = {'oslogin_add', 'guarded_start', 'before_upload', 'guest_schedule',
    'remote_mkdir', 'upload', 'unpack', 'before_worker', 'worker', 'before_retrieve',
    'pack_capture', 'download', 'guarded_stop'}
need, sha, read = c.need, c.sha, c.read


def encode(value):
    return (json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n').encode()


def safe_bytes(path):
    need(path.is_file() and not path.is_symlink(), 'regular evidence file required: ' + str(path))
    raw = path.read_bytes()
    need(b'PRIVATE KEY' not in raw, 'private key marker in evidence')
    raw.decode('utf-8')  # No binary input is accepted by an export allowlist.
    return raw


def redact(raw):
    # Explicit port of the three patterns from publish_closed.py,
    # SHA256 eb4f56273691405c569cbe236531a4f0a385ed01284ac6e3b4118d058230e992.
    # Extended to address literals anywhere in the selected guard logs.
    need(b'PRIVATE KEY' not in raw, 'private key marker in guard log')
    text = raw.decode('utf-8')
    text = re.sub(r'[A-Za-z0-9_.+%-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}', '<account redacted>', text)
    text = re.sub(r'(?:ssh-ed25519|ssh-rsa|ecdsa-sha2-\S+) [A-Za-z0-9+/=]+(?: [^\r\n]*)?',
                  '<public SSH key redacted>', text)
    text = re.sub(r'(Instance (?:internal|external) IP is )[^\r\n]+', r'\1<address redacted>', text)
    def address(match):
        try:
            ipaddress.ip_address(match.group(0))
        except ValueError:
            return match.group(0)
        return '<address redacted>'
    return re.sub(r'(?<![\w:])(?:[0-9]{1,3}\.){3}[0-9]{1,3}(?![\w.])|(?<![\w:])[0-9a-fA-F]*:[0-9a-fA-F:]+(?![\w:])', address, text).encode()


def projection(after):
    # Same explicit allowlist as the historical helper, no metadata/OS Login.
    return {key: after[key] for key in ('name', 'selfLink', 'status', 'lastStartTimestamp', 'lastStopTimestamp',
            'zone', 'machineType', 'labels', 'scheduling') if key in after}


def stop_certificate(after, generation):
    target = c.TARGET
    need(after.get('name') == target['instance'] and after.get('status') == 'TERMINATED' and
         after.get('zone', '').rsplit('/', 1)[-1] == target['zone'] and
         after.get('selfLink', '').endswith('/projects/'+target['project']+'/zones/'+target['zone']+'/instances/'+target['instance']) and
         after.get('lastStartTimestamp') == generation, 'exact stopped generation/target')
    start = datetime.fromisoformat(generation.replace('Z', '+00:00'))
    stop = datetime.fromisoformat(after['lastStopTimestamp'].replace('Z', '+00:00'))
    need(start.tzinfo is not None and stop.tzinfo is not None and stop >= start, 'stop chronology')
    return dict(generation=generation, stopped=after['lastStopTimestamp'], vm_elapsed_seconds=(stop-start).total_seconds(),
        billed_cost_usd=None, reason='Elapsed allocation only; no billing export or verified hourly price')


def inventory(folder):
    need(folder.is_dir() and not folder.is_symlink(), 'publication directory')
    values = {}
    for path in sorted(folder.rglob('*')):
        need(not path.is_symlink(), 'symlink in publication')
        if path.is_file() and path != folder/'SHA256SUMS':
            values[str(path.relative_to(folder))] = sha(path)
    return values


def check_commands(directory, rows, remember, recipe=None):
    need(type(rows) is list, 'command list')
    names = set()
    for row in rows:
        name = row['name']
        need(type(name) is str and re.fullmatch('[a-z0-9_]+', name) and name not in names,
             'unique safe command name')
        names.add(name)
        need(read(directory/(name+'.command.json')) == row, 'stored command binding')
        intent = read(directory/(name+'.intent.json'))
        need(all(row.get(key) == value for key, value in intent.items()), 'command intent binding')
        need(row.get('group_closed') is True and row['ended_epoch'] >= row['started_epoch'], 'command not closed')
        if recipe is not None:
            need(name in recipe and row['argv'] == recipe[name], 'exact remote recipe')
        for suffix in SUFFIXES:
            path = directory/(name+suffix)
            remember(path)
            if suffix in ('.stdout', '.stderr'):
                need(sha(path) == row[suffix[1:]+'_sha256'], 'command raw stream hash')
    if recipe is not None:
        need([row['name'] for row in rows] == list(recipe)[:len(rows)], 'remote commands must be recipe prefix')


def classify(host, manifest, generation, host_receipt, vm_receipt):
    """Never promote a preserved failed host or worker from individual timings."""
    outcome = dict(status='failed', semantic_replay=False, GPU_execution='unqualified_or_unknown')
    if host_receipt.get('status') == 'completed' and vm_receipt and vm_receipt.get('status') == 'completed':
        try:
            result = session.validate_received(host, manifest, generation)
        except (ValueError, KeyError, OSError, TypeError) as error:
            outcome['validation_error'] = type(error).__name__+': '+str(error)
        else:
            outcome.update(status='completed', semantic_replay=True, GPU_execution='qualified_S2',
                           gate=result['gate'], high_gate=result['high_gate'], measures=result['measures'])
    return outcome


def comparisons(measures):
    result = {}
    for width in (4, 48):
        records = {name: [] for name in ('resident_af369c44', 'survivors_6af40d886')}
        first_calls = {}
        for first in ('baseline', 'candidate'):
            case = measures['w'+str(width)+'_'+first]
            for row in case['runs']:
                if row['first_cuda_call']:
                    first_calls[row['implementation']] = row['times_ms']['adapter']
                if not row['first_implementation_call']:
                    records[row['implementation']].append(row)
        need(all(len(rows) == 2 for rows in records.values()) and len(first_calls) == 2,
             'two actually repeated operators per implementation/width')
        medians = {}
        for name, rows in records.items():
            medians[name] = {field: statistics.median(row['times_ms'][field] for row in rows)
                for field in ('adapter', 'adapter_plus_native_release_noncontiguous', 'consume_outer',
                              'open', 'arena', 'order_convert', 'native_output_release')}
            medians[name]['samples'] = 2
        old, new = (medians[name]['adapter'] for name in ('resident_af369c44', 'survivors_6af40d886'))
        need(new > 0, 'nonzero comparative duration')
        result[str(width)] = dict(first_process_cuda_call_adapter_ms=first_calls,
            repeated_operator_medians_ms=medians,
            median_ratio_baseline_over_candidate=old/new)
    return result


def receipt_readme(summary):
    lines = ['# G4 — tri résident des survivants, comparaison S2', '',
        'État : **'+summary['status']+'**. Sources : `'+summary['source_commit']+'`.',
        'Autorité LIVE : snapshot, originaux privés, sorties VM et arrêt de la même génération.',
        'Aucune tour FULL ni nouveau contrat de temps ne sont qualifiés ici.', '']
    if summary['status'] == 'completed':
        value = summary['measures']['w4_baseline']
        masses = value['runs'][0]['masses']
        lines += ['Trame entière sans sol ng00, grille 1 mm/u18, 39 885 sites, K5/s8.',
            'Même sortie ordonnée : P='+str(masses['P'])+', E='+str(masses['E'])+
            ', S='+str(masses['S'])+'.',
            'Quatre processus : ABBA puis BAAB, pour chaque largeur W4 et W48.',
            'Le front CPU W1 et l’oracle W4 sont calculés une fois avant CUDA dans chaque processus.',
            'Chaque opérateur recrée ses propriétaires ; leur fermeture ne réinitialise pas CUDA.',
            'Les lignes suivantes prennent uniquement les deux appels par version qui ne sont',
            'ni le premier appel CUDA du processus, ni le premier appel de cette implémentation.',
            'Ce sont deux observations, pas une estimation statistique robuste.', '',
            '| Arène CPU | Adaptateur témoin médian (ms) | Candidat médian (ms) | Rapport témoin/candidat |',
            '|---|---:|---:|---:|']
        for width in ('4', '48'):
            item=summary['comparisons'][width]; med=item['repeated_operator_medians_ms']
            lines.append('| W'+width+' | '+format(med['resident_af369c44']['adapter'], '.6f')+' | '+
                format(med['survivors_6af40d886']['adapter'], '.6f')+' | '+
                format(item['median_ratio_baseline_over_candidate'], '.4f')+' |')
        lines += ['', '| Arène CPU | Premier CUDA témoin (ms) | Premier CUDA candidat (ms) |',
            '|---|---:|---:|']
        for width in ('4', '48'):
            item=summary['comparisons'][width]['first_process_cuda_call_adapter_ms']
            lines.append('| W'+width+' | '+format(item['resident_af369c44'], '.6f')+' | '+
                format(item['survivors_6af40d886'], '.6f')+' |')
        lines += ['', 'L’adaptateur comprend ouverture, index device, rectangles, arène, consommation',
            'et destruction des propriétaires Prepared/Session/Decision. La sortie S et les',
            'R masques restent retenus ; sa destruction est chronométrée séparément après',
            'la comparaison, avant l’opérateur suivant. La somme publiée avec cette destruction',
            'est donc non contiguë. Aucun coût d’initialisation n’est soustrait.',
            'Lecture, index CPU, front, oracle et diagnostics sont payés dans le harnais,',
            'mais exclus de l’adaptateur. Tous les sous-temps ne sont pas additifs.',
            'Le tri device et le transfert final sont détaillés dans les sorties brutes.',
            'Ce témoin est le prototype résident antérieur, pas le moteur FULL historique.',
            'Ni plusieurs scènes, ni nouvelle mesure de croissance, ni gain FULL ne découlent de ce lot.', '',
            'Porte géométrique CUDA : '+str(summary['gate']['cases'])+' lots / '+
            str(summary['gate']['operators'])+' opérateurs.',
            'Porte collecteur haut-u64 CUDA : '+str(summary['high_gate']['cases'])+' cas / '+
            str(summary['high_gate']['refused'])+' refus ; clés au-delà de 2^32 et 2^63.',
            'La compilation fraîche ferme les options indirectes, dépendances, objets, archives et binaires.',
            'Elle ne réécrit ni ne requalifie rétroactivement les captures antérieures.', '']
    else:
        lines += ['Sorties partielles et erreurs conservées ; aucune mesure favorable ne transforme',
            'une session en échec en campagne qualifiée.', '']
    lines += ['Allocation observée : '+format(summary['cost']['vm_elapsed_seconds'], '.3f')+
        ' s ; montant facturé non estimé.', '',
        'Relecture LIVE normale puis avec `-O` :', '', '```sh',
        'python3 -B morsehgp3D_v9/audits/b_q34_survivors_g4_readback_20260927/readback.py --readback CHEMIN_DU_RECU',
        '```', '', 'Le snapshot et les originaux privés liés dans `PRIVATE_LINKS.json` restent nécessaires.',
        '`SHA256SUMS` couvre toutes les pièces sauf lui-même. Aucune archive, clé, donnée KITTI',
        'nouvelle, réponse OS Login brute ou réponse GCE non expurgée n’est publiée.']
    return ('\n'.join(lines)+'\n').encode()

def collect(session_dir, package_dir):
    """Read closed private artifacts; returns a deterministic publication map."""
    session_dir, package_dir = session_dir.absolute(), package_dir.absolute()
    host = session_dir/'full_host'
    need(all(path.is_dir() and not path.is_symlink() for path in (session_dir, package_dir, host)), 'private evidence roots')
    private_pins, public = {}, {}
    def remember(path):
        need(path.is_file() and not path.is_symlink(), 'private evidence nonsymlink')
        private_pins[str(path)] = sha(path)
    def raw(source, name):
        remember(source)
        value = safe_bytes(source)
        # Raw remote/compiler/JSON artifacts may mention key FILE paths, but
        # must never contain a public key body or a private key marker.
        need(not re.search(rb'(?:ssh-ed25519|ssh-rsa|ecdsa-sha2-\S+) [A-Za-z0-9+/=]+', value), 'SSH key body in raw export')
        public[name] = value

    manifest_path = package_dir/'source_manifest.json'
    snapshot = package_dir/'snapshot.tar.gz'
    manifest = read(manifest_path)
    provenance = package.verify_committed(snapshot, manifest)
    metadata = read(package_dir/'PACKAGE.json')
    need(metadata['snapshot_sha256'] == sha(snapshot) and metadata['manifest_sha256'] == sha(manifest_path) and
         metadata['provenance'] == provenance and metadata['worker_sha256'] == manifest[c.PREFIX+'/worker.py'], 'package pins')
    for path in (snapshot, manifest_path, package_dir/'PACKAGE.json'):
        remember(path)
    host_receipt = read(host/'receipt.json')
    need(host_receipt.get('target') == c.TARGET and host_receipt.get('status') in
         ('completed', 'worker_failed', 'failed', 'capture_failed', 'capture_incomplete'), 'closed host status/target')
    need(host_receipt.get('snapshot_sha256') == sha(snapshot) and host_receipt.get('manifest_sha256') == sha(manifest_path) and
         host_receipt.get('worker_sha256') == manifest[c.PREFIX+'/worker.py'] and
         host_receipt.get('controller_sha256') == c.HELPER_PIN, 'host package identity')
    need(sha(host/'snapshot.tar.gz') == sha(snapshot) and sha(host/'source_manifest.json') == sha(manifest_path), 'copied package identity')
    remember(host/'snapshot.tar.gz'); remember(host/'source_manifest.json')
    legacy = c.load_legacy()
    for name, pin in legacy.GUARDS.items():
        remember(host/name)
        need(sha(host/name) == pin, 'private copied guard differs from pinned lifecycle')
    starts = [row for row in host_receipt['commands'] if row['name'] == 'guarded_start']
    for row in starts:
        need(row['argv'] == [str(host/'start_and_verify.sh'), '--yes', '--guest-shutdown-minutes', '30',
            '--handoff-file', str(host/'handoff.json'), '--lifecycle-state-file', str(host/'lifecycle.txt'),
            '--guard-mark-dir', str(host/'guardmarks')], 'exact copied start guard invocation')
    check_commands(host, host_receipt['commands'], remember)
    need({row['name'] for row in host_receipt['commands']} <= HOST_COMMANDS,
         'unknown host command outside pinned lifecycle')
    generation = host_receipt.get('generation')
    if generation:
        need(host_receipt.get('targeted_shutdown_certified') is True, 'targeted stop required before publication')
        stops = [row for row in host_receipt['commands'] if row['name'] == 'guarded_stop']
        need(len(starts) == 1 and len(stops) == 1 and stops[0]['exit_code'] == 0 and
             stops[0]['argv'] == [str(host/'stop_and_verify.sh'), '--yes', '--expected-last-start-timestamp', generation],
             'versioned copied stop invocation')
        after = read(session_dir/'after_stop.json'); remember(session_dir/'after_stop.json')
        cost = stop_certificate(after, generation)
        public['host/after_stop.projected.json'] = encode(projection(after))
    else:
        need(host_receipt.get('no_start_lifecycle_created') is True and
             not host_receipt.get('targeted_shutdown_certified'), 'explicit no-start required without generation')
        cost = dict(vm_elapsed_seconds=0, billed_cost_usd=None, reason='No new lifecycle was created')
    for source, target in ((host/'receipt.json', 'host/receipt.json'), (manifest_path, 'source_manifest.json'),
                           (package_dir/'PACKAGE.json', 'PACKAGE.json')):
        raw(source, target)
    public['provenance.json'] = encode(provenance)
    for row in host_receipt['commands']:
        for suffix in ('.command.json', '.intent.json'):
            raw(host/(row['name']+suffix), 'host/'+row['name']+suffix)
    redactions = []
    for stem in ('guarded_start', 'guarded_stop', 'guest_schedule', 'worker', 'pack_capture'):
        for suffix in ('.stdout', '.stderr'):
            source = host/(stem+suffix)
            if source.exists():
                remember(source)
                target = 'host/'+stem+'.redacted'+suffix
                public[target] = redact(safe_bytes(source))
                redactions.append(dict(source_path=str(source), source_sha256=sha(source),
                    published_name=target, published_sha256=hashlib.sha256(public[target]).hexdigest()))
    guard = host/'guardmarks/double_guard_verified'
    if guard.exists():
        raw(guard, 'host/double_guard_verified')
    public['PUBLICATION_REDACTIONS.json'] = encode(dict(redactions=redactions,
        after_stop_source_sha256=private_pins.get(str(session_dir/'after_stop.json')),
        policy='Allowlisted text only; no keys, archive, LiDAR payload, OS Login reply, unprojected GCE state, or controller stdout'))

    vm = host/'received/output'
    vm_receipt = read(vm/'receipt.json') if (vm/'receipt.json').is_file() else None
    if vm.exists():
        need(not vm.is_symlink() and not (host/'received').is_symlink(), 'received symlink')
        need(vm_receipt is not None, 'received artifacts without receipt require manual review')
        remote = host_receipt.get('remote_directory', '')
        need(type(remote) is str and re.fullmatch('/tmp/ehgp-full-v7-[0-9a-f]{16}\\.[A-Za-z0-9]{10}', remote), 'remote directory')
        recipes = worker.recipes(Path(remote)/'source', Path(remote)/'survivors_build', c)
        check_commands(vm, vm_receipt['commands'], remember, recipes)
        allowed = VM_BASE | {name+suffix for name in recipes for suffix in SUFFIXES}
        for source in sorted(vm.iterdir()):
            need(source.name in allowed and source.is_file() and not source.is_symlink(), 'unexpected VM file: '+source.name)
            raw(source, 'vm/'+source.name)
    if host_receipt.get('capture_sha256'):
        remember(host/'capture.tar.gz')
        need(sha(host/'capture.tar.gz') == host_receipt['capture_sha256'], 'downloaded capture hash')
    outcome = classify(host, manifest, generation, host_receipt, vm_receipt)
    if outcome['status'] == 'completed':
        need(host_receipt.get('capture_received') is True and host_receipt.get('worker_receipt_present') is True and
             host_receipt.get('worker_exit_code') == 0 and bool(host_receipt.get('capture_sha256')), 'complete retrieved capture identity')
    verdict_path = session_dir/'verdict.json'
    if verdict_path.exists():
        verdict = read(verdict_path)
        need(verdict.get('status') == outcome['status'] or
             (not generation and verdict.get('status') == 'no_start'), 'outer verdict disagreement')
        raw(verdict_path, 'controller_verdict.json')
    summary = dict(schema=SCHEMA, scope='S2_only_no_FULL', FULL_executed=False,
        public_status='not_claimed', contract_certified=False, global_subquadratic_claim=False,
        GCP_calls_by_reader=False, original_host_status=host_receipt['status'],
        original_worker_status=vm_receipt.get('status') if vm_receipt else None,
        host_error=host_receipt.get('error'), capture_error=host_receipt.get('capture_error'),
        worker_error=vm_receipt.get('error') if vm_receipt else None,
        source_commit=provenance['commit'], snapshot_sha256=sha(snapshot), manifest_sha256=sha(manifest_path),
        targeted_shutdown_certified=bool(generation), cost=cost, **outcome)
    if outcome['status'] == 'completed':
        summary['measure_order'] = ['w4_baseline', 'w4_candidate', 'w48_baseline', 'w48_candidate']
        summary['comparisons'] = comparisons(outcome['measures'])
        summary['GPU_baseline_comparison'] = 'paired_resident_prototypes_same_GEOMETRY_not_native_FULL'
        summary['upstream_scope'] = dict(source_commit=provenance['commit'], workers_front=1,
            reference_workers=4, timings_outside_adapter=['input', 'index', 'front', 'reference'])
        summary['timing_scope'] = ('Fresh operator owners in each ABBA/BAAB pass, same CUDA process context; '
            'first_cuda_call only position 0, first_implementation_call positions 0 and 1; '
            'adapter includes own allocations, open, rectangle filter, arena, pair waves, order and owner cleanup, '
            'with S and R output retained. Native output release after comparison is separate and its sum is noncontiguous. '
            'No initialization subtraction or global context destruction. Harness also pays upstream/oracle/diagnostics; not FULL')
    public['SUMMARY.json'] = encode(summary)
    public['README.md'] = receipt_readme(summary)
    need(private_pins == {path: sha(path) for path in private_pins}, 'private evidence changed during read')
    public['PRIVATE_LINKS.json'] = encode(dict(schema=SCHEMA, session_dir=str(session_dir), package_dir=str(package_dir),
        snapshot=str(snapshot), original_files_sha256=private_pins,
        reader_sources_sha256={str(path): sha(path) for path in (HERE/'readback.py',)}))
    for name, value in public.items():
        need(not name.endswith(('.tar.gz', '.bin', '.u32le', '.f32le', '.pub')) and
             b'PRIVATE KEY' not in value and
             not re.search(rb'(?:ssh-ed25519|ssh-rsa|ecdsa-sha2-\S+) [A-Za-z0-9+/=]+', value), 'final secret/payload check')
        # Guard text is redacted. Unexpected identifying literals anywhere else
        # fail closed rather than silently changing canonical JSON/command hashes.
        need(redact(value) == value, 'unredacted account/address/key outside selected guard logs')
    return public, summary


def export(session_dir, package_dir, destination):
    need(not destination.exists() and not destination.is_symlink(), 'fresh publication only')
    public, summary = collect(session_dir, package_dir)
    destination.mkdir(parents=True)
    for name, raw in public.items():
        target = destination/name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as stream:
            stream.write(raw)
    values = inventory(destination)
    with (destination/'SHA256SUMS').open('x') as stream:
        stream.write(''.join(pin+'  '+name+'\n' for name, pin in values.items()))
    replay(destination)
    return summary


def replay(directory):
    inventory_file = directory/'SHA256SUMS'
    expected = {}
    for line in safe_bytes(inventory_file).decode().splitlines():
        pin, name = line.split('  ', 1)
        need(name not in expected and re.fullmatch('[0-9a-f]{64}', pin), 'inventory form')
        expected[name] = pin
    need(expected == inventory(directory), 'published inventory differs')
    links = read(directory/'PRIVATE_LINKS.json')
    need(links['schema'] == SCHEMA and links['reader_sources_sha256'] == {str(HERE/'readback.py'): sha(HERE/'readback.py')}, 'LIVE reader source')
    need(links['original_files_sha256'] == {path: sha(path) for path in links['original_files_sha256']}, 'LIVE original evidence')
    public, summary = collect(Path(links['session_dir']), Path(links['package_dir']))
    need(set(public) == set(expected), 'exact published allowlist')
    for name, raw in public.items():
        need((directory/name).read_bytes() == raw, 'published artifact differs from rederived source: '+name)
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    command = parser.add_mutually_exclusive_group(required=True)
    command.add_argument('--export', type=Path)
    command.add_argument('--readback', type=Path)
    parser.add_argument('--session', type=Path)
    parser.add_argument('--package', type=Path)
    args = parser.parse_args()
    if args.export:
        need(args.session is not None and args.package is not None, 'private session/package paths required')
        summary = export(args.session, args.package, args.export)
    else:
        summary = replay(args.readback)
    print(json.dumps(summary, sort_keys=True))


if __name__ == '__main__':
    main()
