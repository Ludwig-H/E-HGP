# HGP-HSA pour la segmentation sémantique de SemanticKITTI

**La question :** une hiérarchie de clusters construite sur la densité — et non apprise — peut-elle donner à un réseau un meilleur contexte multi-échelle pour segmenter un scan LiDAR ?

**L'état, au 14 août 2026 :** conception et falsification. **Aucune expérience apprise.** Aucun statut SOTA revendiqué. `public_status=not_claimed`.

---

## Commencez ici

> ### → **[GUIDE.md](GUIDE.md)**
> Tout le projet en neuf chapitres qui se lisent seuls : le problème, HGP en dix minutes, les descripteurs, l'opérateur, les six façons dont ça peut échouer, quoi mesurer, quelle venue. Avec le [glossaire](GLOSSAIRE.md) à côté.

Les autres documents sont plus précis et plus sévères, mais ils ne s'entrent pas sans le guide.

| Vous voulez… | Lisez, dans l'ordre |
|---|---|
| **comprendre** (30 min) | [GUIDE.md](GUIDE.md) |
| **décider quoi faire** (2 h) | [GUIDE.md](GUIDE.md) → [ORDRE_DES_PREUVES.md](ORDRE_DES_PREUVES.md) → [RISQUES.md](RISQUES.md) |
| **implémenter** (1 jour) | ci-dessus, puis [ARCHITECTURE.md](ARCHITECTURE.md) → [DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md) → [CONTRAT_HGP.md](CONTRAT_HGP.md) → [EXPERIMENTAL_PROTOCOL.md](EXPERIMENTAL_PROTOCOL.md) |

---

## Les quatre faits qui contraignent toute proposition

Développés au chapitre 6 du guide. À connaître **avant** de proposer quoi que ce soit.

1. **Le goulot n'est probablement pas la partition.** Dans la littérature superpoint, l'oracle de partition est déjà vingt points devant les modèles (SPG : $62{,}1$ contre $88{,}2$). Un diagnostic d'oracle est donc une **porte de réfutation, pas de promotion**.
2. **HGP sous-segmente les objets fins** — le manuscrit le documente lui-même sur `birch2`. Or toute la marge de mIoU est sur `pole`, `traffic-sign`, `bicycle`, `person`, `bicyclist`.
3. **Le descripteur de nœud est le levier le plus faible** : $-0{,}7$ à $-4{,}1$ mIoU quand on retire toutes les features de nœud, contre $-3{,}0$ à $-6{,}3$ pour l'adjacence et jusqu'à $-8{,}4$ pour le nombre de niveaux.
4. **La barre val en régime strict est $\approx70{,}3$ mIoU** — MinkUNet34v2 de mmdetection3d, config, poids et log publiés, sans TTA. Pas $76$, qui est un score *test* obtenu avec caméra, temps ou TTA. Voir [CONCURRENCE.md](CONCURRENCE.md) : le classement officiel curaté est vide, le serveur a redémarré à zéro, et « mono-scan » ne désigne qu'un jeu d'étiquettes, pas une contrainte d'entrée.

---

## Où va le projet

La cible reste la segmentation sémantique de SemanticKITTI. Mais le **régime** a changé, pour une raison mesurable : en supervision complète la marge est d'environ un point et la variance de graine vaut $1{,}5$ point — un gain n'y serait ni mesurable ni attribuable. En régime à peu d'étiquettes, la marge est de **10,3 points** (linear probing $62{,}0$ contre supervisé $72{,}3$).

La revendication est donc devenue négative et précise :

> Toute l'auto-supervision LiDAR fabrique ses unités avec HDBSCAN, **condense l'arbre en une partition plate et le jette**. Nous ne condensons pas : les nœuds internes, la relation parent–enfant et les niveaux sont le signal.

