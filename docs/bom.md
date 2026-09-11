# Nomenclature

Environ **30 €** en partant d'un ESP32 déjà possédé. Beaucoup d'articles se vendent par lots ;
les prix ci-dessous sont ceux du lot.

## Électronique

### Étape 1 — banc 4 pads (~10 €)

| Qté | Article | ~Prix | Note |
|---:|---|---:|---|
| 1 | ESP32 WROOM-32 DevKit (pont CP2102) | – | déjà possédé |
| 5 | **74HC595**, boîtier **DIP-16** | 2 € | 3 fils → 8 sorties, chaînables. 4 utiles + 1 rechange. ⚠️ Bien **HC**, pas **HCT** : le HCT n'est spécifié qu'à 5 V, on est en 3,3 V |
| 10 | Résistance **1 MΩ** ¼ W | ~0 € | pull-down du bracelet. Une seule sert, vendues par 10 |
| 10 | Résistance **220 Ω** ¼ W | ~0 € | en série sur SCK et MOSI, propreté des fronts |
| 10 | Condensateur **100 nF** céramique | ~0 € | découplage, un par 74HC595, au plus près de ses broches d'alim |
| 1 | Breadboard 830 points | 3 € | le 595 et ses résistances, sans souder |
| 1 | Lot de fils Dupont M/M (×40) | 2 € | breadboard ↔ ESP32 |
| 1 | Lot de pinces crocodile (×10) | 3 € | bracelet et pads de test sans rien souder |

### Étape 4 — passage à 29 pads (~12 €)

| Qté | Article | ~Prix | Note |
|---:|---|---:|---|
| 1 | Plaque à trous ~7×9 cm, pastilles cuivre | 2 € | les 4 × 595 en dur, une fois le banc validé |
| 4 | Support DIP-16 (tulipe) | 1 € | pour ne pas souder les 595 directement |
| 1 | Nappe 34 conducteurs, 1,5 m | 4 € | 29 pads + masse + rab. Ou 30 m de fil monobrin 22 AWG |
| 1 | Barrette femelle 2,54 mm sécable (×40) | 1 € | brancher l'ESP32 sur la plaque sans le souder |
| 1 | Chatterton | 2 € | isoler les pattes des attaches parisiennes au dos |

### À ne **pas** acheter

- **74HCT595** (5 V seulement) ni version SMD (la plaque à trous veut du DIP)
- **Level shifter** : tout est en 3,3 V d'un bout à l'autre
- **Pull-ups** sur les pads : ils sont pilotés, c'est tout l'intérêt de l'inversion

**Optionnel, pour plus tard**

| Qté | Article | ~Prix | Note |
|---:|---|---:|---|
| 1 | 74HC14 (Schmitt trigger) | 0,50 € | plan B si le bracelet donne un signal instable |
| 1 | Piézo 27 mm | 1 € | ajoute la vélocité, cf. architecture § upgrades |
| 1 | ESP32-**S3** DevKitC-1 | 8 € | le jour où tu veux l'USB-MIDI plug-and-play |

## Mécanique

| Qté | Article | ~Prix | Note |
|---:|---|---:|---|
| 1 | Carton double cannelure **70×70 cm** | 0 € | un carton de déménagement fait l'affaire |
| 1 | **Ruban alu adhésif** type gaine/HVAC, 50 mm × 25 m | 8 € | ⚠️ **pas** du papier alu de cuisine |
| 1 | Boîte d'**attaches parisiennes laiton** (×100) | 3 € | l'alu ne se soude pas — voir ci-dessous |
| 1 | Rouleau de ruban cuivre adhésif (optionnel) | 6 € | si tu préfères des pastilles soudables |
| 1 | Sangle velcro | 2 € | le bracelet |
| 2 | Baguettes de pan à **pointe laiton** | – | ou baguettes bois + embout laiton collé |
| – | Colle en bâton, cutter, réglet | – | |

## Les deux pièges matériels

**Le papier alu de cuisine ne tient pas.** Il se déchire, se froisse, et le contact devient
irrégulier là où il se soulève. Le ruban alu adhésif (celui des gaines de ventilation) est
épais, autocollant sur toute sa surface, et se coupe proprement au cutter.

**L'aluminium ne se soude pas.** Sa couche d'oxyde se reforme instantanément. La connexion
pad → fil se fait donc mécaniquement : une **attache parisienne en laiton** traverse la
pastille et le carton, on écarte ses pattes au dos et on y sertit (ou soude) le fil. C'est
solide, ça coûte 3 centimes et ça survit aux frappes.

> Si tu veux souder directement, utilise du **ruban cuivre** pour les pastilles. C'est plus
> cher (~6 € pour couvrir un tenor) mais soudable. Le laiton des attaches marche aussi très
> bien contre l'alu — c'est un contact métal-métal propre.

## Baguettes

Il faut un chemin conducteur continu **pointe → main**. Les baguettes de pan classiques ont
un manche bois et un embout caoutchouc : ni l'un ni l'autre ne conduit.

Le plus simple : une tige d'**aluminium ou laiton de 8 mm** (rayon quincaillerie), coupée à
25-30 cm, avec un embout laiton ou une bille d'acier collée à l'époxy chargée, et un bout de
gaine thermo **seulement sur la partie que la main ne touche pas**. La paume doit être en
contact avec le métal nu.

> ⚠️ Ne mets pas de manchon isolant là où tu tiens la baguette : c'est précisément ce
> contact-là qui ferme le circuit.
