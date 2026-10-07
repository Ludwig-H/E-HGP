"""Resume des compteurs deterministes du generateur v10 (sorties JSON de mhgp10_catalogue).
usage : python3 resume_compteurs.py DOSSIER_OUT [motif]
Imprime, par (entree, K, fils) : boules, boules/site, noeuds, feuilles, m moyen, tests du filtre par site et par boule,
candidats de feuille par boule, juges par boule, temps par etage, octets RSS par boule (si fichier .time)."""
import glob
import json
import os
import sys


def load(path):
    rows = [json.loads(l) for l in open(path) if l.startswith('{')]
    return rows


def main():
    out = sys.argv[1]
    pat = sys.argv[2] if len(sys.argv) > 2 else '*'
    print('entree K W | sites boules b/site niveaux | noeuds feuilles ignores m_moy m_max | filtre/site filtre/boule | '
          'dom paires triplets droites quads juges (par boule) | cand/boule | t_front t_boxes t_order t_asm total | '
          'us/boule | RSS o/boule')
    for f in sorted(glob.glob(os.path.join(out, pat + '.json'))):
        rows = load(f)
        if not rows or rows[0].get('status') != 'ok':
            print(os.path.basename(f), 'REFUS' if rows else 'VIDE', rows[:1])
            continue
        r = rows[0]
        st = r['catalogue_stages']
        b = max(r['balls'], 1)
        s = r['sites']
        cand = (r['leaf_dominance_tests'] + r['triple_tests'] + r['quad_tests']) / b
        tm = f[:-5] + '.time'
        rss = ''
        if os.path.exists(tm):
            try:
                t = json.loads(open(tm).read().strip().splitlines()[-1])
                rss = '%.0f' % (t['maxrss_kb'] * 1024.0 / b)
            except Exception:
                rss = '?'
        name = os.path.basename(f)[:-5]
        print('%s K%d W%d | %d %d %.1f %d | %d %d %d %.2f %d | %.0f %.1f | %.1f %.1f %.1f %.1f %.1f %.2f | %.1f | '
              '%.3f %.3f %.3f %.3f %.3f | %.2f | %s' % (
                  name, r['K'], r['threads'], s, r['balls'], r['balls'] / s, r['levels'], r['nodes'], r['leaves'],
                  r['skipped_bbox'], r['sum_m'] / max(r['leaves'], 1), r['max_m'], r['filter_tests'] / s,
                  r['filter_tests'] / b, r['leaf_dominance_tests'] / b, r['pair_tests'] / b, r['triple_tests'] / b,
                  r['line_hits'] / b, r['quad_tests'] / b, r['judged'] / b, cand, st['t_frontier'], st['t_boxes'],
                  st['t_order'], st['t_assemble'], r['catalogue_s'], 1e6 * r['catalogue_s'] / b, rss))


if __name__ == '__main__':
    main()
