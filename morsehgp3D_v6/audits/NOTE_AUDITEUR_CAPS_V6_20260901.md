# Note en vol à Claude — dernier ajustement des plafonds v6

Date : 1er septembre 2026.

Coupe observée à 08:19 UTC : worktree v6 non committé au-dessus de
`9627f013`. Cette note n'est pas un verdict sur un commit ; elle sera absorbée
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

Sur la coupe fonctionnelle stable de 08:13, une construction Release isolée
avec GCC 13.3 réussit. Le contre-rejeu des trois CTests `^mhgp6_caps_` passe
3/3 : nominal en 64,95 s, mutant émission en 30,90 s et mutant vague tardive
en 29,83 s (64,97 s réel). Les 74 autres tests hors échelle passent ensuite
74/74 en 196,75 s réel, E6 compris.

`caps.hpp`, `generate.hpp` et `selftest.cpp` ont reçu des alignements de
commentaires pendant ce second rejeu. Le 74/74 prouve donc la coupe compilée à
08:13, pas encore le futur commit. Ces verts prouvent les statuts, la fermeture
de l'objet et le moment de la **fusion globale** ; ils ne prouvent pas un refus
avant toute allocation locale.

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

Le mécanisme est recevable **sous réserve** du nettoyage de contrat dans le
même patch. Sur la coupe 08:19, il reste exactement :

- remplacer dans `caps.hpp` « borne les tampons » et dans `RunOptions`
  « conservatif sur la capacité » par l'autorité réellement livrée : proxy de
  payload logique nommé ;
- retirer du commentaire du throttle le terme d'une ancre, déjà supprimé par
  la propagation, et retirer de la fixture le commentaire `4096+65` ;
- corriger le commentaire de la réserve, qui dit encore que les fenêtres se
  calculent sur sa capacité, ainsi que le doublon de commentaire de fixture ;
- décrire `diag_candidates_capacity` comme saisi avant tri, pas après RLE.

La signature CLI du budget et du cap effectif ne bloque pas ce checkpoint,
mais elle bloque toute campagne qui utiliserait `--mem-budget`. Une ligne
stable peut porter `memory_budget_scope=partial_named_payload_proxy_v1`, le
budget demandé et les caps bruts demandé/effectif ; le cap effectif doit venir
du même helper que l'exécution. De même, `TIMEOUT 600` sur les trois CTests caps
est une hygiène utile, pas une nouvelle exigence mathématique.

Le GO G4 `d98f4729` n'est pas révoqué, mais son pin refuse correctement ce
worktree normatif sale. Aucun lancement ne doit partir de cet état ; un
nouveau re-pin sera requis après le commit v6 propre.

GCP non utilisé par cette revue.
