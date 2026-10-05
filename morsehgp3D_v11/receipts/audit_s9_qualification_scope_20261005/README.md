# S9 publiée et périmètre de sa prochaine qualification

5 octobre 2026. Sources publiées : S9 `3d47eaa93`, réponse développeur
`90675014a`, sélection G4 `c97776ea8`. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

**Le correctif du tri est publié.** Ses quatre fichiers sont identiques
octet pour octet à la capture du reçu `audit_s9_sort_fix_20261005`, déjà
contre-éprouvée. Le manifeste des mutants ajoute `tri_refus_ignore` ; sa
présence n'est pas un résultat d'exécution. Aucun nouveau résultat G4 S9
ou L2b n'est constaté. Les anciens reçus restent inchangés.

**Préserver l'identité exacte native/Python sur les trames.** La nouvelle
matrice exclut les quatre CTests `points_vs_python`, dont les trois trames
entières à K5. Les succès locaux rapportés restent distincts. Les portes
conservées couvrent l'oracle indépendant borné jusqu'à K4, la structure et
le déterminisme des fichiers, avec des contrôles exacts échantillonnés sur
les grands fichiers. Elles ne remplacent pas le différentiel complet.

Une [contre-épreuve ciblée](math/REPORT.md) rend cette distinction
observable : dans une sortie synthétique F8, changer seulement le plancher
de 6 à 5 laisse passer l'oracle standard et les contrôles sémantiques
pendaison/arbre du lecteur. Le niveau suivant reste sous la date : le
plancher est faux. Ce n'est ni une sortie native produite, ni un essai du
lecteur complet de fichier. Le moteur contrôle bien le rang suivant ;
aucun défaut produit n'en est déduit. Les rejeux de cette capsule après
intégration passent en normal et sous `-O`.

Les retraits de portes sanitizers sont également explicites : 82 portes
de chaque inventaire historique B, comprenant toutes ses 50/65 absences.
C'était une réduction de périmètre, pas une clôture des anciens résultats.
La [projection reproductible](matrix/README.md) sépare l'ancien inventaire
filtré des nouvelles portes S9. Elle dépend de l'archive locale B épinglée.

**Mise à jour avant publication :** a7711b506 rétablit ces portes dans
huit configurations dédiées (deux ASan/UBSan et six TSan). La note
développeur d26328fe2 confirme cette reprise. La capture c977 reste une
preuve de l'étape précédente ; ce nouveau découpage ne rétablit pas les
quatre différentiels `points_vs_python`, qui nécessitent toujours le plan
Python épinglé. Aucun résultat d'exécution des huit lots n'est attribué ici.
[Recalcul des unions et intersections](shards/summary.json) : les 82 portes
historiques sont rétablies par sanitizer, toutes les anciennes absences
sont couvertes, sans doublon. Chaque ensemble base + lots sélectionne aussi
les 24 portes S9 hors différentiels longs. Le [rejeu](shards/replay.py),
normal et `-O`, utilise le pin d26328fe2 et la même archive B épinglée.

## Plan ciblé proposé

[points_differential_plan.json](points_differential_plan.json) sélectionne
directement les quatre CTests existants, chacun dans une commande séparée,
avec `python_packages="pinned"`. Aucun filtre de `g4_matrix.json` ne
s'applique à ces commandes. Les seules cibles construites sont la sonde
points et l'exportateur de référence ; le profil par défaut est Release u21.

Le développeur peut intégrer ces commandes dans la session gardée de la
source finale poussée (`--commit`), ou employer ce plan dédié. Fournir
hors dépôt les six fichiers `lidar_ng00`, `lidar_ng01`, `lidar_ng02`
avec suffixes `.u32le` et `.ids.u32le`, sous les mêmes empreintes que le
protocole. La préflight réelle doit laisser assez de temps à l'ensemble
des commandes et à la fermeture ciblée ; la validation ci-dessous ne
juge ni ces données, ni la cible cloud, ni cette fenêtre temporelle.
Ce plan ne remplace pas les portes courtes sous sanitizers, les mutants
ni l'identité des deux voies de L2b.

```sh
python3 -B replay.py /chemin/du/depot
python3 -B -O replay.py /chemin/du/depot
```

Le rejeu appelle seulement `validate_plan` du contrôleur au pin c97776ea8,
vérifie les quatre sélections et les SHA du correctif publié. Les résultats
normal/`-O` sont conservés dans `plan_check*.json`. Aucune préflight cloud,
construction, exécution native ou session G4 n'a été lancée par l'auditeur.
Le dépôt contenant les commits publics épinglés et le reçu antérieur est
requis ; aucune source produit inchangée n'est recopiée.
