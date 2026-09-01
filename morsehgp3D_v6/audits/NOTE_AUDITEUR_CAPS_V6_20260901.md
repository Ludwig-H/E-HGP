# Note en vol à Claude — second jet des plafonds v6

Date : 1er septembre 2026.

Coupe observée : worktree v6 non committé au-dessus de `7c4d5e0a`, sources
stables observées entre 07:23 et 07:26 UTC. Cette note n'est pas un verdict
sur un commit ; elle sera absorbée puis retirée après le checkpoint propre.

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
sur ces témoins est une non-régression mesurée, pas encore une borne générale.

## Trois corrections utiles avant le checkpoint

1. **Faire remonter l'arrêt d'émission q3/q4.** Les helpers retournent après
   avoir vu `sc.emit.stopped`, mais leur appelant continue les ancres
   suivantes. Chacune peut alors pousser encore un candidat avant de
   retourner : l'overshoot n'est pas borné par `T × 4096 + une ancre` comme
   l'annonce le commentaire. Le sondage supplémentaire du drapeau toutes les
   64 émissions ne propage pas ce retour hors du helper. Faire retourner un
   booléen, ou tester
   `sc.emit.stopped || cap_stop.load()` immédiatement après chaque appel,
   ferme ce trou à faible coût.

2. **Choisir explicitement le niveau de garantie des caps.** `lout`, `lnext`
   et les shards candidats sont matérialisés avant leurs contrôles globaux ;
   `Σ|lnext| <= 2|wave|` est une borne utile, mais autorise précisément la
   première allocation à dépasser `wave_cap`. Les capacités de `std::vector`,
   la capacité brute conservée par `cands` après RLE et la coexistence
   shard/sortie ne sont pas déduites du budget. Deux solutions sont
   recevables :

   - pour le claim fort « pré-allocation », acquérir un quota avant chaque
     append local et compter la résidence transitoire ;
   - pour ce checkpoint, conserver le mécanisme actuel mais parler de
     « garde pré-fusion globale » et de « cap de cardinalité à overshoot
     borné », jamais d'arrêt « avant matérialisation » ni de borne exacte en
     octets.

   La fixture vague doit alors employer un seuil proche de 2048 pour exercer
   la transition dynamique : le probe donne pic nominal 1999 et pic mutant
   2518. Avec 64, elle ne teste que la nouvelle garde initiale. Ajouter aussi
   un cas `alive_rects_cap_for_tests`, aujourd'hui non exercé.

3. **Fermer deux bords de preuve.** Dans le run réussi, `fold_phases` est
   incrémenté depuis l'étage A et les threads B sans synchronisation : la
   fixture a une data race. Employer un compteur atomique et exiger un nombre
   strictement positif au succès. Ensuite, refuser avant génération lorsque
   `0 < memory_budget_bytes < sizeof(BallCandidate)` : forcer actuellement le
   cap dérivé à 1 permet déjà une allocation supérieure au budget. Tester
   `sizeof(BallCandidate)-1`, l'égalité et zéro callback.

Deux micro-nettoyages peuvent accompagner ce même patch : le test
`fits_budget(4, UINT64_MAX/2, 3, ...)` déborde dès le premier produit et ne
couvre donc pas le second (un témoin comme `2, UINT64_MAX/4, 3` le ferait) ;
le « mur réel 1,6–3,2 M » de `caps.hpp` doit être nommé ordre de grandeur
extrapolé, et le commentaire CLI « code 2 refus avant calcul » n'est plus vrai
pour les refus après génération.

Le budget complet du fold n'est **pas** demandé pour ce checkpoint : sa
requalification explicite en budget partiel est une réponse acceptable. De
même, les caps structurels maximaux peuvent rester un jalon ultérieur dès
lors que le claim local est exact.

Le GO G4 `d98f4729` n'est pas révoqué, mais son pin refuse correctement ce
worktree normatif sale. Aucun lancement ne doit partir de cet état ; un
nouveau re-pin sera requis après le commit v6 propre.

GCP non utilisé par cette revue.
