#!/usr/bin/env python3
"""Mutants du pilote MES-B : chaque mutant est applique a une copie de pilote_b.py dans un dossier temporaire, la porte
test_pilote_b.py y est rejouee et doit echouer (mutant tue). Un mutant dont le texte d'origine n'apparait pas exactement
une fois est invalide (ecart). Python 3.10 nu, aucun assert. Codes : 0 tous tues ; 1 un mutant survit ou est invalide.
Mutant equivalent ecarte : retirer le refus des constantes non finies (NaN, Infinity) ne change aucune lecture, car
chaque champ numerique est deja exige entier ; le refus reste comme defense en profondeur.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
MUTANTS = {
    'sans_cles_exactes': ("    if set(row) != keys:\n", "    if not set(row) >= keys:\n"),
    'booleen_admis': ("    return type(value) is int and 0 <= value < (1 << 64)",
                      "    return isinstance(value, int) and 0 <= value < (1 << 64)"),
    'sans_inclusion_mur': ("    if sum(st[k] for k in ('P', 'C', 'G', 'raccord', 'TMVR')) > row['wall_ns'] or \\",
                           "    if False or \\"),
    'refus_tout_statut': ("end['status'] in REFUSALS", "end['status'] != 'ok'"),
    'b1_seuil': ("for n, v in rows if v > 2.0]", "for n, v in rows if v > 2.5]"),
    'b2_tous': ("small = [r for r in k5 if r['sites'] < 10_000_000]", "small = list(k5)"),
    'b3_seuil': ("        if s > 1.1:", "        if s > 1.5:"),
    'empreinte_voies': ("        if len(everything) != 1:",
                        "        if any(len(v) != 1 for v in by_voie.values()):"),
    'empreinte_passes': ("        if len(everything) != 1:", "        if len(by_voie) > 1 and len(everything) != 1:"),
    'etiquette_tronquee': ("        label = prefix + name[-(23 - len(prefix)):]",
                           "        label = (prefix + name)[-23:]\n        break"),
    'delai_ignore': ("        if forecast > remaining:", "        if False:"),
    'cpu_appareil': ("    if case['voie'] == 'cpu' and (row['appareil_octets'] or row['epinglee_octets'] or "
                     "row['pic_appareil_octets']):", "    if False:"),
    'open_partage': ("open_row['budget_appareil'] != 'separe'",
                     "open_row['budget_appareil'] not in ('separe', 'partage')"),
    'cles_repetees': ("row = json.loads(raw, object_pairs_hook=unique_object, parse_constant=reject_constant)",
                      "row = json.loads(raw, parse_constant=reject_constant)"),
    'entier_non_borne': ("    return type(value) is int and 0 <= value < (1 << 64)",
                         "    return type(value) is int and value >= 0"),
    'liberation_rang': ("not is_int(free['pass']) or free['pass'] != i", "free['pass'] != i"),
    'ouverture_raison': ("if open_row['status'] != 'ok' or open_row['reason'] != 'none':",
                         "if open_row['status'] != 'ok':"),
    'b1_sans_echec': ("            if r['etat'] == 'echec' or (r['etat'] == 'refus' and r['sites'] < 10_000_000)]",
                      "            if r['etat'] == 'refus' and r['sites'] < 10_000_000]"),
    'b1_refus_partout': ("            if r['etat'] == 'echec' or (r['etat'] == 'refus' and r['sites'] < 10_000_000)]",
                         "            if r['etat'] in ('echec', 'refus')]"),
    'b4_sans_refus': ("for r in k10 if r['etat'] != 'ok']", "for r in k10 if r['etat'] == 'echec']"),
    'memoire_sans_coherence': ("mem[k][0] > mem[k][1] for k in MEM_STAGES) or max(mem[k][1] for k in MEM_STAGES) != "
                               "row['pic_octets']:", "mem[k][0] > mem[k][1] for k in MEM_STAGES):"),
    'memoire_usage_libre': ("                mem[k][0] > mem[k][1] for k in MEM_STAGES)",
                            "                False for k in MEM_STAGES)"),
}


def main():
    with open(os.path.join(HERE, 'pilote_b.py'), encoding='utf-8') as handle:
        original = handle.read()
    bad = []
    for name, (old, new) in MUTANTS.items():
        if original.count(old) != 1:
            bad.append('%s : mutant invalide (%d occurrences)' % (name, original.count(old)))
            continue
        with tempfile.TemporaryDirectory() as folder:
            with open(os.path.join(folder, 'pilote_b.py'), 'w', encoding='utf-8') as out:
                out.write(original.replace(old, new))
            shutil.copy(os.path.join(HERE, 'test_pilote_b.py'), folder)
            done = subprocess.run([sys.executable, '-S', os.path.join(folder, 'test_pilote_b.py')],
                                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=600)
        if done.returncode == 0:
            bad.append('%s : survit' % name)
    for line in bad:
        print(line, file=sys.stderr)
    if bad:
        return 1
    print('mutants_pilote_b_ok tues=%d' % len(MUTANTS))
    return 0


if __name__ == '__main__':
    sys.exit(main())
