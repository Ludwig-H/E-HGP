#!/usr/bin/env python3
"""Mutants du lecteur partage des sorties de la sonde FULL : chaque mutant est applique a une copie de lecteur_full.py
dans un dossier temporaire, la porte test_lecteur_full.py y est rejouee et doit echouer (mutant tue). Un mutant dont le
texte d'origine n'apparait pas exactement une fois est invalide (ecart). Python 3.10 nu, aucun assert. Codes : 0 tous
tues ; 1 un mutant survit ou est invalide.
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
    'entier_non_borne': ("    return type(value) is int and 0 <= value < (1 << 64)",
                         "    return type(value) is int and value >= 0"),
    'sans_inclusion_mur': ("    if sum(st[k] for k in ('P', 'C', 'G', 'raccord', 'TMVR')) > row['wall_ns'] or \\",
                           "    if False or \\"),
    'refus_tout_statut': ("end['status'] in REFUSALS", "end['status'] != 'ok'"),
    'cpu_appareil': ("    if attendu['voie'] == 'cpu' and (row['appareil_octets'] or row['epinglee_octets'] or "
                     "row['pic_appareil_octets']):", "    if False:"),
    'budget_ignore': ("                open_row['budget_appareil'] != attendu['budget_appareil']:",
                      "                open_row['budget_appareil'] not in ('separe', 'partage'):"),
    'cles_repetees': ("row = json.loads(raw, object_pairs_hook=unique_object, parse_constant=reject_constant)",
                      "row = json.loads(raw, parse_constant=reject_constant)"),
    'liberation_rang': ("not is_int(free['pass']) or free['pass'] != i", "free['pass'] != i"),
    'ouverture_raison': ("if open_row['status'] != 'ok' or open_row['reason'] != 'none':",
                         "if open_row['status'] != 'ok':"),
    'memoire_sans_coherence': ("mem[k][0] > mem[k][1] for k in MEM_STAGES) or max(mem[k][1] for k in MEM_STAGES) != "
                               "row['pic_octets']:", "mem[k][0] > mem[k][1] for k in MEM_STAGES):"),
    'memoire_usage_libre': ("                mem[k][0] > mem[k][1] for k in MEM_STAGES)",
                            "                False for k in MEM_STAGES)"),
    'mur_nul_admis': ("    if row['wall_ns'] == 0 or row['sites'] == 0:", "    if False:"),
    'trame_ignoree': ("row['status'] != 'ok' or row['trame'] != label or", "row['status'] != 'ok' or"),
    'trame_unique': ("    label, sites = attendu['trames'][i % len(attendu['trames'])]",
                     "    label, sites = attendu['trames'][0]"),
    'empreinte_toujours': ("    keys = FULL_KEYS | {'full_sha256'} if attendu['empreinte'] else FULL_KEYS",
                           "    keys = FULL_KEYS | {'full_sha256'}"),
}


def main():
    with open(os.path.join(HERE, 'lecteur_full.py'), encoding='utf-8') as handle:
        original = handle.read()
    bad = []
    for name, (old, new) in MUTANTS.items():
        if original.count(old) != 1:
            bad.append('%s : mutant invalide (%d occurrences)' % (name, original.count(old)))
            continue
        with tempfile.TemporaryDirectory() as folder:
            with open(os.path.join(folder, 'lecteur_full.py'), 'w', encoding='utf-8') as out:
                out.write(original.replace(old, new))
            shutil.copy(os.path.join(HERE, 'test_lecteur_full.py'), folder)
            done = subprocess.run([sys.executable, '-S', os.path.join(folder, 'test_lecteur_full.py')],
                                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=600)
        if done.returncode == 0:
            bad.append('%s : survit' % name)
    for line in bad:
        print(line, file=sys.stderr)
    if bad:
        return 1
    print('mutants_lecteur_full_ok tues=%d' % len(MUTANTS))
    return 0


if __name__ == '__main__':
    sys.exit(main())
