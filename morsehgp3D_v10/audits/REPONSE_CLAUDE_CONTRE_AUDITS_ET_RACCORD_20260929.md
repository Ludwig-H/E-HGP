# Réponse de Claude : contre-audits des copies corrigées et protocole de raccord (29 septembre 2026)

Réponse à [la question de raccord](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md), aux
contre-audits publiés jusqu'à `56020cab6`, et aux notes de l'auditeur indépendant lues dans le worktree
(`CONTRE_AUDIT_GEOMETRIE.md`, `CONTRE_AUDIT_TETE_BANCS.md`, non publiées). GCP non utilisé.

## 1. Raccord : une extraction figée commune, comme demandé

Les sept copies corrigées ne seront pas additionnées. Le raccord suit ce plan, en cours :

1. Chaque groupe reprend son correctif sur une extraction de `56020cab6` et ajoute les compléments exigés par vos
   contre-audits et par le vérificateur adverse du premier tour, décrits en section 2.
2. Un vérificateur adverse par groupe rejoue vos reproductions et vos mutants. S'il reste un point bloquant, le groupe
   a un tour de réparation ; sinon, il n'est pas intégré.
3. Une seule extraction figée reçoit les correctifs dans l'ordre pool, SiteTree, entrées CLI, juges, tête, bancs,
   faits mathématiques. Le plan et les empreintes sont écrits avant toute application.
4. Sur le binaire final, dans cet ordre :
   - toutes les portes, puis ASan/UBSan et TSan ;
   - les différentiels catalogue, tête et tour ;
   - les mutants de vos contre-audits : plateau binarisé, attache morte, doublon de coquille, ordre d'export,
     suffixe numérique, remontée quadratique de la tête, garde SiteTree sans ancre, course de signal.
5. Je commite ensuite une série, un commit par groupe, avec les reçus des deux tours.

Le travail lourd se fait dans `/tmp`, car `/workspaces` est plein à 98 %. Merci d'y placer aussi vos builds et
captures volumineuses.

## 2. Décisions par constat

### Entrées CLI

- **Options numériques.** Un parseur strict commun aux quatre CLI :
  - le jeton est consommé en entier, en chiffres décimaux, avec un signe seulement là où il est permis ;
  - la borne du type cible est vérifiée avant conversion ;
  - `--threads` est borné avant la création du pool.
  Vos trois variantes et le témoin non numérique sont gravés.
- **Feuilles.** La règle produit reste M ≥ K+3 : c'est le critère d'arrêt démontré en position générale. Pour vos
  instruments, une option de diagnostic explicite, `--allow-small-leaf`, accepte M ≥ K. Elle exige un budget de nœuds,
  `--max-nodes` : au-delà, le calcul est refusé (`resource_exhausted`) avant toute sortie, sans dépendre du nombre de
  fils. M < K reste refusé.
  - R2 (`max(8,K+1)`) et les coupes forcées à (K=2, M=2) passent avec cette option.
  - Le cas explosif des huit coins est refusé par le budget au lieu d'expirer.
- **Autres frontières.**
  - Borne de `orders[kk-1]` dans `mhgp10_cluster` (SIGSEGV à 1 point pour K=2).
  - Sorties non écrites refusées au lieu de rendre le code 0.
  - Porte `fast` alignée sur les cibles construites par les plans G4.
  - Garde du nombre de boules rendue causale.

### Juges

- Le juge catalogue revient à un contrôle sur les listes publiées, avant toute normalisation :
  - I et U strictement croissants, donc sans doublon et dans l'ordre de Morton, et disjoints ;
  - poids recalculés sur ces listes ;
  - ordre (niveau exact, S*) vérifié ligne à ligne.
  Cela annule la régression du poids de coquille relevée au premier tour.
- Le juge de tour :
  - refuse une fusion enfant d'une fusion au même niveau exact (mutant `binarized_same_level` sur `TRIANGLE`) ;
  - refuse une attache à un nœud mort à la date d'entrée ;
  - refuse un bloc d'attaches manquant.
  Les naissances absorbées dans leur plateau restent admises.

### Tête

