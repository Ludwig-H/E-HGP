# Zoltan — vers un modèle de fondation 3D pour le LiDAR extérieur

Dossier de conception de la collaboration **Inria / Szegedi Tudományegyetem**
(Louis Hauseux, Zoltán Kató, Josiane Zerubia). Remis à zéro le
26 septembre 2026 : le corpus `HierarchicalSelfAttention/` a été retiré et
remplacé par `FoundationModel/`, écrit à partir de la tour FULL
`morsehgp3D_v9` réellement mesurée, des parties I–II du manuscrit et du
poster 3IA 2026.

```text
phase=conception_modele_fondation_hors_registre
backend=reference_cpu_et_cuda (producteur morsehgp3D_v9)
profile=quantized_u18_input_only
mode=conception_et_falsification
public_status=not_claimed
```

## Ce que contient le dossier

| dossier | rôle |
| --- | --- |
| [`PolyhedralEncoding/`](PolyhedralEncoding/) | la présentation du 16 septembre 2026 (sources LaTeX et PDF), **conservée telle quelle** ; c'est le seul document antérieur qui survit au nettoyage |
| [`FoundationModel/`](FoundationModel/) | l'architecture proposée, le contrat de la tour, le jeton, le protocole de test et les risques |

## L'idée, en une phrase

Le nuage de points est un artefact du capteur ; la surface, elle, varierait
beaucoup moins. On remplace donc le point par une **pièce géométrique
polyédrique** issue de la hiérarchie HGP, et on donne au modèle le contexte
multi-échelle que fournit l'arbre de fusion de ces pièces — exactement
l'hypothèse de recherche du poster 3IA 2026.

## Où commencer

1. [`FoundationModel/README.md`](FoundationModel/README.md) — parcours d'entrée.
2. [`FoundationModel/OBJET.md`](FoundationModel/OBJET.md) — ce que la tour v9
   fournit vraiment, avec ses chiffres mesurés.
3. [`FoundationModel/ARCHITECTURE.md`](FoundationModel/ARCHITECTURE.md) — le modèle.
4. [`FoundationModel/PROTOCOLE.md`](FoundationModel/PROTOCOLE.md) — comment le tester
   en cours de route.

## Ce que ce dossier n'est pas

Ni une suite de tests exécutable, ni une expérience apprise, ni un résultat.
Aucun chiffre d'apprentissage n'y est revendiqué. Les seuls nombres cités comme
acquis sont ceux des reçus `morsehgp3D_v9/receipts/`, et ils sont toujours
accompagnés de leur reçu.

## Licences

Le code et la documentation de ce dossier sont sous MIT, comme la racine. Les
poids pré-entraînés de la lignée Pointcept (Sonata, Concerto, Utonia) sont en
**CC-BY-NC 4.0** : ils se comparent, ils ne s'importent jamais dans la ligne
produit. Aucun octet SemanticKITTI n'est versionné ici.
