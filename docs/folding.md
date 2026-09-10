# Phase 2 — rendre l'instrument pliable

Objectif : un tenor de 57 cm qui rentre dans un sac. Ce document tranche entre les familles
de pliage possibles, **calcule** ce que la géométrie du tenor autorise réellement, et donne
le protocole de test qui coûte le moins cher.

> Tous les chiffres de ce document sortent de `tools/analyse-folding.mjs`, appliqué au SVG
> de panisto à l'échelle 0.847 (57,2 cm, taille d'un vrai tenor). Ils sont reproductibles :
> `node tools/analyse-folding.mjs --inset 2`

---

## Les trois contraintes

Elles éliminent l'essentiel de la littérature origami, et deux d'entre elles ne sont traitées
par aucun papier :

1. **Les plis ne doivent jamais traverser une note.** Une pastille à cheval sur une charnière
   se déchire et son contact devient intermittent.
2. **La surface doit rebondir là où on frappe.** Voir la nuance importante plus bas — ce n'est
   pas un problème de capteur, c'est un problème de jouabilité.
3. **29 conducteurs doivent franchir les plis** sans casser en fatigue.

---

## Ce que la géométrie du tenor autorise vraiment

### Plis droits : impossible

L'origami rigide impose des plis **rectilignes** — une feuille pliée n'a pas le choix.
Le balayage de toutes les directions radiales donne :

| | |
|---|---|
| Directions radiales ne coupant aucune note | **2 / 1440** |
| Rayons réguliers possibles (moyeu 0 à 120 mm) | **1 / 12** |

**Verdict : impossible.** Les deux anneaux de notes sont décalés angulairement — une droite
qui passe entre deux notes extérieures tape systématiquement une note du milieu.

> ⚠️ Ça invalide une intuition séduisante : le tenor a 12 notes par anneau, disposées selon le
> cycle des quintes, donc on croit voir une symétrie d'ordre 12 exploitable. Elle existe en
> **hauteur de note**, pas en **géométrie**. Les anneaux sont tournés l'un par rapport à
> l'autre.

### Plis en zigzag : possible, et c'est ce qui rend l'idée des tuiles décisive

Dès que les notes sont des **tuiles rigides séparées** plutôt qu'une feuille continue, la
charnière n'a plus besoin d'être droite : elle peut se faufiler entre les tuiles.

Recherche de 12 charnières **disjointes** (chacune consomme 3 mm de matière) et **monotones
en rayon** (du moyeu vers le bord, sans contourner), moyeu central de 60 mm :

| Retrait des tuiles | Passage le plus étroit de chaque charnière (mm) | Goulot |
|---|---|---|
| 0 mm (contour dessiné) | 2,0 2,0 1,8 1,7 1,5 1,3 1,2 1,0 1,0 1,0 1,0 0,7 | **0,7 mm** |
| **2 mm** | 4,0 4,0 3,8 3,7 3,5 3,3 3,2 3,0 3,0 3,0 3,0 2,7 | **2,7 mm** |

**12 charnières sur 12 sont réalisables.** Elles découpent le pan en 12 pétales d'environ
2 notes chacun (une extérieure, une médiane), plus un moyeu central rigide portant les 5 notes
aiguës.

Le levier décisif est le **retrait des tuiles** : les contours dessinés par panisto sont une
convention graphique, pas une cote. Chaque millimètre retiré élargit chaque couloir d'un
millimètre de chaque côté — la relation est exactement linéaire. **Rentrer les tuiles de 2 mm
fait passer le goulot de 0,7 à 2,7 mm**, c'est-à-dire un couloir de plus de 5 mm de large.
C'est confortable pour une charnière vivante ou une charnière en toile.