- **H3.** Une garde de domaine conjointe, avant condensation :
  - fusion au niveau zéro refusée ;
  - nœud né à zéro refusé si sa masse atteint `mcs` ;
  - λ fini et normal sur tout niveau positif consommé ;
  - masse totale multipliée par λ maximal sous 2^1000, par votre borne des stabilités.
  Vos cinq cas et les trois du premier tour sont gravés. Le domaine du producteur u18 passe toujours, et les sorties
  du producteur ne changent pas.
- **H4.** La porte devient causale par une observable externe : un peigne assez grand pour qu'une remontée quadratique
  dépasse d'au moins un facteur 50 la limite de temps. Le compteur interne reste un diagnostic.

### Bancs

- Correction de la course de signal validée par le vérificateur, avec sa simulation déterministe en porte.
- Domaine des métriques contrôlé sur les lignes non refusées : l'ARI de 1,25 est refusé ; le témoin NaN refusé reste
  admis.
- Cas isolant les quatre mutants survivants.

### SiteTree

- La doctrine v4 s'applique : le filtre flottant est coupé hors `FE_TONEAREST`, et la voie exacte prend le relais.
- La porte tourne sous les quatre modes, avec un plancher de replis fixé avant l'exécution dans les modes dirigés.
- Les deux mutants survivants de la garde sont tués.
- `filtered() == false` n'est pas présenté comme un refus sûr d'une requête forgée.

### Pool

- Borne de réclamation écrite dans le contrat et imposée.
- Formes alignées de `operator new` ajoutées aux harnais d'injection.

### Faits mathématiques

- Convention fermée déclarée. Dépendance de F1 et F3 aux égalités exactes écrite, avec un contrôle juste au-dessus
  des rayons.
- Votre témoin de vote par argmax recalculé à chaque coupe devient la fixture F5, en `false_in_general`.
- Tableau OP1–OP15 et mentions contraires corrigés.

### Sonde CUDA

Déjà faite : commit `779dd38a9`.

- Un périphérique présent mais illisible donne `cuda_error` (code 3).
- Une durée non positive ou non finie donne `timing_invalid` (code 5), sans débit publié.
- Le lanceur a des codes distincts et refuse une sortie dont le `status` ne correspond pas au code.
- Contrôle local :
  - nvcc 12.9 compile pour sm_120, avec `-Wall -Wextra` côté hôte ;
  - sans GPU, la sonde rend `no_cuda` avec le code 2 ;
  - la fonction de contrôle du lanceur passe douze cas.
- Aucune nouvelle exécution GPU.

## 3. Réponses aux trois verrous

1. **Frontière.** D'accord : aucune structure coûteuse avant un signal sur quelques bras dev, et un dénominateur fixe
   pour la preuve de laminarité. L'expérience suivante compare, à arbre FULL fixé :
   - core (contrôle) ;
   - cover (attache immédiate actuelle) ;
   - ancrage K2 différé à plusieurs η ;
   - majorité à masses fixes, uniformes (contrôle attendu en échec) puis décroissantes en 1/β ;
   - attache immédiate quand une seule composante couvre le point.
   L'univers des témoins est propre à K (p+q_min ≤ K), indépendant de Kmax.

   Diagnostics, fixés avant toute exécution :
   - emboîtement exhaustif ;
   - rappel frontière avant la première fusion parasite ;
   - masse différée et ses dates ;
   - singletons ;
   - hauteurs sur les panels de paires ;
   - variation sous perturbation appariée ;
   - coûts D et requêtes.
   La sélection EOM ne vient qu'après. Vos huit fixtures `0, 1, L, L+1` deviennent une porte de conception.
2. **Validation CSR du prototype ordre/tête.** Les prototypes de performance ne sont pas intégrés. Avant toute
   intégration de l'ordre/tête, la borne finale de chaque tranche sera contrôlée avant la boucle de lecture, avec votre
   fixture en porte.
3. **Préintégration.** Voir la section 1 : aucune qualification de copie ne sera reportée sur le binaire intégré sans y
   être rejouée.

## 4. Rangement

Le README des audits et la passation portent les liens de la relocalisation de l'audit v9, encore non commitée. Je les
commiterai juste après le commit de relocalisation de son auteur.
