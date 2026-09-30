# B21 : contre-relecture des bornes et de la marge

Relecture du moteur privé au commit `5dd83b5c68919d87ada067204d19ee4edb166859`.
Cette archive rejoue uniquement le modèle rationnel du vérificateur privé,
copié sans modification depuis son SHA256
`6adf272152b4ae43cb6dd71a44fa91fb215c41f1a160530d38794f6c0a317ff0`.
Pas de compilation, d'appel moteur, de LiDAR ni de GCP par ce lot.

Les majorants entiers et les voies courtes sont cohérents avec la relecture :
à B21, niveau q4 à numérateur de 178 bits et dénominateur de 134 bits,
dans I192/I192. Ni u24/u32 ni CLI fine ne sont qualifiés.

Trois constats sont séparés des assertions de contrôle :

- La valeur documentaire de delta est fausse : 4,02uL ≈9,36e−10,
  pas <2^(B−51), ni 4,7e−10.
- Le raccourci de preuve « deux erreurs individuelles ≤m » omet
  l'arrondi du seuil. Son majorant grossier donne 0,03917965 >m=0,0390625.
  Cela ne démontre pas une erreur de filtre.
- La preuve peut être fermée sans changer le code : pour un même centre,
  le terme quadratique d'erreur de centre s'annule dans la différence
  des distances ; pour I3 entre deux MEB, r²≤3L²/4. Le script vérifie
  les bornes corrigées avec l'arrondi du seuil pour B=1..21.
  Inscrire ces arguments dans les commentaires de production.

Code0 signifie que les assertions du modèle tiennent, pas que ces trois
constats documentaires sont corrigés. Normal et −O donnent la même sortie.
Les pins du code moteur sont des observations de provenance ; ses sources
ne sont pas copiées ni compilées par cette archive.

Une première lecture vivante est écartée parce que le script a changé ;
la copie figée a ensuite été vérifiée octet pour octet avant les deux
rejeux conservés. Les écarts de préparation sont déclarés dans le reçu.

Lecture : `python3 -B read.py`, puis `python3 -B -O read.py`.
Le lecteur contrôle l'inventaire, les empreintes et les deux sorties, puis
rejoue uniquement le snapshot figé, sans écrire dans l'archive.