Ce qu'elle n'est pas : ni « utiliser une hiérarchie » (cTree, NeurIPS 2020), ni « utiliser la densité » (standard du domaine depuis TARL), ni « sans caméra » (déjà le cas de TARL, SegContrast, BEVContrast, ALSO). Les concurrents directs sont **DOS** et **PointINS**, pas Concerto. Détail au [chapitre 8 du guide](GUIDE.md) et dans [STRATEGIE_PUBLICATION.md](STRATEGIE_PUBLICATION.md).

---

## Carte du dossier

| Document | Contenu |
|---|---|
| [GUIDE.md](GUIDE.md) | Le parcours complet. **Point d'entrée.** |
| [GLOSSAIRE.md](GLOSSAIRE.md) | Tous les termes, une ligne chacun. |
| [ORDRE_DES_PREUVES.md](ORDRE_DES_PREUVES.md) | Quoi mesurer, dans quel ordre, et où placer le budget de nouveauté. |
| [RISQUES.md](RISQUES.md) | Les réfutations possibles, leurs tests, les règles d'arrêt chiffrées. |
| [CONCURRENCE.md](CONCURRENCE.md) | L'état de l'art, les régimes de comparaison, l'espace de nouveauté. |
| [DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md) | Comment résumer un nœud : support, canaux radiaux, canal de masse, points contre polyèdre reconstruit. |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Le modèle, ses contrats d'entrée, les baselines appariées. |
| [CONTRAT_HGP.md](CONTRAT_HGP.md) | L'objet HGP marqué, ses quatre carriers, sa sérialisation, ses fixtures. |
| [THEOREMES.md](THEOREMES.md) | Le programme théorique T0–T6 et la proposition QC-HSA. |
| [RESEARCH_PLAN.md](RESEARCH_PLAN.md) | Les lots de travail, leurs livrables et leurs portes. |
| [EXPERIMENTAL_PROTOCOL.md](EXPERIMENTAL_PROTOCOL.md) | Splits, métriques, régimes, matrice d'ablation. |
| [STRATEGIE_PUBLICATION.md](STRATEGIE_PUBLICATION.md) | Positionnement, claims autorisés, pivots selon le résultat. |
| [REFERENCES.md](REFERENCES.md) | Sources primaires et statut de chaque chiffre. |

Sources locales : le [papier HSA (NeurIPS 2025)](NeurIPS-2025-hierarchical-self-attention-generalizing-neural-attention-mechanics-to-multi-scale-problems-Paper-Conference.pdf) est dans ce dossier ; le manuscrit de thèse est en `docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, dont les parties I et II sont la définition normative de HGP.

---

## Cible et règles

**Cible.** Un scan, LiDAR seul, entrées $(x,y,z,\mathrm{remission})$. Entraînement sur `00–07, 09, 10`, validation sur `08`, test caché `11–21` après verrouillage. Sans TTA ni ensemble. Métrique : mIoU.

**Trois effets à séparer**, sinon on ne peut rien conclure : l'**arbre** (HGP contre les autres structures), la **représentation** (quel descripteur), l'**opérateur** (attention contre pooling). Un gain global non décomposé ne suffira pas.

**Règles.** Les chiffres de concurrence sont des instantanés à réauditer avant soumission, jamais des cibles à optimiser sur la séquence 08. Le serveur de test ne sert jamais à régler un hyperparamètre. Les mots « exact », « temps réel », « GPU-friendly » et « état de l'art » ne peuvent apparaître comme **résultats** sans le protocole correspondant.

**Dépendance ouverte.** La v3 ne livre pas encore le payload facettes–cofaces–incidences complet ; sa route réduite s'arrête aux composantes $H_0$. Vérifier au runtime la fraîcheur de `morsehgp3D_v3/audits/AUDIT_ETAT_COURANT.md`.

**Portée instance.** Fermée comme contribution. Mais [ORDRE_DES_PREUVES.md](ORDRE_DES_PREUVES.md) montre que le pipeline ALPINE, avec substitution du seul clusterer, est le **diagnostic** le moins cher de l'effet arbre : une phase fermée pour la publication peut rester ouverte pour la mesure.

`python tools/check_docs.py` exclut `tests/**` : son succès ne valide pas ce corpus.
