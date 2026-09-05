# HGP FULL : feuilles Gabriel, multifusions et ancres verticales

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**La proposition FULL est correcte sous le domaine régulier déclaré, avec l'extension de fenêtre précisée au §5.** Les naissances de composantes sont exactement les facettes Gabriel de cardinal K ; les seules fusions possibles ont des niveaux Gabriel de cardinal K+1. Toute continuation FULL conserve les points couverts. Le certificat final peut donc porter les feuilles avec leurs points et dates, puis les multifusions avec leurs parents et dates, sans journal de croissance ponctuelle. Les attaches restent nécessaires pour déterminer les bons parents pendant la construction.

Cette preuve est distincte de la [preuve réduite](level_proof_review.md). La v7 CPU E qualifiée représente les composantes non triviales aux ordres supérieurs ; ni son certificat horizontal ni la sonde de portails réduits ne sont réattribués à FULL. Aucun produit, C++, benchmark ou Git n'est modifié ici. Les octets sources et les attendus analytiques sont conservés dans [full_proof_review.json](full_proof_review.json).

## 1. Objet FULL et prémisses

Soit un nuage fini $X\subset\mathbb{R}^3$ de positions distinctes, de cardinal n, et $1\leq K\leq n$. À la coupe de rayon carré t, Gamma FULL possède **toutes** les K-facettes F satisfaisant $\beta(F)\leq t$, isolées comprises. Les cofaces Q de cardinal K+1 et niveau au plus t relient toutes leurs K-facettes. La proposition5 du manuscrit conserve les composantes en se limitant à ces adjacences élémentaires.

Cet objet correspond aux définitions21–22 du [manuscrit](../../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf), pages PDF84–85, relues directement : la figure6.5 compte expressément l'arête isolée CD comme un K-polyèdre. Les définitions25–28, le fait12 et le théorème4, PDF110–115, fournissent les miniballs, supports et remplacements stricts. La proposition6 n'est pas une prémisse permettant d'effacer les rattachements. La présente revue vise les nouveaux §§1.1 et6.1 de la [note constructeur](../../docs/AUDIT_NIVEAUX_GABRIEL_20260905.md), publiée dans `94a3513b081bd61a8276c3e73e7d91ca5aa42abe`, SHA256 `0b9cd8e17636fcaeb2211bc2c9446bc7ebc6a356e07c399c42529a6f84c9abfd`.

La preuve régulière suppose que chaque boule pertinente a pour coquille globale exactement son support minimal essentiel U, affinement indépendant, avec centre dans l'intérieur relatif de conv(U). Pour un label d'au moins deux points, |U|≥2. Aucun autre point du nuage n'est sur cette frontière. La position générale de la définition26 est suffisante ; le §5 réduit cette exigence au domaine de fenêtre accepté.

Une composante conserve une identité abstraite indépendante de l'union U(C) de ses points. Deux couvertures qui se recouvrent ne sont pas fusionnées pour cette raison. Les coupes strictes et fermées sont distinctes ; les parents d'un lot a viennent exclusivement de Gamma FULL à $\beta<a$.

## 2. Les naissances sont exactement les facettes Gabriel

Soit F de cardinal K≥2 et b=β(F). Si F est Gabriel, sa régularité donne $B_F\cap X=F$. Toute extension F∪{x}, x extérieur à F, a un rayon au moins égal à celui de F. Une égalité imposerait la même miniball par unicité, et donc x∈B_F, contradiction. Ainsi $\lambda(F)>b$, avec λ(F)=∞ si K=n. La facette F est isolée à la coupe fermée b et pendant un intervalle suivant non vide : c'est une vraie naissance FULL.

Si F n'est pas Gabriel, choisir un intrus strict z donne Q=F∪{z} avec β(Q)=b. Cette coface régulière possède les facettes strictes $Q\setminus\lbrace u\rbrace$, u∈U, déjà présentes à β<b. Elles peuvent être isolées : FULL les conserve quand même comme anciennes composantes. L'arrivée de Q rattache donc F à au moins une composante préexistante ; F n'est pas une naissance de composante.

Il ne suffit pas de constater que F n'est pas isolée pour exclure une naissance de groupe. Ici, toute coface du groupe incident à F touche une facette stricte préexistante. La composante atomique de ce groupe a donc au moins un parent ancien. À l'inverse, une nouvelle F Gabriel ne peut être incidente à aucune coface de niveau b ; ses naissances sont disjointes des groupes de connexion du même lot.

**Cas à un seul intrus.** Si F possède exactement un intrus strict z, Q=F∪{z} peut être Gabriel. Ses facettes strictes peuvent appartenir à plusieurs composantes antérieures, et Q peut produire une multifusion. Le lemme d'apex unique des cofaces non-Gabriel ne s'applique pas à cette Q. Si F possède au moins deux intrus, une première extension garde un intrus extérieur et est non-Gabriel ; l'apex unique devient alors applicable.

