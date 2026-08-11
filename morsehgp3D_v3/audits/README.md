# Audits de MorseHGP3D v3

Ce dossier contient les preuves, questions, mesures et audits propres à
`morsehgp3D_v3`. Un audit daté décrit uniquement son snapshot. Il ne devient
pas une autorité produit et ne doit jamais contredire l'état courant sans être
explicitement classé comme réfutation ou archive.

## État autoritatif de travail

- [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) : verdict consolidé du
  snapshot `ab5a3c8` et du chantier concurrent identifié par SHA-256, limites
  exactes du contrat, verrous 50 k et ordre des prochaines portes.
- [`REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md`](REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md) : réponse mathématique canonique à Claude pour le pont
  de haut rang, le fast path ex æquo et la source q4. Elle remplace les réponses
  fragmentaires sur ces trois sujets.
- [`NOTE_CLAUDE_LIVRAISON_PORTES_CPU_20260811.md`](NOTE_CLAUDE_LIVRAISON_PORTES_CPU_20260811.md) : provenance de la dernière livraison CPU committée, de ses
  campagnes et de ses masses.
- [`NOTE_CLAUDE_FAST_EXAEQUO_RECU_ET_PRUNE_20260811.md`](NOTE_CLAUDE_FAST_EXAEQUO_RECU_ET_PRUNE_20260811.md) : réception du fast ex æquo selon la réponse pont
  (neuvième forme, garde stricte pré-lot, fenêtre `q<=k+1`, mutants ciblés,
  plancher anti-vacuité, fixture amputée à raison exacte), masques remesurés,
  prune convexe d'axe (R_4 dix-huit fois plus petit à n=2400, résidu ~n^1,9)
  et la question du théorème 2 multi-lot ou de la route EMST pour k=1.

L'autorité du théorème d'inertie reste
[`INCIDENCES_SILENCIEUSES_GAMMA.md`](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md), et le statut public reste régi par
[`SPECIFICATION_MORSEHGP3D.md`](../../docs/SPECIFICATION_MORSEHGP3D.md) et
[`STATUT_PREUVES_ET_HEURISTIQUES.md`](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md).

## Contrat de cohérence

Les documents courants emploient les décisions suivantes :

- `q_min` décrit la provenance Morse; `q_cert` est la plus grande arité d'un
  support propre positif effectivement certifié et sert seulement à prouver
  l'inertie H0. Une absence de grand support n'est jamais déduite d'un support
  canonique.
- Le pont `p+q_cert>=K+2` concerne le quotient horizontal normalisé
  `H0 + union des PointId`. Il ne supprime ni le transcript Gamma facetté v2,
  ni ses incidences, ni les obligations verticales.
- Le fast principal dans un lot multiple exige `q<=k+1`, une capability
  `CarrierClosure` validée et des lookups stricts pré-lot. `q>k+1` reste au
  fallback tant qu'une réduction distincte n'est pas reçue.
- `prefix-all` est exact relativement à la table reçue; il ne prouve jamais la
  complétude géométrique de cette table.
- Une mesure G4 sur le join d'un catalogue préconstruit ne qualifie pas le
  pipeline 50 k de bout en bout.

## Archives

Les autres fichiers de ce dossier sont des snapshots historiques conservés
pour la traçabilité des preuves, contre-exemples, campagnes et décisions. Leurs
phrases au présent ne décrivent pas l'implémentation courante. En cas de
divergence, `AUDIT_ETAT_COURANT.md` et les autorités mathématiques ci-dessus
prévalent; toute contradiction nouvelle doit devenir une fixture et une mise à
jour explicite de l'état courant.

Les deux fichiers `morsehgp3D_v3/AUDIT_PROPOSITION*.md` sont uniquement des
pointeurs vers les audits historiques homonymes de ce dossier.

GCP non utilisé pour la consolidation du 11 août 2026.
