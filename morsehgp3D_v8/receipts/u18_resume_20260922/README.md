# Reprise u18 / arrêt anticipé d'atlas — 22 septembre 2026

Cadre : `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u18_input_only`, `public_status=not_claimed`. Aucun usage GCP.
Voir [l'analyse et le périmètre](../../docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md).

## Essais conservés, non qualifiants

- `preflight_center_oracle/` : premier juge du centre inversé ; source et
  sortie conservées, puis test corrigé. Le build de préflight est mutable.
- `release/` : 134/136 tests exécutés passent ; deux lecteurs de mutations
  échouent. L'un refuse les nouveaux champs u18, l'autre exige le diagnostic
  d'une fixture ultérieure alors que le mutant a déjà été tué.
- `sanitize/` : compilation réussie, suite interrompue volontairement par
  SIGINT après l'échec Release. Le signal et les sorties sont conservés.

Les captures en échec ne sont pas masquées par la reprise. Leur lecteur
refuse normalement de rendre un verdict PASS.

## Protocole de reprise

`bench/run_u18_resume_checks.py run` impose un nouveau build et un nouveau
dossier de capture. Les commandes effectives, sorties brutes/base64,
inventaires de tests, dépendances précompilées par unité, binaires et
fermetures avant/après sont enregistrés. Les lectures `read` et `python3 -O`
recontrôlent les fichiers présents, sans reconstruire le build.

Les trois exclusions Release historiques sont explicites ; cinq mutations
supplémentaires sont désactivées sous sanitizers car leurs lanceurs compilés
ne savent pas lier ce profil. Une exclusion n'est jamais un test réussi.

Les reçus sont des preuves **LIVE locales**, dépendantes des builds,
compilateurs et en-têtes non versionnés ; ce répertoire n'est pas une
archive d'exécution autonome. Les identités géométriques testées ne
qualifient ni un catalogue global ni FULL ni GPU.

Le diagnostic de croissance 8k/16k/32k est explicitement celui d'une
arête fournie avec sortie q4 vide connue ; ne pas l'étendre au nombre
d'arêtes, au générateur LiDAR ou au coût de toute la tour.

## Première mesure entière sans sol à 1 mm

`ground_1mm_first/` : une ligne sélectionnée, statut de campagne `partial`
car les autres configurations n'ont pas été exécutées. Trame08/000000,
39 885 sites retenus, K5/s8/W8, option de saturation encore désactivée :
104,63s mur, 812,82 CPU·s, 691 284 supports q3 et 158 496 q4.
L'entrée `.u32le` complète, la sonde élargie et tous les fichiers lus sont
épinglés ; le lecteur v2 normal/−O avec `--check-live` passe (187 fichiers).
Zéro paire W1/W8 et zéro référence : aucune identité parallèle nouvelle.
Pas FULL, ni GPU, ni segmentation dans le chrono.

```sh
python3 -B morsehgp3D_v8/bench/run_ground_baseline.py read --output morsehgp3D_v8/receipts/u18_resume_20260922/ground_1mm_first --variant only --check-live
python3 -O -B morsehgp3D_v8/bench/run_ground_baseline.py read --output morsehgp3D_v8/receipts/u18_resume_20260922/ground_1mm_first --variant only --check-live
```