À K1, les facettes sont les points, de niveau zéro, et toutes sont Gabriel. Dans l'extension aux coupes ouvertes, Gamma FULL à β<0 est vide ; la coupe fermée zéro active les n feuilles ponctuelles. Cette frontière doit être déclarée et testée pour FULL : initialiser inconditionnellement les points dans un lecteur réduit ne prouve pas le comportement de la coupe ouverte zéro.

## 3. Une coface ne crée aucun point hors de ses parents FULL

Soit Q régulière, de cardinal K+1 et niveau a, avec support essentiel U de cardinal au moins deux. Retirer u∈U laisse pour points de frontière un sous-ensemble propre du support ; l'optimalité de la miniball donne $\beta(Q\setminus\lbrace u\rbrace)<a$. Ces facettes sont donc toutes des sommets pré-lot de Gamma FULL, même si aucune coface ne leur était encore incidente.

Leur union vaut Q : $\bigcup_{u\in U}(Q\setminus\lbrace u\rbrace)=Q$. Tous les points de Q sont donc déjà dans l'union des couvertures des anciens parents touchés. Les autres facettes de Q, éventuellement nouvelles au niveau a, ne peuvent ajouter un PointId hors de cette union. Une coface n'est jamais une naissance ex nihilo de composante FULL.

L'argument se compose sur un lot entier : pour chaque coface, tous ses points sont couverts par ses parents stricts ; l'union des cofaces d'un groupe est donc couverte par l'union des parents de ce groupe. Son état fermé a exactement l'union des couvertures parentales. Si le groupe ne rencontre qu'un parent, sa continuation a un delta de points nul. Si le groupe en rencontre au moins deux, sa couverture est leur union, sans supplément ponctuel.

Une coface non-Gabriel a en outre un apex strict unique : le remplacement d'un essentiel par un intrus relie ses facettes strictes avant a. Les contacts égaux entre cofaces non-Gabriel confluent par unicité de la miniball et par une coface de remplacement strict. La [preuve de plateau](level_proof_review.md), §§2–4, s'applique : un groupe entièrement non-Gabriel ne peut fusionner deux anciens parents, isolés ou non. Son apex est déjà non trivial, donc c'est une ancienne composante FULL valide.

Les groupes mixtes ne rendent pas les cofaces silencieuses dispensables pendant la construction. Ils démontrent que leur rattachement peut être représenté par les bons parents pré-lot : une coface directe ne partage pas de facette égale avec une non-directe distincte de même niveau, car elles auraient la même boule et la directe garderait un intrus étranger. Leur contact est strict et se résout dans l'apex ancien.

Ainsi, les seules modifications FULL sont les naissances Gabriel de cardinal K et les multifusions portées par un lot contenant une directe Gabriel de cardinal K+1. Une valeur Gabriel peut encore être sans modification publique ; toutes ses occurrences ne sont pas des nœuds nécessaires. À un niveau sans coface directe, des **naissances de facettes Gabriel de cardinal K restent possibles** : le corollaire réduit « aucune modification sans coface directe » ne doit donc pas être transféré tel quel à FULL.

## 4. Rejeu suffisant et information conservée

Pour chaque ordre, conserver chaque feuille F Gabriel avec son identité, ses K PointId et β(F), puis chaque vraie multifusion avec son identité, son niveau et l'ensemble de ses parents distincts. À chaque coupe, activer les feuilles admises et contracter atomiquement les parents des multifusions admises. Les couvertures se calculent par union des labels des feuilles descendantes.

L'induction repose sur les §§2–3 : les feuilles représentent toutes les naissances de composantes ; les continuations ne modifient ni composante abstraite ni couverture ; chaque fusion prend exactement l'union de ses parents. Les facettes non-Gabriel peuvent changer le membership de la composante sans changer cet état reconstruisible. Les identités, la généalogie et la graduation conservent les recouvrements des points sans les utiliser comme critère de connexion.

Si L_K feuilles et I_K multifusions donnent R_K racines dans le préfixe considéré, on a $\sum_{v\text{ fusion}}(q_v-1)=L_K-R_K$, où q_v≥2. Donc $I_K\leq L_K-R_K$, et le nombre de liens parentaux vaut $L_K+I_K-R_K$. Le stockage topologique est linéaire en L_K ; conserver les labels coûte O(KL_K) identifiants. Cela n'est pas une borne linéaire en n pour K>1, ni une borne du travail requis pour découvrir les parents.

