#!/usr/bin/env node
// ---------------------------------------------------------------------------
// gen-template — génère, depuis UNE SEULE source, tout ce qui doit rester
// cohérent entre le carton, le firmware et l'application :
//
//   out/template.html      gabarit à imprimer, carrelé, échelle 1:1
//   out/template-1up.svg   le même en une seule feuille (traceur / A0)
//   out/padmap_tenor.h     table pad -> note MIDI, pour le firmware
//   out/padmap.json        la même, pour le web
//
// La source est le SVG d'instrument de panisto. C'est le point crucial du
// projet : si le mapping était saisi à la main d'un côté et dessiné à la main
// de l'autre, ils divergeraient, et déboguer « la note qui sonne faux » sur un
// carton de 70 cm est un enfer.
//
// Aucune dépendance npm : node tools/gen-template.mjs suffit.
//
//   node tools/gen-template.mjs --svg ../panisto/src/instruments/tenor_opt.svg
//
// Options :
//   --svg <path>     SVG d'instrument (défaut : ../panisto/src/instruments/tenor_opt.svg)
//   --offset <n>     tonalOffset de l'instrument (défaut : 12, celui du Tenor)
//   --order angle|pitch   ordre des index de pad (défaut : angle)
//   --scale <f>      échelle d'impression (défaut : 1). 0.85 ramène le tenor
//                    de 67,5 cm à ~57 cm, la taille d'un vrai instrument.
//   --page a4|a3|a0  format des feuilles du gabarit carrelé (défaut : a4)
//   --out <dir>      dossier de sortie (défaut : out/)
// ---------------------------------------------------------------------------

import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');

// --- Arguments --------------------------------------------------------------

const argv = process.argv.slice(2);
const arg = (name, def) => {
  const i = argv.indexOf('--' + name);
  return i >= 0 && argv[i + 1] ? argv[i + 1] : def;
};

const SVG_PATH = resolve(arg('svg', join(ROOT, '..', 'panisto', 'src', 'instruments', 'tenor_opt.svg')));
const TONAL_OFFSET = Number(arg('offset', 12));
const ORDER = arg('order', 'angle');
const SCALE = Number(arg('scale', 1));
const PAGE = arg('page', 'a4').toLowerCase();
const OUT = resolve(arg('out', join(ROOT, 'out')));

const PAGES = {
  a4: { w: 210, h: 297 },
  a3: { w: 297, h: 420 },
  a0: { w: 841, h: 1189 },
};
const MARGIN = 10;   // marge non imprimable, mm
const OVERLAP = 10;  // recouvrement entre feuilles, mm — sert à raccorder

// --- Table de notes de panisto ----------------------------------------------
// Reprise telle quelle de src/features/Player.tsx : notesLat / spectrumLat.
// midiVal = spectrumLat.indexOf(id) + tonalOffset, donc :
//           midiVal = octave * 12 + index(nom) + tonalOffset
const NOTES_LAT = ['Do', 'Dod', 'Re', 'Mib', 'Mi', 'Fa', 'Fad', 'Sol', 'Sold', 'La', 'Sib', 'Si'];
const NOTES_EN = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'G#', 'A', 'Bb', 'B'];
const NOTE_ID_RE = /^(Dod|Mib|Fad|Sold|Sib|Do|Re|Mi|Fa|Sol|La|Si)(\d+)$/;

// --- Mini-parseur SVG -------------------------------------------------------
// Le SVG d'instrument est plat et régulier (paths et circles enfants directs de
// #layer1, un seul transform sur le calque). Une extraction ciblée suffit et
// évite d'imposer une dépendance DOM à un script qu'on lance trois fois.

