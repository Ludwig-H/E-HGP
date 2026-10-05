# Réponse du développeur : hiérarchie de points $H^{r}_{k+1}$ après les réponses Q1–Q8

3 octobre 2026, 23 h 38 UTC (Claude, développeur). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Répond aux
[réponses Q1–Q8](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md) (48e09cb23) et au
[suivi](../receipts/hm_followup_20261003/README.md). Livraison :
[HIERARCHIE_POINTS.md](../docs/HIERARCHIE_POINTS.md), registre
[STATUT_PREUVES_ET_HEURISTIQUES.md](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md) (section V11), reçus
[points_g4](../receipts/developpement_20261003/points_g4/README.md) et
[points_math](../receipts/developpement_20261003/points_math/README.md). Empreintes livrées :
`points_gate.py` 13884d03, `points_reference.py` 2f05ceb4, `points_radius.py` 457b997f.

Merci pour ces réponses. Toutes sont adoptées ; aucune n'est contestée.

| Point | Suite donnée |
| --- | --- |
| Q1 | H3 cite la chaîne complète et ses hypothèses ; « stable par insertion » inscrit `false_in_general` avec votre témoin $\lbrace 0,2,4\rbrace$ plus un site à $\eta$ ; constante 3 dite atteinte sur profils abstraits seulement |
| Q2 | H4 porte la réserve de domaine (image des nuages, $n=k+1$ à $1\varepsilon$) |
| Q3 | H5 distingue la coupe fermée ($\leq F$) de « strictement avant $F$ » ($<F$) et donne la condition exacte de retard après le cœur |
| Q4 | théorème conditionnel inscrit `conditional_theorem` ; P5 de la note rappelle que retards et rayons ont la même échelle |
| Q5 | $H^{r}_{k+1}$ reste une hiérarchie à $k$ fixé ; votre piste de transport des attaches est P6 |
| Q6 | mon impossibilité était fausse sous trois axiomes : inscrite `false_in_general` ; le paragraphe du verrou et P1 nomment désormais stabilité par insertion et localité au profil |
| Q7 | C et S confirmés ; votre version en rayon d'ER0h est ajoutée |
| Q8 | contrat retenu pour le port natif, non commencé : type de date distinct des niveaux, budgets u18/u21/u24, refus explicite au-delà |

**Propriétaire au plateau exact.** Votre témoin des quatre sites collinéaires est une fixture permanente de la porte
(`plateau_proprietaire_k2_m1` : date $\sqrt{2}+\sqrt{18}-\sqrt{8}=\sqrt{8}$, propriétaire $(8,\lbrace 0,1,3\rbrace)$),
et ce nuage est aussi comparé site par site à l'oracle. L'oracle 2f05 tranche rival et propriétaire par sa propre
routine exacte à deux racines contre deux, le décimal ne servant qu'à la lecture ; 4 000 comparaisons croisées avec la
routine du banc (1 986 égalités construites) en sont une fixture. La porte grave douze fixtures et tue quatre mutants
causaux (qualification décalée, marge carrée, absence de marge, coupe ouverte) : essai local à sec conforme, 12/12 et
4/4.

**Campagnes.** C (f023f6d0) et D (58952a8b) publient 258 scènes communes identiques hors chronométrage : le correctif
arithmétique ne change aucune décision mesurée ; le lecteur du reçu le vérifie. Ces sessions sont des `dev_snapshot`.
La session E rejoue sur le commit poussé la porte stricte, le synthétique, les trames voisines et les démos ; son
verdict viendra dans le commit suivant.

**Addendum, 4 octobre 0 h 32 UTC : sessions E et F.** E (commit 6c88fe0ed) s'est arrêtée à la porte : les mutants
exportaient dans un sous-dossier jamais créé (`FileNotFoundError`), défaut que mon essai à sec ne voyait pas, car il
simulait aussi les écritures ; campagnes refusées, arrêt certifié. Corrigé en f02f91c7e, que la session F a qualifié :
porte stricte conforme (2 854 nuages, 215 974 sites comparés exactement, 12/12 fixtures, 4/4 mutants tués nativement),
puis démos, 72 voisines et synthétique ; les 201 scènes communes à D et F sont identiques hors chronométrage.
[Reçu](../receipts/developpement_20261003/points_g4/README.md).

**Question restante, et sa réponse.** Je demandais si, sous les axiomes supplémentaires (stabilité par insertion,
localité au profil qualifié, entrée immédiate sans rival), une impossibilité tenait pour T0/Q1bis contre Q2. Votre
4820664ed y répond : au sens de Hausdorff, l'insertion est déjà incompatible avec l'entrée immédiate, quelle que soit
la cible ; il faut une métrique qui fasse payer la masse. Adopté dans la note (P1) et le registre, comme l'obstruction
de Palm (P5, `conditional_theorem`), les comptes des archives (66 et 68 voisines persistées, 000882 = démo 02), le
domaine réel (32 462 à 126 267 sites, sol gardé dans la démo 04) et la portée de « identiques » (observables publiés,
sans dump canonique des décisions internes). La question reformulée, avec une distance de mesures, reste ouverte et
n'est pas urgente.
