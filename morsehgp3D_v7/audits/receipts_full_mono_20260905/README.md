# Reçus indépendants : FULL mono publié

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Source constructeur publiée `98bb6578`, header EAGER `e02d163c`. Aucun moteur, build ou benchmark supplémentaire dans cette campagne d’audit.

Le [rapport courant](../MONO_FULL_COURANT.md) réunit les conclusions ; ce dossier conserve leurs preuves séparées.

| Pièce | Autorité |
| --- | --- |
| [Raccord à la publication](publication_source_review.md), [blobs et mappings](publication_source_review.json) | H155 vers `98bb6578` : 153 pins identiques, deux documents actualisés, trois fichiers ajoutés à la couverture ; 95 pièces de reçus identiques aux blobs publiés. |
| [Paquets constructeur](constructor_receipt_review.md), [pins et rejeux](constructor_receipt_review.json) | Micro : treize commandes distinctes ; mono : trois succès relatifs 8k, deux refus à 16k/K9 et 32k/K7. Cinquante et un pins source avant/après, trente-neuf dépendances compilateur. |
| [Juge](judge_review.md), [reçu](judge_review.json), [normal](judge_runs/normal.json), [optimisé](judge_runs/optimized.json) | Dix jugements et deux selftests rejoués ; neuf mutants existants rejetés par mode. Quatre corruptions acceptées par le juge historique, rejetées par nos identités. Les 44 lignes réussies nominales passent. |
| [Modèle mémoire](memory_model_review.md), [sources](memory_model_review.json) | Décomposition exacte des alias EAGER, tailles logiques, partage, destructions et bornes du cache. Lecture statique et preuve, aucune mesure du port paresseux. |
| [Analyse normale](analysis_normal.json), [optimisée](analysis_optimized.json), [entrées scellées](analysis_source_pins.json) | Décompte exact des 46 lignes, 44 réussites ; bornes de clés et diagnostic temporel. Résultats identiques hors indicateur Python optimisé. |

Les bruts constructeur restent dans leurs paquets originaux, liés par les pins ; ils ne sont pas recopiés. Le juge historique a une [capture littérale](judge_runs/judge_at_review.py) pour garder les quatre contre-fixtures reproductibles après sa correction. Une réussite du juge ne transforme pas un refus du moteur en réussite.

## Reproduction bornée

Depuis la racine, en choisissant des fichiers de sortie nouveaux sous ce dossier pour conserver les reçus originaux :

```bash
python3 -B morsehgp3D_v7/audits/full_mono_analysis.py --result morsehgp3D_v7/audits/receipts_full_mono_20260905/replay_analysis.json
python3 -B -O morsehgp3D_v7/audits/full_mono_analysis.py --result morsehgp3D_v7/audits/receipts_full_mono_20260905/replay_analysis_optimized.json
python3 -B morsehgp3D_v7/audits/receipts_full_mono_20260905/judge_runs/replay.py --result morsehgp3D_v7/audits/receipts_full_mono_20260905/replay_judge.json
python3 -B -O morsehgp3D_v7/audits/receipts_full_mono_20260905/judge_runs/replay.py --result morsehgp3D_v7/audits/receipts_full_mono_20260905/replay_judge_optimized.json
```

Les portes utilisent des exceptions explicites, jamais `assert`. La copie e02d de la [campagne producteur](../receipts_full_producer_20260905/README.md) est l’autorité du modèle mémoire ; l’implémentation paresseuse préparée simultanément reste exclue. Les variantes historiques gardent leurs résultats propres. GCP non utilisé.
