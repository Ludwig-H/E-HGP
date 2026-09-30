# Audit indépendant : arithmétique, nuage et catalogue

29 septembre 2026. Sujet figé : `6206d1d118794c9e1cabb6faeaec2aaa77d37e5b`. Les lectures ont porté sur
`src/arith/`, `src/cloud/`, `src/catalogue/`, les appels correspondants dans la tour, la porte catalogue, les unités,
`docs/SPEC_V10.md` et `docs/conception/GEN_v2.md`. Les sondes utilisent le snapshot sous
`build/v10-audit-independent-20260929/source/morsehgp3D_v10` et sa bibliothèque Release ; aucun source produit,
build épinglé ou état GCP n'a été modifié.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=audit_independant_geometrie_catalogue
public_status=not_claimed
GCP non utilisé
```

## Verdict utile au développeur

Le chemin du catalogue apparaît cohérent avec son contrat **actuel** : filtres stricts de dominance, support
canonique survivant, recensement exact, propriété demi-ouverte et réparation exacte de l'ordre des niveaux. Aucune
boule fausse, manquante ou dupliquée n'a été trouvée dans les comparaisons bornées ci-dessous. Les bornes u18 des
calculs géométriques tiennent dans leurs types ; aucune raison mathématique de bloquer le chantier GPU sur cette
brique n'a été trouvée.

Un défaut concret existe dans le contrat de l'API rationnelle `SiteTree` : la marge flottante fixe est fausse pour
un centre circonscrit u18 très éloigné du nuage. Le chemin courant de la tour utilise des centres de miniboules
validées, ce qui lui donne l'invariant supplémentaire nécessaire. Il faut écrire et contrôler cet invariant dans
l'API pour qu'une future utilisation ne transforme pas cette dette en perte de coquille dans la tour.

## G1 — Contrat trop large de `SiteTree::closed_ball` et `nearest` (majeur pour l'API, portée produit bornée)

[site_tree.hpp:28](../../../../src/cloud/site_tree.hpp#L28) accepte un centre rationnel issu des formes q3/q4 et promet les requêtes exactes sans
imposer que le centre reste dans le domaine du nuage. [site_tree.cpp:64](../../../../src/cloud/site_tree.cpp#L64) fixe `kMargin=0.02` ; `closed_ball` élimine
un site par sa distance approchée avant sa clé exacte ([site_tree.cpp:212](../../../../src/cloud/site_tree.cpp#L212)). La justification « erreur < 1e-3 en
u18 » exige aussi une borne sur la position du centre. Des sites u18 ne suffisent pas à la garantir.

Quatre sites valides suffisent : `(0,0,0)`, `(225077,1,0)`, `(225068,1,0)`, `(152369,7,1)`. Leur tétraèdre est non
coplanaire et son centre exact est `(225072.5, -25328815117.5, 154615790175.5)`. Les quatre clés de puissance exactes
sont nulles. La distance approchée du dernier site dépasse celle de l'ancre de **4 194 304**, bien au-delà de 0,02.

La [sonde C++](../../probes/geometry_site_tree.cpp), liée à la bibliothèque figée, donne :

```text
strictly_inside=0 expected_shell=4 got_shell=3 got_interior=0
site=1 point=(152369,7,1) exact_side=0 approx_delta=4194304 in_shell=0
nearest_count=3 got=3 exact=0
```

`nearest(count=3)` conserve trois résultats mais perd le départage exact par indice des ex æquo. Le
[reçu](../../geometry_site_tree_result.json) conserve la commande, le code **1** et la sortie ; le code 1 est l'écart
attendu de cette sonde adverse.

**Portée.** Les deux appels de recensement de la tour (`tower.cpp:886` et `:897`) portent sur une MEB. Le chemin
rapide `verify_meb` contrôle l'enveloppe convexe fermée du support avant son filtre de distance (`:466`) ; le repli
Welzl produit la MEB finale (`:528`). Ces centres appartiennent donc à l'enveloppe du nuage u18. `nearest` utilisé
pour l'attache des points a un centre égal à un site (`:1621`). Aucun appel produit passant notre centre adverse
n'a été trouvé. La validation indépendante du repli appartient à l'audit de la tour.

**Correction conseillée.** Rendre explicite et vérifiable le domaine de centre servi par le filtre : centre de MEB
validée u18, ou garde rationnelle du cube u18. Pour une API qui conserve sa généralité actuelle, utiliser une
borne dépendant du centre ou un repli entièrement exact hors de ce domaine. Ajouter cette fixture à l'unité
rationnelle, dont le commentaire revendique actuellement des centres de sphères arbitraires (`unit_main.cpp:322`).
Il n'est pas nécessaire d'invalider les mesures LiDAR pour traiter cette dette.

## G2 — Conception GEN v2 et implémentation pondérée divergent (dette de raccord, pas perte d'exactitude)

[GEN_v2.md:94](../../../../docs/conception/GEN_v2.md#L94) impose l'admission positionnelle unique `p_w + q_min <= K+1` et supprime explicitement l'ancien
sur-ensemble pondéré. Le code courant conserve le contrat de [SPEC_V10.md:38](../../../../docs/SPEC_V10.md#L38) : `judge` admet une coquille pondérée
si `p<K` ([generator.cpp:262](../../../../src/catalogue/generator.cpp#L262)) et les seuils q3/q4 sont relâchés dès qu'un site pondéré est dans la liste de feuille
([:341](../../../../src/catalogue/generator.cpp#L341)). Ce régime est conservateur ; il peut payer davantage de candidats et de boules sans perdre une boule.

Notre campagne trouve **111 occurrences** d'enregistrements pondérés admis par le contrat actuel et exclus par
la règle plus serrée de GEN v2. Ces occurrences couvrent plusieurs K du même nuage ; ce ne sont pas 111 identités
de boules distinctes. Aucun désaccord avec SPEC n'est observé. La tour refuse toujours les multiplicités : ceci
ne modifie donc pas les mesures de tour LiDAR de poids 1.

Le passage de la conception à la voie pondérée doit réunir seuils de feuille, admission, oracle pondéré, reçus et
texte d'état dans une seule tranche. Ne pas promouvoir l'ancienne campagne de la sonde de conception en preuve
de cette nouvelle implémentation. De même, GEN v2 retire la stagnation arbitraire, alors que le produit dispose
d'un arrêt conservateur après neuf niveaux sous le pas de grille (`generator.cpp:23`). L'arrêt énumère encore la
feuille, ou refuse explicitement si elle dépasse `max_leaf` : il ne tronque pas une sortie acceptée.

## Vérifications mathématiques et expérimentales

Les points suivants ont été relus causalement :

- Une domination est **strictement** valable sur toute la boîte fermée. Son poids prouve l'exclusion d'un site du
  voisinage K-NN fermé ; l'induction sur les listes reste valide pour un réservoir partiel de témoins.
- Une coquille dont l'intérieur pèse moins de K reste entièrement dans la liste certifiée. Les seuils des unions
  de dominateurs ne peuvent donc éliminer son support canonique admis.
- Le lemme Z du code est la séparation exacte du zonogone affine image du pavé ; ses générateurs non nuls
  donnent les normales suffisantes de ses côtés. La version binaire à côtés distincts conserve cette propriété.
- Ajuster la boîte à l'enveloppe de la liste, augmentée d'une unité sur le bord haut entier, conserve tous les
  centres critiques présents : leur centre appartient à l'enveloppe convexe de leur support. Le partage au milieu
  est demi-ouvert ; aucun choix flottant ne décide du propriétaire d'une boule.
- Les formules q3 sont le Gram/croisement exact et les q4 sont Cramer avec normalisation `D>0`. L'aigu strict et
  les quatre orientations de faces refusent les supports avec poids barycentrique nul. Les cas de bord sont
  récupérés par les supports de moindre cardinal.
- Avec `E<2^18`, les majorants conservateurs `288 E^6` pour le recensement q3 et `180 E^6` pour l'intérieur q4
  sont inférieurs à `2^117`. Les orientations de centre q3 sur une autre face passent en arithmétique large.
  Les produits croisés des niveaux restent largement sous 320 bits. Les opérations larges qui ignorent un booléen
  de débordement sont sûres **dans ces bornes**, pas pour un `Center` arbitraire construit à la main.
- Le tri flottant des niveaux est seulement une proposition ; les bandes réparées et la comparaison exacte des
  voisins valident l'ordre final, puis les rangs denses des plateaux. Aucun niveau approché ne décide d'une fusion.

La [campagne](../../probes/geometry_catalogue_probe.py) compare au calcul exhaustif `Fraction` indépendant des prédicats C++
dans la référence du dépôt. Elle contrôle les **enregistrements entiers** : cardinal minimal, support canonique
Morton, I et U complets, poids et drapeaux, unicité et rangs exacts. Elle ne se réduit pas à un nombre de boules.

| Couverture | Résultat |
| --- | --- |
| 16 nuages, chacun sans puis avec multiplicités | 32 entrées ; poids 1 à 4 |
| K=1,2,5,10 ; 1 et 4 fils ; feuilles 2,3,12,64 selon le régime | 128 appels ; 126 terminés, deux expirations bornées |
| Extrêmes 0/262143, génériques u18, plans, rectangle, triangle aigu, cube, octaèdre, tétraèdre presque plat | 2 783 boules terminées ; zéro écart de géométrie/canonique/rangs |
| Deux cas expirés rejoués avec feuille 12 | 27 et 21 boules, égales à l'oracle ; 48 boules supplémentaires |

Les deux expirations à 2 s viennent du cube de huit sites et de l'octaèdre de six sites à `K5, --leaf=3`. Ce
paramètre impose des subdivisions alors que les listes de cinq voisins ne peuvent se réduire à trois. Les
[retests](../../geometry_catalogue_timeout_default_retests.json) avec feuille 12 terminent avec `catalogue_s=0.0001` dans
les deux cas ; le défaut K5 vaut 16. Ceci motive une recommandation de paramétrage (`M>=K`, plus marge sur les ex
æquo), pas une réfutation de l'algorithme ni un résultat de performance LiDAR. Les expirations sont conservées
dans le [reçu](../../geometry_catalogue_results.json), avec leurs entrées exactes.

Une première tentative utilisait aussi les très petites feuilles sur les nuages u18 étendus ; elle a expiré à
20 s sans journal par cas. Son erreur de collecteur et le retest de son hypothèse de première entrée sont conservés
dans [geometry_first_timeout.json](../../geometry_first_timeout.json). Ce premier essai ne sert pas de preuve causale
pour une entrée particulière. La reprise écrit désormais chaque cas avant de poursuivre.

**Limites.** Ce lot ne qualifie pas le pire cas, le nombre global de candidats, la vitesse du GPU, la tour pondérée,
la concurrence par TSan ou une population LiDAR. Il complète les portes produit par des extrêmes et des poids,
sans remplacer les reçus de qualification attendus pour chaque port.

## Reproduction bornée

Depuis le worktree `build/v9-open-worktree` :

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -I /workspaces/E-HGP/build/v10-audit-independent-20260929/source/morsehgp3D_v10/src morsehgp3D_v10/receipts/audit_independant_20260929/probes/geometry_site_tree.cpp /workspaces/E-HGP/build/v10-audit-independent-20260929/release/libmhgp10_core.a -pthread -o /workspaces/E-HGP/build/v10-audit-independent-20260929/geometry_site_tree
/workspaces/E-HGP/build/v10-audit-independent-20260929/geometry_site_tree
python3 -B morsehgp3D_v10/receipts/audit_independant_20260929/probes/geometry_catalogue_probe.py --build /workspaces/E-HGP/build/v10-audit-independent-20260929/release --source /workspaces/E-HGP/build/v10-audit-independent-20260929/source/morsehgp3D_v10 --output /tmp/geometry_catalogue_replay.json
```

La première sonde rend 1 sur le défaut G1. La campagne rend 1 si elle reproduit les deux expirations de paramétrage
adverse ; lire le détail du reçu pour distinguer expiration de budget et désaccord géométrique. Aucun gate n'est
porté par `assert` ; la sonde Python conserve ses contrôles sous `-O`.
