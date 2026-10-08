#!/usr/bin/env python3
"""Carte de lien de la porte native de terminaison (tests/tower/region_unit.cpp, cible mhgp12_tower_region).

La cible compile src/tower/pipeline_run.cpp avec MHGP12_REGION_HOOKS et se lie a l'archive statique mhgp12, qui
contient le meme fichier SANS crochets. Recommandation de l'auditeur Codex (a_terminaison_porte) : verifier la carte
de lien, pour ne pas tester accidentellement le corps non instrumente de l'archive. La porte exige :
  1. la carte (option de lien -Map de la cible) lisible et parlante : un membre de l'archive y est nomme sous la forme
     libmhgp12.a(<objet>), et pipeline.cpp.o (open_session, admit_session) y figure, sans quoi le controle suivant
     serait vide ;
  2. aucun membre pipeline_run.cpp.o de l'archive lie ;
  3. l'objet pipeline_run.cpp.o de la cible elle-meme (dossier mhgp12_tower_region.dir) lie ;
  4. l'archive du produit lisible (format ar), avec son membre pipeline_run.cpp.o, et sans aucune occurrence de
     region_hook (ni crochet ni marqueur region_hooks_build : la construction produit n'en porte aucun).
Le marqueur region_hooks_build, controle par le test lui-meme, prouve en outre que le corps lie est l'instrumente.

Usage : region_map_check.py <carte> <libmhgp12.a>. Ligne si conforme :
    region_carte_ok corps=instrumente membre_archive=absent archive_sans_crochet=oui
region_map_check.py --auto-test juge le juge sur des cartes et des archives fabriquees (une conforme, puis membre
pipeline_run.cpp.o de l'archive lie, objet de la cible absent, crochet dans l'archive, carte muette, archive sans le
membre, archive illisible) ; ligne region_carte_auto_test_ok cas=7.
Codes : 0 conforme ; 1 ecart ; 2 usage ou entree illisible. Python 3.10 nu, aucun assert (tient sous -O).
"""
import os
import re
import sys
import tempfile

ARCHIVE_MEMBER = re.compile(r'libmhgp12\.a\(([^)\s]+)\)')
TARGET_DIR = 'mhgp12_tower_region.dir'
HOOK = b'region_hook'


class Refus(Exception):
    """Entree illisible ou controle vide : code 2."""


def archive_members(data):
    """Noms des membres d'une archive ar (GNU : table des noms longs //, table des symboles / et /SYM64/)."""
    if not data.startswith(b'!<arch>\n'):
        raise Refus('archive : en-tete !<arch> absent (archive mince ou autre format)')
    names, members, pos = b'', [], 8
    while pos < len(data):
        header = data[pos:pos + 60]
        if len(header) != 60 or header[58:60] != b'`\n':
            raise Refus('archive : en-tete de membre illisible a l\'octet %d' % pos)
        raw = header[:16].rstrip(b' ')
        size_text = header[48:58].strip()
        if not size_text.isdigit():
            raise Refus('archive : taille de membre illisible a l\'octet %d' % pos)
        size = int(size_text)
        body = data[pos + 60:pos + 60 + size]
        if len(body) != size:
            raise Refus('archive : membre tronque a l\'octet %d' % pos)
        if raw == b'//':
            names = body
        elif raw in (b'/', b'/SYM64/'):
            pass
        elif raw.startswith(b'/') and raw[1:].isdigit():
            offset = int(raw[1:])
            end = names.find(b'/\n', offset)
            if end < 0:
                raise Refus('archive : nom long hors de la table')
            members.append(names[offset:end].decode('ascii', 'replace'))
        elif raw.startswith(b'#1/'):
            raise Refus('archive : noms BSD (#1/) non lus par cette porte')
        else:
            members.append(raw.rstrip(b'/').decode('ascii', 'replace'))
        pos += 60 + size + (size % 2)
    return members


