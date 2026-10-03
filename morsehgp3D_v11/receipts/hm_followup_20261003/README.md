# Suivi de la règle intérieure en rayon

3 octobre 2026, WIP développeur e26b48055. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only /
not_claimed`. Reçu neuf ; les capsules précédentes restent closes et inchangées.
Aucun build, test natif, fit ni commande GCP dans ce suivi.

- **Qualification H5 réfutée**, reproduction Gram/Γ indépendante :
  `check_qualified_delay.py`,126 gardes exactes, sorties normal/−O identiques.
  H^r_3 retarde un point cœur à √250−5/2 après la fusion F12, malgré α+d₂/2≤F.
  Condition suffisante corrigée : t′+d_k/2≤F, t′ première couverture qualifiée.
- **Optimalité intrinsèque** : sources primaires `source/workflow/axiomes/`
  et contre-vérification `source/workflow/verif_axiomes/`. Théorème C valable
  sur tous profils abstraits ; la borne supérieure géométrique3ε est prouvée,
  la minimalité géométrique reste ouverte.
- **Deux défauts arithmétiques levés sur58952a8b** : `radicals/check_followup.py`,
 1536 gardes normal/−O, vraies classes/helpers extraits par AST et empreintes.
  L'égalité par classes de carrés est certifiée ; un signe non séparé est refusé ;
  le filtre utilise l'erreur absolue de tous les termes.
- **Croisement multi-k confirmé** : `check_vertical.py`,55 gardes exactes
  normal/−O. Sur six sites collinéaires, les blocs qualifiés P1 aux ordres2/3
  se croisent au même rayon7. L'impossibilité générale v10 exige aussi
  l'entrée immédiate non ambiguë ; une synthèse avec retard explicite reste possible.
- **Source réellement jouée** : `provenance/claudepts3_source.json`, paquet
  a6b44d9a vérifié. À22:38:44UTC, session sans DONE/reçu : rayonf023f6d0,
  antérieur au correctif58952a8b. Observation datée, aucune qualification
  d'une campagne encore ouverte. `provenance/check.py` passe10 contrôles
  normal/−O, uniquement empreintes de copies de code et métadonnées.

`source/` conserve le document966af6de, l'oracle bd05 et la porte cc56
relus : contexte Decimal des sommes28 chiffres et comparaison double tolérée,
insuffisants pour certifier tous les propriétaires aux plateaux exacts.
`points_gate.after.py` eb467b91 ajoute des gardes arithmétiques dans le WIP,
mais garde les sommes hors du contexte Decimal200 ; aucun transfert à G4.
Les conseils actifs sont mis à jour en place dans `audits/`.

Reproduction depuis ce dossier, Python et NumPy de la campagne :

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -B check_qualified_delay.py
PYTHONDONTWRITEBYTECODE=1 python3 -O -B check_qualified_delay.py
PYTHONDONTWRITEBYTECODE=1 python3 -B check_vertical.py
PYTHONDONTWRITEBYTECODE=1 python3 -O -B check_vertical.py
PYTHONDONTWRITEBYTECODE=1 python3 -B radicals/check_followup.py
PYTHONDONTWRITEBYTECODE=1 python3 -O -B radicals/check_followup.py
PYTHONDONTWRITEBYTECODE=1 python3 -B provenance/check.py
```

Les témoins proches à2^-9000 sont hors192bits ; le témoin scalaire
annulation est dans192bits/u24 mais sans Cloud générateur certifié.
Ces contrôles sont des gardes mathématiques et de helpers Python, jamais
une qualification native u21/u24, un contrat de temps ou un gain statistique.
