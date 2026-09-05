# Certificat FULL minimal : suffisance et restriction réduite

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Sous régularité, feuilles Gabriel avec leurs labels et niveaux, multifusions avec leurs parents et niveaux, puis ancres verticales certifiées suffisent à reconstruire la tour FULL des composantes et de leurs points. La restriction réduite s'en déduit sans bit ni date supplémentaire de première non-trivialité.** C'est une revue de contrat, sans moteur exécuté ni qualification du futur constructeur. Les [pins](full_certificate_pins.json) attribuent cette conclusion aux §§1.1 et 6.1–6.5 de la [note constructeur](../../docs/AUDIT_NIVEAUX_GABRIEL_20260905.md), SHA256 `0b9cd8e17636fcaeb2211bc2c9446bc7ebc6a356e07c399c42529a6f84c9abfd`, distinctement de la [première revue réduite](minimal_certificate_review.md).

## Objet et données finales

Les définitions 21–22 du manuscrit, PDF 84, prennent tous les sommets actifs de Gamma, isolés inclus ; la figure 6.5, PDF 85, compte explicitement la composante isolée CD. L'[extrait directement relu](full_certificate_manuscript_pdf84_85.txt) confirme donc que FULL est l'objet du manuscrit. Les composantes gardent une identité distincte de leur couverture : l'intersection de leurs points ne décide pas leur fusion.

Le certificat porte l'identité de l'entrée et ses PointId, les ordres, l'horizon, la convention de coupe ouverte/fermée, les niveaux exacts comparables entre ordres et une autorité terminale de complétude. Pour chaque ordre K : chaque minimum Gabriel de cardinal K est une feuille identifiée, avec ses K PointId et sa naissance ; chaque vraie multifusion identifiée conserve ses parents distincts et son niveau. Les parents sont des branches actives du snapshot strict précédent et ne sont consommés qu'une fois. Un plateau produit ses multifusions atomiques, sans chaîne binaire arbitraire au même niveau. À K1, les points naissent à zéro : la coupe ouverte zéro les exclut, la fermée les inclut.

Une facette Gabriel régulière n'a aucun point étranger dans sa miniball fermée ; sa première incidence est strictement après sa naissance. Une facette non-Gabriel possède dès sa naissance une coface obtenue avec un intrus strict ; celle-ci touche des facettes strictes préexistantes et ne crée donc pas de racine. Avec un seul intrus, cette coface peut être Gabriel et fusionner plusieurs anciens parents : l'apex silencieux unique ne lui est pas applicable.

Chaque coface régulière a au moins deux facettes strictes dont l'union la couvre. Ses points sont donc déjà couverts par ses parents FULL pré-lot. Une continuation ne change ni composante abstraite ni couverture. Le lemme silencieux et la confluence excluent une fusion d'un lot purement non-Gabriel. Le rejeu des seules naissances et multifusions conserve ainsi toute la couverture par union des labels des feuilles descendantes, sans journal de croissance ponctuelle.

Pour L feuilles, I nœuds internes et R racines finales, chaque multifusion a un degré entrant d au moins deux : $\sum_v(d_v-1)=L-R$, donc $I\leq L-R$, avec $L+I-R$ liens parentaux. Le stockage topologique est linéaire en L, les labels coûtent O(KL) PointId ; cela ne borne ni L par n, ni la taille en bits des niveaux, ni le travail pour découvrir les parents. Si K=n et le domaine est régulier, la feuille X est seule et ne fusionne jamais ; la restriction réduite est vide à cet ordre.

## Pourquoi aucune entrée réduite invisible ne manque

Considérons une branche FULL ayant une seule feuille minimale F. Si elle acquérait un premier sommet supplémentaire, considérer sa première incidence avec une coface Q au niveau a. F est Gabriel régulière, donc existe strictement avant a. Q possède au moins deux facettes strictes distinctes ; il en existe donc une S différente de F. Si S était déjà dans la composante de F, cette composante n'était plus isolée, en contradiction avec le choix de la première incidence. S appartient donc à une autre composante pré-lot : ce premier contact impose une vraie fusion FULL. Le traitement atomique du plateau conserve cette conclusion, même si plusieurs cofaces contribuent au groupe.