Pour référence, les écarts réels entre notes voisines : le plus serré est **2,45 mm**
(D4 / G4, puis Eb4 / G#4), mais la médiane des paires proches est de **10,6 mm**. C'est cette
disparité qui explique la dispersion des goulots.

Nombre de charnières visé (retrait 0) :

| Charnières | Goulot |
|---:|---|
| 4 | 1,7 mm |
| 6 | 1,3 mm |
| 8 | 1,0 mm |
| 12 | 0,7 mm |

Passer de 12 à 6 pétales n'achète que 0,6 mm : le nombre de pétales n'est pas le paramètre
sensible, **le retrait des tuiles l'est**.

> `out/folding.svg` trace les charnières retenues sur le gabarit — à superposer pour
> vérifier à l'œil.

---

## Une correction importante sur la rigidité

J'ai d'abord affirmé qu'une facette souple « dégrade directement la détection ». **C'est
faux**, et ça change la hiérarchie des priorités.

Pour un capteur **par contact**, une surface molle *allonge* le contact. Or notre mode de
défaillance était l'inverse : les contacts trop courts, ceux qui disparaissent avant que la
dichotomie ait identifié la note. Une surface souple est donc neutre, voire favorable au
capteur.

Le vrai coût de la souplesse, c'est le **rebond**. La technique du pan repose sur le retour de
la baguette ; sur une surface morte, les roulements deviennent physiquement épuisants.

**C'est un problème d'ergonomie de jeu, pas de capteur.** Et ça renforce l'approche par tuiles
rigides : tuile dure sur support souple, c'est exactement la construction d'un pad de batterie
électronique — le dur donne le rebond, le souple donne l'amortissement.

---

## Les deux familles de pliage

### Famille A — la courbure par pavage ❌

Waterbomb, Ron Resch, Miura-ori, kirigami : la courbure émerge de **beaucoup de petites
facettes**. Très étudié, très beau, et inutilisable ici — chaque pli traverserait des notes.
À connaître pour ne pas y perdre de temps.

- **[*From flat sheets to curved geometries: Origami and kirigami approaches*](1-s2.0-S1369702117306399-main.pdf)** — Callens & Zadpoor, *Materials Today* 21(3), 2018. **La revue à lire si tu n'en lis qu'une** ; copie locale dans `docs/`.
- [*Approximating 3D surfaces using generalized waterbomb tessellations*](https://www.sciencedirect.com/science/article/pii/S2288430017300556)
- [*Constructing foldable cylindrical surfaces via unfolded waterbomb origami units*](https://doi.org/10.1093/jcde/qwac062)
- [Ron Resch, site officiel](http://www.ronresch.org/ronresch/gallery/extreme-paper/) · [généralisation par Tachi](https://origami.c.u-tokyo.ac.jp/~tachi/cg/FreeformOrigamiTessellationsTachi2013ASME.pdf)
- [*Boundary curvature guided programmable shape-morphing kirigami sheets*](https://www.nature.com/articles/s41467-022-28187-x), Nature Comms

### Famille B — peu de grandes facettes ✅

- [*Origami With Rotational Symmetry: A Review*](https://zhaolab.stanford.edu/sites/g/files/sbiybj21256/files/media/file/amr_075_05_050801.pdf) (labo Zhao, Stanford) — la revue exactement sur les volumes de révolution
- [Tachi, *One-DOF Cylindrical Deployable Structures with Rigid Quadrilateral Panels*](https://origami.c.u-tokyo.ac.jp/~tachi/cg/RigidFoldableCylindricalOrigami_tachi_IASS2009.pdf) — *flat-foldable*, *rigid-foldable*, 1 DDL, et explicitement **« thick : facets can be substituted with thick or multilayered panels »**. Nos tuiles collées sont exactement ça.
- [Miura & Tachi, *Synthesis of rigid-foldable cylindrical polyhedra*](https://origami.c.u-tokyo.ac.jp/~tachi/cg/FoldableCylinders_miura_tachi_ISISSymmetry2010.pdf)
- [Lang, Magleby & Howell, *Single-Degree-of-Freedom Rigidly Foldable Origami Flashers*](https://scholarsarchive.byu.edu/facpub/1621) — **le flasher, c'est le tabouret qui se déplie en tournant, mais pour un disque au lieu d'un cylindre.** Même geste de vrille-enroulement, et rigidement pliable par construction.
- [STL imprimable : flasher hexagonal à charnières vivantes](https://www.printables.com/model/535915-folding-origami-flasher-hexagon-with-living-hinges) — par le labo BYU, à imprimer pour comprendre le mécanisme en 10 minutes
- [BYU Compliant Mechanisms — Maker Resources](https://www.compliantmechanisms.byu.edu/maker-resources)

### ⚠️ Kresling et tuiles rigides s'excluent

Le Kresling est bistable, ce qui semblait offrir un verrouillage gratuit. Mais :

> *Kresling origami is not rigidly foldable; therefore, deploying or collapsing this pattern
> introduces kinematic incompatibility, which causes stretching and bending of the creases and
> triangular panels.* […] *the Kresling unit's multistability is primarily driven by panel
> stretching.*
> — [*Rigid-foldable cylindrical origami with tunable mechanical behaviors*](https://www.nature.com/articles/s41598-023-50353-4), Sci. Rep. 2023

**La bistabilité vient de la déformation des facettes.** Coller des tuiles rigides empêche
cette déformation : soit le mécanisme se bloque, soit les collages lâchent, soit on perd la
bistabilité. C'est le même phénomène physique — on ne peut pas avoir les deux.

Le Kresling [existe en version conique](https://www.sciencedirect.com/science/article/abs/pii/S0263823123004585)
si l'on accepte des facettes déformables, donc sans tuiles rigides.

### Outils

- **[Origami Simulator](https://origamisimulator.org/)** (Amanda Ghassaei) — simule n'importe quel patron dans le navigateur. Commence par là.
- **[Logiciels de Tomohiro Tachi](https://origami.c.u-tokyo.ac.jp/~tachi/software/)** — Freeform Origami, Origamizer, Rigid Origami Simulator
- [*Approximating a Target Surface with 1-DOF Rigid Origami*](https://arxiv.org/pdf/1905.04773) — Tachi
- [Design de charnières vivantes découpées laser](https://www.ponoko.com/blog/how-to-make/how-to-design-a-living-hinge/) · [DesignSpark](https://www.rs-online.com/designspark/laser-cut-living-hinges-for-neater-designs)

---

## Les tuiles rigides

Découpler la structure pliante de la surface de frappe résout plusieurs problèmes d'un coup.

**Le gain principal n'est pas la rigidité, c'est que la tuile devient l'électrode.** Le point
le plus bricolé de la v0 — ruban alu + attache parisienne en laiton — disparaît.

| Matériau | Pour | Contre |
|---|---|---|
| **Laiton / acier étamé 0,5 mm** | soudable au dos, dur, excellent rebond | découpe laser ou jet d'eau |
| **PCB découpé à la forme** | face cuivre = électrode, pastille de soudure au dos, parfaitement plat | coût |
| **PCB par pétale** (2-3 notes + pistes vers le moyeu) | supprime aussi le câblage : le circuit *est* la structure | à chiffrer, cartes de ~15×25 cm |
| ~~Dibond / composite alu~~ | — | ❌ l'alu ne se soude pas, on retombe sur le problème de départ |

**Électriquement, les tuiles métalliques sont neutres à favorables.** La capacité ajoutée est
du côté **piloté**, où le 74HC595 attaque en ~50 Ω : elle ne coûte pas de temps de
stabilisation. Et le contact métal-métal est plus reproductible que le ruban.

> En revanche, un point de calibration à connaître dès l'étape 1 : le temps de stabilisation
> est fixé par le côté **poignet**, dont la capacité est dominée par celle du corps humain vers
> la terre, **~100-200 pF**. Avec ~300 kΩ de résistance corporelle, 3τ ≈ 180 µs. Le
> `SETTLE_US = 100` de `config.h` risque donc d'être **trop bas**, pas trop haut. Si `ghost`
> monte au banc, monte-le à 200 avant de chercher ailleurs.

### Le câblage à travers les plis

Les tuiles ne résolvent pas ça. Les conducteurs franchissent toujours les charnières.

- **Pragmatique** : fil silicone souple 30 AWG, une boucle avec du mou à chaque charnière.
  Point de fatigue connu, à surveiller.
- **Élégant** : pétales en PCB rigide reliés par des sections flex — un **rigid-flex**. C'est
  littéralement le « thick panel origami » avec le câblage intégré. Cher.

---

## Géométrie visée

⚠️ Des tuiles sur **la jupe** — la paroi latérale d'un cylindre — feraient frapper un mur
vertical. On frappe un pan **par le dessus**.

Ce qu'il faut est un **tronc de cône inversé et très ouvert** (un entonnoir très plat) : une
approximation facettée honnête d'une cuvette de pan, qui se replie à plat en un anneau. C'est
aussi ce que produit un flasher déployé.

---

## Prochaine étape : le test A/B qui ne coûte rien

**Ne construis pas la structure pliante pour tester la tuile.** Colle une seule tuile rigide
sur le carton v0, à côté d'une pastille en ruban alu, et compare :

| Mesure | Comment | Ce qu'on cherche |
|---|---|---|
| **Rebond** | roulement à la main sur l'une puis l'autre | la tuile doit permettre un roulement nettement plus long sans fatigue |
| **Contacts perdus** | compteur `ghost` du transport Serial, 50 frappes sur chacune | doit rester à 0 sur les deux |
| **Régularité** | 50 frappes → 50 notes sur chacune | aucune frappe manquée |
| **Stabilisation** | baisser `SETTLE_US` jusqu'à ce que `ghost` monte | la tuile ne doit pas dégrader le seuil |

Tu réponds à « la tuile rigide vaut-elle le coup ? » en une soirée, sans avoir décidé du
pliage. Même discipline que pour la v0 : chaque question isolée, prouvée seule.

## Ce qu'il reste à trancher

- La cinématique exacte : pétales sur moyeu (parapluie) ou flasher enroulant ? Le calcul
  ci-dessus donne les 12 charnières, pas encore le mouvement.
- Le **verrouillage** en position déployée — c'est le vrai problème d'ingénierie, pas le
  pliage. Bague de tension type parapluie, ou bistabilité.
- Le coût réel d'une solution PCB par pétale.
