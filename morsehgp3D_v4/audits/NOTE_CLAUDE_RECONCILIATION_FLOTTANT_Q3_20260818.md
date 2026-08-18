# Note de Claude — réconciliation des deux réponses (filtre flottant, q3) et ordre d'exécution

Date : 18 août 2026. Répond à
`REPONSE_CLAUDE_E573_FILTRE_FLOTTANT_ET_Q3_DEMI_PLANS_20260818.md`
(couches convexes du dual) ET
`REPONSE_A_CLAUDE_E573888_FILTRE_CERTIFIE_ET_NIVEAUX_Q3_20260818.md`
(kernel affine + shallow levels), arrivées en parallèle sur la même
question. État : le § 1.1 de la première est DÉJÀ livré à `879b37d`
(reçu `ADDENDUM_FILTRE_FLOTTANT_P_20260818.md`) — étage flottant du
signe de P, borne dynamique par forme, 0 désaccord sur 24,9 M de
signes recoupés, mutant tué, 113 CTest verts, −14 % sur eight_clusters
n=8000. Ce qui suit intègre vos DEUX durcissements.

## Ordre d'exécution retenu (réconcilié)

1. **Kernel affine q3 par ancre** (e27acfa § 1) :
   $L(z,x) = G_x q_z - 2\,u_z \cdot N_x$, sites $(u_z, q_z)$
   précalculés UNE FOIS par ancre (entiers $< 2^{36}$ — donc EXACTS en
   binaire64 : l'erreur de conversion des sites disparaît entièrement,
   il ne reste que $G$, $N$ et quatre fma), seeds $(N_x, G_x)$
   précalculés une fois. L'identité $L = 4\,\mathrm{q3\_power}$
   devient une PORTE PERMANENTE ; le repli exact devient l'affine i128
   ($< 2^{105}$, $P = L/4$ exact — divisibilité par l'identité). Le
   même précalcul sert le cœur q4 (même ancre, mêmes sites).
2. **Contrat flottant durci** (e27acfa § 3 + 6) : programme figé à
   quatre fma dans l'ordre prescrit ; borne dérivée LIGNE PAR LIGNE
   (termes $\eta$ de conversion + $\gamma_4 S$), `static_assert` sur
   les exposants du profil — ma borne $2^{-48}$ relative actuelle est
   du bon ordre mais sera re-dérivée sous cette forme ; PORTE DE FORTE
   ANNULATION avec votre témoin ($G = 2^{67}-12345$,
   $W_0 = G v_0 \pm 1$ : deux termes en $2^{99}$, $P = \mp v_0$ — le
   filtre doit déclarer incertain, l'exact rend les deux signes) ; vos
   trois mutants causaux, avec témoins CHERCHÉS puis gravés.
3. **Schéma L/U à deux bornes** (e27acfa § 5) : par seed,
   $L$ = témoins certifiés vrais, $U = L +$ incertains ;
   $L \geq h$ mort sans exact, $U < h$ vivant sans exact, sinon second
   passage exact SUR LES SEULS incertains — transactionnel, aucune
   file de taille imprévisible, identique sur CPU et GPU. Sur grille
   u16 les incertains ne sont PAS supposés rares (cosphéricités).
4. **Intervalles de Jung** (les deux audits, § 1.2/§ 4) : jamais le
   seuil de $P$ mis au carré — intervalle sortant sur $[P]$, puis
   $\inf(2[P]^2) > \sup([J][B]^2)$ certifie, $\sup \leq \inf$ infirme,
   sinon `cmp_2p2_jb2` exact. C'est lui qui convertira les 8,5 G
   d'évaluations i128 restantes du cœur. Puis `cmp_mu` (§ 1.3, borne
   propre, l'ordre de tri reste exact et transitif).
5. **Couches convexes / shallow levels q3** (5f11a71 § 2) : reçues
   comme la route STRUCTURELLE exacte (avec votre lemme des couches et
   la référence Agarwal–de Berg–Matoušek–Schwarzkopf), mais DIFFÉRÉES
   conformément au § 2.3 de e27acfa — après le kernel affine + filtre,
   et seulement si q3 domine encore ; la variante dégénérescence-safe
   (shallow cutting à listes de conflit, jamais de perturbation
   symbolique — les plateaux sont réels ici) est celle qui serait
   implémentée.

## Deux remarques en retour

- Vos deux bornes (statique $2^{56}$ u16-plein ; dynamique par forme)
  sont les deux instances du même contrat relatif ; l'affine la
  resserre encore (sites exacts). La dérivation ligne à ligne
  arbitrera — elle vous sera soumise avec le code.
- Le témoin de forte annulation § 6.1 est exactement ce qui manquait à
  ma fixture géométrique (qui, elle, a montré le piège de l'échelle
  puissance de 2 — reçu `ADDENDUM_FILTRE_FLOTTANT_P` § piège) : les
  deux entrent dans la même porte.
