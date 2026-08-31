# Note Claude — bilan du 31 août au soir : l'arc E6 est bouclé honnêtement, le protocole GCP attend votre GO

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

## L'arc E6 → E3/G16, de la sonde au verdict hors échantillon

1. **Sonde** (`sonde_e6_20260831`) : les seeds tuées par cœur sur les
   ancres lourdes sont dans des cellules presque mortes (60–77 % à
   min ≥ h/2) ; le veto `near_m` excluait la grille des ancres riches.
2. **Mécanisme** (`e6_grille_appariee_20260831`) : grille paramétrique
   `CellGridT<G>`, bras séparés `--e3-g16=...` (votre factorisation :
   g8_lourdes / g16_politique / g16_nearm / g16_ratio / g16_leve, repli G8,
   scan de politique compté, cellules consultées par résolution), objet
   BIT-IDENTIQUE sur les CINQ bras (porte avec planchers de kills
   additionnels et d'économie stricte de W1) ; W_sweep1 −33/−40 % à 32000
   g5 ; nommage corrigé (E3/G16, pas le Tier R).
3. **Oracle** (`tests/cell_grid_oracle.cpp`) : porté de la v5 et paramétré
   G=8 ET G=16 — 4,8 M + 19,2 M paires cellule-à-cellule contre
   l'évaluation directe i128, localisateur rationnel 256 bits, F9–F11,
   extrêmes u16, synthétiques 2^100 ; 0 désaccord ; ns/eps0/h−1 tués. Les
   références fausses du code à cet oracle sont désormais vraies.
4. **Verdict hors échantillon** (`campagne_confirmation_20260831`,
   protocole ENTIER antérieur aux données, binaire construit depuis
   l'archive du pin, jugé par ses copies archivées) : **E6_active=non** —
   médianes pas2 toutes < 2 ; la queue est INTERMITTENTE (excursions à
   6,41 ; disparition à −0,68), pas une loi stable. La porte préenregistrée
   a refusé de confirmer l'hypothèse ajustée dans l'échantillon.

Conséquence assumée : le moteur v6 COURANT est sous-quadratique en médiane
inter-graines sur l'échantillon frais ; E3/G16 reste opt-in comme réducteur
de variance mesuré. Votre lecture décidera de la suite (bras à comparer,
seuil 1024, éventuel balayage préenregistré).

## Vos quatre défauts GCP du quatrième tour : exécutés

Canon parsé axe par axe avec identité `PROFIL_NOM` (le mutant « decision_v1
réduit » rend `verifie_non_decisionnel cause=canon non decisionnel`, jamais
`decision_complete`) ; validateur idempotent (résumés hors de `out/`,
second passage vert), cas remote/scp/profil sur snapshots propres à cause
exacte ; reçu en `mktemp -d` unique, manifeste excluant `SHA256SUMS*`,
`sha256sum -c` vérifié AVANT publication `mv -Tn`, références mortes
purgées ; fixture K11 sans doublon ; bundle distant reçu en `.recu` et
promu sous contrôle du boot_id ; data race des callbacks corrigée (mutex +
atomique — l'UB était réel), CHAQUE inflight doit échouer, census ⟹ zéro
`on_forest`, fold-A K2 ⟹ préfixe exact {K1}.

État des suites au HEAD courant : 74/74 portes v6 (oracle G8/G16 et cinq
bras compris) ; selftests GCP verts ; intégration au vrai garde OK ;
81/81 sûreté. Les captures du jour : exploratoire, réplication,
confirmation — chacune au statut que vous avez prescrit.

Reste à vous : le GO statique GCP (rien ne sera lancé avant), et votre
lecture du verdict de confirmation.
