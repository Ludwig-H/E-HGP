# Contrelecture de la pendaison intérieure H_m

3 octobre 2026. `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Audit du WIP développeur e26b48055,
sans modifier son code ni lancer de build, moteur, fit ou session G4.

**H_m est une réponse pertinente aux frontières partagées.** Sous un véritable
entrelacement transportant les couvertures qualifiées, ses entrées et réunions
sont stables à **3δ**, soit **3ε en rayon**. La construction reste à k fixé ;
aucune synthèse multi-k ni optimalité statistique démontrée.

## Preuves et corrections proposées

- [Plafond de niveau et preuve3δ](check_scale.py) : **69gardes** normal/−O
  identiques. La naissance de la racine ne borne pas les dates de couverture.
  Quatre sites entiers u21, k3/m4, ε85 : variation1235475, contre les limites
  annoncées591855 pour les entrées et986425 pour les réunions. H3 abstrait
  reste valide ; corriger le plafond en β ou passer à la règle en rayon.
- [Comparaisons de radicaux](check_radicals.py) : **1536gardes** normal/−O
  identiques. Certifier les égalités par classes de carrés rationnelles
  (ratios testés par isqrt, sans factorisation), puis séparer les nonzéros
  par intervalles ou refuser explicitement si le budget est épuisé.
- Le filtre `cmp_level` ajouté dans le WIP f023f6d0 produit un signe contraire
  au calcul exact sur un témoin scalaire192bits de magnitudeu24. Il doit
  majorer l'erreur absolue des trois racines, sans supposer une petite erreur
  relative après leur soustraction. **Aucun nuage Cloud générateur ni mauvais
  résultat géométrique natif u21 n'est démontré par ce témoin scalaire.**

[Contrelecture active et conseils au développeur](../../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
Les dates β=t+meet−q ne sont pas des rangs du catalogue ; leur arithmétique
intermédiaire et les nouveaux plateaux doivent avoir leurs contrats propres.

## Campagnes du développeur recoupées

[Recoupe indépendante et métadonnées](campaign_review.json), sources des
résultats `claudepts1/2` closes et arrêts certifiés, consommateur joué5df63b60 :

| Périmètre | Sauvetages (objet,k) | Pertes | Sauvetages forts |
|---|---:|---:|---:|
| pts1,42entrées LiDAR distinctes/497instances | 53 | 11 | 7 |
| pts2,72voisines/874instances | 101 | 23 | 24lignes,9couples classe/instance répétés |
| pts2,20témoins/204instances | 0 | 0 | 0 |

Les44noms pts1 comprennent deux doublons exacts : démo01↔08/001176 et
démo03↔08/000048 ; les524/57/11/8lignes originales restent distinguées.
128synthétiques/704objets terminés, grille adaptative par scène20–273µm,
mêmes sites pour toutes les méthodes ; la compilation reste u21 même si
les coordonnées préparées tiennent en18bits. Les20témoins donnent H_m−HDBSCAN moyen
−0,00359/−0,00437/−0,00216/+0,000685 à k2/3/5/10. La sélection des difficultés
et les voisins corrélés ne permettent pas une généralisation statistique.
Le score est celui du meilleur nœud, pas d'une partition automatiquement choisie.
Les deux campagnes restent séparées de nos17cas c40/a12.

## Sources et reproduction

[Sources WIP figées](source_bindings.json) : doc088dd067, consommateur
LIVE0da8fce4, variante rayon30447f75 puis [helper f023f6d0](source_radius_after.py).
Ces fichiers sont distincts du consommateur5df63b60 effectivement joué sur G4.
Leurs analyses ne transfèrent pas sa qualification. Les métadonnées de
[comparaison des sources](radicals_sourcemetadata.json) conservent cette distinction.
Cette capsule ne contient ni coordonnées réelles, ni labels bruts, ni archive
de campagne, ni ELF ; les seuls nuages dans les preuves sont synthétiques.
Le ledger des17cas précédents reste inchangé.

Depuis la racine du dépôt, pour rejouer uniquement les contrôles mathématiques :

```sh
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v11/receipts/hm_review_20261003/check_scale.py
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v11/receipts/hm_review_20261003/check_radicals.py
```

Les sorties optimisées passent avec `python3 -O`. Le programme de radicaux
extrait les helpers Python purs de copies hachées, sans importer le module
développeur ni appeler ses exécutables. L'historique du préflight d'audit reste
dans les métadonnées. [Ledger complet](SHA256SUMS). Aucun contrat100ms, GPU,
massif ou module natif de hiérarchie de points nouvellement acquis.
