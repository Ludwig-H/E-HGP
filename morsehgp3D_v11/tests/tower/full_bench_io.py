"""Petite porte native du banc FULL, seulement dans la matrice G4, sans experience de performance."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'bench'))
import catalogue_g4 as events
import full_semantic as semantic
from full_campaign import check_order_diagnostics, check_domain_diagnostics, dense, regular, unsigned


def write_inputs(xyz, ids, points, names, order):
    # A permutation iterator (notably reversed(...)) must be consumed exactly once.
    order = tuple(order)
    xyz.write_bytes(b''.join(struct.pack('<III', *points[i]) for i in order))
    ids.write_bytes(b''.join(struct.pack('<I', names[i]) for i in order))


def main():
    executable, bits = Path(sys.argv[1]), int(sys.argv[2])
    points = [(0,0,0),(2,0,0),(4,0,0)]
    names = [2**32-1,7,99]
    attempts, successes, refusals = 0, 0, 0
    with tempfile.TemporaryDirectory(prefix='mhgp11-full-io-') as temporary:
        root = Path(temporary)
        xyz, ids, output = root/'xyz', root/'ids', root/'proof'

        def write(order):
            write_inputs(xyz, ids, points, names, order)

        def child(kmax=3, budget=1 << 28, workers=1, destination=None, optimizations=None):
            nonlocal attempts
            attempts += 1
            output.unlink(missing_ok=True)
            argv = [str(executable),str(xyz),str(ids),str(destination or output),str(kmax),
                    '16','256','0',str(2**32-1),str(budget),str(workers)]
            if optimizations is not None: argv.append(str(optimizations))
            result = subprocess.run(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE, timeout=30, check=False)
            semantic.need(not result.stderr, 'stderr natif : '+result.stderr.decode('utf-8','backslashreplace'))
            rows = [events.event_json(line) for line in result.stdout.decode().splitlines() if line.strip()]
            return result, rows

        hashes, semantic_hashes = [], []
        successes_to_run = [(range(3),1,None),(range(3),1,None),(reversed(range(3)),4,None)]
        tested_modes = tuple(i for i in range(512) if not i & 128 or i & 8) + (512,519,527,639,767,1023) + (
            1024,1031,1035,1151,1279,1280,1535,1544,2043,2047,2048,2051,2055,2063,4095,4103,16379,16383,
            32763,49147)  # voie GPU sur l'hote : feuille source unique, puis lot de feuilles
        successes_to_run += [(range(3),4,mode) for mode in tested_modes]
        for order, workers, optimization in successes_to_run:
            write(order)
            result, rows = child(workers=workers,optimizations=optimization)
            semantic.need(result.returncode == 0, 'FULL natif petit temoin : '+json.dumps(
                dict(attempt=attempts,code=result.returncode,stdout=result.stdout.decode('utf-8','backslashreplace'),
                     stderr=result.stderr.decode('utf-8','backslashreplace')),sort_keys=True))
            semantic.need([r['phase'] for r in rows] == ['cloud','domain','full','exit'], 'phases FULL')
            cloud, domain, full, final = rows
            semantic.need(final == dict(phase='exit',status='ok',reason='none'), 'sortie FULL')
            semantic.need(full['status'] == 'ok' and full['reason'] == 'none' and
                          full['coord_bits'] == bits and full['kmax'] == 3 and full['workers'] == workers,
                          'profil et parametres FULL')
            semantic.need(type(full['optimizations']) is int and full['optimizations'] == (optimization or 0),
                          'mode exact retourne par FULL')
            semantic.need(cloud['sites'] == cloud['points'] == 3 and domain['catalogue_balls'] == 3,
                          'population entiere et catalogue')
            semantic.need(type(cloud['cloud_peak_bytes']) is int and cloud['cloud_peak_bytes'] > 0,
                          'pic preparation conserve avant reset')
            for key in ('wall_ns','index_ns','domain_ns','forest_ns','peak_reserved_bytes','reserved_after_bytes'):
                semantic.need(type(full[key]) is int and full[key] >= 0, 'compteur entier '+key)
            semantic.need(full['wall_ns'] >= sum(full[k] for k in ('index_ns','domain_ns','forest_ns')) and
                          full['peak_reserved_bytes'] >= full['reserved_after_bytes'] > 0, 'chronos/reservations')
            check_domain_diagnostics(domain, full)
            check_order_diagnostics(full, rows[0]['sites'])
            dense.validate(full,cloud['sites'],domain['catalogue_balls'],semantic.need,unsigned)
            regular.validate(full,domain['catalogue_balls'],semantic.need,unsigned)
            semantic.need([o['work']['vertical_reuses'] for o in full['orders']] ==
                          ([0,2,1] if (optimization or 0) & 1024 else [0,0,0]),
                          'ligne024 : toutes les naissances positives sont regulieres')
            semantic.need([o['work']['vertical_descents'] for o in full['orders']] ==
                          ([0,0,0] if (optimization or 0) & 1024 else [0,2,1]),
                          'ligne024 : seules les descentes effectivement lancees sont comptees')
            parsed = semantic.inspect(output,bits,3,3)
            semantic.need((parsed['nodes'],parsed['births'],parsed['merges'],parsed['edges'],parsed['verticals']) ==
                          (8,6,2,5,4), 'structure analytique ligne024')
            semantic.need(parsed['raw_sha256'] == hashlib.sha256(output.read_bytes()).hexdigest(), 'hash brut reel')
            semantic.need([(x['k'],x['nodes'],x['births'],x['edges'],x['verticals']) for x in full['orders']] ==
                          [(1,4,3,3,0),(2,3,2,2,3),(3,1,1,0,1)], 'events et payload')
            hashes.append(parsed['raw_sha256']); semantic_hashes.append(parsed['sha256']); successes += 1
        semantic.need(len(set(hashes)) == len(set(semantic_hashes)) == 1, 'repetition/permutation/W changent FULL')
        cases = [('truncated_xyz','input_unreadable'),('truncated_ids','input_unreadable'),
                 ('duplicate_id','duplicate_point_id'),('out_of_domain','coordinate_out_of_domain'),
                 ('budget','memory_budget'),('kmax_above_n','parameter_out_of_range'),
                 ('weight','multiplicity_unsupported'),('output','output_unwritable'),('workers',None),
                 ('opt_negative',None),('opt_large',None),('opt_text',None),('opt_requires_lanes',None),
                 ('opt_concurrent_requires_lanes',None),('opt_replay_requires_batch',None),
                 ('opt_placement_requires_concurrent',None)]
        for mode, reason in cases:
            write(range(3))
            if mode == 'truncated_xyz': xyz.write_bytes(xyz.read_bytes()[:-1])
            elif mode == 'truncated_ids': ids.write_bytes(ids.read_bytes()[:-1])
            elif mode == 'duplicate_id': ids.write_bytes(struct.pack('<III',7,7,99))
            elif mode == 'out_of_domain': xyz.write_bytes(struct.pack('<I',2**bits)+xyz.read_bytes()[4:])
            elif mode == 'weight': xyz.write_bytes(xyz.read_bytes()[:12]+xyz.read_bytes()[:12]+xyz.read_bytes()[24:])
            result, rows = child(kmax=4 if mode == 'kmax_above_n' else 3,
                                 budget=0 if mode == 'budget' else 1 << 28,
                                 workers=0 if mode == 'workers' else 1, destination=root if mode == 'output' else None,
                                 optimizations={'opt_negative':'-1','opt_large':'524288','opt_text':'x','opt_requires_lanes':'128',
                                                'opt_concurrent_requires_lanes':'8192','opt_replay_requires_batch':'131072',
                                                'opt_placement_requires_concurrent':'262144'}.get(mode))
            semantic.need(result.returncode == 2 and not output.exists(), 'refus publie un payload : '+mode)
            if reason is None:
                semantic.need(not rows, 'usage refuse avant execution')
            else:
                semantic.need(rows and rows[-1]['phase'] == 'exit' and rows[-1]['reason'] == reason and
                              rows[-1]['status'] != 'ok', 'raison de refus : '+mode)
            if mode == 'output':
                semantic.need(any(r['phase'] == 'full' and r['status'] == 'ok' for r in rows),
                              'echec de sortie conserve apres calcul reussi')
            refusals += 1
    semantic.need((attempts,successes,refusals) == (429,413,16), 'plancher IO')
    print('full_io_verdict conforme attempts429 successes413 refusals16')


if __name__ == '__main__':
    main()
