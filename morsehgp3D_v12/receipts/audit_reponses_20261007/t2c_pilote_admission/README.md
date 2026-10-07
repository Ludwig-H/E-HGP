# T2-c : admission des prises du pilote avant publication

7 octobre 2026. Lecture du prototype `v12_tour_Gc/repo2`, non livré dans
`main` au pin `0377684ec10e8bd06210abb236f1b79d6be56c21`. Le pilote capturé
porte le SHA-256 `fa1b7cd6aa23609879d56594332aaf1eb91b4f8b5b61ab9bb4a4b31e79f6feac`.
Les quatre entrées de [capture.json](capture.json) ont les mêmes hashes avant
et après le rejeu. Ce reçu concerne le lecteur et le juge Python, aucune
performance mesurée ni défaillance d'une campagne G4 livrée.

## Deux contre-exemples causaux

1. `prise()` accepte des prises structurellement invalides. Un exécutable
   Python remplace la sonde et émet la forme JSON complète dérivée des C++
   épinglés : six `tour_g`, ordres 1..5 après la dernière passe, un digest
   final, puis `exit` avec `order: 0`. Le nominal passe ; code 2 et cinq
   passes pour six demandées sont refusés. Les cinq mutations suivantes
   restent `valide: true` : tous les `wall_ns` à `true` ou à `-1`, six
   indices `pass: 0`, configuration déclarée K10/W1/u24 pour K5/W48/u21
   demandée, suppression de tous les objets `ordre`. Les contrôles de
   `prise()` (lignes 223–237 du snapshot) comptent les lignes et utilisent
   `isinstance(wall_ns, int)`, sans lier les métadonnées à la commande.
2. `juger()` réauthentifie les octets des journaux mais ne les relit pas.
   Trois trames × huit tours × cinq bras donnent 120 journaux synthétiques
   complets, tous à 60 000 000 ns par passe : tous les leviers sont rejetés.
   Modifier seulement les 24 champs récapitulatifs `apres.g_ns` à
   30 000 000, en conservant journaux, hashes, `murs_ns` et tous les autres
   champs, fait **adopter `lot_t2c` et `G-L7`**, sans motif de refus.
   `verifier_journaux=True`, bootstrap 10 000, graine et règles inchangés.
   Les rapports apparents et leurs IC deviennent 0,5 sur les trois trames.
   Cela prouve une rupture entre preuve brute et verdict ; ces nombres
   sont des valeurs injectées, jamais des chronos.

[check.py](check.py) rejoue ces huit appels réels à `prise()` (sous-processus
Python très courts), puis ces deux appels à `juger()`. Les compteurs et
digests sont synthétiques : leur **forme** suit le producteur, leur
signification géométrique n'est pas qualifiée. Le programme ne construit
aucun binaire HGP, ne lit aucune donnée LiDAR et n'utilise aucun service.

## Empreinte de référence obsolète

Le pilote impose encore `231d826bb0d4fe57` pour ng00/K5, alors que le
`tests/tower/tests.cmake` capturé pour le producteur à empreinte d'objet
seul impose `e5a81154fb1b15f1`. Le test de préfixe du juge (lignes 319–320)
rejetterait cette nouvelle empreinte pourtant attendue par sa porte ; il
s'agit d'une comparaison statique des deux références, pas d'un résultat
natif obtenu ici. Les constantes uniformes sont également anciennes
(`5304…/fdd4…/9fd0…` contre `a40f…/cf7c…/d1f0…`), mais elles ne constituent
pas une condition du verdict K5 LiDAR étudié.

## Correction ciblée avant campagne

Employer un lecteur strict commun à `prise()` et à `juger()` : succès
complet, schéma JSON fermé/préenregistré, entiers non booléens et bornés,
séquence exacte des passes, configuration demandée, ordres et digest
final/exit conformes au producteur. Le rejeu de `juger()` doit recalculer
les murs et leur médiane chaude depuis les journaux validés, puis vérifier
leur concordance avec le récapitulatif. Épingler la référence d'objet
issue du même protocole d'empreinte que les bras, avec contrôle positif
sur la vraie forme émise. Pas de patch produit appliqué dans ce reçu.

La forme des diagnostics a évolué dans repo2 ; le lecteur doit connaître
les schémas effectivement produits par chaque bras, sans confondre les
anciens chronos `resolve_ns` englobants avec les nouveaux temps disjoints.
Ces contrôles restent structurels : l'oracle et les empreintes canoniques
assurent la sémantique de l'objet. Le protocole statistique, l'ordre des
bras et le coût catalogue + G sont contre-lus séparément.

## Relecture légère

Depuis ce dossier :

```sh
python3 -S check.py --check
python3 -S -O check.py --check
sha256sum -c SHA256SUMS
```

Normal et `-O` rendent les mêmes [résultats](results.json). Le snapshot
immuable permet la relecture même si le prototype change ; aucun transfert
automatique de ce constat à une future version corrigée. Le schéma test
reprend aussi `route_t1` dans les compteurs de travail et conserve le
`exit` final réel, deux détails à ne pas omettre dans les doubles.
