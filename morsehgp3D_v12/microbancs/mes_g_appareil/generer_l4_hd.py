#!/usr/bin/env python3
"""Etape 2 (hors depot) : genere les classes DWelzl du bras L4 de T2-d-B en source unique __host__ __device__, en
binaire64 et en binaire32, a partir du texte du produit (proposal.hpp, empreinte 92495aa6...) et des substitutions
exactes du bras (microbancs/mes_t2d_b/bras_t2d_b.json, empreinte apres a82de524...). Copie textuelle : seules les
fonctions membres sont annotees et les types renommes ; la variante binaire32 remplace double par float et suffixe f
les constantes (1e-9, 1e-10, 0.5, 0.25). Sortie : un bloc C++ a inserer dans noyau_g.hpp.

  python3 generer_l4_hd.py <proposal.hpp du produit> <bras_t2d_b.json> > l4_hd.inc
"""
import hashlib
import json
import re
import sys

BASE = "92495aa67223577c8acef74967f89853c0dab3c5881afe1fb2c7826327f28c18"
APRES = "a82de524758fe65a70cd08251530b77e2a44479ff4828e2d827440b69f86eff0"


def principal(argv):
    src = open(argv[1], encoding="utf-8").read()
    if hashlib.sha256(src.encode()).hexdigest() != BASE:
        print("proposal.hpp differe de la base du bras L4", file=sys.stderr)
        return 3
    spec = json.load(open(argv[2], encoding="utf-8"))["bras"]["proposition"]["fichiers"]["src/tower/proposal.hpp"]
    l4 = src
    for sub in spec["substitutions"]:
        if l4.count(sub["cherche"]) != 1:
            print("motif du bras absent ou multiple", file=sys.stderr)
            return 3
        l4 = l4.replace(sub["cherche"], sub["remplace"])
    if hashlib.sha256(l4.encode()).hexdigest() != APRES:
        print("texte L4 different de l'empreinte du bras", file=sys.stderr)
        return 3
    corps = l4[l4.index("struct DBall {"):l4.index("}  // namespace mhgp12::tower_detail")].rstrip() + "\n"
    sortie = []
    for suffixe, flottant in (("L4HD", False), ("L4F32HD", True)):
        t = corps
        t = re.sub(r"\bDBall\b", "DBall" + suffixe, t)
        t = re.sub(r"\bDWelzl\b", "DWelzl" + suffixe, t)
        if flottant:
            t = re.sub(r"\bdouble\b", "float", t)
            for c in ("1e-9", "1e-10", "0.5", "0.25"):
                t = re.sub(r"(?<![0-9.e-])" + re.escape(c) + r"(?![0-9f])", c + "f", t)
        lignes = []
        motif = re.compile(r"^  (DBall%s|bool|void|int) ([a-z_0-9]+)\(.*\)( const)? \{$" % suffixe)
        for ligne in t.split("\n"):
            if motif.match(ligne):
                ligne = "  MESG_HD_MEMBRE " + ligne[2:]
            lignes.append(ligne)
        sortie.append("\n".join(lignes))
    print("\n".join(sortie), end="")
    return 0


if __name__ == "__main__":
    sys.exit(principal(sys.argv))
