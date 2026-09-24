# Contre-audit B — clôture requise pour les épingles brutes de C

24 septembre 2026, lecture pendant le calcul CPU local, sans toucher
aux processus ni aux fichiers de C. Script actif `pin_raw.sh` SHA-256
`12088f8386d18d6f1225c53f61c771638c26949dc6098f89e1f3643b50697461` ;
le [README de C](c_raw_pins_20260924/README.md) est encore provisoire.
Ces essais utilisent les trois **trames entières avec sol** de la
séquence 08 sur grille entière 1 mm, K5/K10/s8/W8, moteur CPU et lots
CPU ; ils ne sont ni float32 originaux, ni GPU/G4, ni plusieurs séquences.

Le script a capturé `BASE.txt=093d943cee7bd2a465a034f8f9bae879a0cd5f5b`,
et le binaire a été construit d'une archive de ce SHA. C'est une base
précise, mais le libellé « `origin/main` courant » du README n'est plus
exact une fois `main` avancé. Le SHA du binaire et les empreintes des
trois entrées doivent être épinglés **intégralement** dans le reçu ; les
entrées sont empruntées au reçu v8, pas à l'archive du code. Une
vérification indépendante de leurs trois SHA/taille/FNV a passé, mais
le script ne la fait pas lui-même.

`pin_raw.sh` n'active que `set -u` et enregistre le code de retour de
chaque sonde sans l'exiger nul ; après la boucle, il crée `DONE`
**inconditionnellement**. Il ne lit pas les JSON pour contrôler statut,
schéma, nombres de sites, digests de tour/catalogue/présentations ou
nœuds par ordre, ni les six égalités moteur↔lots. `DONE` doit donc être
interprété comme **fin de boucle**, jamais preuve de 12 succès. Aucun
échec des cas déjà terminés n'est allégué : les premières paires
relues sont concordantes.

Avant publication et avant utilisation comme épingles R21 : fermer les
12 codes de retour et 12 statuts `complete_relative`, les six paires
de digests de tour, catalogue **et présentations**, les dix nombres
de nœuds à K10 et cinq à K5 pour chaque paire, les tailles/empreintes/FNV
de chaque entrée, SHA
source/binaire et la correspondance commande→sortie ; conserver aussi
les échecs plutôt que les écraser. Un lecteur indépendant normal/`-O`
doit refuser causalement un code non nul, un cas manquant ou un digest
muté. Les condensés seront des **références CPU absolues pour ces
entrées/ce moteur** une fois clos, sans prouver à eux seuls la
complétude mathématique au-delà de `complete_relative`.

GCP non utilisé dans ce contre-audit.
