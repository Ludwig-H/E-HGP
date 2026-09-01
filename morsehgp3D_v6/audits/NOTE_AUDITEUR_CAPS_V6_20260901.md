# Note en vol à Claude — dernier ajustement des plafonds v6

Date : 1er septembre 2026.

Coupe observée à 07:45 UTC : worktree v6 non committé au-dessus de
`629b2053`, empreinte du diff suivi avec `caps.hpp`
`e8d0c6cae1e63735ab10b133c02be80004430578872b9018f18e8de4942e19b4`.
Cette note n'est pas un verdict sur un commit ; elle sera absorbée puis
retirée après le checkpoint propre.

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

## Réponse au troisième jet — chemin court vers le checkpoint

Les corrections demandées sont maintenant effectivement présentes : arrêt
q3/q4 remonté jusqu'au rectangle, vague dynamique exercée à 2 048, garde
`alive` dédiée, compteurs de callbacks atomiques, petit budget refusé avant
génération, second produit de `fits_budget` exercé et commentaire CLI corrigé.
Le cap coopératif à overshoot borné est une solution recevable pour ce
checkpoint ; aucun quota atomique par candidat n'est demandé.

Il reste **une seule fragilité de porte** : un rejeu Release a échoué sur
`mhgp6_caps_refus` avec `(c) : message attendu`, tandis que deux rejeux
concurrents ont passé les trois caps. Cette alternance vient d'un décalage
mécanique : le test choisit son budget avec `temoin.emitted`, alors que les
gardes utilisent désormais la `cands.capacity()` résidente. Pour rendre les
étages causaux sans valeur magique :

1. exposer la capacité candidate du témoin comme diagnostic non-payload ;
2. calculer par helpers contrôlés
   `tri_need = capacity × sizeof(BallCandidate) × 2` et
   `census_need = capacity × (sizeof(BallCandidate) + sizeof(Survivor) + 2 × sizeof(BallData))` ;
3. tester le tri avec `tri_need - 1`, puis le préfiltre/census avec
   `tri_need` : l'égalité passe exactement le tri et reste strictement sous
   `census_need`.

Un petit calcul est également à aligner dans la garde du fold. Pendant
`expand_events_k`, les shards `lev` et la sortie fusionnée coexistent, alors
que jusqu'à `fold_inflight` stages précédents peuvent encore être résidents :
le facteur cardinalitaire conservateur du tampon `ForestEvent` est donc
`inflight+2`, pas `inflight+1`. Changer ce facteur et exercer une fois ce refus
suffit ; le budget complet des autres structures du fold n'est pas demandé.

Deux alignements textuels terminent le patch. Les commentaires parlent encore
de tranches de 64 K alors que le code publie toutes les 4 096 émissions, et
`caps.hpp`, `GenerateOptions`, `run_pipeline` ainsi que CMake disent encore
« avant matérialisation/allocation ». Le contrat réellement livré est plus
précis : **cap de cardinalité à overshoot borné, arrêt avant fusion globale et
tri**. Les shards WSPD sont bornés par construction mais non gardés avant leur
allocation locale ; conserver le terme « pré-fusion globale ». La
qualification explicite du budget comme partiel reste acceptée.

Après ces ajustements, rejouer les trois caps, la suite v6 hors échelle et les
portes documentaires suffit pour proposer le checkpoint à contre-audit.

Le GO G4 `d98f4729` n'est pas révoqué, mais son pin refuse correctement ce
worktree normatif sale. Aucun lancement ne doit partir de cet état ; un
nouveau re-pin sera requis après le commit v6 propre.

GCP non utilisé par cette revue.
