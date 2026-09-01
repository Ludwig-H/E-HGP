# Note en vol à Claude — dernier ajustement des plafonds v6

Date : 1er septembre 2026.

Coupe observée à 07:52 UTC : worktree v6 non committé au-dessus de
`58270bac`. Cette note n'est pas un verdict sur un commit ; elle sera absorbée
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
sur ces témoins est une non-régression mesurée, pas encore une borne générale.

## Réponse au quatrième jet — une dernière correction locale

Les corrections demandées sont désormais présentes. Le facteur du tampon
`ForestEvent` vaut `inflight+2` et sa fixture séparée `smax=2` est causale :
sur le témoin observé, tri et census passent à 2,12 Mio et 4,48 Mio, puis seul
le fold refuse à 5,24 Mio avec `inflight=3`. Le rejeu Release passe les trois
CTest caps en 84,92 s. Le cap coopératif à overshoot borné reste une solution
recevable ; aucun quota atomique par candidat n'est demandé.

Ne pas conserver en revanche les deux `shrink_to_fit()` introduits pour
stabiliser la fixture. En C++ cette requête est non contraignante : elle ne
prouve pas `capacity()==size()`. Surtout, celle placée avant la garde du tri
peut allouer et copier tout le vecteur avant le refus censé protéger son
tampon.

Le chemin court est déjà disponible dans `generate_candidates` : après la
somme exacte et le contrôle du cap, réserver `exact` dans `out` **avant** la
fusion globale, puis fusionner les shards. Cela évite les croissances variables
sans ajouter de copie pré-garde. Conserver cette capacité résidente à travers
le RLE, l'exposer juste avant le tri et calculer les fenêtres causales à partir
d'elle ; aucun compactage supplémentaire n'est nécessaire.

Enfin, aligner les quatre libellés encore anciens : `GenerateOptions` parle de
64 K et d'un refus avant matérialisation, le message de refus brut répète cette
dernière promesse, le commentaire de somme exacte parle de « matérialisation
unique », et l'en-tête de `run.hpp` annonce toujours `fold_inflight+1`. Le
contrat livré est : **cap de cardinalité à overshoot borné, arrêt avant fusion
globale et tri**, avec budget partiel de tampons nommés.

Après ce remplacement local, rejouer deux fois le nominal caps, les deux
mutants, la suite v6 hors échelle et les portes documentaires suffit pour
proposer le checkpoint à contre-audit.

Le GO G4 `d98f4729` n'est pas révoqué, mais son pin refuse correctement ce
worktree normatif sale. Aucun lancement ne doit partir de cet état ; un
nouveau re-pin sera requis après le commit v6 propre.

GCP non utilisé par cette revue.
