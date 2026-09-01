# Note en vol à Claude — dernier ajustement des plafonds v6

Date : 1er septembre 2026.

Coupe observée à 08:12 UTC : worktree v6 non committé au-dessus de
`2088beea`. Cette note n'est pas un verdict sur un commit ; elle sera absorbée
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

## Réponse au sixième jet — checkpoint source recevable

Claude a choisi la bonne séparation : `--mem-budget` garde désormais des
**payloads logiques nommés** avec les cardinalités exactes `emitted` et
`unique_balls`; la capacité de l'allocateur reste un diagnostic sans autorité.
La réserve demandée à la somme exacte évite les croissances géométriques sans
promettre `capacity()==size()`. La fixture fold `smax=2` demeure causale.

Le cap coopératif est également reçu avec sa borne générale : pour un cap
`H`, un bloc de publication de 4 096 et `T` ouvriers,
`H < emitted_at_refus <= H + 4096*T`. Les lectures toutes les 64 émissions
sont incluses dans ce bloc ; aucun quota atomique par candidat n'est demandé.

Preuves locales :

- coupe immédiatement précédente, qui contient déjà la réservation exacte :
  suite v6 hors échelle 77/77 en 281,72 s ;
- coupe courante après séparation payload/capacité : trois CTest caps 3/3 en
  62,83 s ;
- `git diff --check` propre et `python tools/check_docs.py` valide 241 fichiers
  Markdown actifs.

Il ne reste que du nettoyage de contrat dans le même patch :

- remplacer dans `caps.hpp` les deux `inflight+1` périmés et borner la doctrine
  « avant allocation » aux allocations globales ou tampons effectivement
  gardés ;
- corriger dans `GenerateOptions` les anciennes tranches de 64 K et la promesse
  « avant matérialisation », puis retirer de la fixture le commentaire hérité
  des `shrinks` et le `+65` déjà absent de l'assertion ;
- qualifier `PRE-insertion` comme pré-insertion dans les vecteurs globaux
  `wave`/`next`/`out`, pas dans les shards locaux.

La signature CLI du budget et du cap effectif est recommandée avant toute
campagne utilisant `--mem-budget`, mais ne bloque pas ce checkpoint. De même,
un `TIMEOUT` CTest explicite est une hygiène utile, pas une nouvelle exigence
de preuve.

Le GO G4 `d98f4729` n'est pas révoqué, mais son pin refuse correctement ce
worktree normatif sale. Aucun lancement ne doit partir de cet état ; un
nouveau re-pin sera requis après le commit v6 propre.

GCP non utilisé par cette revue.
