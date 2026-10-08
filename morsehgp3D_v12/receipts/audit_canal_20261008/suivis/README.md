# Compactage des suivis — 8 octobre 2026

Capture de la proposition documentaire puis de son application au registre par l’auditeur root ; aucun état ni résultat requalifié.
Base `4cbcd38521fad5cf61e4009b7011da20bdc33755` ; SHA-256 du registre `204b38f52953ff2e8b58c0507accaa80b7e75c8f807bd8758f63060a92a65c00`.
`proposition.json` garde les huit cellules avant/après, y compris les espaces extérieurs ; les neuf premières colonnes restent identiques.
Les liens dans les chaînes JSON gardent leur base d’origine `morsehgp3D_v12/audits/` : ce JSON n’est pas un document Markdown déplacé.
Les liens lisibles ci-dessous ont été recalculés depuis ce reçu.

| Constat | Avant, octets | Après | Gain | Liens directs gardés / déplacés ici |
| --- | ---: | ---: | ---: | ---: |
| [CST-0018](#cst-0018) | 1618 | 803 | 815 | 3 / 7 |
| [CST-0021](#cst-0021) | 712 | 388 | 324 | 1 / 3 |
| [CST-0024](#cst-0024) | 676 | 445 | 231 | 1 / 1 |
| [CST-0104](#cst-0104) | 672 | 473 | 199 | 1 / 3 |
| [CST-0113](#cst-0113) | 984 | 531 | 453 | 2 / 4 |
| [CST-0211](#cst-0211) | 840 | 410 | 430 | 1 / 4 |
| [CST-0212](#cst-0212) | 687 | 464 | 223 | 1 / 3 |
| [CST-0218](#cst-0218) | 873 | 454 | 419 | 1 / 1 |

Registre : **53698 → 50604 octets** ; canal au pin : **64454 → 61360 octets** ; gain **3094 octets** (3.02 Kio). Les 75 lignes, leur ordre, les colonnes 1–9 et tous les liens de preuve antérieurs sont conservés (directement ou dans ce reçu).

## Relecture et patch

```sh
python morsehgp3D_v12/receipts/audit_canal_20261008/suivis/verifier.py --repo /workspaces/E-HGP --check
python morsehgp3D_v12/receipts/audit_canal_20261008/suivis/verifier.py --repo /workspaces/E-HGP --patch /tmp/compactage_suivis.patch
```

Patch sans contexte pour préserver les ajouts voisins : après revue des huit cellules, `git apply --check --unidiff-zero /tmp/compactage_suivis.patch`, puis la même commande sans `--check`.

Rejeu normal/`-O` identique ; application et `check_constats.py --hygiene` réussis sur une copie temporaire du canal root (75 constats, 61 365 octets avec ses cinq octets de changements indépendants). La préparation ne modifiait aucun fichier vivant. Root a ensuite appliqué les huit suivis et rejoué le contrôle : 61 365 octets ; ses mises à jour indépendantes 0105/0107 sont préservées. Voir `verifications.json`.

Le lecteur charge le registre épinglé avec `git show`, refuse un ancien texte ou un hash divergent, vérifie les colonnes et les liens, puis peut écrire un patch. Il ne modifie jamais le registre. Avant application sur une branche avancée, le responsable doit comparer les huit cellules vivantes et conserver tout ajout intervenu depuis le pin ; ne pas remplacer aveuglément une ligne plus récente. Le contrôle `tools/check_constats.py` reste à exécuter sur la copie intégrée.

## Conservation des preuves

Les commentaires ci-dessous expliquent la contraction. Le texte antérieur intégral est dans `proposition.json` ; chaque liste conserve tous ses liens, y compris ceux restés directement au registre.

## CST-0018

Les corrections de juges sont acquises dans leurs portées respectives ; les quatre résidus du plan D6, son refus du schéma Gc, le défaut CLI huit et l’absence de collecte C restent explicites. Les livraisons antérieures, errata et bilans primaires demeurent liés ici.

- [Historique et livraison `28cf75cd1`](../../audit_reponses_20261007/livraison_28cf75/README.md)
- [Livraison `af0c2ecd7`](../../audit_reponses_20261007/livraison_juges/README.md)
- [corrigée dans `781fbe8d1`](../../audit_reponses_20261008/cuda_juge_livraison/README.md)
- [session I réelle contre-vérifiée](../../audit_reponses_20261008/session_i_catalogue/README.md)
- [D6](../../audit_reponses_20261008/d6_compatibilite/README.md)
- [MES-P](../../audit_mes_p_livraison_20261008/README.md)
- [T2-c intégré au prototype](../../audit_reponses_20261007/t2c_pilote_integration/README.md)
- [livraison Gc `10050a96e`](../../audit_reponses_20261008/gc_livraison/README.md)
- [session J](../../audit_reponses_20261008/session_j_gc/README.md)
- [Essai Gc](../../audit_reponses_20261008/gc_pilote_essai/README.md)

## CST-0021

L’obstacle est levé pour D seulement. Les lacunes historiques/B et l’absence de falsification démontrée restent explicites ; la correction locale et les comptes antérieurs sont archivés.

- [suivi au pin `95247cf4b`](../../audit_socle_microbancs_20261007/preuves/README.md)
- [session B relue](../../audit_b_m5_20261007/campagne_b/README.md)
- [correction locale M3/M4](../../audit_cd_corrections_20261007/m34/README.md)
- [D](../../audit_cd_corrections_20261007/campagnes/README.md)

## CST-0024

La cause, la correction mono, les résultats avant/après et la limite de contre-lecture restent au registre. La localisation TSan et le développement complet restent dans le texte antérieur et les deux preuves.

- [reçu](../../developpement_20261007/cst_0024_course_v10.md)
- [contre-audit `274592a30`](../../audit_t2_20261007/corrections/README.md)

## CST-0104

Les portes natives sont acquises ; le mutant de date reste proposé et non joué. Identités, inégalités exactes et absence de transfert T/M/V/R sont conservées ; les deux adaptations de mutant restent liées.

- [T2 Python](../../audit_t2_20261007/mathematiques/README.md)
- [Portes natives livrées `99fa83246`](../../audit_reponses_20261008/d2_memo/README.md)
- [Mutant de date proposé](../../audit_reponses_20261008/d2_memo_mutant/README.md)
- [Adaptation du mutant à Gc](../../audit_reponses_20261008/d2_memo_mutant_gc/README.md)

## CST-0113

Le lecteur et ses témoins sont acquis, les quatre portées produit restent à qualifier. Les corrections successives de contrat/format sont réunies ici ; le nouveau témoin TMVR reste directement lié au registre.

- [complément T1 `4147c5460`](../../audit_session_t1_20261007/contrat/README.md)
- [M5 `e30000dec`](../../audit_b_m5_20261007/mathematiques/README.md)
- [réponse T1 `07ee13ef6`](../../audit_cd_corrections_20261007/contrat/README.md)
- [format `f4a11f49e`](../../audit_u32_20261007/format/README.md)
- [contre-audit `274592a30`](../../audit_t2_20261007/transition/README.md)
- [Témoin TMVR de translation](../../audit_reponses_20261008/translation_tmvr/README.md)

## CST-0211

Le contrat et le correctif prototype R sont distingués de la livraison et du budget fini encore à qualifier. L’annonce ancienne des offsets absents est conservée ici sans masquer leur correction.

- [contre-lecture du socle](../../audit_socle_microbancs_20261007/session/REPORT.md)
- [réponse T1 `07ee13ef6`](../../audit_cd_corrections_20261007/contrat/README.md)
- [R non publié : offsets CSR absents de la seconde admission](../../audit_registre_branches_20261007/README.md)
- [correction prototype R](../../audit_tmvr_admission_20261008/README.md)
- [repo5 u21](../../audit_tmv_repo5_u21_20261008/README.md)

## CST-0212

La clôture concerne M4 ; le nouveau codage et sa qualification produit restent distincts. Les portes locales TMVR ne transfèrent pas leur qualification avant intégration.

- [contre-lecture M4](../../audit_socle_microbancs_20261007/tour/README.md)
- [T2 `274592a30`](../../audit_t2_20261007/mathematiques/README.md)
- [contrat `76adb8fa9`](../../audit_juges_emst_20261007/t2_corrections/README.md)
- [portes TMVR locales et patch vérifiés](../../audit_tmv_profils_20261007/README.md)

## CST-0218

Clôture de l’outil et de la régénération tracée, sans rehash des données ni recalcul géométrique indépendant. L’ancienne demande de régénération est archivée avec les comptes détaillés ; elle ne reste pas présentée comme travail ouvert.

- [contre-audit `1f7642e10`](../../audit_juges_emst_20261007/donnees/README.md)
- [Contre-lecture `f601b36ac`](../../audit_reprise_20261007/donnees/README.md)