function parseSvg(src) {
  const viewBox = /viewBox="([^"]+)"/.exec(src)?.[1] || '0 0 100 100';
  const [vx, vy, vw, vh] = viewBox.trim().split(/[\s,]+/).map(Number);

  const layer = /<g[^>]*id="layer1"[^>]*transform="translate\(([^)]+)\)"/.exec(src);
  const [tx, ty] = layer ? layer[1].split(/[,\s]+/).map(Number) : [0, 0];

  const paths = [];
  for (const m of src.matchAll(/<path[^>]*id="([^"]+)"[^>]*\sd="([^"]+)"/g)) {
    paths.push({ id: m[1].replace(/-\d+$/, ''), d: m[2] });
  }

  const circles = [];
  for (const m of src.matchAll(/<circle[^>]*cx="([^"]+)"[^>]*cy="([^"]+)"[^>]*r="([^"]+)"/g)) {
    circles.push({ cx: +m[1], cy: +m[2], r: +m[3] });
  }

  return { vx, vy, vw, vh, tx, ty, paths, circles };
}

// Extrait les points d'ancrage d'un chemin. On ignore les points de contrôle
// des courbes : pour un centroïde de placement d'étiquette et une bbox, les
// ancres suffisent largement sur ces formes arrondies.
function pathPoints(d) {
  const pts = [];
  let x = 0, y = 0, sx = 0, sy = 0, cmd = '';
  const tok = d.match(/[a-zA-Z]|-?\d*\.?\d+(?:e[-+]?\d+)?/gi) || [];
  let i = 0;
  const num = () => Number(tok[i++]);

  while (i < tok.length) {
    if (/[a-zA-Z]/.test(tok[i])) cmd = tok[i++];
    if (i >= tok.length) break;
    const rel = cmd === cmd.toLowerCase();
    switch (cmd.toLowerCase()) {
      case 'm':
        x = rel ? x + num() : num();
        y = rel ? y + num() : num();
        sx = x; sy = y;
        pts.push([x, y]);
        cmd = rel ? 'l' : 'L';  // un m implicite enchaîne en lineto
        break;
      case 'l':
        x = rel ? x + num() : num();
        y = rel ? y + num() : num();
        pts.push([x, y]);
        break;
      case 'h': x = rel ? x + num() : num(); pts.push([x, y]); break;
      case 'v': y = rel ? y + num() : num(); pts.push([x, y]); break;
      case 'c':
        i += 4;  // deux points de contrôle, ignorés
        x = rel ? x + num() : num();
        y = rel ? y + num() : num();
        pts.push([x, y]);
        break;
      case 's':
      case 'q':
        i += 2;
        x = rel ? x + num() : num();
        y = rel ? y + num() : num();
        pts.push([x, y]);
        break;
      case 't':
        x = rel ? x + num() : num();
        y = rel ? y + num() : num();
        pts.push([x, y]);
        break;
      case 'a':
        i += 5;
        x = rel ? x + num() : num();
        y = rel ? y + num() : num();
        pts.push([x, y]);
        break;
      case 'z':
        x = sx; y = sy;
        break;
      default:
        i++;  // commande inconnue : on avance pour ne pas boucler
    }
  }
  return pts;
}

// --- Construction du modèle -------------------------------------------------

const svgSrc = (() => {
  try {
    return readFileSync(SVG_PATH, 'utf8');
  } catch {
    console.error(`\nIntrouvable : ${SVG_PATH}\n`);
    console.error('Ce script lit le SVG d\'instrument de panisto. Clone-le a cote :');
    console.error('    gh repo clone davvdg/panisto ../panisto\n');
    console.error('ou indique le fichier :  --svg <chemin/vers/tenor_opt.svg>\n');
    process.exit(1);
  }
})();

const svg = parseSvg(svgSrc);

const notes = [];
for (const p of svg.paths) {
  const m = NOTE_ID_RE.exec(p.id);
  if (!m) continue;
  const nameIdx = NOTES_LAT.indexOf(m[1]);
  const octave = Number(m[2]);
  const midi = octave * 12 + nameIdx + TONAL_OFFSET;

  const pts = pathPoints(p.d).map(([x, y]) => [x + svg.tx, y + svg.ty]);
  if (!pts.length) continue;
  const cx = pts.reduce((s, q) => s + q[0], 0) / pts.length;
  const cy = pts.reduce((s, q) => s + q[1], 0) / pts.length;
  const xs = pts.map(q => q[0]), ys = pts.map(q => q[1]);

  notes.push({
    id: p.id, d: p.d, midi, octave,
    name: NOTES_EN[nameIdx] + octave,
    cx, cy,
    w: Math.max(...xs) - Math.min(...xs),
    h: Math.max(...ys) - Math.min(...ys),
  });
}

