# Reçu local de MES-M5 (7 octobre 2026)

Microbanc du parcours des boîtes de centres en largeur sur le GPU (tranche T0), joué sur le codespace sans GPU contre
la v11 gelée (`ac081a06f`). Code : [`../../microbancs/mes_m5_parcours/`](../../microbancs/mes_m5_parcours/README.md) ;
rapport de l'agent : [`RAPPORT.md`](RAPPORT.md) (ses chemins `v12_m5/` désignent le dossier de code).

- **Établi sur l'hôte** (warp simulé) : le parcours en largeur, en source unique, rend exactement le parcours de la
  v11 (même statut, même grand livre, même ensemble de feuilles, tous les nœuds visités) sur ng00–02 à K5/16, K5/24 et
  K10/24, sur les uniformes de 8 000, 16 000 et 32 000 sites, sur une découpe et sur six fixtures, dont une coquille u32
  qui atteint la profondeur $96=3B$ (`CST-0205`) et un bord à 33 bits (`CST-0204`) ; six mutants tués (le mutant
  « repère de l'enfant » ne l'est que par les fixtures u32, tout passant en `i64` sur u21) ; juge à trois verdicts
  auto-testé sur 16 injections, dont les faux verdicts de `CST-0018`.
- **Non établi** : aucun temps GPU (le banc compile pour `sm_120`) ; la règle d'adoption, écrite avant la mesure, se
  juge sur G4 : identité partout, sanitizers propres, borne haute de l'IC 95 % au plus **1/4** du temps de la v11
  (frontière plus passe unique, 48 fils) sur chaque cas à feuilles de 24.
- **Données** : vidages dérivés de SemanticKITTI hors dépôt ; ce reçu ne contient que des comptes et des empreintes.

`phase=exploration_v12_hors_registre`, `public_status=not_claimed`. GCP non utilisé.
