# HGP-HSA pour la segmentation sémantique de SemanticKITTI

**La question :** une hiérarchie de clusters construite sur la densité — et non apprise — peut-elle donner à un réseau un meilleur contexte multi-échelle pour segmenter un scan LiDAR ?

**La cible :** l'état de l'art SemanticKITTI mono-scan. En val, régime strict, le chiffre à battre est **$73{,}1$** (DOS) depuis une baseline reproductible à $68{,}0$–$70{,}3$ — soit $+3{,}2$ à $+5{,}5$ points.

**L'état, au 14 août 2026 :** conception et falsification. **Aucune expérience apprise.** Aucun statut SOTA revendiqué. `public_status=not_claimed`.

---

## Commencez ici

> ### → **[GUIDE.md](GUIDE.md)**
> Tout le projet en neuf chapitres qui se lisent seuls : le problème, HGP en dix minutes, les descripteurs, l'opérateur, les six façons dont ça peut échouer, quoi mesurer, quelle venue. Avec le [glossaire](GLOSSAIRE.md) à côté.

Les autres documents sont plus précis et plus sévères, mais ils ne s'entrent pas sans le guide.

| Vous voulez… | Lisez, dans l'ordre |
|---|---|
| **comprendre** (30 min) | [GUIDE.md](GUIDE.md) |
| **décider quoi faire** (2 h) | [GUIDE.md](GUIDE.md) → [VOIES.md](VOIES.md) → [RISQUES.md](RISQUES.md) |
| **implémenter** (1 jour) | ci-dessus, puis [ARCHITECTURE.md](archive/ARCHITECTURE.md) → [DESCRIPTEURS_DE_NOEUD.md](archive/DESCRIPTEURS_DE_NOEUD.md) → [CONTRAT_HGP.md](archive/CONTRAT_HGP.md) → [PROTOCOLE.md](PROTOCOLE.md) |

---

## Les quatre faits qui contraignent toute proposition

Développés au chapitre 6 du guide. À connaître **avant** de proposer quoi que ce soit.

1. **Le goulot n'est probablement pas la partition.** Dans la littérature superpoint, l'oracle de partition est déjà vingt points devant les modèles (SPG : $62{,}1$ contre $88{,}2$). Un diagnostic d'oracle est donc une **porte de réfutation, pas de promotion**.
2. **HGP sous-segmente les objets fins** — le manuscrit le documente lui-même sur `birch2`. Or toute la marge de mIoU est sur `pole`, `traffic-sign`, `bicycle`, `person`, `bicyclist`.
3. **Le descripteur de nœud est le levier le plus faible** : $-0{,}7$ à $-4{,}1$ mIoU quand on retire toutes les features de nœud, contre $-3{,}0$ à $-6{,}3$ pour l'adjacence et jusqu'à $-8{,}4$ pour le nombre de niveaux.
4. **La barre val en régime strict est $\approx70{,}3$ mIoU** — MinkUNet34v2 de mmdetection3d, config, poids et log publiés, sans TTA. Pas $76$, qui est un score *test* obtenu avec caméra, temps ou TTA. Voir [CONCURRENCE.md](CONCURRENCE.md) : le classement officiel curaté est vide, le serveur a redémarré à zéro, et « mono-scan » ne désigne qu'un jeu d'étiquettes, pas une contrainte d'entrée.

---

## Où va le projet

**Une seule voie a un chemin arithmétique vers l'état de l'art**, et c'est la voie 3 de [VOIES.md](VOIES.md) : un pré-entraînement auto-supervisé dont l'axe de supervision est le **niveau de filtration**.

Le raisonnement tient en trois lignes. Le dossier a longtemps porté l'idée que l'auto-supervision ne paie qu'à peu d'étiquettes — Sonata donne $+0{,}3$ en fine-tuning complet. **DOS le réfute** : $73{,}1$ contre $69{,}1$ pour PTv3 supervisé, soit $+3$ à $+4$ points à supervision complète, pour $2$ A100 pendant $20$ h. Un pré-entraînement bien conçu vaut donc, sur ce benchmark, **le même ordre de grandeur que l'écart à combler**.

