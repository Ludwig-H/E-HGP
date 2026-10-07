"""Bilan des mutants : portes hors oracles (ctest -LE oracle) et differentiel exact des deux portes d'oracle."""
import json
import os
import re

B = '/tmp/v11-audit/l08_tests_portes/mut'
for m in sorted(d for d in os.listdir(B) if d.startswith('M')):
    row = [m]
    full = os.path.join(B, m, 'ctest.log')
    part = os.path.join(B, m, 'ctest_hors_oracles.log')
    for path, label in ((full, 'suite complete'), (part, 'hors oracles')):
        if os.path.exists(path):
            s = open(path).read()
            t = re.search(r'(\d+)% tests passed, (\d+) tests failed out of (\d+)', s)
            failed = re.findall(r'\d+ - (\S+) \((\w+)\)', s)
            passed = len(re.findall(r'Test +#\d+: \S+ \.+ +Passed', s))
            row.append('%s : %s ; passes %d ; echecs %s' % (label, t.group(0) if t else 'inachevee', passed,
                                                           [f[0] for f in failed] or 'aucun'))
    orc = os.path.join(B, m, 'oracles_rapides.json')
    if os.path.exists(orc) and os.path.getsize(orc) > 20:
        s = open(orc).read()
        try:
            j = json.loads(s[:s.rindex('}') + 1])
            row.append('oracle catalogue : %d/%d dumps identiques a HEAD, %d rejuges, ecarts %d, translation %s' % (
                j['catalogue']['dumps_identiques'], j['catalogue']['entrees'], j['catalogue']['rejuges'],
                len(j['catalogue']['ecarts']), j['catalogue']['translation']))
            row.append('oracle tour : %d/%d dumps identiques a HEAD, %d rejuges, ecarts %d' % (
                j['tour']['dumps_identiques'], j['tour']['entrees'], j['tour']['rejuges'], len(j['tour']['ecarts'])))
            row.append('verdict oracles : ' + j['verdict_portes_oracle'])
            if j['catalogue']['ecarts']:
                row.append('  ex. catalogue : ' + j['catalogue']['ecarts'][0])
            if j['tour']['ecarts']:
                row.append('  ex. tour : ' + j['tour']['ecarts'][0])
        except ValueError:
            row.append('oracles : en cours')
    print('\n  '.join(row))
