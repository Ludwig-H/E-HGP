# S8 — arithmétique exacte de la future sortie points

5 octobre 2026. Source publiée
`53c027fe848b0d890f164eb87ebf347338c58d55` ; cadre
`exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`.

**Aucun défaut important nouveau établi.** La revue porte sur les entiers
et rationnels bornés, les sommes de radicaux et la table de racines.
Elle couvre les décisions géométriques, les refus, les allocations,
les écritures et leurs bornes. Le filtre de signature n'est jamais une
preuve d'égalité ; le test de carré parfait reste obligatoire. Une
capacité insuffisante rend un refus explicite, jamais un signe inventé.

La [contre-épreuve mathématique](math/REPORT.md) recoupe 80 signes dont
20 égalités avec un juge algébrique indépendant, une collision de
signature, 60 décisions de la table, une quasi-égalité à 192 bits et
un refus à l'épuisement des précisions. Normal et `-O` concordent.
Ce modèle Python vérifie le raisonnement ; il n'exécute pas le C++.

La [revue native](native/README.md) identifie douze fichiers lus
entièrement et les rapports locaux consultés. Les bornes d'entiers,
de rangs et de sommes sont vérifiées en lecture. Les limites déjà
déclarées restent dans `native/review.json`, sans nouvelle alerte active.

Les rapports du développeur conservés dans `math/developer_reports/`
portent le pin local antérieur `adfcdc692`. Ils ne deviennent pas des
tests de l'auditeur ni une qualification de la source publiée ; le
plafond u32 de `RootTable` est déjà corrigé dans le pin relu. Les portes
S8 u18/u24, sanitizers, mutants numériques complets et le coût W48
restent à qualifier. La [session G4 A2](../audit_g4_a2_20261005/README.md)
porte b319efc84, antérieur à S8. S9 reste en développement et n'est pas
couverte par ce verdict.

Rejeu portable depuis un dépôt contenant le pin :

```sh
python3 replay.py /chemin/du/depot
python3 -O replay.py /chemin/du/depot
```

Le script vérifie les empreintes des fichiers de la capsule, les douze
sources natives, puis reconstruit par Git les sources du modèle et le
rejoue dans un dossier temporaire. Il compare sa sortie aux octets
capturés. Aucun binaire, nuage, réseau, build ou appel GCP n'est requis.
`packaged_checks.json` garde le résultat du rejeu après intégration.
Le premier rejeu d'intégration cherchait à tort les deux rapports locaux
dans Git ; cet échec du harnais et sa source sont conservés dans
`attempts/`. Le rejeu corrigé les vérifie contre leurs copies épinglées.
