# Supports et puissances float32 q3/q4

21 septembre 2026. CPU local, exploration hors registre,
`lossless_float32_input_only`, `native_ball_predicates`, `not_claimed`.
GCP non utilisé. Voir le [contrat et les bornes](../../docs/BOULES_FLOAT32_Q3_Q4_20260921.md).
Ce lot ne produit ni WSPD, ni census, ni clés canoniques, ni tour FULL.

Instantané constructeur qualifié : `12d885d8`. L'ajout ultérieur des
clés/événements étend `fixed_signed.hpp` et le pont privé de `Float32Ball` :
les lecteurs **vivants** ci-dessous ne sont donc plus applicables tels
quels aux sources courantes. Captures et builds sont conservés sans
réécriture ; les948contrôles entiers et1636cas de boules sont réexécutés
dans la [qualification commune suivante](../float32_identity_20260921/README.md).

## Qualifications closes

| Capture | Profil | Commandes | Verdict |
|---|---|---:|---|
| `release/ball_04x59id1` | GCC Release | 12 | PASS |
| `sanitize/ball_9ut9u2_k` | Clang ASan/UBSan, fuites activées | 12 | PASS |

Builds désormais épinglés : `build/v8_float32_ball_20260921` et
`build/v8_float32_ball_sanitize_20260921`. Ne pas les écraser.
Qualification autonome, CMake et sources du moteur u16 inchangés ; aucune
exécution de l'ancienne suite CTest n'est revendiquée pour ce nouveau code.

Chaque capture compile trois unités avec leurs dépendances séparées,
lie la sonde et le test d'entier, puis lance ce test et les gates Python
normale/−O. Les mêmes sources sont épinglées avant/après ; dépendances
système recensées et hachées **avant compilation** puis à la fermeture.
Lecture finale : commandes et sorties brutes/base64, oracles rejoués,
sources, objets, binaires et dépendances vérifiés avant et après lecture.
Les dépendances système et exécutables restent dans les builds locaux,
avec empreintes conservées ; ce lecteur est une vérification **vivante**,
pas une archive autonome rejouable sans ces chemins et versions.

## Couverture fonctionnelle

1636 cas Fraction : centre trouvé par élimination rationnelle du système
de Gram, validité par toutes les coordonnées barycentriques strictement
positives, signe par distance au centre moins rayon carré. L'oracle
n'emploie pas les formules cofactorielles du produit.

| Voie | Supports positifs par cas | Refus | Intérieurs | Contacts | Extérieurs |
|---|---:|---:|---:|---:|---:|
| q3 | 294 | 155 | 81 | 117 | 96 |
| q4 | 731 | 456 | 160 | 408 | 163 |

Les cas répètent des supports avec plusieurs requêtes et permutations ;
ne pas convertir ces comptes en nombre de boules distinctes.
53 entrées CLI invalides doivent retourner exactement1 ; 13 corruptions
de résultats/compteurs sont détectées et rejugées aux lectures finales.

Selftest natif :1287 contrôles,548 requêtes, deux modes invalides refusés
sans mutation des compteurs,18 coordonnées non finies rejetées, quatre
arrondis×quatre combinaisonsFTZ/DAZ. Les passages ExactOnly préservent les
drapeaux d'exception flottante. Copie et remplacement des objets sources
n'altèrent pas les supports préparés. Quatre threads lisent les mêmes
supports pour128requêtes avec états privés : test fonctionnel sous
sanitizers, **pas une gate ThreadSanitizer**.

L'entier fixe passe948 contrôles : addition/soustraction/produit signés,
retenues et emprunts multi-mots, résidus infimes après annulation, capacité
exacte1728bits, dix dépassements refusés et six NaN/Inf refusés. Un produit
dont les longueurs actives totalisent55mots peut encore tenir dans54mots :
ce cas est positivement exercé, pas rejeté par un plafond conservateur.

Support préparé :128octets sur les deux builds. Les tableaux entiers de
repli sont locaux au worker ; cette taille ne les inclut pas. Le q4 exact
évite dix produits et dix additions de poids déjà certifiés à chaque
requête, sans revendiquer un gain chronométrique de pipeline.

Les compteurs filtrés comprennent999 décisions de préparation et360
puissances décidées sans entier ;665 puissances utilisent l'exact, dont
les525contacts. Corpus volontairement difficile, pas une estimation du
coût moyen sur SemanticKITTI. Aucun benchmark8k/16k/32k ou trame entière
de la chaîne native n'a été réalisé dans ce lot.

## Fermeture et reproduction

Les [quatre relectures de qualification](READBACK.json) normal/−O passent
et sont identiques pour chaque build. Les
[mutations compilées](mutations/README.md) détectent trois erreurs
géométriques précises en25commandes : stricteté des poids, orientation
de la puissance, échelle commune normal/sous-normal. R2 fait autorité ;
R1, antérieure aux durcissements du lecteur, est conservée. Builds épinglés
`build/v8_float32_ball_mutants_20260921` et
`build/v8_float32_ball_mutants_r2_20260921` ; aucune modification du moteur
entre ces deux captures. Les quatre relectures des mutants, normal/−O
avec/sans contrôle live, sont séparées de celles de la qualification.

- Clôture Release : `748aaae37f5b4b0c35dec63e0ac9babd4a50b6904d87c136a87e68dd951c4e33`.
- Clôture ASan/UBSan : `14f712011ff602e1413bfbd09faef1a500ab0a39d14a8877d8883bbc243e5f9f`.
- Clôture mutations R2 : `07c97cb290cf1806458f5837daa0f726dbfe82a213668eb9647cab0894c8ae8c`.

```bash
python -B morsehgp3D_v8/bench/run_float32_ball_checks.py read --path morsehgp3D_v8/receipts/float32_ball_20260921/release/ball_04x59id1
python -B -O morsehgp3D_v8/bench/run_float32_ball_checks.py read --path morsehgp3D_v8/receipts/float32_ball_20260921/sanitize/ball_9ut9u2_k
```

Pour reconstruire, employer `run --build NEUF --output NEUF --compiler
/usr/bin/g++` ; ajouter `--sanitize --compiler /usr/bin/clang++` pour la
qualification instrumentée. Ne jamais réécrire les captures ci-dessus.