def check(map_path, archive_path):
    """Liste des ecarts (vide si conforme) ; Refus si une entree est illisible ou si un controle serait vide."""
    try:
        with open(map_path, 'r', encoding='utf-8', errors='replace') as handle:
            text = handle.read()
        with open(archive_path, 'rb') as handle:
            data = handle.read()
    except OSError as error:
        raise Refus('lecture : %s' % error) from error
    linked = set(ARCHIVE_MEMBER.findall(text))
    if 'pipeline.cpp.o' not in linked:
        raise Refus('carte : aucun membre libmhgp12.a(pipeline.cpp.o) nomme (format inconnu ou carte d\'une autre '
                    'cible) ; le controle du membre pipeline_run.cpp.o serait vide')
    members = archive_members(data)
    if 'pipeline_run.cpp.o' not in members:
        raise Refus('archive : aucun membre pipeline_run.cpp.o ; le controle serait vide')
    gaps = []
    if 'pipeline_run.cpp.o' in linked:
        gaps.append('corps non instrumente de l\'archive lie : libmhgp12.a(pipeline_run.cpp.o) dans la carte')
    own = [line for line in text.splitlines() if TARGET_DIR in line and 'pipeline_run.cpp.o' in line]
    if not own:
        gaps.append('objet pipeline_run.cpp.o de la cible (%s) absent de la carte' % TARGET_DIR)
    hooks = data.count(HOOK)
    if hooks != 0:
        gaps.append('archive du produit : %d occurrence(s) de region_hook' % hooks)
    return gaps


def fake_archive(members):
    """Archive ar au format GNU (noms longs dans //) faite des membres (nom, contenu) donnes."""
    table = b''.join(name.encode('ascii') + b'/\n' for name, _ in members)
    out = [b'!<arch>\n', b'//'.ljust(48) + str(len(table)).encode('ascii').ljust(10) + b'`\n', table]
    if len(table) % 2:
        out.append(b'\n')
    offset = 0
    for name, body in members:
        out.append(('/%d' % offset).encode('ascii').ljust(48) + str(len(body)).encode('ascii').ljust(10) + b'`\n')
        out.append(body)
        if len(body) % 2:
            out.append(b'\n')
        offset += len(name) + 2
    return b''.join(out)


def auto_test():
    """Verdicts du juge sur des entrees fabriquees : 0 si chacun est celui attendu."""
    own = 'CMakeFiles/%s/src/tower/pipeline_run.cpp.o' % TARGET_DIR
    pulled = 'libmhgp12.a(pipeline.cpp.o)     %s (admit_session)\n' % own
    good_map = pulled + 'LOAD %s\n' % own
    good_archive = fake_archive([('pipeline.cpp.o', b'\x7fELF corps'), ('pipeline_run.cpp.o', b'\x7fELF run_region')])
    cases = [
        ('conforme', good_map, good_archive, 0),
        ('membre_lie', good_map + 'libmhgp12.a(pipeline_run.cpp.o)  x.o (run_region)\n', good_archive, 1),
        ('objet_absent', pulled.replace(own, 'region_unit.cpp.o'), good_archive, 1),
        ('crochet_archive', good_map, fake_archive([('pipeline.cpp.o', b'a'), ('pipeline_run.cpp.o', b'region_hook')]),
         1),
        ('carte_muette', 'LOAD %s\n' % own, good_archive, 2),
        ('sans_membre', good_map, fake_archive([('pipeline.cpp.o', b'a')]), 2),
        ('archive_illisible', good_map, b'pas une archive', 2),
    ]
    wrong = 0
    with tempfile.TemporaryDirectory() as folder:
        map_path, archive_path = os.path.join(folder, 'carte.map'), os.path.join(folder, 'lib.a')
        for name, map_text, archive, expected in cases:
            with open(map_path, 'w', encoding='utf-8') as handle:
                handle.write(map_text)
            with open(archive_path, 'wb') as handle:
                handle.write(archive)
            try:
                got = 1 if check(map_path, archive_path) else 0
            except Refus:
                got = 2
            if got != expected:
                wrong += 1
                print('ECART auto_test %s : verdict %d, attendu %d' % (name, got, expected))
    if wrong:
        return 1
    print('region_carte_auto_test_ok cas=%d' % len(cases))
    return 0


def main(argv):
    if len(argv) == 2 and argv[1] == '--auto-test':
        return auto_test()
    if len(argv) != 3:
        print('usage : region_map_check.py <carte> <libmhgp12.a> | --auto-test')
        return 2
    try:
        gaps = check(argv[1], argv[2])
    except Refus as refusal:
        print('REFUS region_carte : %s' % refusal)
        return 2
    for gap in gaps:
        print('ECART region_carte : %s' % gap)
    if gaps:
        return 1
    print('region_carte_ok corps=instrumente membre_archive=absent archive_sans_crochet=oui')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