Ce certificat FULL ne remplace pas le catalogue des facettes pondérées du §9.1 du manuscrit : les minima Gabriel de cardinal K et les facettes de toutes les cofaces Gabriel de cardinal K+1 sont deux familles différentes. Il ne reconstruit pas les adjacences, tous les labels actifs, leurs dates de membership ou leurs masses sans contrat supplémentaire. L'affectation d'une masse à la première incidence Gamma est une politique possible, pas une prescription temporelle explicite attribuée ici à l'Algorithme1.

## 5. Fenêtre de rang : extension sans régularité globale

Fixons des ordres demandés jusqu'à M≤n et une fermeture de rang $r_{\max}=\min(M+1,n)$. L'autorité S signifie que toute miniball positive de support minimal s, avec p points strictement intérieurs globaux et p+s≤rmax, est représentée et a reçu un census fermé exact dont la coquille est exactement le support. Les boules au-dessus de la fenêtre peuvent être irrégulières. C'est la prémisse mathématique raccordée par le [certificat horizontal E](../CERTIFICAT_HORIZONTAL_COURANT.md) pour sa propre fenêtre ; son résultat publié reste réduit.

Une facette Gabriel F de cardinal K≤M contient tous les intérieurs de sa boule et un support minimal : p+s≤K≤rmax. Elle est donc régulière sous S, son saturé vaut F et p+s=K. Toutes les naissances FULL relèvent ainsi du catalogue Gabriel de rang K dans cette même fermeture.

Considérons une facette non-Gabriel F. Si sa boule est dans la fenêtre, elle est régulière et le raisonnement du §2 s'applique : sa première extension par un intrus la rattache à des facettes strictes. Si sa boule est hors fenêtre, p+s>rmax ; pour K≤M<n, cela donne p+s≥K+2. Le [théorème transverse4.2](../../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#531-inertie-h_0-exacte-des-blocs-saturés-au-dessus-de-la-fenêtre-de-rang) fournit un graphe strict connexe de K-facettes couvrant le saturé. F s'attache à un apex préexistant ; elle ne peut créer ni naissance ni point. Lorsque M=n et rmax=n, le cas hors fenêtre est impossible puisque p+s≤n.

Les cofaces Q sont traitées de même : régulières dans la fenêtre, inertes par leur bloc saturé au-dessus. Dans ce second cas, le graphe strict couvre tout le saturé, donc tous les points de Q. Les contacts de deux blocs par une facette stricte partagent leur apex ; un contact par une facette nouvellement née de niveau a identifie leurs boules par unicité. Aucun groupe silencieux irrégulier hors fenêtre ne crée donc une fusion cachée ou un point supplémentaire. Toute coface Gabriel pertinente est dans la fenêtre, car p+s≤K+1≤rmax.

Cette extension ferme les événements FULL sous S. Elle ne certifie pas un nouveau chemin de résolution qui traverserait une boule irrégulière non contrôlée : chaque portail exécuté doit garder ses contrôles, ses budgets et son autorité terminale. Un autre choix d'intrus ne reçoit pas automatiquement le certificat des anciens maillons E.

## 6. Ordre terminal K=n et bornes du catalogue

À K=n, il existe exactement une facette X et aucune coface. X est Gabriel par absence de point extérieur ; dans le domaine accepté, elle donne une unique feuille au niveau β(X), qui reste isolée pour toujours. La hiérarchie FULL de cet ordre n'est donc pas vide. La hiérarchie réduite est vide, ce qui explique pourquoi son ancienne borne d'ordres ne peut être reprise sans adaptation.

Pour une tour demandée 1..M, les naissances utilisent les cardinalités1..M, les connexions les cardinalités2..min(M+1,n). Les points constituent les minima de cardinal1. Une même boule saturée de rang m fournit une feuille d'ordre m et une coface d'ordre m−1 ; aucune seconde géométrie n'est nécessaire pour ces deux rôles. Si M=n, il faut émettre la feuille X même sans boucle de connexions d'ordre n. La règle produit actuelle K_eff=rmax−1 ne l'émet pas à elle seule lorsque rmax=n.

Pour la demande bornée 1..10, les points et les rangs2..11 suffisent aux valeurs pertinentes, en tronquant les cardinalités à n. Cela ne borne ni le nombre de minima Gabriel par n, ni le nombre de cofaces ou de portails à examiner.

## 7. Cartes FULL entre ordres

Pour K≥2, une K-facette F active à une coupe t devient une coface active de l'ordre K−1 à la **même coupe**. Ses K faces de cardinal K−1 sont donc dans une unique composante inférieure. Si deux labels F et G sont adjacents à l'ordre K, ils peuvent être reliés par des adjacences élémentaires ; deux voisins partagent une face de cardinal K−1. Leurs images inférieures coïncident. Par connexité, le choix de F dans une composante supérieure C est indifférent : il définit une application fonctionnelle V_K,t sur les composantes FULL.

