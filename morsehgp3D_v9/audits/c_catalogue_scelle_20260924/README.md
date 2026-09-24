# Mesure locale de la passe 1 de la validation FULL (auditeur C)

Pièces de la [proposition du catalogue scellé](../PROPOSITION_C_CATALOGUE_SCELLE_20260924.md).
Base `48791e721`. Le patch `pass1_variants.patch` ajoute deux macros de
compilation qui retirent les puissances, le support déclaré ou les deux ;
le code produit n'est pas modifié. `seal_measure.sh` construit les quatre
variantes de la sonde et mesure 08/000000 à K5 et K10, W8, deux
répétitions entrelacées. Ce sont des mesures locales, sur un hôte **chargé** :
les temps sont indicatifs, et les condensés sont identiques entre les
variantes. GCP non utilisé.
