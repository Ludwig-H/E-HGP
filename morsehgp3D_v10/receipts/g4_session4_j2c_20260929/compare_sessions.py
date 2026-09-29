"""Compare les temps de sessions G4 v10 de meme plan : catalogue (total, boites, frontiere), tour, tete."""
import json
import os
import sys


def load(d):
    out = {}
    for c in sorted(os.listdir(d)):
        name = c.split('_', 1)[1]
        p = os.path.join(d, c, 'stdout')
        if not os.path.exists(p):
            continue
        for line in open(p):
            if line.startswith('{'):
                try:
                    out[name] = json.loads(line)
                    break
                except ValueError:
                    pass
    return out


def main():
    labels = sys.argv[1::2]
    dirs = [load(d) for d in sys.argv[2::2]]
    last = dirs[-1]
    print('%-30s %s' % ('catalogue_s', ' '.join('%10s' % l for l in labels)))
    for name in last:
        if name.startswith(('cat_', 'tower_')):
            print('%-30s %s' % (name, ' '.join('%10s' % ('%.4f' % d[name]['catalogue_s'] if name in d else '-')
                                                for d in dirs)))
    print('\n%-30s %s' % ('t_boxes / t_frontier', ' '.join('%17s' % l for l in labels)))
    for name in last:
        if name.startswith('cat_'):
            cells = []
            for d in dirs:
                st = d.get(name, {}).get('catalogue_stages')
                cells.append('%8.4f/%8.4f' % (st['t_boxes'], st['t_frontier']) if st else '%17s' % '-')
            print('%-30s %s' % (name, ' '.join(cells)))
    print('\n%-30s %s' % ('catalogue + tour (48 fils)', ' '.join('%10s' % l for l in labels)))
    for name in last:
        if name.startswith('tower_') and 'w48' in name:
            print('%-30s %s' % (name, ' '.join('%10s' % ('%.3f' % (d[name]['catalogue_s'] + d[name]['tower_s'])
                                                          if name in d else '-') for d in dirs)))
    print('\n%-30s %s' % ('chaine : cat / tour / tete = total', ' | '.join('%27s' % l for l in labels)))
    for name in last:
        if name.startswith('cluster_'):
            cells = []
            for d in dirs:
                j = d.get(name)
                cells.append('%.3f/%.3f/%.3f = %.3f' % (j['catalogue_s'], j['tower_s'], j['head_s'],
                                                        j['catalogue_s'] + j['tower_s'] + j['head_s'])
                             if j else '%27s' % '-')
            print('%-30s %s  amas %s' % (name, ' | '.join(cells), [d.get(name, {}).get('clusters') for d in dirs]))


if __name__ == '__main__':
    main()
