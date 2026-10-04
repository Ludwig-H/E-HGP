#!/usr/bin/env python3
"""Profil G4 de la voie GPU du catalogue : Nsight Systems (frise CPU/GPU) et Nsight Compute (metriques des noyaux).

    python3 bench/gpu_profile.py --bench BUILD/mhgp11_full_bench --data DIR --work DIR --out DIR
        [--frame lidar_ng00] [--mode 81915] [--workers 48] [--passes 3] [--kmax 5] [--leaf 16]

Outils telecharges sur la VM depuis le depot officiel NVIDIA, empreintes et tailles epinglees avant usage :
Nsight Systems CLI 2025.3.1 (paquet .deb de la v9, extrait sans installation) et Nsight Compute 2025.2.1.3 (archive
redistribuable de CUDA 12.9.1). Etapes : (1) inventaire du GPU (nvidia-smi) ; (2) nsys profile d'un processus a
--passes passes FULL (cuda, osrt ; sans echantillonnage CPU), puis nsys stats (noyaux, copies, API CUDA, OS) en CSV ;
(3) ncu --set full sur count_kernel et fill_kernel de la premiere passe (compteurs materiels : relance sous sudo -n si
le pilote les reserve a l'administrateur), puis export des details et du source (CUDA et SASS) en CSV ; (4) resume
JSON des metriques cles par noyau (duree, occupation theorique et atteinte, registres, pile locale, debit SM et
memoire, efficacite des warps, causes d'attente principales). Les rapports bruts (.nsys-rep, .ncu-rep) sont gardes
dans --out s'ils font au plus 64 Mio, pour une relecture hors ligne. Aucune coordonnee n'est ecrite. Bibliotheque
standard seule. Codes : 0 profils obtenus ; 1 un outil ou une capture a echoue (publie) ; 2 entree refusee.
"""
import argparse
import csv
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile

NSYS = dict(url='https://developer.download.nvidia.com/devtools/repos/ubuntu2204/amd64/'
                'NsightSystems-linux-cli-public-2025.3.1.90-3582212.deb',
            sha256='d2484ad0faf6831b11fa0bf73c54232d9ea8beafb50414019e6ba299c4ed5718', size=175210028,
            relative='opt/nvidia/nsight-systems-cli/2025.3.1/target-linux-x64/nsys')
NCU = dict(url='https://developer.download.nvidia.com/compute/cuda/redist/nsight_compute/linux-x86_64/'
               'nsight_compute-linux-x86_64-2025.2.1.3-archive.tar.xz',
           sha256='02ab8867197aaf6a6ae3171293d70b6a9ddb20296be94ff4287741338cc2e1df', size=300347320)
KEY_METRICS = ('Duration', 'Registers Per Thread', 'Theoretical Occupancy', 'Achieved Occupancy',
               'Achieved Active Warps Per SM', 'Block Limit Registers', 'Compute (SM) Throughput', 'Memory Throughput',
               'L1/TEX Hit Rate', 'L2 Hit Rate', 'DRAM Throughput', 'Executed Ipc Active', 'Issue Slots Busy',
               'Warp Cycles Per Issued Instruction', 'Avg. Active Threads Per Warp',
               'Avg. Not Predicated Off Threads Per Warp', 'Local Memory Spilling Requests', 'Stack Size',
               'Branch Instructions Ratio', 'Branch Efficiency', 'Avg. Divergent Branches', 'Waves Per SM',
               'Elapsed Cycles', 'SM Frequency', 'Threads', 'Grid Size', 'Block Size')


def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 22), b''):
            h.update(chunk)
    return h.hexdigest()


def command(log, name, argv, timeout, env=None):
    done = subprocess.run(argv, capture_output=True, text=True, timeout=timeout, env=env)
    log.append(dict(name=name, argv=argv, code=done.returncode, stdout=done.stdout[-4000:], stderr=done.stderr[-4000:]))
    return done


