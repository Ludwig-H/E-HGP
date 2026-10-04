# Supports : preuve du rattachement et contrat courant

4 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Revue ciblée après la publication
de S2/257aabb92 et S4/f98aeed67 ; modèles stdlib/Fraction/AST uniquement.
Aucun build, exécution native, fit, CUDA ou GCP.

La [réponse primaire](evidence/USER_DECISION_20261004T202529.json) à 20:25 UTC
retient **Q_b seul**, **K-parties reliées par la boule**, puis
**supports, points, plat**. La proposition `POP=P_b` est donc dépassée.
Les estimations de volume dans les options restent celles du développeur,
pas des mesures de cet audit.

Trois résultats utiles, sans nouveau défaut produit établi :

- [Preuve D2](tower/README.md) : cinq sites entiers donnent
  `41 < beta(F)=64 < beta(b)=1681/25`. La trace peut apparaître après le
  niveau critique précédent ; la constance de H0 transporte sa classe.
  La condition erronée de la preuve ne doit pas devenir un refus natif.
- [Contrat d'identité](evidence/README.md) : le réétiquetage change les
  PointId stockés ; permutation des lignes et provenance brute changent
  aussi les labels plats et hashes. Comparer les structures après transport,
  et réserver l'identité en workers à la même entrée et requête.
- [Port Q_b/S6](qb/README.md) : extraire les supports minimaux avant la
  fermeture zêta ; conserver les q4 du cube à K1, même avec zéro coface.
  Les champs locaux proposés sont bornés ; cela ne qualifie pas les
  agrégations globales ni un énumérateur natif encore absent.

**6 456 gardes**, 77 + 79 + 6 300, normal/−O identiques. Le juge Python SHA
de S4 est également relu sur un transport fictif (1 242 requêtes) ; le C++
SHA n'est pas exécuté. Les comptes de gardes décrivent ces modèles bornés,
sans qualification native, stabilité du carrier ou gain de vitesse nouveau.

S2 est un déplacement de déclarations/en-tête public ; S4 livre io.
Le journal S3 et le module supports S6 restent à juger à leur livraison.
Les résultats locaux annoncés par leurs auteurs sont conservés comme
rapports, sans transfert de qualification G4. [io/BEFORE.json](io/BEFORE.json)
et [io/AFTER.json](io/AFTER.json) fixent les neuf fichiers relus, inchangés
durant cette lecture ; `retract()` est déjà livré. Les remarques connues
de `verif_s4.md` ne sont pas republiées comme des découvertes nouvelles.

Les capsules sont recopiées sans modifier leurs clôtures. Le [registre](SOURCE.json)
fixe leurs empreintes ; SHA256SUMS inventorie tout sauf lui-même.
Les sources de conception copiées sont des propositions datées ; la
décision primaire ci-dessus prévaut. Les chemins privés dans les métadonnées
servent uniquement à la provenance ; le rejeu utilise les copies locales.

Rejeu portable de l'ensemble, depuis ce dossier :

```sh
python3 -B -S check.py
python3 -B -S -O check.py
```

Le lecteur vérifie l'inventaire exact, rejoue les trois modèles en normal
et −O, et compare leurs sorties conservées. Les stdout du rejeu parent
doivent correspondre à [RESULTS.json](RESULTS.json). Aucun paquet requis.
