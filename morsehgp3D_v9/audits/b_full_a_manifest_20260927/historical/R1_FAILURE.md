# R1 : commandes réussies, clôture dépendances refusée

Le handle 77819 a terminé avec code 1 après les 17 commandes réussies :
Release GCC, Clang ASan/UBSan/LSan, gates complets identiques et refus CLI.
L'exception finale exacte est `RuntimeError: actual compiler dependency inclusion`.
Le fichier `capture.json` conserve `status=completed` pour les commandes,
mais aucun `summary.json` de qualification n'a été publié. **R1 non qualifiée**.

Cause du harnais : les scans préalables `-M` n'incluaient pas les options
d'optimisation ni sanitizers. Les `.o.d` effectifs comportaient donc des
headers glibc fortify activés par `-O3`/`-O1` et l'ignorelist ASan non
épinglés avant compilation. Le lecteur les a correctement refusés.
Aucun défaut C++/géométrie/sanitizer n'est déduit de cet échec de preuve.

Les reçus/builds R1 restent inchangés, `r1_run.py` conserve le runner alors
exécuté (SHA256 original `82d67e29e4b8061881fcc8bd4fe6c6ea4b148c509420e09d20e9c3e5fe100982`).
Seul le runner est corrigé pour R2 : mêmes options dans le scan préalable
et la compilation, et conservation explicite d'un éventuel refus du
lecteur final. Toutes les sources C++ restent identiques. La copie
historique n'est pas relogeable telle quelle (`HERE` change), et ne devient
pas un lecteur LIVE réhabilitant R1.
