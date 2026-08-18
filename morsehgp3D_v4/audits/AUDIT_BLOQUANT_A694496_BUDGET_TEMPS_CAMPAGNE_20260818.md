# Audit bloquant après `a694496` — le budget temporel de la session ne contient pas la campagne

Date : 18 août 2026.  
Pins audités :

- moteur/profil : `fa7cac4bf12689bd08a0d94e995d86e1070ceaf3` ;
- extension de campagne : `a694496c9d6bd9116a73e3f1b29893ec7cd4b311`.

## Verdict

Le nouveau profil grossier est utile et la réutilisation du tampon de counting sort est sémantiquement neutre. La phase d'échelle de fils est également la bonne expérience à ajouter : les runs `t8` et `tmax` à `n=32000`, puis `tmax` à `n=64000`, répondent enfin à la question de parallélisabilité sur des tailles qui comptent.

Il existe toutefois un verrou transactionnel avant de lancer cette campagne : **la durée maximale de la session est incompatible avec la liste de runs désormais exigée**.

Ce n'est pas une réserve de confort. Dans l'état actuel, l'auto-extinction de la VM peut interrompre la campagne avant le rapatriement, précisément dans la phase massive ajoutée par `a694496`.

---

## 1. Incompatibilité arithmétique des bornes

Le runner distant fixe par défaut :

```text
RUN_TIMEOUT = 10 800 s = 3 h par run.
```

La nouvelle phase 3 contient huit runs **séquentiels** :

```text
4 runs à n=32000 : uniform/eight_clusters × t8/tmax ;
4 runs à n=64000 : quatre familles × tmax.
```

Sa borne de pire cas est donc déjà

```text
8 × 10 800 s = 86 400 s = 24 h,
```

sans compter le build, les 4 pilotes ni les 24 runs de couverture.

Or le lanceur de session conserve :

```text
MAX_RUN_SECONDS         = 21 600 s = 6 h ;
GUEST_SHUTDOWN_MINUTES  = 350 min  = 21 000 s ;
TTL de la clé SSH       = 370 min.
```

Deux runs atteignant leur timeout consomment déjà toute la durée maximale de l'instance. Même sans timeout, la campagne complète n'a désormais aucune garantie de tenir avant l'auto-extinction.

La robustesse actuelle aux ruptures SSH ne ferme pas ce cas : si le guest s'éteint avant l'étape de `scp`, le rapatriement « toujours » ne peut plus contacter la VM et les statuts partiels risquent de rester sur le disque éphémère.

---

## 2. Correction recommandée : séparer les produits de campagne

Je déconseille de simplement porter la session monolithique à 24 ou 30 heures. Sur une VM spot, cela augmente la probabilité de perdre une campagne presque terminée et rend le reçu plus difficile à reprendre.

La solution robuste est de séparer au minimum :

```text
A. campagne de référence actuelle :
   4 pilotes + 24 runs de couverture ;

B. scale_threads_n32000 :
   4 runs isolés t8/tmax ;

C. scale_threads_n64000 :
   4 runs tmax, éventuellement un run/famille par session si la mesure
   pilote montre qu'une seule famille approche le timeout ou la RAM.
```

Chaque session doit avoir son propre manifeste de noms attendus, son propre reçu et une durée compatible avec **la somme de ses timeouts**, pas seulement avec la durée espérée.

Pour les sessions B/C, dériver les trois durées depuis une unique quantité :

```text
required_seconds
  = build_and_ctest_margin
  + somme des RUN_TIMEOUT des runs séquentiels
  + upload/download/validation_margin.
```

Puis exiger avant tout démarrage :

```text
MAX_RUN_SECONDS >= required_seconds ;
60*GUEST_SHUTDOWN_MINUTES > required_seconds ;
SSH_KEY_TTL_SECONDS > 60*GUEST_SHUTDOWN_MINUTES + marge.
```

Si l'on choisit malgré tout une campagne unique, ces trois inégalités doivent être satisfaites avec les 36 runs. Le défaut actuel doit être un refus avant toute action GCP.

---

## 3. Porte causale

Étendre le selftest du protocole avec une vérification purement arithmétique, sans attendre plusieurs heures :

```text
sequential_timeout_budget
  = somme des timeouts des runs qui ne se chevauchent pas ;
```

et refuser si la vie de la session/du guest/du credential est plus courte que ce budget plus la marge.

Mutant utile :

```text
campaign-add-sequential-run-without-extending-budget
```

Il ajoute un run de phase 3 sans modifier les durées ; le préflight doit mourir avant `start_and_verify.sh`.

La porte doit également vérifier que les paramètres effectivement transmis au guest sont ceux utilisés dans le calcul. Une constante commentée dans le runner n'est pas un budget partagé.

---

## 4. Suite recommandée

```text
1. scinder la campagne et ses manifestes ;
2. graver le préflight de budget temporel ;
3. seulement ensuite lancer les n=32000/64000 ;
4. utiliser leurs résultats pour décider flat scan GPU / couches convexes CPU.
```

Les développements géométriques récents sont reçus. Le blocage est uniquement celui du protocole de mesure : la phase massive demandée est bonne, mais elle doit survivre assez longtemps pour produire un reçu. Les ordinateurs n'ont pas encore appris à terminer vingt-quatre heures de timeouts dans une VM programmée pour mourir au bout de cinq heures cinquante.