if (!notes.length) {
  console.error(`Aucune note reconnue dans ${SVG_PATH}. Les ids attendus ressemblent a "Do4", "Fad5".`);
  process.exit(1);
}

// Centre du pan : le plus grand cercle du dessin.
const big = svg.circles.slice().sort((a, b) => b.r - a.r)[0];
const centre = big
  ? { x: big.cx + svg.tx, y: big.cy + svg.ty, r: big.r }
  : { x: svg.vx + svg.vw / 2, y: svg.vy + svg.vh / 2, r: svg.vw / 2 };

// Ordre des index de pad.
//
//   angle : dans le sens horaire depuis midi. Les pads voisins sur la chaine de
//           595 sont voisins sur le carton, donc la nappe suit le bord du pan
//           sans croisement. C'est ce qui rend le cablage tenable.
//   pitch : par hauteur croissante. Plus simple a verifier a l'oreille, mais le
//           cablage saute d'un bout a l'autre du pan a chaque note.
for (const n of notes) {
  const dx = n.cx - centre.x, dy = n.cy - centre.y;
  n.angle = (Math.atan2(dx, -dy) + 2 * Math.PI) % (2 * Math.PI);
  n.radius = Math.hypot(dx, dy);
}
notes.sort(ORDER === 'pitch'
  ? (a, b) => a.midi - b.midi
  : (a, b) => (a.angle - b.angle) || (b.radius - a.radius));
notes.forEach((n, i) => { n.pad = i; });

// --- Sorties firmware / web -------------------------------------------------

mkdirSync(OUT, { recursive: true });

const header = [
  '// GÉNÉRÉ par tools/gen-template.mjs — NE PAS ÉDITER À LA MAIN.',
  `// Source : ${SVG_PATH}`,
  `// tonalOffset=${TONAL_OFFSET}  ordre=${ORDER}  ${notes.length} notes`,
  '//',
  '// Régénère-le en même temps que le gabarit imprimé : c\'est ce qui garantit',
  '// que le carton, le firmware et panisto parlent des mêmes notes.',
  '#pragma once',
  '#include <stdint.h>',
  '',
  'static constexpr uint8_t PAD_MIDI_NOTE[] = {',
  ...notes.map(n =>
    `    ${String(n.midi).padStart(3)},  // pad ${String(n.pad).padStart(2)}` +
    ` -> 595 n°${Math.floor(n.pad / 8)} Q${'ABCDEFGH'[n.pad % 8]}` +
    `  ${n.name.padEnd(4)} (${n.id})`),
  '};',
  '',
].join('\n');
writeFileSync(join(OUT, 'padmap_tenor.h'), header);

writeFileSync(join(OUT, 'padmap.json'), JSON.stringify({
  source: SVG_PATH,
  tonalOffset: TONAL_OFFSET,
  order: ORDER,
  scale: SCALE,
  pads: notes.map(n => ({
    pad: n.pad, midi: n.midi, id: n.id, name: n.name,
    sr: Math.floor(n.pad / 8), out: 'ABCDEFGH'[n.pad % 8],
    cx: +n.cx.toFixed(2), cy: +n.cy.toFixed(2),
  })),
}, null, 2));

// --- Gabarit ----------------------------------------------------------------

const S = SCALE;
const W = svg.vw * S, H = svg.vh * S;

