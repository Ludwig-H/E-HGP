# Ouverture proposée de la v12 (texte pour `AGENTS.md` et `CLAUDE.md`)

7 octobre 2026. **Appliqué le même jour** (décision D15) : les sections effectivement écrites dans `AGENTS.md`
(« Ouverture v12 ») et `CLAUDE.md` (« Cible de travail : morsehgp3D_v12 ») reprennent ce texte, décisions D1 à D9
remplies ; elles font foi. Ce fichier garde la proposition d'origine.

## 1. Section à ajouter en tête de `AGENTS.md`

```markdown
## Ouverture v12 — [date] (chantier actif)

`morsehgp3D_v12/` remplace `morsehgp3D_v11/` comme chantier actif, sur demande de l'utilisateur du 7 octobre 2026 :
« faire une v12 aussi propre, simple et efficace que possible », feu vert GCP G4. Lire d'abord
`morsehgp3D_v12/README.md`, `docs/DECISIONS.md`, `docs/ARCHITECTURE.md`, `docs/PLAN.md`, puis l'audit géant
`morsehgp3D_v11/docs/AUDIT_GEANT_V11.md`. Cadre à annoncer : `phase=exploration_v12_hors_registre`,
`backend=[cpu_reference ; cuda_g4]`, `objet=full_pi0`, `quantification=quantized_u21_input_only`,
`public_status=not_claimed`.

Même objet que la v11 (tour FULL des ordres 1..K, verticales comprises) ; contrat [décisions D1–D7 : régime, latence ou
cadence, périmètre, K10, GPU, profil, données]. La v11 gelée (`ac081a06f`) et la v10 (`777406b82`, figée `c764e121a`)
sont des sources différentielles : tout port est explicite, épinglé et requalifié (`morsehgp3D_v12/docs/PROVENANCE.md`),
et la conformité se prouve par l'oracle borné et par les empreintes de la v11. Les changements d'algorithme sont
déclarés d'avance (catalogue résident sur le GPU, plus petite boule proposée puis certifiée, forêt sans lots, registre
d'événements dont les sorties sont des vues) et mesurés d'abord par microbancs sur G4. Un seul chemin produit, qui est
le chemin mesuré. Tests lourds sur G4, sessions gardées, une seule VM, arrêt certifié. Pousser sur `main`, sans branche,
un worktree par acteur, index vérifié avant tout `git add`. Aucun octet de données KITTI ni identité de compte dans le
dépôt. Canal d'audit : `morsehgp3D_v12/audits/`. La section v11 ci-dessous devient historique.
```

## 2. Section à substituer à « Cible de travail : morsehgp3D_v11 » dans `CLAUDE.md`

```markdown
## Cible de travail : morsehgp3D_v12 (chantier actif depuis le [date])

`morsehgp3D_v12/` est le chantier actif (`AGENTS.md` § « Ouverture v12 »). Lire `morsehgp3D_v12/README.md`,
`docs/DECISIONS.md`, `docs/ARCHITECTURE.md`, `docs/PLAN.md`, `docs/MESURE.md` et `audits/` avant toute tâche. Cadre à
annoncer :

    phase=exploration_v12_hors_registre
    backend=[cpu_reference ; cuda_g4]
    objet=full_pi0
    quantification=quantized_u21_input_only
    public_status=not_claimed

Objet : la tour HGP FULL (même objet que la v11), sur trames SemanticKITTI sans sol [plage et séquences de D7], grille
1 mm, moteur entier exact, [contrat de D1–D5] ; puis une hiérarchie de points comparée à `sklearn.cluster.HDBSCAN`.
Principes : contrat, oracle et microbancs avant le moteur ; changements d'algorithme déclarés d'avance ; un seul chemin
produit, qui est le chemin mesuré ; différentiel contre les empreintes de la v11 dès la première tranche.
```

## 3. Corrections à apporter en même temps à `CLAUDE.md`

| Lieu | Aujourd'hui | Correction |
| --- | --- | --- |
| section v11 (l. 21–32) | cadre en `quantized_u18_input_only` | la v11 était en u21 ; la section devient « Chantier précédent : morsehgp3D_v11 (2–7 octobre 2026) », close, source différentielle |
| sections v9, v5, v4 | « chantier actif » sous des titres historiques | marquer historiques ; ajouter une ligne pour la v10 (28 septembre – 2 octobre), jamais déclarée |
| puce Zoltan (l. 134) | condensation « à seuil relatif », attribuée au § 9.1 de la thèse | la thèse prescrit un seuil absolu sur la masse $m_\tau$ (p. 97) ; le seuil est une décision ouverte (D9) ; le chaînon manquant n'est plus `ChainResult` (type de la v9) mais un exportateur depuis le registre d'événements (D13) |
| commandes | sections v5, v4, v3 | ajouter les commandes de la v12 à son ouverture ; rappeler que `tools/check_docs.py` n'examine pas les dossiers v11 et v12 |
| règle des tailles (l. 15) | 8 000 / 16 000 / 32 000 seulement | ajouter : toute décision de vitesse se prend d'abord sur des trames LiDAR réelles de plusieurs séquences (consigne du 6 octobre) |