Par conséquent, pour K≥2 une branche FULL est non triviale exactement lorsqu'elle a déjà passé une fusion, soit au moins deux feuilles minimales descendantes. Une branche issue d'une seule feuille ne peut accumuler silencieusement des facettes non-Gabriel et devenir réduite sans fusion. **Les seuls labels des feuilles et le graphe parental permettent donc de reconnaître les branches à conserver.**

À chaque fusion FULL, compter ses parents déjà non triviaux, soit les parents qui sont des nœuds internes :

| Nombre de parents non triviaux | Événement réduit déduit |
| --- | --- |
| 0 | Naissance réduite ; couverture égale à l'union des labels des parents isolés |
| 1 | Continuation de ce parent ; delta égal aux points des parents isolés absents de sa couverture |
| Au moins 2 | Multifusion de ces parents ; éventuel delta égal aux points des parents isolés absents de leur union |

Le niveau reste inchangé. Une continuation réduite de delta vide peut être contractée pour le seul objet composantes–généalogie–couverture ; les changements d'identifiants techniques du moteur ne sont pas une obligation de ce quotient. À K1, conserver toutes les branches, y compris les feuilles. Cette construction ne reconstitue pas les memberships de toutes les facettes Gamma ni les anciens tokens octet pour octet. Dans l'autre sens, le réduit seul perd les naissances isolées et ne détermine pas FULL.

## Ancres et masses : suppléments distincts

Pour une naissance FULL F de cardinal K≥2, F est elle-même une coface directe à l'ordre K−1 au même niveau. Ses faces appartiennent à une unique composante inférieure **après fermeture** de ce plateau ; conserver cette cible certifiée fournit l'ancre. Elle peut résulter d'une fusion du même lot et n'est pas nécessairement une composante inférieure pré-lot. Le scan `born` du [contrat vertical réduit](../CONTRAT_VERTICAL_COURANT.md) ne se transpose donc pas tel quel. Normaliser les ancres dans l'histoire inférieure aux coupes suivantes ; à chaque fusion supérieure, exiger l'égalité des images après le lot inférieur. La naturalité donne les compositions, sans table exhaustive ordres–coupes ni chemins géométriques persistants. Une égalité de couvertures ne certifie pas cette cible.

Les feuilles FULL ne sont pas l'univers pondéré de l'Algorithme 1 : celui-ci prend toutes les facettes des cofaces Gabriel avant la réduction par arbre couvrant. Pour A=(0,0,0), B=(4,0,0), C=(1,1,0), K2, les feuilles FULL sont AC de niveau 1/2 et BC de niveau 5/2. AB est non-Gabriel, mais appartient à l'univers pondéré via ABC, directe de niveau 4. Avec $\psi\equiv1$, les trois scores valent 1, chaque T vaut 2 et chaque facette pèse 1. Retenir seulement AC et BC perd une masse 1 ; renormaliser sur ces seules feuilles leur donne chacune 3/2 et change le profil. Ce calcul analytique illustre le [contrat des masses](../CONTRAT_MASSES_VOTE_COURANT.md), sans prétendre à un nouvel essai moteur. Les anciennes captures des PDF 122–126 restent dans la [revue réduite](minimal_certificate_review.md).

Les scores Gabriel n'exigent pas un catalogue Čech exhaustif. Leur affectation temporelle aux composantes reste un contrat supplémentaire : l'Algorithme 1 ne prescrit pas explicitement une date « première incidence Gamma ». Ni les masses, ni les votes, ni la condensation n'héritent automatiquement de ce certificat topologique.

## Limite du domaine

Cette démonstration directe suppose les miniballs pertinentes régulières, sans point étranger sur leur shell. Le contre-exemple AB=(0,0,0),(2,0,0), C=(1,1,0) donne une facette strictement Gabriel et une incidence au même niveau 1 ; il interdit d'émettre AB comme feuille isolée par la règle régulière hors de son domaine. L'extension aux boules hors fenêtre relève de la [preuve FULL distincte](full_proof_review.md), §5, et de son autorité d'inertie ; elle ne dispense pas le futur constructeur de rétablir les prémisses consommées. La petite sortie finale ne supprime ni les portails certifiés, ni leurs contrôles de shell et de budget pendant la construction. Aucun résultat de la suite CPU réduite n'est réattribué ici à un fold FULL implémenté. GCP non utilisé.
