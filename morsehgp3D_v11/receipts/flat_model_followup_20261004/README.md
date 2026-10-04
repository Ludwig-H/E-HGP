# Factorisation des masses et monotonie de la sélection — 4 octobre 2026

Revue du modèle **privé** `build/v11-points-select`, pas du moteur ni d'une qualification G4. Aucun fit, code natif, build ou réseau exécuté. Les sources initiales ont été copiées avant lecture à 07:56 UTC ; le rapport apparu ensuite a sa capture propre. Les deltas ultérieurs sont conservés séparément dans `source_after/`, sans remplacer les preuves testées.

**Résultats positifs.** La proposition de factorisation des gros blocs et le théorème de raffinement en z sont corrects dans leur cadre. Le modèle conserve désormais plateaux atomiques, masses entières, cohortes datées et distinction entre existence, sélection et complétion. Cela ne transforme pas les petites expériences privées en validation statistique.

## Factorisation : conserver le calendrier des cohortes

Pour un treegramme fini u, de diagonale e, et des dates fixes s_i ≥ e_i, poser R_i comme la mcs-ième valeur de max(u_ij,s_j), pour 1 ≤ mcs ≤ n. La masse du bloc de i à r est exactement le nombre de termes max(u_ij,s_j) ≤ r. Le bloc est donc gros si et seulement si R_i ≤ r ; i et j partagent un gros bloc si et seulement si max(u_ij,R_i,R_j) ≤ r. Ce maximum définit bien un treegramme ultramétrique de diagonale R_i. La première **contribution comptée** de i est toutefois max(R_i,s_i).

Le facteur conserve la filtration des blocs admissibles. Une condensation standard qui compterait les points dès sa nouvelle diagonale R_i peut changer les scores EOM lorsque s_i > R_i. Il faut transporter s_i et les cohortes comptées, ou se restreindre au critère A, où s_i=e_i ≤ R_i. La statistique d'ordre formulée ici concerne les sites unitaires ; pour des poids fixes, la preuve utilise un seuil de masse cumulée, pas un rang non pondéré.

Garde exacte nouvelle : six sites colinéaires (0,2,4,7,9,11), ordre k=2, qualification m=3. Le nerf Γ₂ exhaustif donne deux couvertures qualifiées ABC et DEF dès r=2, puis leur fusion à r=5/2 ; aucun rival qualifié. Le modèle H a donc ces mêmes entrées et réunions. Avec mcs=2 et calendrier admissible général s=(2,2,3,2,2,2), ABC existe de 2 à 5/2, mais seuls A et B sont comptés pendant sa vie. Son score en λ=1/r est **1/5**, contre **3/10** si on condense le facteur en comptant chacun dès R_i=2. Le score redevient 1/5 si on transporte s. Ce calendrier est une garde pour la factorisation générale ; il n'est pas présenté comme les dates B/C/E de ce nuage.

Le modèle privé choisit pour les labels les membres engagés du bloc avant sa mort, y compris un membre non encore compté. Cette convention et les cohortes du score sont donc deux informations distinctes. Ce n'est pas une erreur du sweep ni un nouveau défaut moteur.

## Raffinement lorsque z augmente

Fixer **la même condensation, les mêmes cohortes**, l'admissibilité des racines et la politique parent aux égalités. Pour un nœud C, poser a=r_b(C)=r_d(D) pour chaque enfant D et multiplier les scores par a^z/z. Le score normalisé de C intègre M_C(r)(a/r)^z dr/r sur r≥a, donc décroît avec z. Les scores de ses descendants intègrent sur r≤a, donc croissent ; leur optimum par sommes et maxima croît également. Le prédicat « les enfants gagnent strictement » est monotone. Un ancêtre qui choisissait déjà ses descendants continue de le faire : le masquage par un ancêtre ne permet pas une remontée de la sélection. La sélection à z₂>z₁ raffine celle à z₁.

Cette garantie ne compare ni les arbres A et C, ni des calendriers différents, ni les IoU. Une garde à neuf feuilles montre qu'un calendrier différé peut choisir à z=3 un ancêtre des feuilles choisies à z=1 avec un autre calendrier. Chaque calendrier séparé respecte pourtant le théorème.

Dans la preuve du rapport, la limite z→∞ des scores descendants ne relève pas d'une convergence dominée : un intervalle de masse positive strictement sous a fournit directement une borne croissante. La limite vers les feuilles concerne les feuilles **admissibles** ; une racine isolée exclue demeure bruit. Ce sont des précisions de preuve, pas une réfutation du raffinement.

## Contrôles et portée

`python3 check.py > normal.json` et `python3 -O check.py > optimized.json` : **12 917 gardes concordantes**, 27 profils de factorisation, 7 coupes Γ₂ fermées exhaustives du témoin géométrique, **728 comparaisons exactes de sélection sur 26 condensations à cohortes fixes**. Le programme utilise Fraction et deux fonctions pures extraites par AST du snapshot privé ; l'oracle des blocs est la formule directe, le nerf scalaire est indépendant du code privé. Aucun import de ses oracles, de sklearn ou de NumPy.

Sources testées : `modele_lib.py` SHA256 `5c0f0f1bd40beb7d156ffe9b45e31ad3fa3bac82bf41c518e6c2db07da193b9d`, inchangé pendant les contrôles. Preuve relue : rapport avant SHA256 `8bd31894f6cb3610a00722c8da54dbced4e812690ffda2c69081754f6b2e29d6`, §1.4 et §3.2. `completion.py`, `fixtures_existence.py` et le rapport ont évolué : leurs deltas sont capturés, sans transfert de contrôle. HEAD développeur lu à clôture : `f1a53fe1ccc121bc80736473753ca156b0efee46`. Tous les hashes et dates sont dans `SOURCES.json`.

Le rapport après ajoute des chiffres arrondis pour f9 qui dépassent légèrement le majorant d₂/2 qu'il invoque. Une pièce exacte est nécessaire avant toute interprétation ; ce reçu n'impute aucun défaut au modèle à partir de ces chiffres. La borne L6 déjà démontrée indépendamment reste le garde de référence.

La complétion privée remonte explicitement les premières couvertures qualifiées, ce qui peut réaffecter un point avant son entrée H. Elle reste une politique supplémentaire de partition plate, à publier séparément de la pendaison fidèle ; le témoin R1+b3 déjà affiché par le développeur n'a pas été rejoué ici. Aucun ancien reçu n'a été modifié.
