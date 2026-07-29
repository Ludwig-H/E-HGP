# Noyau mathématique de MorseHGP3D

Le parcours normatif est volontairement court. Les notes de jalons antérieurs restent dans ce dossier comme bases de preuve liées par le registre, mais ne constituent plus des architectures concurrentes.

## Parcours normatif

| document | question résolue |
|---|---|
| [Définition HGP 3D](DEFINITION_HGP_3D.md) | quelle hiérarchie doit être calculée ? |
| [Catalogue critique 3D](CATALOGUE_CRITIQUE_3D.md) | quels événements de Morse de support au plus quatre peuvent modifier $H_0$ ? |
| [Catalogue exact des paires diamétrales](CATALOGUE_PAIRES_DIAMETRALES_EXACT.md) | comment énumérer les rangs fermés deux à $K_{\max}+1$ avec payload complet et cutoff Yao48 exact ? |
| [Frontière directe des supports trois et quatre](FRONTIERE_DIRECTE_SUPPORTS_3_4.md) | pourquoi les triangles aigus gardent-ils une frontière indépendante et quels tétraèdres restent ensuite ? |
| [Incidences silencieuses](INCIDENCES_SILENCIEUSES_GAMMA.md) | pourquoi le flot Gabriel brut peut-il manquer une liaison hiérarchique ? |
| [Attaches par miniball](ATTACHES_DESCENTE_MINIBALL.md) | comment rattacher un bras connu à sa composante globale ? |
| [Preuves et heuristiques](STATUT_PREUVES_ET_HEURISTIQUES.md) | quelles affirmations sont démontrées, conditionnelles, ouvertes ou fausses ? |

## Cascade active

Posons $K_{\mathrm{eff}}=\min(K_{\max},n)$ et $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$. Alors :

1. injecter les événements ponctuels de rang un;
2. exécuter une passe exacte multi-ordre des paires jusqu'au rang $s_{\max}$;
3. fermer directement tous les candidats dont le support minimal est une paire;
4. rechercher indépendamment les triangles affinement indépendants et aigus, en rejetant les cas de support deux;
5. émettre depuis chaque triangle les tétraèdres qu'il supporte, puis rechercher indépendamment les tétraèdres affinement indépendants dont le centre circonscrit est strictement intérieur;
6. classer le rang fermé et le shell complet de chaque support survivant;
7. réduire les événements par lots de niveau exact, avec incidences silencieuses et attaches requises par le profil;
8. publier seulement après fermeture des frontières et rejeu des certificats.

Les paires n'épuisent pas les triangles aigus : la fixture Hartigan permanente donne un support trois de rang trois dont tous les côtés ont rang quatre. Les triangles droits, obtus ou dégénérés se réduisent en revanche à un support de taille deux. La même dichotomie ramène tout tétraèdre non bien centré à un support de taille deux ou trois.

## Replis et oracles

La [tour globale de boules saturées](TOUR_BOULES_SATUREES.md) et [Gamma exhaustif](../../reference/README.md) sont des oracles de preuve bornés. La [réduction Delaunay ordinaire à une arête](DELAUNAY_ORDINAIRE_GAMMA2.md) est un théorème et un falsificateur utiles, mais son univers explicite est interdit dans le produit. Les quelques replis encore maintenus sont inventoriés dans [`docs/research`](../research/README.md); les voies réfutées sont dans les [archives](../archive/abandoned/README.md).

## Conventions

Les niveaux sont des rayons carrés. Un rang est le nombre de points dans la boule fermée. Un support est un sous-ensemble minimal de points frontière dont l'enveloppe convexe relative contient le centre. Un lot regroupe tous les événements d'un même niveau exact.

Les propositions flottantes ne décident jamais seules une inclusion fermée ou une égalité de coque. Toute égalité descend vers une décision exacte; une frontière inachevée reste explicitement budgétée.
