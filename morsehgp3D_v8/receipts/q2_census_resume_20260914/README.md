# Reprise du census q2 : preuves locales closes

14 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Cette capture qualifie un composant reprenable, pas son raccord au
répartiteur, le Pool asynchrone, q3/q4, FULL ou G4. Aucun GCP utilisé.

## Qualifications

- [Release](qualification_0ir4r0bq/COMPLETION.json) : 62 CTests PASS,
  puis gate de reprise et contre-fixtures q3/q4, trois commandes closes.
- [Clang ASan/UBSan](qualification_g4tpzakt/COMPLETION.json) : mêmes
  62 CTests et trois commandes PASS ; détection des fuites active.
- [Clang ThreadSanitizer](tsan_dy2fgg87/completion.json) : gate de reprise
  PASS, y compris transfert et chevauchement volontaire d'appels.
- [Analyse normale et −O](analysis_c656a1em/COMPLETION.json) : lecteurs
  et bilans identiques ; [synthèse machine](analysis_c656a1em/SUMMARY.json).

La gate propre compare 195 références et 768 reprises, avec un oracle
indépendant de 590 paires / 37 188 visites de sites. Elle compte
254 668 contrôles, 24 644 avances, 1 470 supports, coquille maximale30.
Pauses réellement exercées : 11 049 après crédit, 2 184 sur enfant après
division créditée, 76 immédiatement après changement de phase, 1 694 sur
enfant en phase différée, 452 au stade émission. **Zéro plage multiple
partiellement émise** : ce cas reste distinct d'une simple pause Emit.
Les tests incluent deux contextes entrelacés, perte du propriétaire externe,
transfert entre deux threads, exception, réentrance et chevauchement refusé.

Les lecteurs de lignes passent chacun 24 petites captures réelles,
552 mutations rejetées et sept CLI invalides, en mode normal et −O.
Six contre-fixtures rationnelles permanentes réfutent les filtrages
q2→q3/q4 et q3 accepté→seed q4 ; elles ne qualifient pas un moteur q3/q4.

Les [préflights](PREFLIGHT.md) restent visibles : configuration sans chemin
Boost et compilations commencées avant correction du constructeur privé.
Un premier lecteur local a aussi été lancé sur une sonde non reconstruite
après ajout des champs frère/ordre : `KeyError: sibling_work`, corrigé par
reconstruction avant gel. La contrelecture du runner a fait renforcer ses
ensembles de champs et la validation avant publication du statut PASS.
Ces étapes ne sont pas réinterprétées comme des qualifications réussies.

## Deux périmètres de mesure à ne pas confondre

[144 mesures](matrix_7ys75y08/COMPLETION.json) testent **une ancre choisie**
dans le front réel : quatre familles, n=8k/16k/32k, Kmax=5/10, s=8/10/12,
quanta1/256, seed3. Chaque mesure calcule aussi sa référence récursive :
sorties ordonnées complètes et 36 compteurs géométriques/structurels égaux.
L'ancre choisie peut changer quand n ou s change ; on ne déduit aucune
croissance globale de ce sous-échantillon.

Maximum observé : 101 220 transitions pour une ancre, 12 tâches simultanément
pendantes, 6 272 octets de pile réservée et 96 octets de buffers de payload.
Ces maxima mesurés ne bornent ni toutes les coquilles ni le nombre d'ancres.
La mémoire d'index, les métadonnées et le stockage de l'appelant sont séparés.

La [régression du chemin q2 complet](full_q2_regression/COMPLETION.json)
contient 12 mesures : quatre familles, n=8k/16k/32k, K10/s8, quatre workers,
16 jobs/worker, Pool64, ordonnancement historique Coarse. C'est le chemin
existant, **sans continuation**, car le raccord n'est pas encore fait.
Ses sorties et tous les champs discrets, Pool inclus, égalent les 12
configurations correspondantes de la tranche14, consommées en lecture seule.

Les six principaux postes restent sous×3 à chaque doublement. Pour les
visites géométriques du census :

| Régime | 8k | 16k | 32k | Rapports successifs |
| --- | ---: | ---: | ---: | --- |
| Uniforme | 171 895 354 | 413 553 244 | 1 001 201 993 | ×2,406 / ×2,421 |
| Terrain | 20 472 635 | 42 798 408 | 95 128 515 | ×2,091 / ×2,223 |
| Amas | 81 112 664 | 239 954 275 | 648 207 562 | ×2,958 / ×2,701 |
| Rangées | 5 742 485 | 11 886 273 | 24 566 835 | ×2,070 / ×2,067 |

C'est une vérification de non-régression et de croissance sur ces régimes,
pas une preuve sous-quadratique générale et pas un nouveau gain produit.
Les quanta ne changent aucun test géométrique ; ils ajoutent une gestion
linéaire des transitions du parcours existant.

Les mesures ont coexisté avec la qualification sanitizer et d'autres
mesures locales : **aucune conclusion comparative de vitesse** à partir
de leurs chronos. Les affinités sont enregistrées : CPU0 pour la sonde,
0/2/4/6 pour le chemin à quatre workers. La comparaison de temps référence/
reprise a aussi des périmètres et caches différents, détaillés dans la
[note de contrat](../../docs/P0_CENSUS_REPRENABLE_Q2.md). Les valeurs restent
publiées, sans les convertir en gain de performance.

## Reproductibilité et suite

Les manifests épinglent les sources, binaires, caches CMake, commandes et
sorties brutes ; chaque fermeture vérifie les hashes. Les tentatives se
créent dans des dossiers neufs. Les lecteurs normal/−O relisent commandes,
champs, égalités de masses, transitions, couverture et sorties brutes.

Les builds `v8_census_resume_20260914`,
`v8_census_resume_sanitize_20260914` et
`v8_census_resume_tsan_clang_20260914` sont désormais **épinglés**.
Ne pas les reconstruire pour poursuivre. Les scripts de lecture sont :

```bash
python -B morsehgp3D_v8/receipts/q2_census_resume_20260914/analyze.py
python -B -O morsehgp3D_v8/receipts/q2_census_resume_20260914/analyze.py
```

La campagne B 7b86e36b rapporte séparément 258 624 continuations sans
désaccord ; ses deux hashes moteur/en-tête correspondent à nos sources.
Elle reste une preuve indépendante, non additionnée à nos nombres de tests.

Prochain travail : détacher des frères B non visités en tâches indépendantes,
avec leur état propre et partage possédé du plan parental, puis raccorder le
dispatcher. Une migration de la pile entière n'est pas encore une division
du census entre plusieurs workers.
