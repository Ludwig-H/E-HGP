# Tour T/M/V/R : traces du prototype et piste R — 7 octobre 2026

`phase=exploration_v12_hors_registre`, `backend=cpu_reference`,
`public_status=not_claimed`. Lecture seule des traces annoncées 22:03–22:45,
profil 21. Aucun moteur, build, test natif, donnée ou vidage relu par cet audit.

Les annonces sont cohérentes avec les traces conservées :

- CTest : 652 tests sélectionnés, **651 Passed et une sentinelle LiDAR
  Skipped**, zéro échec. `LastTest.log` confirme les commandes du prototype,
  l'oracle normal/−O, le témoin de forme et la porte `forest_branches`.
- Mutants : témoin vert, **15 TUE par code**, sans signal, délai ni échec de
  construction. Les identifiants correspondent au manifeste. L'empreinte
  source recalculée sur 357 fichiers, `c1950bb8…`, est identique à celle du
  rapport ; manifeste `32c67e2b…` également identique.
- MES-M0 : neuf cas × deux modes de cibles annoncés conformes, avec JUG-EMST ;
  chaîne : trois lots disjoints de trois cas couvrant exactement les neuf cas.
  Les six codes archivés sont 0. Le lecteur `mes_m0.py` exige les empreintes
  gravées et compare 1/8 fils (tranches 97) ; en mode chaîne il relit le FUL1 par
  le lecteur sémantique. En mode vidage/u21 il compare l'empreinte des octets ;
  JUG-EMST est exécuté pour le mode `graines` seulement.

Le cache **build21c**, celui désigné par CTest et le script de campagne,
pointe **repo3**, Release/u21/tous modules. La formule du rapport « chaîne
complète sur main » signifie ici **prototype sur base `9c5809919` avec les
ajouts T/M/V/R**, pas une livraison sur la tête observée `0377684ec`.
`patch_tour_TMV.diff` reste antérieur à R : aucun `registry_branches.cpp`.
Le corps R observé reste `9cc388d6…` : admission des offsets CSR encore à
corriger selon `../audit_registre_branches_20261007/` (proposition `0d616e69…`).

Ce sont des **traces concordantes**, pas une contre-qualification native.
Les copies des mutants et les sorties temporaires MES-M0 ont été supprimées
par leurs pilotes ; les logs disponibles sont des bilans. Les hashes des
binaires sont ceux observés maintenant, sans chaîne de compilation historique
reconstituée. Aucun octet FULL, coordonnée ou résultat géométrique n'a été
rehaché/recalculé ici ; aucun gain de temps n'est déduit. Profils 24/32 en cours
hors de ce reçu. Les chemins privés et les logs bruts ne sont pas versés.

## Piste mathématique R, à mesurer avant adoption

Pour une ligne retenue de rang r, suivre les attaches de rang≤r−1 donne la
racine de sa composante à la coupe ouverte. Ces racines sont en bijection
avec les composantes ouvertes. La seconde moitié de `component_at` cherche
le dernier événement de chaque survivant de rang≤r−1, ou sa naissance : elle
traduit chaque racine en un nœud M distinct. Cette bijection utilise une coupe
qui ferme **tout** le plateau antérieur, pas un préfixe d'un plateau.

On peut donc écrire les P_R racines dans `branch_nodes`, trier/dédupliquer
chaque ligne, traduire une seule fois ses racines distinctes puis trier les
A nœuds résultants. Même `ant(b)`, sans allocation nouvelle ; la montée doit
rester factorisée dans la source de `component_at`. Les montées restent P_R,
les recherches historiques passent de P_R à A ; le tri supplémentaire porte
sur A. `branch_reads=P_R`, `branches=A` restent inchangés ; mémoire provisoire
4 P_R et sortie 4 A + CSR coexistent toujours. L'admission corrigée reste requise.

Sur les comptes annoncés ng00/ordre 5, P_R=770782 et A=576388 : **25,22 %
d'appels de recherche en moins**, aucune borne de gain sur les comparaisons
ou le temps (historiques de longueurs différentes, second tri à payer).
Référence mathématique existante, sans nouveau modèle redondant :
`../audit_t1b_tour_prepublication_20261007/branches/` (5 + 64 cas). Aucun patch
natif ni gain mesuré proposé dans ce reçu.

## Relecture des métadonnées

```sh
python3 -B -S check.py --prototype DOSSIER_TMV
python3 -O -B -S check.py --prototype DOSSIER_TMV
```

Les deux sorties sont identiques au résultat de `capture.json`. Le lecteur
recalcule les hashes sources/traces, la couverture et l'arbre du rapport
mutant ; il refuse une source modifiée. Il ne relance aucune commande native.
