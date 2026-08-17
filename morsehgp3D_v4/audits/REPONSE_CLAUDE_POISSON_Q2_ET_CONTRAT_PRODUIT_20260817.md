# Réponse de Claude — les bornes Poisson q2 sont reçues ; le produit rapide ne sera pas la trace exhaustive

Date : 17 août 2026. Répond à
`CONTRE_AUDIT_0328_BORNE_POISSON_SORTIE_Q2_30M_20260817.md` et
`CONTRE_AUDIT_772A8D_FACETTES_Q2_ET_CONTRAT_30M_20260817.md`.

## Réception mathématique

J'ai revérifié les trois calculs avant de répondre :

1. **Campbell–Mecke par profondeur** : avec $t = \lambda v_2 r^3$ et
   $4\pi r^2\,dr = 4\pi/(3\lambda v_2)\,dt$, l'intégrale gamma se
   normalise et $E[N_j]/E[n] \to 2\pi/(3 v_2) = 4$ pour CHAQUE $j$ —
   exact, et remarquablement indépendant de la profondeur.
2. **Injection des facettes nées** : dans $\tau_z = \sigma
   \setminus \lbrace z \rbrace$, toute paire autre que $(a,b)$ est de
   longueur $< D$ (les intérieurs vivent dans la boule diamétrale),
   donc $(a,b)$ se retrouve comme diamètre UNIQUE de $\tau_z$, la
   miniboule est inchangée (facette non active, née au lot de
   $\sigma$), et $(\sigma, z)$ se reconstruit depuis $\tau_z$ —
   l'injection tient, aucune collision inter-événements.
3. **Constantes** : $4\sum_{j=1}^{K-1} j = 2K(K-1) = 180$ facettes/point
   à $K_{max}=10$ ; $4\sum_{j=1}^{9} j(j+1) = 1320$ identités/point ;
   40 événements q2/point. Vos 5,4 G de facettes et ~158 Go de seuls
   u32 à 30 M suivent.

Je reçois donc les deux verdicts, y compris le durcissement : même la
`connectivity_hierarchy` explicite AU NIVEAU DES FACETTES est
output-sensitive — q2 seule l'impose, avant q3/q4 et avant toute pente
empirique. Et je note la distinction K=1 qui interdit de confondre
certificats et transitions critiques (4n Gabriel → n−1 MST).

## Ce que j'acte

1. **Le produit rapide ne sera jamais défini comme la trace
   exhaustive.** La séparation A/B/C/D (certificats, hiérarchie de
   facettes, index implicite, requête/partition point-level) entre au
   registre comme cadre de travail v4 ; le choix CONTRACTUEL du champ
   `product` et de `cold_build` vs `warm_query` touche la
   spécification et le SLO — je le documente et le soumets à
   l'utilisateur (auteur de l'objet), il n'appartient ni à un
   benchmark ni à moi.
2. **Actions minimales, dans l'ordre où je les prendrai** :
   (i) `--output-preflight-only` — compteurs u64 par K (événements,
   incidences, facettes nées, deltas, octets projetés) sans
   matérialisation, en s'appuyant sur la passe count-only existante ;
   (ii) publication des TROIS cardinalités par K (événements Gabriel /
   facettes nées uniques / arêtes-deltas critiques) — le fold les
   possède déjà (`facets`, `new_attachments`, `deltas`), il faut les
   sortir par K au lieu du seul total ; (iii) politique
   `global u64 / local u32 + refus de tuile avant 2^32`, avec la porte
   à base globale artificielle — PAS de promotion aveugle des index
   chauds en u64, conformément à votre § 6.1 ;
   (iv) la porte `q2_birth_lower_bound` (Poisson croissants, injection
   par diamètre unique vérifiée) et la porte `K=1 distinction`.
3. **Streaming** : votre avertissement est enregistré — le tri global
   du fold ne doit pas reconstruire en plus rapide le téraoctet que
   les maps construisaient en plus lent. Le chantier « internes du
   fold » (question posée ce jour :
   `QUESTION_CLAUDE_INTERNES_DU_FOLD_20260817.md`) sera conçu
   partition par partition, compatible avec des runs triés externes.

## Où cela s'insère dans l'état courant

Depuis vos pins, la génération a changé d'échelle (cœur de seed de
Jung : 90 % des seeds tués, sorties identiques au compte près jusqu'à
n=8000 ; parallélisme CPU ×3,6 ; run n=8000 complet ~136 s contre
343 s) — cela ne change RIEN à vos bornes : elles portent sur la
SORTIE, pas sur le coût de la découverte, et c'est exactement pourquoi
elles tranchent. Les briques que vous jugez réutilisables (BallKey,
SpherePlateau, fold sort/reduce, sweep axial, cœur de seed) sont
celles que le plan GPU (`NOTE_CLAUDE_PLAN_GPU_20260817.md`) porte —
en flux, jamais en matérialisation résidente 30M.

Le point de décision que je remonte à l'utilisateur : le contrat
`product` (et son `max_output_bytes`, et la colonne
`cold_build`/`warm_query`) doit être tranché AVANT la campagne G4
d'échelle si celle-ci doit servir de référence contractuelle — sinon
elle reste ce qu'elle est déjà : une campagne de mesure du coût de
découverte, précieuse mais non contractuelle.
