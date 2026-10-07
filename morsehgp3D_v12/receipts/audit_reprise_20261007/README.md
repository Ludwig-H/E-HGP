# Reprise de l'audit pendant le développement

7 octobre 2026. Base publiée **`f601b36ac`** ; les observations sur le cache et le
pilote des petits nuages sont des captures séparées du travail non commis, fixées
par leurs empreintes. Aucun fichier produit vivant n'a été modifié par l'audit.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 pour le catalogue
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Contre-épreuve | Conclusion utile au développeur |
| --- | --- |
| [Cache mémoire](buffer/README.md) | comptage physique rétabli dans la capture, marge de 1/8 prouvée sur 208 classes ; refus concurrent après admission reproduit, `CST-0007/0019` restent à résoudre |
| [Juges M5/M6](juges/README.md) | réserves précédentes corrigées, mais M6 accepte encore des indices mal typés et un schéma inconnu ; `CST-0018` reste en cours |
| [Analyse MES-P](mes_p_provisoire/README.md) | `CST-0238` : les nombres de fils sont fusionnés ; les séparer avant comparaison CPU ou choix du seuil |
| [EMST](emst/README.md) | `CST-0232` clos : 107 appels indépendants par mode, 141 contrôles officiels, identités et géométrie conformes |
| [Admission G1](g1/README.md) | les deux contournements historiques sont refusés sur les mêmes octets ; clôture de cette portée de `CST-0018`, pas du constat entier |
| [Découpes LiDAR](donnees/README.md) | rapprochement des 69 lignes corrigées, des métadonnées de préparation et des onze prises E ; limites de contre-vérification explicites |

**Aucun nouveau chrono CPU/GPU.** La priorité performance reste la finition du
catalogue, puis le travail physique évitable des feuilles CPU
([audit précédent](../audit_performance_20261007/README.md), `CST-0233–0237`).
Les portes ou refus reproduits ici ne qualifient ni FULL v12 ni les 100 ms.

Les harnais restent bornés : petits témoins publics, métadonnées et journaux
existants, compilations unitaires. Aucun calcul LiDAR ni GCP. Les reçus ne
contiennent ni coordonnées réelles ni identité de compte ; les détails
historiques restent dans leurs reçus d'origine. Les fermetures locales sont
complétées par `SHA256SUMS` et `VALIDATION.json` du lot intégré.
