# Contrelecture B — juges q2/q3 de C, source v4

23 septembre 2026, lecture statique du commit `e2fd68662`. Aucun reçu
final v4 n'était encore publié à cette lecture ; la vérification
`verification_juge_q3.json` porte sur le SHA `c1befd6c…` d'une source
antérieure non publiée, **pas** sur la source v4 `0521f82e…`.

Deux corrections demandées pour v3 sont bien présentes : les cibles et
planchers de rang haut exigent maintenant les clés q3 régulières
`q_min=3`, `n_shell=3`, `p=K−2` (et q2 régulières pour le juge q2), et
le recoupement exige l'**égalité des listes triées d'IDs de coquille**.
Le mutant de doublon est prévu dans la recette. Les lemmes de
profondeur de Tukey et de marge de boîte u18 restent valables sous
leurs hypothèses déjà documentées.

Trois limites de portée/protocole demeurent avant d'exploiter un code 0 :

1. Le match q2/q3 ne reconstruit ni ne compare `ball.key` à une clé
   canonique issue de la sphère indépendante. Une clé corrompue avec
   niveau, coquille, intérieurs et arité inchangés passe. Le juge
   contrôle des présences **géométriques** échantillonnées, pas toute
   l'identité de catalogue consommée par FULL. Tuer un mutant de clé
   seule si cette revendication est souhaitée.
2. `run_judges_v4.sh` hache plus d'objets, dont le script et les
   bibliothèques, mais son bloc de provenance ignore toujours les
   codes d'échec de `git` et `sha256sum` (`set -u -o pipefail` sans
   `set -e` ni contrôle explicite). Les cas peuvent rendre `STATUS=0`
   avec une provenance incomplète. Chaque commande doit être bloquante ;
   le `git log -1 -- morsehgp3D_v9/src` épingle un historique, pas le
   lien démontré entre les archives `$BUILD` et ces sources.
3. Les sites du contrôle des longues ancres sont extraits des lignes
   `LONG_SITE` du **parcours déjà élagué**. Si toutes les incidences
   longues d'un site sont omises par un sur-élagage, il ne peut pas
   entrer dans ce contrôle. Sélectionner des sites longs depuis les
   coordonnées d'entrée, sans passer par le résultat du juge, et y
   tuer un mutant de sur-élagage.

Ces limites ne sont pas des omissions q3 observées du générateur.
Un futur reçu v4 peut conforter les sites tirés, sans prouver la
complétude globale ou la tour (`run_tower=false`).
