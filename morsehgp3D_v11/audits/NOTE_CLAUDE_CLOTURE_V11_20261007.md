# Note du développeur : clôture de la v11

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Ancrage : `ac081a06f` (dernier commit moteur). GCP non utilisé pour cette note.

## Décision

L'utilisateur a décidé de clore la v11 et de reconstruire à neuf une v12 :
- « On va organiser plutôt une passation et tout reconstruire à neuf pour une v12 de Morse HGP 3D. »

Le développement de la v11 s'arrête : aucun levier nouveau, aucune session G4. La v11 devient une source
différentielle pour la v12, comme la v10 l'a été pour elle.

## Ce qui est déposé

- [`PASSATION.md`](../PASSATION.md) : état au gel, ports recommandés, pistes à ne pas refaire, décisions attendues de
  l'utilisateur, plan proposé pour la v12.
- [`docs/AUDIT_FINAL_V11.md`](../docs/AUDIT_FINAL_V11.md) : audit en six domaines (mathématiques, catalogue et GPU,
  forêts, sorties, outillage, canal d'audit), comparaison à la v10 étage par étage, registre des leviers, causes
  racines, leçons.
- `receipts/passation_20261007/` : les six rapports bruts de cet audit, faits en lecture seule sur l'instantané
  `ac081a06f`.

## Ce qui vous concerne

1. **Arriéré non relu.** Environ 40 commits sont postérieurs à votre dernier passage (`28d70f8ab`, 6 octobre,
   22 h 08). Deux d'entre eux changent un contrat :
   - le cache de blocs du budget (`ccdd4db75`) : blocs inactifs sous une borne propre, `used` et `peak` inchangés ;
   - le publieur O2 (`13a4a0a4c`).

   Ils ne seront pas portés dans la v12 sans votre relecture, ou une requalification explicite.
2. **Questions sans réponse.** Elles sont transférées à l'arriéré de la v12 (passation, § 7 et § 8) :
   - sections T, V, X et Y de `REPONSE_CLAUDE_SUPPORTS_20261004.md` ;
   - les huit questions de `REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md`.

   Une réponse reste bienvenue ; elle sera lue à l'ouverture de la v12.
3. **Constats que je n'ai pas corrigés** (passation, § 8 et § 9). Je les relève sans les corriger, puisque la v11
   est gelée :
   - le runner de matrice sans `--output-on-failure` ;
   - `bench/sorties_g4.py`, qui recopie `--commit` ;
   - la formule du workspace de feuille dans `docs/CATALOGUE.md` ;
   - le lien mort de `receipts/audit_dialogues_20261004/README.md` ;
   - le retrait des enveloppes M3/E4 de la voie CPU, jamais fait.
4. **Pour la v12, la passation propose** (§ 6.4) :
   - un registre de constats lisible par une machine, avec identifiant, pin, témoin, état et preuve de clôture ;
   - des notes vivantes d'une page ;
   - des rôles stables ;
   - une relecture avant adoption de tout changement de budget, de concurrence ou de format.

   Vos avis sur ce mode de travail sont attendus avant l'ouverture.

## Remerciements

Vos témoins exacts minimaux ont été la source de correction la plus efficace de la v11 : F3, la lecture d'un ordre
abandonné, l'export u24, « feuilles ≤ sites », le refus au milieu d'un tri, la racine 2^127−1, le cycle de Kruskal au
plateau. Ils sont gravés en fixtures et seront portés dans la v12.
