# Reçu du réaudit global de Zoltan — 26 septembre 2026

Base inspectée : `d0e711e23617a7fa2838cac9b370ff125d4be5fb`.
Python 3.12.1. Travail local CPU, aucune exécution native, aucun modèle
entraîné, GCP non utilisé.

## Périmètre et résultat

Les six scripts vérifient **33 fixtures distinctes** et **69 propositions
incorrectes réfutées**. Chaque script a été exécuté en normal et en
`-O`, soit douze commandes. Les deux modes produisent les mêmes octets.
Ces propositions réfutées ne sont pas des mutants natifs compilés.

| script | fixtures | variantes réfutées | capture |
| --- | --- | --- | --- |
| verify_cut_algebra.py | 11 | 19 | rejeu comparé aux anciennes archives |
| verify_guidance_k1.py | 4 | 10 | rejeu comparé aux anciennes archives |
| verify_guidance_algebra.py | 6 | 13 | rejeu comparé aux anciennes archives |
| verify_architecture_choices.py | 4 | 8 | nouvelles sorties ci-dessous |
| verify_guidance_controls.py | 4 | 12 | nouvelles sorties ci-dessous |
| verify_support_carrier.py | 4 | 7 | nouvelles sorties, dont 24 permutations |

Portée : algèbre rationnelle, arbres/distributions abstraits et géométrie
explicite du carré/segments. Les résultats ne qualifient ni FULL, ni
l'export natif, ni la qualité apprise, ni une complexité de moteur.

## Commandes reproductibles depuis la racine

Pour chacun des six scripts nommés ci-dessus, exécuter les deux commandes
ci-dessous en remplaçant le nom ; toutes les commandes exactes exécutées
figurent dans les JSON de rejeu.

```sh
python3 Zoltan/FoundationModel/reference/verify_architecture_choices.py
python3 -O Zoltan/FoundationModel/reference/verify_architecture_choices.py
```

- [replay_existing.json](replay_existing.json) : six commandes, hashes,
  correspondance exacte avec les anciennes archives ;
- [replay_new.json](replay_new.json) : six nouvelles commandes, hashes,
  codes de sortie, stderr et nombres de fixtures ;
- [inventory_before.json](inventory_before.json) : les 52 fichiers initiaux,
  lus depuis le commit de base et hachés avant comparaison.

## Sorties nouvelles

| contrôle | normal | optimisé |
| --- | --- | --- |
| architecture | [normal](architecture_choices_normal.json) | [−O](architecture_choices_optimized.json) |
| guidage | [normal](guidance_controls_normal.json) | [−O](guidance_controls_optimized.json) |
| supports | [normal](support_carrier_normal.json) | [−O](support_carrier_optimized.json) |

Chaque sortie contient le hash de son propre script ; le rejeu compare ce
hash aux octets lus avant exécution, puis vérifie leur conservation après.
Les anciens scripts et leurs huit fichiers de reçus sont inchangés.

## Contrôles documentaires et archive

Le contrôle canonique `python3 tools/check_docs.py` est complété par
l'appel explicite à `tools.check_docs.validate` sur les Markdown actifs
de Zoltan : le périmètre canonique n'inclut pas ce dossier.
Les commandes et résultats sont dans [checks.json](checks.json).
`git diff --check` vérifie le diff courant.

Les 27 fichiers de présentation sont comparés par SHA256 à l'inventaire.
Le PNG SZTE correspond à sa source base64. Texte des 18 pages PDF relu,
page 11 inspectée visuellement ; aucune recompilation LaTeX et aucune
nouvelle vérification du layout complet. Le README historique comporte
un espace final préexistant, conservé dans cette archive intacte.

Voir le [rapport](../../REAUDIT_GLOBAL_20260926.md) pour les conclusions
et leur domaine exact.
