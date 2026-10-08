# Gc rebasé : table contiguë retirée, garde de domaine indépendante

8 octobre 2026, lecture et modèle Python seulement, hors registre, `not_claimed`.
Aucun natif, aucune modification de produit, aucun nouvel état de constat.

Le véritable patch actif est maintenant celui de `git3`, commit
**45976be8ddc489f14494937b13e43d0ca5fa0a55**, après le patch 1
**b1162872ab1577a473abe7dec666d67b9c29f388**. La base Git locale
4f541181ea2257b5da74d1fde85de3b691db2792 archive main
**781fbe8d13e943bc67cbcf3996d6791523f1a422** ; le titre du RAPPORT cite encore
c903774b1. `patch_tour_Gc_2_leviers.diff` (SHA-256 **89d7b4f4…**) est exactement
le diff du parent à 45976be. Aucun fichier catalogue dans les deux patches ;
les 9 fichiers core et les 33 fichiers catalogue sont identiques à main781,
donc conservent le catalogue A/B livré. La table `SupportEntry` de 16 octets est
abandonnée et le pilote retire sa collecte séparée du catalogue.

Les profils, chronos et campagnes de l'ancien lot 1b562/d9 ne se transfèrent pas
à ce nouveau raccord. La comparaison locale instrumentée portait sur le lot de
leviers complet : elle n'isole pas causalement l'effet de la table S*. `lfence`
perturbe les deux variantes ; aucune propriété mathématique ne garantit que
cette perturbation sous-estime le gain. Le retrait peut être une décision
prudente de portée ; aucun gain ou perte G4 n'en découle ici.

Dans le prototype Gc, **d9ec6727da5c03f48a83b75981fc352d3656f98f** corrige
`Catalogue::find_support` après **1b562defb3e77748693ea7cebecdfa1f3da72ac9** :
après avoir exigé 2..4 IDs strictement croissants, il vérifie le **dernier** ID
contre la taille du nuage, au lieu du premier seulement. C'est suffisant pour
les vérifier tous. Le cast u64 avant `+1` exclut le débordement pour `kNone`.
Les clés des requêtes valides et leurs résultats sont inchangés.

L'ancienne construction remplit les cases absentes par `kNone` sans inclure
l'arité dans la clé. Ainsi `[a,b,kNone]` encode exactement le support `[a,b]` ;
`[a,b,c,kNone]` encode `[a,b,c]`. La stricte croissance ne suffit pas à refuser
ces deux entrées. Le correctif refuse aussi les autres IDs hors du nuage, les
tailles invalides restant refusées avant tout accès. Il ne change pas le sens
géométrique : seul S* est indexé, un support minimal non canonique reste absent.

Le delta natif ajoute `no_alias` dans `tests/tower/index_unit.cpp:225-232`, sur
les supports q_min=2, et relève le plancher de 3 à 4. Les vérifications des
supports présents et voisins sont conservées. La construction de la table est
identique entre ces deux pins : tableau temporaire, tri, refus des doublons,
puis échange des deux buffers seulement à la fin. Un refus garde donc l'ancienne
table ; les réservations temporaires sont rendues (le pic peut avoir augmenté).
La garde de recherche n'écrit ni table ni budget. Aucune nouvelle exécution de
cette porte ou preuve d'injection d'échec n'est revendiquée ici.

**Point à conserver au rebasage.** Le RAPPORT annonce le retrait de la table à
fiches contiguës et `repo3` reprend la recherche publiée dans A/B. Cette recherche
présente au commit **c903774b1d16c3b18c15b86fa2a55d11d44b8bc8** garde le même
remplissage par sentinelles et le contrôle du premier ID seulement. Le défaut
de domaine ne dépend donc pas de `SupportEntry`. Au pin 45976be, repo3 retire aussi
`no_alias` et revient au plancher 3 : prototype, pas nouvelle livraison.
Conserver séparément la garde sur le dernier ID et cette porte dans le module
catalogue règle ce résidu sans réintroduire les fiches de 16 octets. Aucun défaut
FULL n'est déduit pour les requêtes valides du moteur.

`proposition_garde.patch` propose uniquement ce contrôle du dernier ID sur la
recherche actuellement livrée. C'est un correctif à relire, pas une intégration.
Son application à blanc est vérifiée ; la porte native `no_alias` reste à
conserver dans le raccord. Aucun changement de construction ou de budget.

`python3 check.py` et `python3 -O check.py` donnent le même résultat. Ce modèle
reproduit seulement le contrôle de domaine et l'encodage des clés : 19 608
requêtes de tailles 0..5, 25 requêtes valides inchangées, deux alias explicites
avant/après. Il ne calcule aucun catalogue. Pins, sortie et hashes de la capture
repo3 en préparation sont dans `capture.json` ; aucun log historique recopié.
Le résultat archivé inclut aussi la relecture des commits :
`python3 check.py --main /chemin/E-HGP --scratch /chemin/v12_tour_Gc`
(même commande avec `python3 -O`).