Les inclusions horizontales conservent chaque label et chaque incidence. Le même témoin F définit donc les deux chemins d'un carré de naturalité. L'argument se répète aux ordres inférieurs et donne les compositions verticales. On a $U(C)\subseteq U(V_{K,t}(C))$ ; ce n'est pas une conservation automatique d'une mesure de feuilles.

À la naissance d'une feuille supérieure F de cardinal K, F est Gabriel et constitue elle-même une directe à l'ordre K−1, au niveau b=β(F). Le groupe inférieur **après tout le lot fermé b** donne son ancre. Avant ce lot, les faces inférieures peuvent être dans plusieurs composantes : choisir arbitrairement l'une de leurs racines strictes serait faux. À la coupe ouverte b, la nouvelle feuille source n'existe pas encore ; son ancre fermée n'est pas une activation anticipée.

Une ancre par naissance supérieure suffit : aux coupes suivantes, la normaliser dans l'histoire inférieure ; lors d'une multifusion supérieure, les images des parents, normalisées à la coupe cible commune, doivent coïncider. La naturalité justifie cette coïncidence. Aucun tableau de toutes les cartes à toutes les coupes n'est requis. Le scan `born` du profil réduit possède une autre prémisse et n'est pas utilisé pour certifier cette feuille FULL.

## 8. Fixture minimale régulière et frontière exclue

Prendre A=(0,0,0), B=(4,0,0), Z=(2,1,0). Les distances carrées AZ et BZ valent5, et AB vaut16. Les deux petites boules diamétrales ont le troisième point strictement extérieur ; AB a Z strictement intérieur, de puissance −3 dans la forme diamétrale. Le nuage est régulier.

À K2, AZ et BZ sont deux feuilles Gabriel isolées, toutes deux nées au niveau5/4. AB est non-Gabriel et ne possède qu'un intrus. Il naît au niveau4 avec la coface Gabriel ABZ, support AB ; cette coface fusionne les deux anciens parents, dont les couvertures {A,Z} et {B,Z} avaient déjà pour union X. Il n'y a ni naissance AB ni delta ponctuel de continuation. Dans le profil réduit, la même coface produit au contraire une naissance, puisque les deux parents isolés ont été retirés.

À K3=n, X est l'unique feuille, née à4, sans aucune coface supérieure. À K1, les deux arêtes Gabriel AZ et BZ du lot5/4 fusionnent atomiquement les trois points ; les deux feuilles K2 nées au même niveau ont donc la même cible K1 **fermée**, tandis que leurs extrémités étaient séparées dans la coupe stricte. Cette même fixture vérifie la nécessité du côté fermé de l'ancre verticale.

La frontière irrégulière signalée par le constructeur est A=(0,0,0), B=(2,0,0), C=(1,1,0). AB est Gabriel au sens de l'intérieur strict vide, mais C est sur son shell ; β(AB)=β(ABC)=1. AB n'a pas de période isolée à sa naissance. Cette fixture réfute la règle « toute facette Gabriel est une feuille » sans la porte régulière. Son support minimal de taille2 et son extra-shell sont dans la fenêtre pertinente, donc l'autorité S l'exclut. Les coordonnées et attendus de ces deux fixtures sont inscrits dans le JSON de cette revue ; ce sont des dérivations analytiques, pas des résultats moteur.

## 9. Décision constructive

La source FULL doit enregistrer les naissances de facettes Gabriel **avant leur première incidence**, y compris lorsqu'elles restent isolées longtemps. La table de construction reçoit ces racines, puis conserve les alias de toutes les facettes réutilisées par les directes, même si une continuation n'émet rien dans le journal final. Le prédicat réduit « déjà incidente » ne peut pas remplacer le prédicat FULL « appartient déjà à une composante née ».

Un lot gèle ses parents stricts, résout les attaches nécessaires, contracte ensemble ses cofaces directes et publie uniquement les vraies multifusions. Ses feuilles Gabriel nouvelles sont disjointes des connexions du même niveau et sont émises comme naissances. Les tables peuvent être compressées ; la suppression d'une clé doit préserver une autorité de résolution. Aucune décision de fusion ne repose sur les points seuls.

Cette construction et son export restent à implémenter et qualifier séparément. La preuve établit la suffisance mathématique des deux catalogues de valeurs et du certificat final FULL ; elle ne promeut ni le fold réduit existant, ni ses identifiants, ni son calendrier de budgets, ni ses performances. Les données pondérées restent un supplément de contrat. GCP non utilisé.
