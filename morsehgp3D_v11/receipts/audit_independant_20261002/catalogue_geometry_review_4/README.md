# Contre-relecture géométrique du catalogue natif

Verdict ciblé : aucun défaut concret trouvé dans les règles de complétude,
de census, de support canonique ou de propriété du port figé
`f391bf13e1a9a982025bde86fc9219b5b7430afc` (produit `643fe47d7`).
Cette conclusion est une relecture et une vérification de certificats
rationnels ; aucune compilation, exécution native, GCP, FULL ou mesure
LiDAR n'a été réalisée.

Les 49 fichiers de `source/` viennent des objets Git du commit, avec
empreintes avant et après dans [SOURCE_BEFORE.json](SOURCE_BEFORE.json)
et [SOURCE_AFTER.json](SOURCE_AFTER.json). Les sources de travail en cours
ne sont pas l'autorité de ce reçu. [RUN.json](RUN.json) conserve les deux
commandes et leurs premières sorties.

## Raisonnement de complétude

Le domaine examiné est celui de sites distincts non pondérés, profils
18/21/24 bits, K dans 1..12, nuage stable pendant l'appel. Une boule
critique positive admise satisfait `p + q_min <= K + 1`, donc `p <= K - 1`.
Un support minimal positif est affinement indépendant, de cardinal 2 à 4.

1. **Liste certifiée.** Un site parmi les K plus proches, ex æquo inclus,
   a moins de K sites strictement plus proches. Il ne peut donc avoir K
   témoins distincts qui le dominent strictement sur toute la boîte fermée.
   Le minimum de la différence des distances carrées est affine et atteint
   un sommet de cette fermeture : c'est exactement la condition utilisée
   dans [boxes.cpp](source/morsehgp3D_v11/src/catalogue/boxes.cpp#L16).
   Le choix du réservoir n'affecte que l'efficacité ; ses témoins peuvent
   eux-mêmes être retirés dans le même filtre, sans invalider le certificat.
   L'induction se transmet aux sous-boîtes. Pour K > n, aucun site ne peut
   avoir K dominateurs distincts ; tous restent disponibles.
2. **Census global.** Si p < K, tout site de la boule fermée a moins de K
   sites strictement plus proches du centre et appartient à la liste. Si
   p >= K, celle-ci contient au moins K intérieurs. Un census local accepté
   avec `p <= K + 1 - q <= K - 1` est donc global et complet, même sur une
   coquille étendue. Le retour anticipé de
   [leaf.cpp](source/morsehgp3D_v11/src/catalogue/leaf.cpp#L68) ne tronque
   que les présentations rejetées.
3. **Préfixes.** Au centre de la boule, un dominateur strict de l'un de ses
   sommets de coquille est un intérieur. Leur union, comptée une seule fois,
   est donc incluse dans I. Pour chaque préfixe de taille r du support
   canonique, son cardinal est au plus `p <= K + 1 - q_min <= K + 1 - r`.
   Aucun de ces préfixes n'est éliminé. Le DFS poursuit les triplets obtus
   vers q4 ; l'absence de boule q3 n'est pas un rejet de préfixe.
4. **Propriétaire.** Le centre est dans l'enveloppe convexe de son support,
   dont tous les sites restent dans la liste. L'enveloppe entière
   `[min, max + 1)` conserve donc ce centre, même à un maximum. Les coupes
   demi-ouvertes lui donnent une seule feuille ; leurs certificats restent
   calculés sur les fermetures. L'ajustement ne peut perdre un centre admis.
   Une feuille trop large ou une limite de ressource rend un refus complet.

Le support canonique est trouvé dans la coquille complète, d'abord par
cardinal puis par SiteIdx : paire de milieu égal au centre ; triangle aigu
coplanaire avec ce centre ; tétraèdre de poids strictement positifs. Pour
des sites co-sphériques, ces tests caractérisent les présentations de la
même boule. Voir [support.cpp](source/morsehgp3D_v11/src/catalogue/support.cpp).
Lorsque `m == q`, un support aigu/strict possède déjà le cardinal minimal :
un sous-support serait une face propre dont l'enveloppe ne contient pas
son centre intérieur. L'ordre des SiteIdx est préservé depuis la liste
racine, par tous les filtres puis par les scans I/U.

Une présentation d'arité supérieure peut être rejetée au seuil
`K + 1 - q`, alors que la boule est admise par son plus petit support.
Cela est sûr : le DFS visite également le support canonique dans la même
feuille, et c'est cette présentation qui doit être émise. Il n'est pas
nécessaire de conserver toutes les présentations pour obtenir CatK.

## Contrôles et limites

[check.py](check.py) n'importe aucun module produit ou juge développeur.
Il résout de petits systèmes rationnels et vérifie les certificats à partir
des distances aux sommets fermés, avec quatre nuages de 4 à 9 sites. Les
deux lectures `python3 -B check.py` et `python3 -O -B check.py` donnent
**21 327 gardes identiques**, dont 51 gardes d'empreintes, sur **109 boules**.
Les contrôles incluent deux filtrages successifs, les ex æquo aux frontières,
des supports q3/q4 partageant une coquille, un q4 à préfixe obtus et le
raccourci `m == q`. Ils illustrent les preuves ci-dessus ; les échantillons
finis de centres ne remplacent pas la preuve affine sur toute la boîte.

Les limites de feuille (`<= 1024`, 16 mots de masque), les indices
`N < kNone`, les offsets u64 et les rangs avec zéro sont cohérents à la
lecture. Les cas virtuels proches de kNone n'allouent aucun tableau.
Les budgets `2B + 5 <= 63` et `5B + 6 <= 127` conviennent aux trois profils ;
ils ne se transfèrent pas à un moteur brut u32. Le potentiel des largeurs
explique la profondeur 3B, pas le nombre de feuilles ou de candidats.

Le [juge développeur Fraction](source/morsehgp3D_v11/tests/catalogue/fraction_model.py)
est réellement distinct du générateur natif : systèmes de Gram, groupement
centre/rayon exacts, census global et support minimal, sans import R2 ou
référence FULL. Son comparateur vérifie nombre complet de boules, niveaux,
rangs, I/U et supports, y compris les boules inertes. Les contrôles natifs,
mutants et refus doivent encore être jugés par leurs propres reçus G4 ;
ce reçu ne les rejoue pas et n'en hérite aucune qualification. Le coût de
canonicalisation et les grosses coquilles restent une question séparée.

Après fermeture, le lecteur vérifie aussi chaque entrée de `SHA256SUMS`
si ce manifeste est présent. Les chemins sont relatifs ; le reçu reste
lisible sans le dépôt ou les builds d'origine.