// Le contenu, en coordonnées millimétriques finales. Réutilisé tel quel par la
// feuille unique et par chaque carreau — seul le viewBox change.
function content() {
  const g = [];
  g.push(`<g transform="scale(${S}) translate(${svg.tx} ${svg.ty})">`);
  g.push(`<circle cx="${big?.cx ?? 0}" cy="${big?.cy ?? 0}" r="${big?.r ?? 0}" class="rim"/>`);
  for (const n of notes) g.push(`<path d="${n.d}" class="note"/>`);
  g.push('</g>');

  for (const n of notes) {
    const x = (n.cx * S).toFixed(2), y = (n.cy * S).toFixed(2);
    // Taille d'étiquette bornée par la plus petite dimension de la note, pour
    // que le texte ne déborde jamais des petites notes aiguës.
    const fs = Math.max(5, Math.min(13, Math.min(n.w, n.h) * S * 0.28));
    g.push(`<text x="${x}" y="${y}" class="pad" font-size="${fs.toFixed(1)}">${n.pad}</text>`);
    g.push(`<text x="${x}" y="${(+y + fs * 0.95).toFixed(2)}" class="lbl" font-size="${(fs * 0.5).toFixed(1)}">${n.name} · ${n.midi}</text>`);
  }
  return g.join('\n');
}

// Règle de contrôle : SANS elle, une impression à 96 % passe inaperçue et tout
// le carton est faux. À mesurer au réglet avant de découper quoi que ce soit.
function ruler(x, y) {
  const t = [`<g transform="translate(${x} ${y})">`,
    `<rect x="0" y="0" width="100" height="6" class="ruler"/>`];
  for (let i = 0; i <= 100; i += 10) t.push(`<line x1="${i}" y1="0" x2="${i}" y2="6" class="ruler"/>`);
  t.push(`<text x="50" y="12.5" class="ruler-txt" font-size="4">100 mm — mesure-moi avant de découper</text>`, '</g>');
  return t.join('');
}

const STYLE = `
  .note { fill: none; stroke: #000; stroke-width: .5; }
  .rim  { fill: none; stroke: #000; stroke-width: .8; stroke-dasharray: 4 3; }
  .pad  { text-anchor: middle; font-family: sans-serif; font-weight: 700; fill: #000; }
  .lbl  { text-anchor: middle; font-family: sans-serif; fill: #666; }
  .ruler{ fill: none; stroke: #000; stroke-width: .3; }
  .ruler-txt { text-anchor: middle; font-family: sans-serif; fill: #000; }
  .crop { fill: none; stroke: #999; stroke-width: .3; stroke-dasharray: 2 2; }
`;

writeFileSync(join(OUT, 'template-1up.svg'),
  `<svg xmlns="http://www.w3.org/2000/svg" width="${W + 40}mm" height="${H + 40}mm"
     viewBox="${-20} ${-20} ${W + 40} ${H + 40}">
<style>${STYLE}</style>
${content()}
${ruler(-10, H + 8)}
</svg>
`);

// Carrelage : chaque feuille est le MÊME contenu vu par un viewBox différent.
const page = PAGES[PAGE] || PAGES.a4;
const usable = { w: page.w - 2 * MARGIN, h: page.h - 2 * MARGIN };
const step = { w: usable.w - OVERLAP, h: usable.h - OVERLAP };
const cols = Math.max(1, Math.ceil(W / step.w));
const rows = Math.max(1, Math.ceil(H / step.h));

const tiles = [];
for (let r = 0; r < rows; r++) {
  for (let c = 0; c < cols; c++) {
    const x = c * step.w, y = r * step.h;
    tiles.push(`<section>
  <div class="tag">${PAGE.toUpperCase()} — ligne ${r + 1}/${rows}, colonne ${c + 1}/${cols}</div>
  <svg xmlns="http://www.w3.org/2000/svg"
       width="${usable.w}mm" height="${usable.h}mm"
       viewBox="${x} ${y} ${usable.w} ${usable.h}">
    <style>${STYLE}</style>
    ${content()}
    ${r === rows - 1 && c === 0 ? ruler(x + 5, y + usable.h - 18) : ''}
    <rect x="${x + .5}" y="${y + .5}" width="${usable.w - 1}" height="${usable.h - 1}" class="crop"/>
  </svg>
</section>`);
  }
}

