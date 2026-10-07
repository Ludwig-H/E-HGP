# Notes de travail de la v11 jusqu'ici hors dépôt

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Versées à la clôture de la v11 pour ne pas perdre leur contenu. **GCP non utilisé.**

## Avertissement : notes de travail, pas des reçus de mesure

Ces fichiers sont des **notes de travail produites par des workflows d'agents**, du 2 au 6 octobre 2026. Ils étaient
dans `build/v11-persist/`, hors dépôt.
- Ils contiennent des **estimations non qualifiées** et des plans antérieurs aux mesures.
- L'état final fait foi ailleurs : dans les reçus de `receipts/`, la [passation](../../PASSATION.md) et
  l'[audit final](../../docs/AUDIT_FINAL_V11.md).
- Une note ne prouve rien par elle-même, et aucune ne revendique de statut exact.

**Revue avant versement.** Une relecture intégrale des 60 fichiers a précédé leur versement :
- ni donnée personnelle, ni secret, ni coordonnée LiDAR réelle ;
- toutes les coordonnées présentes sont des cas de test synthétiques ;
- aucun lien Markdown cassé dans l'arborescence.

Les fichiers sont versés tels quels, sans réécriture.

## Contenu

| Dossier | Origine | Contenu |
|---|---|---|
| `conception/PISTES_DE_RUPTURE.md` | workflow de conception du moteur, 2 octobre | pistes de rupture de la tour et du générateur, avec mesures locales de l'époque |
| `audit_transpositions/` | audit des transpositions v2–v10 vers la v11, 4 octobre | cartes, fouilles par version, vérifications, plans (dont `PLAN_VITESSE_100MS.md`), critique de complétude, scripts de calcul et leurs sorties |
| `gpu_optim/` | carte en six angles et plan GPU, 6 octobre | cartes du catalogue, des forêts, de l'aval, de l'infrastructure, du profil des étages et du GPU existant ; brouillon `PLAN_GPU.md`, critique et plan final |
| `polyedres_reconnaissables/SYNTHESE.md` | workflow « hiérarchie reconnaissable », 6 octobre | synthèse sur les polyèdres où roue, vélo et piéton se reconnaissent |

## Points dépassés depuis

**Plan GPU** (`gpu_optim/PLAN_GPU_FINAL.md`).
- Il recommandait N1 (rang 1), L4 (rang 3) et la feuille coopérative. Les trois ont été rejetés sur G4 et retirés :
  `c1675e4c9` (reçu `developpement_20261006/n1_ab`), `830473218` (`l4_recouvrement`) et `d4228f5e5` (`coop1`–`coop3`).
- Sa décision n° 2 est tranchée : u21 reste le profil par défaut.
- Une copie en est déjà épinglée dans `receipts/audit_plan_gpu_20261006/PLAN_GPU_FINAL.snapshot.md`.

**Brouillon `gpu_optim/PLAN_GPU.md`.** Il est remplacé par le plan final, sans le dire lui-même. La critique
(`gpu_optim/CRITIQUE_PLAN_GPU.md`, § 1.2 et § 2.4) y relève un double compte dans le budget du domaine et un gain
SHA-NI surestimé.

**`conception/PISTES_DE_RUPTURE.md`.** Il annonce « 100 ms atteignable en CPU seul à K = 5 (65 à 91 ms) ». La
mesure l'a démenti : au gel, K5 CPU vaut 255 à 314 ms à chaud.

**Le « trou » de `polyedres_reconnaissables/SYNTHESE.md`.** Le trou dans la mosaïque d'ordre 2 de 08/000100, dont la
reproduction minimale « reste à trouver », a été attribué le 7 octobre à qhull dans le prototype Python. Il est
localisé à deux cellules et reproduit sur 13 sites (`audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md`,
`df304ed9e`). Le `REPONSE_AUDIT.md` « à déposer » ne l'a jamais été.

**`audit_transpositions/AUDIT_TRANSPOSITIONS_V11.md`.** Plusieurs « États » y sont périmés selon
`audit_transpositions/CRITIQUE_COMPLETUDE.md` (§ 5). Les deux fichiers se lisent ensemble.

## Erreurs connues

- **`audit_transpositions/cartes/CARTE_V11.md`, l. 116–117 et 349.**
  - La « meilleure prise, toutes v11 confondues : 320,4 ms » est fausse : la session `claudeab4` contient une prise à
    305,3 ms (ng01). `CRITIQUE_COMPLETUDE.md` (l. 216) le signale déjà.
  - Le « 327,7 ms » de `plans/PLAN_VITESSE_100MS.md` (l. 48 et 470) ne vaut que pour la session `claudeab7`.
- **`audit_transpositions/fouille/v10_moteur.md` (l. 327–328, 409) et `audit_transpositions/verif/v10_moteur.md`
  (l. 241–242).**
  - Les débits de la sonde S7 de la v10 (2 579 et 1 111 G op/s, rapport ×2,3) y sont donnés comme mesurés.
  - Ils sont invalidés par `morsehgp3D_v10/receipts/ERRATA.md` (l. 15 ; débordement d'entiers signés).
  - La correction figure dans `AUDIT_TRANSPOSITIONS_V11.md` (l. 343–345) et dans `PLAN_VITESSE_100MS.md`
    (l. 535–537).
- **`audit_transpositions/verif/v8_annexes/sim_radix.py`, l. 3.** La ligne d'usage porte le nom
  `v8_census_bounds_sim.py`.

## Limites

**Références vers du matériel non copié.** Certaines notes renvoient, en texte simple, à des fichiers absents d'ici :
- sorties structurées de workflow ;
- dossiers `harnais/`, `approche_*/`, `critique/` et `lecture_*/` de la synthèse des polyèdres, et ses images ;
- `CONCEPTION_TOUR.md`, `CONCEPTION_GENERATEUR.md` et `preuves_pistes_de_rupture/` ;
- l'audit de la v10, versé ailleurs sous la forme de [AUDIT_V10_SYNTHESE.md](../../docs/AUDIT_V10_SYNTHESE.md).

**Les images ne sont pas versées.** Ce sont des rendus de trames réelles.

**Les scripts ne se relancent pas tels quels.**
- Ils codent en dur des chemins de travail (`build/v11-claude-20261003`, `build/v11-persist/`, `/workspaces/...`).
- Ils lisent des données hors dépôt, et certains des trames réelles à l'exécution (sans en contenir ni en imprimer de
  coordonnées).
- `audit_transpositions/critique/calculs_critique.py` lit deux sessions G4 jamais versées (`claudecat1`,
  `claudeab8`). Les chiffres de la section P3 de `CRITIQUE_COMPLETUDE.md` ne se reproduisent donc pas depuis le
  dépôt.

**Contexte de session.** Les sorties `.out` ne contiennent que des comptes, plus des lignes `date` et `uptime`. Les
chemins absolus cités désignent des répertoires de ce codespace.

## Intégrité

`SHA256SUMS` couvre tous les fichiers de ce dossier, sauf lui-même :

```sh
cd morsehgp3D_v11/receipts/notes_hors_depot_20261007 && sha256sum -c SHA256SUMS
```
