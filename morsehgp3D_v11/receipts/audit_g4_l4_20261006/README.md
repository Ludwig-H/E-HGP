# L4 recouvrement : lecture du reçu G4 clos

Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. L’auditeur n’a lancé aucun build,
test natif, calcul HGP, réseau ou accès cloud. Aucune donnée LiDAR, sortie
binaire native ou journal brut n’est copié dans cette capsule.

Session `v11.20261006.claudeL4`, arrêt ciblé certifié le 6 octobre à
07:29:37 UTC, statut **completed**, worker 0. Source expérimentale
`cf28afb04040eacf1eb92080d75e8a0ebb58a4eb` ; 636 fichiers utiles du paquet
correspondent exactement aux objets Git. Archive :
`0a1e94c0e8188bbd004d2d95d7b99622809a8b19e91d2d1a37fed6bf79501611`.

Les rapports K5 et K10 ont chacun **conforme**, sans refus : cette
conformité du banc porte sur les statuts, sorties et registres, pas sur
l’atteinte des critères de gain. Les pièces et empreintes du reçu publié
en `830473218` sont vérifiées contre la session locale.

## Périmètre effectivement conservé

Trois trames entières ng00/ng01/ng02, W48, u21, trois modes dans le même
binaire : CPU FULL/16379, GPU série/81915, GPU recouvert/212987. K5 avec
feuilles 16 : cinq processus froids par mode et par trame, puis un
processus résident de douze passes. K10 avec feuilles 24 : trois froids,
puis un résident de huit passes.

Cela donne **90 processus, 252 passes construites et 90 dumps contrôlés**.
Les 72 froids et les dernières passes des 18 processus résidents ont les
empreintes canoniques attendues. Les 162 passes intermédiaires ont un
statut `ok`, sans dump ni registre propre conservé. L’égalité du registre
de chaque sortie conservée est attestée par le juge au pin exécuté et
son verdict ; les lignes natives complètes par prise ne sont pas
archivées. Cette limite est déclarée aussi par le banc.

Tous les `unresolved` conservés valent zéro. Aucun API/CLI ni sanitizer
n’est qualifié par ce plan. Les succès observés restent indépendants du
défaut de durée de vie sur refus identifié dans `audit_overlap_20261006`.

## Décision de performance

Le recouvert échoue aux deux critères du plan sur les trois trames :
K5 ≤ CPU résident, K10 ≤ 0,85 × CPU résident. Sur les passes 2..P,
l’intervalle entier du domaine recouvert dépasse celui du CPU :

| K | CPU résident | Recouvert résident |
| --- | ---: | ---: |
| 5 | 167 à 210 ms | 320 à 357 ms |
| 10 | 649 à 829 ms | 2 660 à 2 731 ms |

Les six comparaisons exactes par trame sont dans `summary.json` ; cette
conclusion ne dépend pas d’une discussion de médianes. Le GPU série
reste un bras distinct, avec un domaine K10 inférieur au CPU dans les
passes conservées.

Le complément extrait **les six dernières passes chaudes** GPU série
contre recouvert : count, fill, exécuteur, attente, queue, pools et
nombre de lots. À K10 ng00, fill passe de **111 438 610 ns** à
**1 564 411 926 ns** ; count de **215 321 341 ns** à **770 558 610 ns**.
Le supplément de fill représente environ 72 % du supplément de
l’exécuteur, sur cette prise ; les deux autres trames K10 donnent la
même attribution bornée. Ce constat justifie l’ablation de répartition
du fill proposée dans l’audit du plan, sans qualifier un gain futur ni
une cause générale depuis une seule prise.

Le retrait publié **830473218e1b35fa725209df14b7855667f4c6d4** restitue
exactement les fichiers `src/` de la base `8ee28873f`. Les défauts de
durée de vie et de garde du recouvert sont donc clos **par retrait**,
sans qualification des chemins de refus. Le reçu expérimental reste
figé ; aucun gain L4 ni contrat de 100 ms n’est acquis.

Le rejeu contrôle aussi l’égalité exacte avec le parent précédant L4,
`c1675e4c9`, sur `src/`, `bench/full_probe.cpp`, `tests/` et le CMake
principal. L’ablation du fill éclaire une partie du surcoût ; elle ne
répare pas à elle seule toute la voie L4.

## Rejeu de lecture

```sh
python3 -B replay.py
python3 -B -O replay.py
```

Les deux sorties sont identiques. `--repo` et `--session` adaptent les
chemins. Le rejeu dépend des objets Git et des archives locales de
`v11.20261006.claudeL4` ; il ne reconstruit ni ne rejoue le moteur.
Cette capsule est une trace compacte de lecture, pas une archive native
autonome. `SHA256SUMS` couvre les trois autres fichiers.