Et ce qui reste libre pour s'y distinguer est étroit mais réel :

> Chez **tous** les antécédents, l'axe de supervision est un **nombre de clusters** — PCL $25\,000$, HCSC $3000$–$2000$–$1000$. Jamais un paramètre de filtration. **Superviser sur le niveau**, que HGP rend signifiant et que la percolation permet de choisir, est à $1/10$ occupé.

Quatre voies sont **fermées** par des chiffres, dont deux formulations que j'ai portées un temps : « hiérarchie de clusters comme structure d'auto-supervision » est $9/10$ occupé (HCSC, MHCCL, HASSL), et « arbre à rayons sur nuage 3D comme prétexte » est $8/10$ occupé par Sharma & Kaul, NeurIPS 2020.

**Estimation honnête : $10$ à $15\,\%$ pour le SOTA val en régime strict.** Les conditions — battre DOS et non le scratch, dépasser $1{,}5$ point de bruit sur trois graines, et faire passer la voie 1 d'abord — sont dans [VOIES.md](VOIES.md).

---

## Carte du dossier

Huit documents au premier plan. Six autres sont dans [`archive/`](archive/README.md) : ils sont corrects, mais sur aucune voie retenue.

| Document | Contenu | Pour qui |
|---|---|---|
| **[GUIDE.md](GUIDE.md)** | tout le projet en neuf chapitres autonomes | **commencez ici** |
| [GLOSSAIRE.md](GLOSSAIRE.md) | chaque terme en une ligne | à garder ouvert à côté |
| **[VOIES.md](VOIES.md)** | la cible, ce qui est fermé et par quel chiffre, les voies retenues et leurs feuilles de route | pour décider |
| [RISQUES.md](RISQUES.md) | les réfutations possibles et les règles d'arrêt | pour décider |
| [CONCURRENCE.md](CONCURRENCE.md) | qui fait quoi, dans quel régime, avec quels chiffres | pour se comparer |
| [PROTOCOLE.md](PROTOCOLE.md) | splits, métriques, régimes, choix de la baseline | pour implémenter |
| [REFERENCES.md](REFERENCES.md) | sources primaires et statut de chaque chiffre | pour citer |
| [archive/](archive/README.md) | descripteur, T0–T6, contrat marqué, ancienne architecture | si une voie aboutit |

Le [papier HSA](NeurIPS-2025-hierarchical-self-attention-generalizing-neural-attention-mechanics-to-multi-scale-problems-Paper-Conference.pdf) est dans ce dossier ; le manuscrit de thèse est en `docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, dont les parties I et II sont la définition normative de HGP.

---

## Cible et règles

**Cible.** Un scan, LiDAR seul, entrées $(x,y,z,\mathrm{remission})$. Entraînement sur `00–07, 09, 10`, validation sur `08`, test caché `11–21` après verrouillage. Sans TTA ni ensemble. Métrique : mIoU.

**Trois effets à séparer**, sinon on ne peut rien conclure : l'**arbre** (HGP contre les autres structures), la **représentation** (quel descripteur), l'**opérateur** (attention contre pooling). Un gain global non décomposé ne suffira pas.

**Règles.** Les chiffres de concurrence sont des instantanés à réauditer avant soumission, jamais des cibles à optimiser sur la séquence 08. Le serveur de test ne sert jamais à régler un hyperparamètre. Les mots « exact », « temps réel », « GPU-friendly » et « état de l'art » ne peuvent apparaître comme **résultats** sans le protocole correspondant.

**Dépendance ouverte.** La v3 ne livre pas encore le payload facettes–cofaces–incidences complet ; sa route réduite s'arrête aux composantes $H_0$. Vérifier au runtime la fraîcheur de `morsehgp3D_v3/audits/AUDIT_ETAT_COURANT.md`.

**Portée instance.** Fermée comme contribution. Mais [VOIES.md](VOIES.md) montre que le pipeline ALPINE, avec substitution du seul clusterer, est le **diagnostic** le moins cher de l'effet arbre : une phase fermée pour la publication peut rester ouverte pour la mesure.

`python tools/check_docs.py` exclut `tests/**` : son succès ne valide pas ce corpus.
