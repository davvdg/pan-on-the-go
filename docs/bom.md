# Nomenclature

Environ **30 €** en partant d'un ESP32 déjà possédé. Beaucoup d'articles se vendent par lots ;
les prix ci-dessous sont ceux du lot.

## Électronique

| Qté | Article | ~Prix | Note |
|---:|---|---:|---|
| 1 | ESP32 WROOM-32 DevKit | – | déjà possédé |
| 4 | **74HC595** (DIP-16) | 2 € | registres à décalage, 3 fils → 32 sorties. Prends-en 6, c'est le même prix |
| 1 | Résistance **1 MΩ** | ~0 € | pull-down du bracelet. Prends aussi une 10 MΩ en secours |
| 2 | Résistance 220 Ω | ~0 € | en série sur SCK et MOSI, propreté des fronts |
| 4 | Condensateur 100 nF | ~0 € | découplage, un par 74HC595, au plus près de ses broches d'alim |
| 1 | Plaque à trous 7×9 cm | 2 € | les 4 puces + le découplage |
| 1 | Nappe IDC 34 points, 1 m | 4 € | 29 pads + masse et rab |
| 1 | Lot de fils Dupont F/F | 3 € | |

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
