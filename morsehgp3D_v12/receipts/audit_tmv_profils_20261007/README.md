# Tour : clôture des traces locales des profils 24/32 — 7 octobre 2026

`phase=exploration_v12_hors_registre`, `backend=cpu_reference`,
`public_status=not_claimed`. Complément unique à
`../audit_tmv_traces_20261007/`, conservé inchangé. Lecture de métadonnées et
sources seulement ; aucune compilation, commande native ou donnée rejouée.

Pour **chacun** des profils 24 et 32, les traces du prototype indiquent :

- construction Release et suite CTest terminées avec code 0 : 652 sélectionnés,
  651 réussis, une sentinelle LiDAR sautée ;
- neuf cas MES-M0 en mode `graines` (trois lots disjoints), plus ng00 K5 en
  mode `v12`, tous conformes, codes 0 ;
- comparaison à un fil contre huit fils avec tranches de 97 événements.
  Le critère est l'empreinte **sémantique**, après relecture stricte du FUL1,
  puis le déterminisme des empreintes d'octets et des compteurs entre fils
  au sein du même profil. Aucun JUG-EMST dans ces appels larges.

Le script emploie **les mêmes chemins d'entrée et les mêmes vidages v11**
qu'au profil 21. `tower_dumps.cpp` charge les entiers sans changement
d'échelle et exige que SITEXYZ du catalogue v11 corresponde au nuage.
L'élargissement porte sur le domaine du build et les largeurs d'entiers
exportées, pas sur une nouvelle quantification. Pour des coordonnées dans
le domaine u21, les bits supérieurs nuls préservent aussi l'ordre de Morton.
`full_reader.py` contrôle le profil d'en-tête et le domaine, puis normalise
les rationnels exacts indépendamment du nombre de mots. L'égalité sémantique
est donc le bon critère ; l'identité binaire entre profils n'est pas exigée.

Ces MES-M0 **ne reconstruisent pas le catalogue et G** : ils adaptent les
vidages v11 vers T/M/V/R. Le mode `v12` est une règle de cibles dérivée de ces
vidages, pas une nouvelle résolution G native. Les neuf cas de chaîne G→T
complète restaient joués au profil 21 dans le reçu précédent. Ne pas annoncer
ici neuf scènes nouvellement quantifiées en u24/u32, ni une campagne GPU ou
un contrat de latence acquis. La suite CTest comprend ses autres témoins
bornés, que cet audit n'a pas rejoués individuellement.

Les caches build24c/build32c pointent repo3 sur base 9c5809919 ; ce sont des
preuves locales du prototype, pas une qualification du HEAD de main. Les
sources export/adaptateur/lecteur/R sont restées identiques entre l'ouverture
du suivi du profil 32 et cette clôture. L'arbre courant correspond toujours
au rapport mutant antérieur (357 fichiers, `c1950bb8…`). Les sorties temporaires
MES-M0 sont supprimées par le pilote : ce reçu contre-lit ses traces et son
contrôle, sans rehasher les données ni recalculer la géométrie. Les hashes des
binaires observés ne reconstruisent pas une chaîne de compilation historique.
Le patch de livraison a été régénéré après la première capture non publiée :
`0968c581…` → `39622284…`. Seul ce conditionnement a changé ; toutes les
sources, les binaires et les traces épinglés restent identiques. Le nouveau
patch inclut R, toujours au corps `9cc388d6…` : son admission `4 * output`
omettant les offsets CSR finaux reste à corriger selon le reçu
`../audit_registre_branches_20261007/`.

`git apply --cached --check` réussit sur l'index propre de main `7e87b58d2`,
sans le modifier. Une extraction temporaire des seuls 31 fichiers concernés
depuis ce commit accepte le patch : 30 fichiers résultants sont identiques
au prototype. La seule différence, `tests/tower/tests.cmake`, conserve les
cinq portes récentes du juge G déjà présentes sur main. Le patch ne modifie
pas `g_determinism.py` : ne pas recopier à sa place l'ancienne version du
prototype. L'empreinte agrégée des 31 résultats est dans `capture.json`.
Cela vérifie l'application textuelle, sans qualifier nativement la combinaison
obtenue ni transférer les 652 sélections historiques à cette nouvelle suite.

## Matrice de reprise à la livraison

Contrelecture statique complémentaire de perf_math, sur les mêmes sources ;
registre observé `b06cd1449`, **aucun changement d'état dans ce reçu**.

| Constat | Éléments présents dans le prototype | Portée avant livraison |
| --- | --- | --- |
| CST-0105 | Garde de `component_at` : naissance de rang 1 interrogée au rang 0 → `tower_query_domain` ; porte `verticales`. | Candidat à clôture après intégration vérifiée. |
| CST-0107 | Naissances triées par rang puis centre exact ; permutation carrée gravée ; mutant `naissances_par_cle` tué. | Même réserve de livraison ; traces MES-M0 favorables. |
| CST-0212 | Limites nombres ≤2^31−1, événements ≤2^31−2, nœuds ≤2^32−3 ; garde avant lecture/allocation, refus 2^31 et budget inchangé. | Déjà clos au microbanc ; qualification produit à étendre, sans prétendre avoir construit ces tailles extrêmes. |
| CST-0106 | LEM-T7 encore réservé à une tranche ultérieure ; témoin cercle25 à quatre sites absent. | Reste ouvert. `circle25_pair` à cinq sites teste les formes et ne le remplace pas. |

## Relecture reproductible

```sh
python3 -B -S check.py --prototype DOSSIER_TMV --git-repository DEPOT
python3 -O -B -S check.py --prototype DOSSIER_TMV --git-repository DEPOT
```

Les sorties sont identiques au champ `result` de `capture.json`. Le lecteur réutilise seulement
la fonction de hash d'arbre du reçu antérieur épinglé, vérifie les deux
cohortes, les douze codes, les caches et les hashes avant/après. L’option
`--git-repository` rejoue aussi l’extraction et l’application en dossier temporaire
depuis le commit épinglé, sans lire ni modifier l’index courant. Aucun payload
licencié, chemin privé ou identité de compte n'est copié dans ce reçu.
