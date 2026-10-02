# Lecture statique IO, deuxième tranche

2026-10-02 09:49:43 UTC. Neuf copies WIP capturées avant lecture après `5c5457a53` ;
toutes identiques à la recoupe. Aucun build, test C++ ou GCP exécuté.
`PROPOSED_G4.cpp` est une fixture à intégrer au juge IO et jouer sur G4 avec UBSan ;
ce fichier n'est pas une reproduction native déjà passée.

## Constat exact : morceau vide après un morceau partiel

`src/io/sha256.cpp:113–116`, copie `fbf1a80e…` : après `update("a")`,
`bytes_ % 64 = 1`. `update(std::string_view{})` est admis par le domaine
et donne `in=nullptr`, `left=take=0` ; la branche appelle `memcpy(destination,
nullptr, 0)`. Ce cas n'est pas valide au sens de la bibliothèque C/C++,
même pour une longueur nulle. Corriger par retour réussi immédiat pour une
entrée vide avant tout accès au pointeur, avec compteur/empreinte inchangés.

Sources primaires consultées : [C++ : cstring.syn](https://eel.is/c++draft/cstring.syn)
renvoie à la bibliothèque C ; [WG14 N1570 §7.24.1¶2 et §7.1.4](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
maintient l'exigence de pointeurs valides quand n=0.
Ce constat est obtenu par suivi du code et des préconditions, sans affirmer
un crash ni un diagnostic de sanitizer observé.

## Points positifs et portée

Le contrôle de longueur SHA-256 précède la somme et protège le codage 64 bits ;
le refus est sans mutation. Le digest travaille sur une copie et préserve
l'absorption suivante. Les deux blocs du bourrage à 56..63 octets sont présents.
Les constantes sont contrôlées en entier à la compilation.

`parse_integer` consomme tout le jeton, refuse signes/base/suffixes inadmissibles,
borne la magnitude avant produit et compare en i128 avant conversion : la lecture
statique ne retrouve pas les anciennes troncatures de CLI. `write_all` traite
EINTR, sorties partielles et progrès nul ; `Descriptor` déplace la propriété
et ne rejoue pas close interrompu sur Linux. Aucun défaut supplémentaire établi
sur ces copies. Formats d'entrée/sortie, pas exact/origine et transactions finales
ne sont pas encore écrits dans cette capture et ne sont pas jugés ici.

`SOURCE_BEFORE.json` et `SOURCE_AFTER.json` distinguent la capture de la recoupe.
Le registre ci-dessous ferme toutes les pièces de cette lecture ; il ne qualifie
aucun module v11.
