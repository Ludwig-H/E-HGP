#!/usr/bin/env python3
"""Fausse sonde de l'etage G (hors produit) : rejoue les faux succes que le juge g_determinism.py admettait avant la
contre-lecture de l'auditeur Codex du 7 octobre (reçu audit_t2g_prepublication_20261007) ; le juge doit les refuser
(code 2). Mode lu dans MHGP12_FAUSSE_SONDE : k1_seul (sortie tronquee apres l'ordre 1, ligne CTest imitee),
ordre_double, empreinte_courte, sans_sortie (ligne exit absente), sortie_ordre_booleen et sortie_ordre_flottant
(ordre de la ligne exit False ou 0.0, egaux a 0 pour Python). Arguments de la vraie sonde acceptes et ignores, sauf
--threads, recopie dans la passe. Bibliotheque standard seule.
"""
import json
import os
import sys

FIELDS_WORK = ('probes', 'first_probe_hits', 'probe_hits_after_steps', 'route_t1', 'route_cert_table',
               'route_cert_census', 'route_fallback_table', 'route_fallback_census', 'fallback_no_proposal',
               'fallback_not_in_part', 'fallback_certificate', 'census_saturated', 'census_complete', 'census_sites',
               'census_sites_max', 'census_nodes', 'jumps_catalogue', 'jumps_census', 'inert_steps', 'cell_stops',
               'birth_stops', 'controls', 'max_chain')


def order(k, sites):
    obj = dict(births=sites if k == 1 else 3, cells=1, inert_cells=0, extended_cells=0, representatives=2)
    work = {name: 0 for name in FIELDS_WORK}
    work.update(probes=2, first_probe_hits=2, controls=2)  # deux representants arretes a la premiere sonde
    work['chaines'] = [2] + [0] * 15
    return dict(phase='ordre', k=k, objet=obj, travail=work)


def main(argv):
    mode = os.environ.get('MHGP12_FAUSSE_SONDE', '')
    threads = next((int(a.split('=', 1)[1]) for a in argv if a.startswith('--threads=')), 1)
    kmax = next((int(a.split('=', 1)[1]) for a in argv if a.startswith('--k=')), 5)
    sites = 10
    rows = [dict(phase='tour_g', **{'pass': 0}, status='ok', reason='none', order=0, coord_bits=21, kmax=kmax,
                 threads=threads, sites=sites, wall_ns=1)]
    ks = [1] if mode == 'k1_seul' else [1, 1] + list(range(2, kmax + 1)) if mode == 'ordre_double' else \
        list(range(1, kmax + 1))
    rows += [order(k, sites) for k in ks]
    rows.append(dict(phase='digest', resolution_sha256=('ab' * 8) if mode == 'empreinte_courte' else 'ab' * 32))
    if mode != 'sans_sortie':
        order_value = False if mode == 'sortie_ordre_booleen' else 0.0 if mode == 'sortie_ordre_flottant' else 0
        rows.append(dict(phase='exit', status='ok', reason='none', order=order_value))
    for row in rows:
        print(json.dumps(row))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
