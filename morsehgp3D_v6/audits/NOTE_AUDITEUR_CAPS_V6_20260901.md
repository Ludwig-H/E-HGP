# Note en vol à Claude — dernier ajustement des plafonds v6

Date : 1er septembre 2026.

Coupe observée à 08:02 UTC : worktree v6 non committé au-dessus de
`25ec4362`. Cette note n'est pas un verdict sur un commit ; elle sera absorbée
puis retirée après le checkpoint propre.

Cadre :

- `phase=exploration_v6_hors_registre`
- `backend=cpu_reference`
- `profile=quantized_u16_input_only`
- `mode=audit_independant_math_and_architecture`
- `public_status=not_claimed`

## Progrès désormais reçus dans le worktree

Le second jet répond substantiellement à la première revue :

- le diff est revenu entièrement dans `morsehgp3D_v6/` ;
- la CLI borne `--n` avant `make_family_input`, et les produits signés des
  familles touchées ont été élargis ;
- le budget est honnêtement qualifié de **partiel**, `fits_budget` refuse les
  produits débordants, la borne conservative préfiltre/census précède
  `prefilter_balls`, et le fold n'annonce plus que son tampon nommé ;
- la vague initiale est contrôlée avant son `reserve`, les tailles
  prospectives sont contrôlées avant la fusion dans `out`/`next`, et les
  shards déjà fusionnés sont libérés ;
- les refus observés sont transactionnels et précèdent les callbacks de
  forêt comme les phases de fold.

Sur cette coupe, une construction Release isolée avec GCC 13.3 réussit. Les
trois CTests `^mhgp6_caps_` passent 3/3 : nominal en 39,97 s, mutant émission
en 11,50 s et mutant vague tardive en 13,70 s. Ces verts prouvent les statuts,
la fermeture de l'objet et le moment de la **fusion globale** ; ils ne
prouvent pas encore un refus avant toute allocation locale.

Les probes Debug de l'autre auditeur sont utiles et conservés : au site exact
du refus pour un cap de 1 000, `uniform(2000, 200, 3)` avait matérialisé 1 009
candidats et `eight_clusters(2000, 400, 3)` en avait matérialisé 1 033. Le
nouveau champ `emitted_at_refus` rend enfin cet overshoot observable ; sa borne
générale est désormais établie ci-dessous.

## Réponse au cinquième jet — rendre le diagnostic portable

Le retrait des deux `shrink_to_fit()` est reçu. La réserve unique avant fusion
globale évite utilement les croissances géométriques de `out`, le message de
refus brut est maintenant honnête et la distinction entre résidence
stationnaire et transitoire du fold est meilleure.

Il reste une correction locale dans ce mécanisme. En C++20,
`out->reserve(exact)` garantit seulement `capacity() >= exact`, pas
`capacity() == exact`. Une capacité observée sur le témoin n'est pas non plus
contractuellement reproductible sur le rejeu. Le commentaire « capacité
déterministe », le commentaire du diagnostic « après RLE » et toute fenêtre
calculée depuis la capacité du run précédent restent donc spécifiques à
l'implémentation locale. La capacité actuelle est capturée **avant le tri**,
puis conservée à travers le RLE.

Le correctif court ne demande ni nouvelle copie ni nouvelle architecture :

1. décrire la réserve comme une allocation unique demandée à la somme exacte,
   sans promettre sa capacité finale ;
2. puisque le budget livré est un proxy de payload logique, garder le tri sur
   `cands.size()` brut et le préfiltre/census sur `cands.size()` post-RLE ;
3. calculer les mêmes seuils depuis `emitted` et `unique_balls` dans la
   fixture : ils sont exacts, déterministes et identiques au contrat gardé ;
4. conserver la capacité réellement observée comme diagnostic éventuel, sans
   l'utiliser comme autorité de seuil ni lui imposer une égalité ;
5. conserver le témoin `smax=2` pour le fold, avec sa fenêtre calculée depuis
   ses cardinalités exactes.

Le cap coopératif est par ailleurs plus net que ses commentaires actuels. Pour
un cap `H`, un bloc de publication de 4 096 et `T` ouvriers, le refus vérifie
`H < emitted_at_refus <= H + 4096*T`. Les observations toutes les 64 émissions
sont incluses dans ce bloc : il ne faut ajouter ni 65 ni une ancre. Aucun quota
atomique par candidat n'est nécessaire pour ce checkpoint.

Enfin, garder une portée précise pour `--mem-budget`. Les formules tri,
préfiltre/census et fold majorent des **payloads logiques nommés**, pas toutes
les allocations : `lsv`, `lb`, `lev` et leurs sorties ont des capacités de
vecteurs non réservées ou coexistantes, et les tris stables de tranches ont
leurs propres tampons. C'est un coupe-circuit de volume utile, mais pas encore
un plafond RAM contractuel. Un vrai claim d'allocation demanderait de réserver
et comptabiliser tous ces buffers ; il peut rester un jalon distinct.

Alignements encore nécessaires avant checkpoint :

- `caps.hpp` généralise trop « avant l'allocation » et conserve deux
  `inflight+1` périmés ;
- `GenerateOptions` annonce 64 K et un refus avant matérialisation ;
- les commentaires wave/alive doivent réserver `PRE-insertion` aux vecteurs
  globaux, les shards locaux étant déjà matérialisés ;
- la sortie CLI doit signer le budget demandé et le cap brut effectif avant
  toute campagne utilisant `--mem-budget`.

Après ces corrections, rejouer les trois caps, la suite v6 hors échelle et les
portes documentaires suffit pour proposer le checkpoint à contre-audit. Les
trois CTests caps gagneraient aussi à recevoir un `TIMEOUT` explicite.

Le GO G4 `d98f4729` n'est pas révoqué, mais son pin refuse correctement ce
worktree normatif sale. Aucun lancement ne doit partir de cet état ; un
nouveau re-pin sera requis après le commit v6 propre.

GCP non utilisé par cette revue.
