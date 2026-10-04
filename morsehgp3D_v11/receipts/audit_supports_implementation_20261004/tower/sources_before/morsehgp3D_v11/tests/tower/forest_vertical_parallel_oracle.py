"""La même Definition indépendante juge l'option verticale, mémo éteint puis actif.

La sonde partage uniquement son encodage avec la porte régulière existante.
Les attendus viennent de Gamma_k/Fraction, jamais d'une sortie FULL sérielle.
"""
import json
import subprocess
import sys
import forest_oracle as oracle


def run(executable):
    profile = subprocess.run([executable,'--profile'],capture_output=True,text=True,timeout=10)
    oracle.require(profile.returncode == 0 and not profile.stderr,'profil natif')
    bits = oracle.data.parse(profile.stdout)['coord_bits']
    oracle.require(type(bits) is int and bits in (18,21,24),'profil entier')
    requests = oracle.requests(bits)
    encoded = ''.join(f"{r['kmax']} {r['budget']} {len(r['records'])}\n" +
                      ''.join(' '.join(map(str,p))+'\n' for p in r['records']) for r in requests)
    totals = dict(requests=0,orders=0,nodes=0,verticals=0,checks=0,refusals=0)
    for memo in (0,64):
        argv = [executable,'--verticals'] + (['--memo',str(memo)] if memo else [])
        process = subprocess.run(argv,input=encoded,capture_output=True,text=True,timeout=180)
        oracle.require(process.returncode == 0 and not process.stderr,'processus natif')
        rows = [oracle.data.parse(line) for line in process.stdout.splitlines()]
        oracle.equal(len(rows),len(requests))
        counts = [oracle.judge(row,req,bits,memo) for row,req in zip(rows,requests)]
        oracle.equal(sum(c['orders'] for c in counts),120)
        oracle.equal(sum(c['nodes'] for c in counts),867)
        oracle.equal(sum(c['verticals'] for c in counts),673)
        oracle.require(sum(c['checks'] for c in counts) >= 26000,'plancher juge')
        for key in ('orders','nodes','verticals','checks'):
            totals[key] += sum(c[key] for c in counts)
        totals['requests'] += len(requests)
        totals['refusals'] += sum(row['status'] != 'ok' for row in rows)
        if memo:
            hits = sum(o['ledger']['descent']['memo']['hits'] for row in rows for o in row['orders'])
            oracle.require(hits > 0,'mémo actif réellement exercé')
    print(json.dumps(dict(verdict='conforme',coord_bits=bits,parallel_verticals=True,**totals),sort_keys=True))


if __name__ == '__main__':
    try:
        oracle.require(len(sys.argv) == 2,'usage forest_vertical_parallel_oracle.py executable')
        run(sys.argv[1])
    except (ValueError,KeyError,TypeError,IndexError,subprocess.SubprocessError) as error:
        print('REFUS '+str(error),file=sys.stderr)
        raise SystemExit(1)