def fetch(log, tools, spec, name):
    target = tools / name
    if not target.exists():
        command(log, 'download_' + name, ['curl', '--fail', '--location', '--silent', '--show-error', '--proto', '=https',
                                         '--max-time', '600', '--max-filesize', str(spec['size']), '--output',
                                         str(target), spec['url']], 700)
    if not target.is_file() or target.stat().st_size != spec['size'] or sha256(target) != spec['sha256']:
        return None
    return target


def keep(path, out, report):
    """Copie un rapport brut dans out s'il fait au plus 64 Mio ; sa taille et son empreinte vont au rapport."""
    if not path.is_file():
        return
    size = path.stat().st_size
    report.setdefault('raw_reports', {})[path.name] = dict(bytes=size, sha256=sha256(path), kept=size <= 64 * 2 ** 20)
    if size <= 64 * 2 ** 20:
        (out / path.name).write_bytes(path.read_bytes())


def ncu_summary(details_csv):
    rows = list(csv.DictReader(io.StringIO(details_csv)))
    kernels = {}
    for row in rows:
        name = row.get('Kernel Name', '')
        short = 'count_kernel' if 'count_kernel' in name else 'fill_kernel' if 'fill_kernel' in name else None
        if short is None:
            continue
        entry = kernels.setdefault(short, dict(metrics={}, rules=[]))
        metric = row.get('Metric Name', '')
        if metric in KEY_METRICS:
            entry['metrics'][metric] = '%s %s' % (row.get('Metric Value', ''), row.get('Metric Unit', ''))
        if row.get('Rule Name') or row.get('Rule Description'):
            entry['rules'].append(row.get('Rule Description', '')[:300])
    return kernels


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--bench', type=Path, required=True)
    ap.add_argument('--data', type=Path, required=True)
    ap.add_argument('--work', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--frame', default='lidar_ng00')
    ap.add_argument('--mode', default='81915')
    ap.add_argument('--workers', default='48')
    ap.add_argument('--passes', type=int, default=3)
    ap.add_argument('--kmax', default='5')
    ap.add_argument('--leaf', default='16')
    args = ap.parse_args()
    if not args.bench.is_file():
        print('refus : banc absent', file=sys.stderr)
        return 2
    args.work.mkdir(parents=True, exist_ok=True)
    args.out.mkdir(parents=True, exist_ok=True)
    tools = args.work / 'tools'
    tools.mkdir(exist_ok=True)
    log, report, problems = [], dict(schema='ehgp.v11.gpu_profile.v1', frame=args.frame, mode=args.mode,
                                     workers=args.workers, passes=args.passes), []
    app = [str(args.bench), str(args.data / (args.frame + '.u32le')), str(args.data / (args.frame + '.ids.u32le')),
           '/dev/null', args.kmax, args.leaf, '256', '0', '4294967295', '8589934592', args.workers, args.mode]
    smi = command(log, 'gpu_inventory', ['nvidia-smi', '--query-gpu=name,driver_version,memory.total,compute_cap,'
                                         'clocks.max.sm,clocks.max.mem,power.limit', '--format=csv'], 60)
    report['gpu'] = smi.stdout.strip()
    # Acces aux compteurs : parametre du pilote (RmProfilingAdminOnly) et sudo non interactif.
    try:
        params = Path('/proc/driver/nvidia/params').read_text()
        report['profiling_admin_only'] = [l for l in params.splitlines() if 'RmProfilingAdminOnly' in l]
    except OSError as error:
        report['profiling_admin_only'] = str(error)
    report['sudo_n'] = command(log, 'sudo_n', ['sudo', '-n', 'true'], 30).returncode
    # (2) Nsight Systems.
    deb = fetch(log, tools, NSYS, 'nsight-systems-cli.deb')
    if deb is None:
        problems.append('paquet Nsight Systems absent ou empreinte fausse')
    else:
        command(log, 'nsys_extract', ['dpkg-deb', '-x', str(deb), str(tools / 'nsys')], 300)
        nsys = tools / 'nsys' / NSYS['relative']
        trace = args.work / 'gpu_full'
        prof = command(log, 'nsys_profile', [str(nsys), 'profile', '--trace=cuda', '--sample=none',
                                             '--cpuctxsw=none', '--stats=false', '--force-overwrite=true',
                                             '--output=' + str(trace)] + app + [str(args.passes)], 1200)
        report['nsys_app_stdout_tail'] = prof.stdout[-3000:]
        if prof.returncode != 0:
            problems.append('nsys profile code %d' % prof.returncode)
        stats = command(log, 'nsys_stats', [str(nsys), 'stats', '--report',
                                            'cuda_gpu_kern_sum,cuda_gpu_mem_time_sum,cuda_gpu_mem_size_sum,'
                                            'cuda_api_sum,cuda_gpu_trace', '--format', 'csv', '--output', '-',
                                            str(trace) + '.nsys-rep'], 600)
        (args.out / 'nsys_stats.csv').write_text(stats.stdout)
        report['nsys_stats_bytes'] = len(stats.stdout)
        if stats.returncode != 0:
            problems.append('nsys stats code %d' % stats.returncode)
        keep(trace.with_suffix('.nsys-rep'), args.out, report)
    # (3) Nsight Compute.
    archive = fetch(log, tools, NCU, 'nsight_compute.tar.xz')
    if archive is None:
        problems.append('archive Nsight Compute absente ou empreinte fausse')
    else:
        root = tools / 'ncu'
        if not root.exists():
            root.mkdir()
            with tarfile.open(archive) as t:
                t.extractall(root)
        found = [p for p in root.rglob('ncu') if p.is_file() and os.access(p, os.X_OK)]
        if not found:
            problems.append('ncu introuvable dans l archive')
        else:
            ncu = found[0]
            report_path = args.work / 'leaf_kernels'
            argv = [str(ncu), '--set', 'full', '--kernel-name', 'regex:count_kernel|fill_kernel', '--launch-count', '2',
                    '--import-source', 'yes', '--force-overwrite', '--export', str(report_path)] + app
            run = command(log, 'ncu_profile', argv, 1800)
            if run.returncode != 0 and 'ERR_NVGPUCTRPERM' in (run.stdout + run.stderr):
                run = command(log, 'ncu_profile_sudo', ['sudo', '-n', '--preserve-env=PATH'] + argv, 1800)
            if run.returncode != 0:
                problems.append('ncu code %d' % run.returncode)
            details = command(log, 'ncu_details', [str(ncu), '--import', str(report_path) + '.ncu-rep', '--page',
                                                   'details', '--csv'], 600)
            (args.out / 'ncu_details.csv').write_text(details.stdout)
            report['ncu'] = ncu_summary(details.stdout)
            if not report['ncu']:
                problems.append('aucune metrique de noyau lue')
            source = command(log, 'ncu_source', [str(ncu), '--import', str(report_path) + '.ncu-rep', '--page',
                                                 'source', '--csv', '--print-source', 'cuda,sass'], 600)
            (args.out / 'ncu_source.csv').write_text(source.stdout[:48 * 2 ** 20])
            keep(report_path.with_suffix('.ncu-rep'), args.out, report)
    report['problems'] = problems
    report['log'] = [dict(name=e['name'], code=e['code'], stdout=e['stdout'][-1500:], stderr=e['stderr'][-600:])
                     for e in log]
    (args.out / 'gpu_profile.json').write_text(json.dumps(report, indent=1) + '\n')
    for name, entry in (report.get('ncu') or {}).items():
        for metric, value in sorted(entry['metrics'].items()):
            print('%-12s %-42s %s' % (name, metric, value))
    print('gpu_profile_verdict %s problemes %d' % ('conforme' if not problems else 'incomplet', len(problems)))
    return 0 if not problems else 1


if __name__ == '__main__':
    sys.exit(main())
