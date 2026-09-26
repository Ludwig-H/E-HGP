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

Le nuage de points reflète le capteur et la scène observée. Une partie des
encodeurs 3D fixe une **échelle métrique** — taille de voxel, liste de rayons,
taille de *patch* — dont le transfert peut demander un réaccord. La tour FULL
de Morse HGP 3D fournit une structure **exacte sur les retours acquis**, à
plusieurs ordres et rayons. Son intérêt pour un réseau et la stabilité des
coupes consommées sont les hypothèses à mesurer.

## Où commencer

1. [`FoundationModel/AUDIT_V9_ET_ARCHITECTURE_20260926.md`](FoundationModel/AUDIT_V9_ET_ARCHITECTURE_20260926.md) — l'audit v9 et les décisions d'interface.
2. [`FoundationModel/README.md`](FoundationModel/README.md) — la thèse et le
   parcours d'entrée.
3. [`FoundationModel/OBJET.md`](FoundationModel/OBJET.md) — ce que la tour est,
   et les six primitives qu'une architecture y lit.
4. [`FoundationModel/ETAT_DE_LART.md`](FoundationModel/ETAT_DE_LART.md) — le
   verrou, et la **table de substitution** qui sert de plan de mesure.
5. [`FoundationModel/ARCHITECTURE.md`](FoundationModel/ARCHITECTURE.md) —
   HGP-UNet.
6. [`FoundationModel/MESURE.md`](FoundationModel/MESURE.md) — comment établir
   l'apport, témoins négatifs compris.
7. [`FoundationModel/PLAN.md`](FoundationModel/PLAN.md) — l'ordre de
   construction et les points de décision.

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