writeFileSync(join(OUT, 'template.html'), `<!doctype html>
<meta charset="utf-8">
<title>Gabarit pan-on-the-go</title>
<style>
  @page { size: ${PAGE}; margin: ${MARGIN}mm; }
  body { margin: 0; font: 13px/1.5 system-ui, sans-serif; }
  section { page-break-after: always; break-after: page; }
  section:last-child { page-break-after: auto; break-after: auto; }
  .tag { font-size: 8pt; color: #888; margin-bottom: 2mm; }
  .intro { padding: 16px; max-width: 640px; }
  .intro li { margin-bottom: 6px; }
  @media print { .intro { display: none; } }
</style>
<div class="intro">
  <h1>Gabarit pan-on-the-go</h1>
  <p><strong>${notes.length} notes · ${rows * cols} feuilles ${PAGE.toUpperCase()} · échelle ${SCALE} ·
     ${(W / 10).toFixed(1)} × ${(H / 10).toFixed(1)} cm</strong></p>
  <ol>
    <li>Imprime en <strong>« Taille réelle » / « 100 % »</strong>, surtout pas « Ajuster à la page ».</li>
    <li><strong>Mesure la règle de 100 mm au réglet.</strong> Si elle ne fait pas 100 mm,
        l'impression est mise à l'échelle et tout le carton sera faux.</li>
    <li>Les feuilles se recouvrent de ${OVERLAP} mm : superpose les traits pointillés avant de scotcher.</li>
    <li>Le grand chiffre au centre de chaque note est son <strong>numéro de pad</strong>,
        c'est-à-dire sa sortie sur la chaîne de 74HC595. Le petit texte donne la note et sa valeur MIDI.</li>
  </ol>
</div>
${tiles.join('\n')}
`);

// --- Rapport ----------------------------------------------------------------

const lo = notes.reduce((a, b) => (a.midi < b.midi ? a : b));
const hi = notes.reduce((a, b) => (a.midi > b.midi ? a : b));
const smallest = notes.reduce((a, b) => (Math.min(a.w, a.h) < Math.min(b.w, b.h) ? a : b));

// Un pan est fabriqué dans un fût de 55 gallons : ~572 mm de diamètre. Les SVG
// de panisto sont dessinés plus larges (ils servent d'affichage, pas de plan).
// Comme tout l'intérêt du carton est de transférer la mémoire musculaire vers
// un VRAI instrument, la taille réelle n'est pas un détail.
const REAL_PAN_MM = 572;
const suggested = +(REAL_PAN_MM / (centre.r * 2)).toFixed(3);
const sizeHint = Math.abs(W - REAL_PAN_MM) < 15
  ? '  <- taille d\'un vrai tenor'
  : `\n  ATTENTION    un vrai tenor fait ~57 cm (fut de 55 gallons), ce gabarit ${(W / 10).toFixed(1)} cm.` +
    `\n               pour la taille reelle :  --scale ${suggested}`;

console.log(`
  source        ${SVG_PATH}
  notes         ${notes.length}  (${lo.name}/${lo.midi} -> ${hi.name}/${hi.midi})
  registres     ${Math.ceil(notes.length / 8)} x 74HC595  (${Math.ceil(notes.length / 8) * 8} sorties, ${Math.ceil(notes.length / 8) * 8 - notes.length} libres)
  ordre         ${ORDER}
  taille        ${(W / 10).toFixed(1)} x ${(H / 10).toFixed(1)} cm  (echelle ${SCALE})${sizeHint}
  + petite note ${(smallest.w * S).toFixed(0)} x ${(smallest.h * S).toFixed(0)} mm  (${smallest.name})
  impression    ${rows * cols} feuilles ${PAGE.toUpperCase()}

  ecrit dans ${OUT}/
    template.html      <- ouvre-le et imprime a 100 %
    template-1up.svg
    padmap_tenor.h     <- copie dans firmware/include/
    padmap.json

  puis dans firmware/platformio.ini :
    -D PAD_COUNT=${notes.length}  -D PADMAP_TENOR   (au lieu de PADMAP_BENCH)
`);
