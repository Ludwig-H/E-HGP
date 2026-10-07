# Reçu : le complexe alpha d'ordre k et la robustesse des niveaux d'un polyèdre

7 octobre 2026, 02 h 35 UTC (Claude, développeur ; heure lue par `date -u`). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. GCP non utilisé.

Ce reçu fige les sorties du workflow « alpha-ordre-k-robuste-v2 » (`wf_cd387a02-4b0`). Il a tourné du 6 octobre
22 h 14 au 7 octobre 01 h 40 UTC, avec 12 agents :

- un oracle exact ;
- quatre notes théoriques ;
- quatre expériences ;
- deux critiques adverses ;
- une synthèse.

Il fait suite aux deux notes de l'auditeur ([objet](../audit_hartigan_delaunay_20261006/README.md), `d2be6bdc7` ;
[robustesse](../audit_hartigan_robustesse_20261006/README.md), `28d70f8ab`) et à la
[réponse du développeur](../../audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_20261006.md) (`ee2df0362`).

**Statut.** Exploration hors registre ; rien n'est qualifié. Ce sont des prototypes Python exécutés en aval de la
tour. L'oracle est borné : n au plus 60, k au plus 8. Les statuts des énoncés suivent la synthèse : [P] prouvé,
[V] vérifié borné, [M] mesuré, [E] extrapolé, [J] jugement visuel, [C] conjecture, [R] réfuté. Trois réfutations
sont gravées au registre au commit `1fbeea5b8` : fixture
[`polyhedron_order_k_counterexamples.json`](../../../tests/fixtures/regressions/polyhedron_order_k_counterexamples.json),
vérificateur `tools/check_polyhedron_order_k_counterexamples.py` et tests à altérations.

## Contenu

| Fichier | Rôle |
| --- | --- |
| [`SYNTHESE.md`](SYNTHESE.md) | la synthèse complète : objet, représentant recommandé, énoncés et verdicts, robustesse, mesures, coûts, questions |
| [`REPONSE_COURTE.md`](REPONSE_COURTE.md) | la version courte pour l'utilisateur |
| [`oracle/oracle_ak.py`](oracle/oracle_ak.py), [`oracle/README.md`](oracle/README.md) | oracle exact borné de A_k(r) (Python standard, fractions ; numpy facultatif, accélérateur vérifié) et son mode d'emploi |
| `notes/theorie_{robustesse,reduction,alternatives,niveaux}.md` | les quatre notes théoriques, avec leurs énoncés et leurs preuves |
| `critiques/critique_{theorie,experiences}.md` | les deux critiques adverses : verdict par énoncé, contre-exemples, rejeux |
| `experiences/{rendu,reduction,echelle}.md`, `experiences/robustesse_tables.md` | comptes rendus des quatre expériences |
| `png/` | neuf planches, toutes synthétiques ou exactes (liste ci-dessous) |
| [`SHA256SUMS`](SHA256SUMS) | empreintes ; contrôle par `sha256sum -c SHA256SUMS` depuis ce dossier |

Les fichiers copiés gardent leurs liens relatifs vers l'arborescence d'origine,
`build/v11-persist/polyedres_ordre_k/`. Ces liens sont morts ici, car cette arborescence n'est pas versionnée. Les
planches principales sont copiées sous `png/`, renommées :

| Planche | Fichier d'origine |
| --- | --- |
| `chaine_velo_roue_avant_k5.png` | `experience_rendu/png/synth_velo_05m_k5_chaine_vélo_roue_avant.png` (chaîne roue → vélo) |
| `chaine_pieton_jambe_gauche_k5.png` | `experience_rendu/png/synth_pieton_05m_k5_chaine_piéton_jambe_gauche.png` |
| `candidats_velo_05m.png` | `experience_rendu/png/candidats_synth_velo_05m_objet0.png` (tous les rendus au nœud « vélo ») |
| `liens_velo_05m_k5_k2.png` | `experience_rendu/png/synth_velo_05m_liens_k5_k2.png` (liens π0 entre ordres) |
| `six_points_k2.png` | `experience_rendu/png/six_points_k2.png` (exemple du § 6.1 de la thèse) |
| `anneau_10m_k2_niveaux.png` | `experience_rendu/png/synth_anneau_perce_10m_k2_noeud120_niveaux.png` (niveaux d'un nœud) |
| `reduction_velo_10m_k5.png` | `experience_reduction/planches/planche_synth_velo_10m_k5_objet0.png` (A, L réduit, ombre) |
| `robustesse_anneau_05m_k2_sigma10.png` | `experience_robustesse/png/planche_synth_anneau_perce_05m_k2_sigma10.png` |
| `robustesse_triangle_auditeur.png` | `experience_robustesse/png/planche_contre_epreuve_triangle.png` |

## Ce que ce reçu ne contient pas

- Les scripts et résultats bruts des agents. Ils restent dans `build/v11-persist/polyedres_ordre_k/`.
- `oracle/fixtures_oracle.py` et `oracle/trou_minimal.py`. La reproduction minimale du trou du prototype (13 sites)
  contient les coordonnées relatives de cinq retours réels de la trame 08/000100 : c'est une donnée dérivée de
  SemanticKITTI, qui reste dans `build/`.
- Les rendus de la découpe réelle et les articles téléchargés.

Le trou du prototype `mosaique.py` est un défaut d'implémentation, pas une contradiction mathématique.
- Il vient de l'échelle des relevés flottants dans qhull : les propositions refusées n'y sont jamais re-proposées.
- Il est localisé à 2 cellules.
- Le contrôle strict le détecte (appariement des 2-faces, Euler) ; le certificat de volume ne le voit pas.

## Rejeu

L'oracle se rejoue seul, en Python standard : voir [`oracle/README.md`](oracle/README.md), § 2. Les trois
réfutations gravées se rejouent depuis la racine du dépôt :

```bash
python3 tools/check_polyhedron_order_k_counterexamples.py
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests/oracle -p 'test_polyhedron_order_k_counterexamples.py'
```

Moteur de référence des expériences : `mhgp11` à `07428324e`. Aucune session G4, aucun natif.
