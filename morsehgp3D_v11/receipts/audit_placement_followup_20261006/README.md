# Placement du pipeline : raccord du lecteur — 6 octobre 2026

Source : **86b3cbf14**, publiée juste après l'audit `5234340a9`.
Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Aucun build, test natif ou GCP.

**La porte IO est corrigée en source.** Le témoin hors domaine passe à
524288. Deux refus nouveaux couvrent le rejeu sans lot et le placement
sans ordres concurrents ; son plancher suit ces ajouts. Aucun PASS natif
n'est déduit de cette lecture. Le patch historique à deux fichiers du
réservoir n'est plus à appliquer : il rétablirait une borne périmée.

**Le lecteur de campagne reste désynchronisé.** `full_probe.cpp` accepte
jusqu'à 524287 avec les dépendances des bits, alors que
`full_campaign.optimization` conserve 131071. Il refuse donc les modes
légaux 180219, 212987, 278523, 344059 et 475131 avant toute campagne.

[Le patch proposé](proposal.patch) change seulement le lecteur : borne
524287 et les deux dépendances ajoutées dans le C++. Garder la borne
ancienne ou l'élargir sans ces dépendances laisserait les deux interfaces
diverger. La suppression d'un refus dans ce lecteur ne qualifie pas le
calcul natif ni les performances du placement.

[Le rejeu](replay.py) extrait la fonction Python exacte par AST, lui
applique la proposition en mémoire et la compare aux sept conditions de
refus de la CLI C++ au pin. Il couvre les 1024 combinaisons des dix bits
qui interviennent dans ces conditions, les témoins légaux ci-dessus et
quatre refus de type/domaine. Les autres bits sont sans effet sur ces
conditions. La fermeture vérifie l'égalité normal/−O et l'applicabilité
du patch ; les [empreintes des sources](sources.json) restent épinglées.

```sh
python3 -B replay.py
python3 -O -B replay.py
sha256sum -c SHA256SUMS
```
