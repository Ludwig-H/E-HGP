# Suivi du levier C et réponses au développeur — 6 octobre 2026

Source relue : **5861c223f31b5d7d6f621522d0ca84d0064c9904**.
Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Lecture de source et contrôles
Python bornés ; aucune compilation, exécution native/CUDA ou session GCP
par l'auditeur.

## Deux raccords importants

- **Mutants d'étendue : profil inopérant dans la campagne officielle.**
  `etendue_seuil_double` et `etendue_sans_fermeture` héritent du profil u18
  de la matrice, où les branches qu'ils visent sont éliminées à la compilation.
  Leur donner explicitement u21. [Preuve et proposition](gates/README.md).
- **Banc multi-source : la reprise peut annoncer la mauvaise archive.**
  Un répertoire `--work` réutilisé garde ses anciens sources et son binaire
  alors que le journal peut publier l'empreinte d'une archive remplacée.
  L'identité de l'archive doit gouverner la reprise du cache.
  [Contre-exemple et portée des reçus](evidence/README.md).

Ces défauts touchent la qualification et sa provenance ; aucun défaut de
sortie du moteur n'est déduit des simulations du banc. Les anciens points
sur les bornes des masques, le juge GPU et l'IoU restent suivis dans la
[note moteur](../../audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).

## Arithmétique C : avis source favorable

La garde porte sur les sites **et la fermeture de la boîte**. Au-delà de
D=2^20, aucun préfixe étroit n'est parcouru ; les émissions et compteurs
provisoires d'une feuille non résolue sont abandonnés avant le rejeu CPU.
Les sommes et différences J2 restent en i64, notamment
`|g_k p0 - f_k p1| <= 6 D^3 < 2^63`. Les différences de coordonnées et de
bornes utilisées en i32 tiennent également en u24.

Les identités des angles q3 et du produit mixte q4 conservent leurs signes.
Les produits de plus haut degré sont élargis en i128 avant multiplication ;
la borne des poids q4 repose sur le **cube commun** de côté D, pas sur des
vecteurs indépendants de normes bornées. Aucun défaut numérique établi
par cette lecture ; les [sources sont épinglées](sources.json).

La variante C archivée sur G4 et les deux en-têtes intégrés sont identiques.
Les comparaisons de dumps de `wfgpu1` concernent C, mais son passage Compute
Sanitizer concerne **A+C**. Le plan G4 de cette session n'inclut ni CTest ni
campagne de mutants. Les portes locales déclarées par le développeur ne
sont donc pas présentées ici comme une nouvelle qualification G4.
[Contrelecture des reçus](evidence/README.md).

## Réponse U2 : repli parallèle

**Oui, une sortie privée par feuille, concaténée dans l'ordre du lot,
suffit pour l'ordre des émissions si les conditions suivantes sont gardées.**
Le `Workspace` est privé par ouvrier ; `Collector`, sortie, registre et
statut sont privés par feuille. Chaque tâche reprend exactement les sites,
la boîte et les paramètres de sa feuille, avec le lot désactivé.

Le piège immédiat est le dimensionnement : les compteurs GPU d'une feuille
non résolue valent zéro, même si son calcul avait déjà émis. Ils ne bornent
pas sa sortie CPU. Réutiliser des pages privées `SinglePassOutput` ou
compter d'abord ; un seul bloc de capacité supposée suffisante introduirait
un nouveau plafond. Les pages, leur remplissage partiel, leurs métadonnées
et les espaces privés doivent rester comptés dans le budget partagé,
pendant leur coexistence avec le résultat GPU.

Après le join, réduire les tailles par additions contrôlées, conserver la
borne **globale stricte** `balls < ball_limit` (incluant les autres sorties),
puis construire les préfixes dans l'ordre des feuilles. Recaler chaque
`Emission.population_begin` par le préfixe global, comme le fait `compact`.
Garder la disposition actuelle des blocs résolus et du bloc de repli ;
dans ce dernier, les feuilles restent dans l’ordre du lot. Additionner
chaque registre exactement une fois. Les limites locales des
collecteurs ne remplacent pas les limites du résultat assemblé.

Le Pool ne doit pas être rappelé depuis l'un de ses propres callbacks.
Attendre toutes les tâches ; un seul refus invalide toute publication,
même si d'autres tâches ont déjà rempli leurs pages privées. L'ordre de
fin des ouvriers ne doit déterminer ni l'ordre des blocs ni celui d'un
résultat partiel publié.

**Porte utile avant intégration :** un lot mêlant D=2^20, D=2^20+1, feuilles
résolues et repli après émissions provisoires ; comparaison des dumps et du
registre complet en W1/W48. Ajouter un refus tardif du repli et un refus
mémoire : pas de sortie partielle, budget rendu. `full_leaf_lanes` doit
atteindre ces chemins, pas seulement les trois trames actuellement sans
feuille non résolue. Le coût mémoire peut varier avec le nombre d'ouvriers ;
l'égalité exigée porte sur les sorties réussies, pas sur les pics mémoire.

## Réponse U1 : moins de SASS statique ne prédit pas le temps

L'hypothèse ATOM/YIELD reste à établir. Le nombre d'instructions statiques
ne mesure ni les instructions exécutées avec prédicat actif, ni la pression
des registres, ni les accès mémoire. A étant rejeté, aucune nouvelle
campagne n'est nécessaire pour poursuivre C. Si A est repris : même file
de feuilles, dumps et registre identiques, puis observation corrélée au
code des instructions dynamiques avec prédicat actif, registres, occupations
et accès mémoire. Isoler ensuite une seule modification pour tester la
cause. Ces indicateurs sont documentés par
[NVIDIA Nsight Compute](https://docs.nvidia.com/nsight-compute/NsightCompute/).
Une liste SASS seule ne permet pas de départager ces explications.

Les pistes B et D de T sont abandonnées dans le workflow reçu ; elles ne
sont pas réouvertes comme tâches par cet audit.

## Vérifications de cette capsule

Les replays des mutants, du cache simulé et des trois reçus passent en
normal et avec `-O`, avec les mêmes résultats. Les deux patches nouveaux
s'appliquent au pin relu ; ils restent des propositions pour le développeur.
Les contrôles des manifestes et liens locaux passent. Les commandes de
rejeu figurent dans chaque sous-dossier ; [résultats de fermeture](verification.json).
