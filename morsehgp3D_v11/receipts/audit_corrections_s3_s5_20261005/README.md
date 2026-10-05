# Corrections S3/S5 — 5 octobre 2026

`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.

Deux avancées ferment les demandes de source de l'audit : la provenance
API est validée avant publication, et le juge exact des attaches est
inscrit dans les portes permanentes. Aucun nouveau défaut mathématique
FULL établi. Aucun build, test natif, benchmark ou GCP lancé par cet audit.

## Manifeste S5

Le [correctif relu](api/README.md), WIP du développeur au contexte
`59743c21000bfa216a5301655a48e98b90aaa20a`, contrôle les tailles contre le
poids du nuage et refuse le budget déclaré nul. Le contrôle précède les
fichiers et le rapport. L'aller-retour API vers le lecteur officiel et
les quatre refus sans publication sont maintenant déclarés.

Les douze sources du delta ont été reconstruites depuis ce pin Git, le
correctif conservé et les deux fichiers nouveaux : toutes les empreintes
de `api/source_manifest.json` concordent. Cette capture dépend de l'objet
Git de contexte ; elle ne prétend pas être une archive autonome.
Le [défaut initial](../audit_api_publication_20261005/README.md) reste
immuable. Le passage natif de la nouvelle porte demeure à acquérir.
Après cette capture, le développeur a corrigé le mutant de tailles pour
qu'il conserve un usage du paramètre, ainsi que l'ancre de l'ancien
mutant de provenance : [delta de suivi](api/followup.patch),
[empreintes et portée](api/followup.json). La réserve de compilation
signalée dans la première contrelecture est donc corrigée en source.

## Rattachement S3

WIP `build/v11-impl-l1b`, contexte public
`19b2fb218b86411a138e5ea351e1b42e265c1127`. Le
[raccord CMake et le protocole de sonde](raccord/README.md) utilisent
réellement `Cat_K`, `build_order(K)`, W1/W3 et une permutation d'entrée.
Refus, arrêt, sortie incomplète ou divergence font échouer le juge.
Le témoin K10 intervient dans le différentiel K1..12 et dans une porte
native dédiée K9..12. Le cœur du moteur reste identique à la capture
précédemment relue.

La [contre-épreuve portable](math/REPORT.md) vérifie cinq nuages et seize
ordres : D2, E5, boule faible, fusion passagère et témoin à douze sites.
Elle éprouve le comparateur avec des sorties synthétiques exactes et neuf
mutations ciblées. Les sorties normales et optimisées sont identiques.
Le rejeu après mise en place dans ce reçu retrouve le même JSON à l'octet.
Ce contrôle n'exécute ni la sonde native ni les 963 ordres de la suite
déclarée ; il ne constitue pas un nouvel oracle numérique indépendant
de S1.

Rejeu depuis ce dossier, sans le worktree du développeur :

```sh
python3 -B math/replay.py
python3 -B math/replay.py --optimized
```

## Suite utile

Intégrer les correctifs, achever S6b/S7 puis qualifier l'assemblage sur
G4 : supports complets, refus entier au-delà de 24 sites, budget et
count/fill, déterminisme et publication. La décision de performance doit
porter sur le journal actif et la sortie réellement livrée. Ces étapes
restent ouvertes ; aucun succès de primitive ne les remplace.
