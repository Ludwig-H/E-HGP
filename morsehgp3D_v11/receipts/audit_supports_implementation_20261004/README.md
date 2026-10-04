# Contrat S0 et premiers raccords S3/S5/S6

4 octobre 2026. `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Revue indépendante de quatre
acteurs WIP base f98aeed67, sans les modifier. Aucun build, test natif,
fit, CUDA ou GCP. Les changements après capture sont consignés, sans
modifier les copies initiales ni transférer de qualification.

Réponses écrites aux quatre questions du développeur :

- D.1 : garder les cofaces par boule ; incidences par Q facultatives.
  La somme de K-parties par boule compte des incidences (b,F), pas une
  cardinalité globale. Sa robustesse vaut seulement à p,m,K fixés.
- D.2 : signature versionnée recalculable depuis SP, avec champs
  géométriques/canoniques à la place du BallIdx non publié. Le moteur
  FullDomain sait la calculer ; l'ancien dump FUL1 seul ne contient pas S*.
- D.3 : état de publication complète distinct d'une erreur de transport
  ou synchronisation, et empreinte du manifeste conservée. Les codes
  de refus ne doivent pas masquer un dossier complet resté visible.
- D.4 : E2 faible conserve le sélecteur comprimé pour la stricte ; son
  mode K-partie arbitraire autorise initial=λ et utilise l'ancêtre fermé.

Deux gardes de capacité précises, sans sortie native erronée reproduite :

- [S6](qb/README.md) : make_shape(UINT32_MAX,2,2,1) accepte par repli
  de p+q en u32. Une comparaison u64 ou borne préalable p<K ferme
  l'entrée invalide de cette factory publique ; Catalogue valide protégé.
  **Corrigé pendant la revue** : p≤11 est contrôlé avant l'addition,
  avec une porte de refus ajoutée. [Recoupe source](followup/README.md),
  sans exécution native ; la capsule initiale reste intacte.
- [S3](tower/README.md) : vérifier la capacité avant le cast u32 du
  nombre de traces. Une coquille de150 sites a un minorant de plus
  de8,36 milliards de traces ; aucune construction colossale exécutée.
  Ne pas reporter le plafond24 de la sortie supports sur FULL.

Les difficultés de rattachement et de Q_b sont relues favorablement :
graines, plateaux fermés, continuations, listes minimales avant fermeture
Euler et q4 à cofaces nulles. Cela reste une revue de WIP, pas sa
qualification. Le contrat mathématique L0 courant reprend la correction
de D2 ; les sources S3 gardent le bon test de rang de la graine.

| Capsule | Gardes | Portée |
| --- | ---: | --- |
| [Tour](tower/README.md) | 462 | plateau, transport des graines, E2, borne scalaire u32 |
| [Shape/Q_b](qb/README.md) | 529 | domaine public de Shape, modèle u32/i32 et frontières |
| [Contrat/empreinte](evidence/README.md) | 36 | signature proposée autonome SP+FullDomain et provenance |
| [Façade](api/README.md) | 92 | trois sites K2, contact, témoins F5 exacts et bornes centre |

**1 119 gardes**, normal/−O identiques. La façade est relue en source ;
les opérations natives de FENV, IO, mémoire, workers et formats ne sont
pas exécutées. Le seul produit exécuté est la copie du nouvel oracle
Python sur des triples de sites, jamais sa suite complète ou un profilage.
Les quinze témoins F5 sont jugés par arithmétique exacte des constantes.

Les inventaires de chaque capsule restent inchangés. [SOURCE.json](SOURCE.json)
fixe leurs SHA ; l'inventaire parent couvre tous les payloads, sauf
SHA256SUMS lui-même. Les chemins privés des métadonnées sont de provenance ;
le rejeu ne lit que les copies locales.

```sh
python3 -B -S check.py
python3 -B -S -O check.py
```

Le lecteur vérifie les inventaires exacts et les huit rejeux portables
(normal/−O pour quatre capsules), comparés aux sorties conservées et
à [RESULTS.json](RESULTS.json). Les rapports locaux du développeur restent
des rapports ; les matrices et mutants natifs sont à qualifier sur G4.
La première collecte, erronée sur les noms de fichiers de sorties des
capsules, est [conservée](attempts/README.md) ; le lecteur corrigé ferme
l'ensemble sans modifier les enfants.
