# Contrelecture S5 — 5 octobre 2026

Corrections **présentes dans la copie WIP**, pas qualification native acquise. Norme publiée `9cbf805c6`, acteur S5 détaché `f98aeed67` avec modifications non commises. Les [45 sources](SOURCE_BEFORE.json), dont les [cinq compléments](ADDITIONAL_BEFORE.json), sont identiques à la [fin de lecture](SOURCE_AFTER.json). Le [rapport de vérification](sources/reports/verif_s5.md) de 06:23 juge une prise antérieure : ses anciens survivants ne sont pas des défauts actuels redémontrés.

| Objet | Source actuelle capturée | Porte préparée |
| --- | --- | --- |
| D.2 | [manifest.cpp](sources/s5/src/api/manifest.cpp), 186–221 et 309–310 : V2, géométrie, naissance, enfants, ordre K publié | [session_test.cpp](sources/s5/tests/api/session_test.cpp), 292–328 : champ réellement publié K1..4, encodage manuel et cinq signatures gravées |
| D.3 | [directory.cpp](sources/s5/src/io/directory.cpp), 217–243 : SHA après fermeture et avant publication ; [manifest.cpp](sources/s5/src/api/manifest.cpp), 224–265 : retrait ou état `published_complete` avec SHA | [publish_test.cpp](sources/s5/tests/api/publish_test.cpp), 132–225 ; [cli_contract.py](sources/s5/tests/cli/cli_contract.py), 362–421 : échecs après commit, fin de Session, stdout et retrait impossible |
| SIGXFSZ | [mhgp11.cpp](sources/s5/cli/mhgp11.cpp), 328–329 : ignoré, comme SIGPIPE | [cli_contract.py](sources/s5/tests/cli/cli_contract.py), 245–257 et 362–394 : signal remis par défaut avant exec, limite RLIMIT_FSIZE, refus structuré sans signal |
| Ordre/identité | [mhgp11.cpp](sources/s5/cli/mhgp11.cpp), 182–198 et 326–343 : inode après `stat`, O_RDONLY, options → stdout → dossier → lecture | [cli_contract.py](sources/s5/tests/cli/cli_contract.py), 155–221 : doubles fautes, alias points/IDs et lien symbolique |

**Limite pertinente du harnais de faute.** [io_fault_preload.cpp](sources/s5/tests/cli/io_fault_preload.cpp), 72–93, consomme six `va_arg(long)` ; l'appel du produit ([directory.cpp](sources/s5/src/io/directory.cpp), 114) fournit cinq arguments `int, const char*, int, const char*, unsigned`. Cela sort du contrat `va_arg` : [N1570 §7.16.1.1](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf), repris par [N4861 §17.13.1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/n4861.pdf). Corriger ce seul hook pour décoder ces cinq types ; un syscall inattendu doit invalider explicitement la porte. Cela concerne les deux cas CLI de double échec injecté. Aucun bug produit, crash observé ou réfutation des succès locaux ABI x86-64 n'est revendiqué. Références primaires et portée dans [PRIMARY_STANDARDS.json](PRIMARY_STANDARDS.json).

**Preuve obtenue.** [check_s5_contract.py](check_s5_contract.py) joue seulement les méthodes Python de jugement extraites par AST des copies, sur quinze issues factices ; contrôle les 60 mutations déclarées (18 API, 20 CLI, 22 IO : cible unique et remplacement distinct), et le contrat scalaire d'arité du hook. **168 contrôles nouveaux**, normal et `−O`, codes 0, sorties identiques. Aucun import produit, sous-processus natif, fit, build, cloud ni rejeu des anciens 83/84 contrôles. La première erreur de regex de notre lecteur est conservée dans [reader_preflight](reader_preflight/attempts.json), sans attribution au produit.

Les cinq signatures gravées comprennent **trois fixtures de référence indépendantes** et **deux régressions dont la structure vient du natif** : les secondes ne deviennent pas des oracles géométriques indépendants. Les 60 définitions de mutants ne prouvent pas ici 60 mises à mort. [tests.cmake](sources/s5/tests/cli/tests.cmake), 21–33, annonce 64 refus CLI en Release et 61 sous ASan/TSan : les deux fautes IO préchargées et le cas FENV préchargé y sont omis. Les nouvelles portes, profils, scénarios entiers et mutants restent à exécuter sur G4 après intégration et gel commun. Les chiffres de l'ancien rapport local ne qualifient pas cette prise.

Rejeu portable, bibliothèque standard seule :

```sh
python3 -B -S check_s5_contract.py
python3 -B -O -S check_s5_contract.py
```

[BILAN.json](BILAN.json) contient les limites exactes et les sorties. `LEDGER.json` inventorie tous les payloads hors lui-même et le SHA racine ; `SHA256SUMS` inventorie tous les fichiers, LEDGER compris, à l'exception de son seul fichier racine. Aucune ancienne capsule, source d'acteur ou note active modifiée.
