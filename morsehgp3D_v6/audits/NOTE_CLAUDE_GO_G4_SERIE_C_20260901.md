# NOTE_CLAUDE — demande de GO : session G4 série C + matrice CPU décisionnelle

> **SUPERSÉDÉE PARTIELLEMENT (§ 5.13, `18b28700`)** : la section « Profil de
> session proposé » ci-dessous est remplacée par le § 5.12 de l'auditeur et
> par `gcp-migration/profils/g4_serie_c_v1.env` (v3) — l'inventaire est de
> **16** portes gpu (jamais 17), le pilote fait échauffement + **4**
> répétitions ABBA (jamais 3), le point **200k est différé**, et la matrice
> est la liste **pré-enregistrée de 16 points** (jamais un produit
> cartésien), séquence `aller retour rotation8`. L'état au pin et les
> questions (a)-(c) restent le témoignage historique de la demande.

Date : 1er septembre 2026. La suite GPU est PRÊTE au sens local du terme et
l'exploitant a donné son feu vert explicite pour la session de mesure dès ce
stade (« une fois que la suite GPU est prête, mesure sur GCP G4 ! tu as mon
feu vert »). Conformément au protocole (votre § 6 : nouveau pin + GO frais),
cette note demande votre GO et vos verrous sur le profil de session.

Cadre : `phase=exploration_v6_hors_registre`, `public_status=not_claimed`.
GCP non utilisé par cette note.

## État au pin `cd606257` (+ reçu matrice `62cd2e28`)

- **113/113 portes `gate`** en local, dont les 21 de la série C : wire
  (aller-retour bit-exact, six candidats bornés, digest gravé
  `3402912149ee…`, trois refus hors-domaine, 2 mutants), census stub
  (bit-identité boule à boule sur candidats réels, 7 dents à motif
  sélectif), pilote stub (objet identique bout en bout 2 familles × 2
  tailles, transactionnalité canarisée, cas vide publié, multi-lots
  `--lot=17` + mutant base-reset).
- Sous `MHGP6_ENABLE_CUDA` (jamais construites ici) : **16 portes** — témoin
  ×4, census device ×8 (relecture intégrale des sept tableaux au premier
  upload, sentinelles téléversées, arch compilée signée), pilote ×4
  (`mhgp6_cuda` : parité de TOUS les digests, coûts séparés, `--min-lots`,
  device observé signé). [Corrigé après votre § 5.12 : la première version
  de cette note en annonçait 17 à tort.]
- Fermetures de vos sept contre-lectures (27eb5026 → bc5812dc) TOUTES
  intégrées : votre table « restent à fermer » (golden digest, dents
  sélectives, cas vide/multi-lots, borne d'allocation, readback intégral,
  cohérence docs/dataflow) est, à notre lecture, soldée au pin — à vérifier.
- Matrice locale (directionnelle, cellules contaminées gravées) :
  `join=1` dégrade le mur partout ; B isolé dominé par
  `materialisation_tri_copie` (14,9/39,7 s), `unite` 1,7 s → branche
  pré-enregistrée **CompactDelta** ; digest forêt 16,2 s cumulées à
  isoler. La décision attend la matrice G4 propre.

## Profil de session proposé (une session, ~4-6 h, vos verrous sollicités)

Sur la VM g4-standard-48 (protocole gardé inchangé : pin → préflight →
selftests à la main → session → validateur → arrêt certifié TERMINATED) :

1. **Reçu device** : build `MHGP6_ENABLE_CUDA` + label `gpu` complet
   (17 portes) — le premier nvcc/device jamais exercé ; architecture
   compilée + device observé au reçu.
2. **Mesure série C** : `mhgp6_cuda` sur uniform/terrain/eight_clusters/
   scanline_single_pass × n=50000 × `--repeat=3` (+ un point n=200000),
   parité exigée à chaque run, coûts séparés — LE reçu de gain qui
   conditionne les séries F et L.
3. **Matrice CPU décisionnelle** : T ∈ {16,24,32,48} × inflight {1,2,4} ×
   join {0,1} sur uniform 16000 + le point de contrat 50000 (t48),
   avec/sans `--digest`, murs au Release NON instrumenté, attribution
   `mhgp6_profile` sur {t16, t48} × {j0, j1} — l'arbre § 5.10 tranche
   ensuite CompactDelta / budget de workers / layout.

Questions : (a) étendre le profil de campagne épinglé existant
(`v6_campaign_remote.sh`, phases conf→…→gpu) ou profil dédié
`g4_serie_c_v1` ? (b) le point 200k du pilote vous convient-il (VRAM très
large, mais mur CPU ~6 min/run) ? (c) exigences de reçu supplémentaires au-
delà de : arch+device signés, relecture d'index, parité par run, résumés
après contrôle d'instrumentation ?
