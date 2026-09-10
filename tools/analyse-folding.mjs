#!/usr/bin/env node
// ---------------------------------------------------------------------------
// analyse-folding — où peuvent passer les lignes de pli sans couper une note ?
//
// C'est LA contrainte qui pilote la phase 2 : un pli qui traverse une pastille
// la déchire et rend son contact intermittent. Plutôt que de le juger à l'œil
// sur le gabarit, on le calcule.
//
//   node tools/analyse-folding.mjs --svg ../panisto/src/instruments/tenor_opt.svg
//
// Ce que ça produit :
//   - les couloirs angulaires libres (aucune note traversée)
//   - pour un moyeu central de rayon r0, combien de rayons régulièrement
//     espacés y tiennent, et avec quelle marge en mm
//   - out/folding.svg : le gabarit avec les rayons retenus, à superposer
//
// Options :
//   --svg <path>    SVG d'instrument
//   --spokes <n>    nombre de rayons visés (défaut : 12, la symétrie du tenor)
//   --scale <f>     échelle d'impression (défaut : 0.847, taille réelle)
//   --out <dir>     dossier de sortie (défaut : out/)
// ---------------------------------------------------------------------------

import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const argv = process.argv.slice(2);
const arg = (n, d) => { const i = argv.indexOf('--' + n); return i >= 0 && argv[i + 1] ? argv[i + 1] : d; };

const SVG_PATH = resolve(arg('svg', join(ROOT, '..', 'panisto', 'src', 'instruments', 'tenor_opt.svg')));
const SPOKES = Number(arg('spokes', 12));
const SCALE = Number(arg('scale', 0.847));
const OUT = resolve(arg('out', join(ROOT, 'out')));

const ANGLE_STEP = 0.25;   // degrés
const CURVE_SEGS = 16;     // segments par courbe de Bézier

// --- Lecture du SVG ---------------------------------------------------------

let src;
try { src = readFileSync(SVG_PATH, 'utf8'); }
catch { console.error(`Introuvable : ${SVG_PATH}\nClone panisto :  gh repo clone davvdg/panisto ../panisto`); process.exit(1); }

const layer = /<g[^>]*id="layer1"[^>]*transform="translate\(([^)]+)\)"/.exec(src);
const [TX, TY] = layer ? layer[1].split(/[,\s]+/).map(Number) : [0, 0];

const NOTE_ID_RE = /^(Dod|Mib|Fad|Sold|Sib|Do|Re|Mi|Fa|Sol|La|Si)(\d+)$/;
const NOTES_LAT = ['Do', 'Dod', 'Re', 'Mib', 'Mi', 'Fa', 'Fad', 'Sol', 'Sold', 'La', 'Sib', 'Si'];
const NOTES_EN = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'G#', 'A', 'Bb', 'B'];

// Aplatit un chemin en polygone. Contrairement à gen-template (qui ne veut
// qu'un centroïde), il faut ici la vraie frontière : c'est elle qu'un pli ne
// doit pas traverser. On échantillonne donc les Bézier.
function flatten(d) {
  const pts = [];
  const tok = d.match(/[a-zA-Z]|-?\d*\.?\d+(?:e[-+]?\d+)?/gi) || [];
  let i = 0, x = 0, y = 0, sx = 0, sy = 0, px = 0, py = 0, cmd = '';
  const n = () => Number(tok[i++]);
  const bez = (x0, y0, x1, y1, x2, y2, x3, y3) => {
    for (let k = 1; k <= CURVE_SEGS; k++) {
      const t = k / CURVE_SEGS, u = 1 - t;
      pts.push([
        u * u * u * x0 + 3 * u * u * t * x1 + 3 * u * t * t * x2 + t * t * t * x3,
        u * u * u * y0 + 3 * u * u * t * y1 + 3 * u * t * t * y2 + t * t * t * y3,
      ]);
    }
  };

  while (i < tok.length) {
    if (/[a-zA-Z]/.test(tok[i])) cmd = tok[i++];
    if (i >= tok.length) break;
    const rel = cmd === cmd.toLowerCase();
    const L = cmd.toLowerCase();
    if (L === 'm') {
      x = rel ? x + n() : n(); y = rel ? y + n() : n();
      sx = x; sy = y; px = x; py = y;
      pts.push([x, y]);
      cmd = rel ? 'l' : 'L';
    } else if (L === 'l') {
      x = rel ? x + n() : n(); y = rel ? y + n() : n();
      px = x; py = y; pts.push([x, y]);
    } else if (L === 'h') { x = rel ? x + n() : n(); px = x; py = y; pts.push([x, y]); }
    else if (L === 'v') { y = rel ? y + n() : n(); px = x; py = y; pts.push([x, y]); }
    else if (L === 'c' || L === 's') {
      let c1x, c1y;
      if (L === 'c') { c1x = rel ? x + n() : n(); c1y = rel ? y + n() : n(); }
      else { c1x = 2 * x - px; c1y = 2 * y - py; }          // réflexion du contrôle précédent
      const c2x = rel ? x + n() : n(), c2y = rel ? y + n() : n();
      const ex = rel ? x + n() : n(), ey = rel ? y + n() : n();
      bez(x, y, c1x, c1y, c2x, c2y, ex, ey);
      px = c2x; py = c2y; x = ex; y = ey;
    } else if (L === 'z') {
      pts.push([sx, sy]); x = sx; y = sy; px = x; py = y;
    } else { i++; }
  }
  return pts;
}

const notes = [];
for (const m of src.matchAll(/<path[^>]*id="([^"]+)"[^>]*\sd="([^"]+)"/g)) {
  const id = m[1].replace(/-\d+$/, '');
  const g = NOTE_ID_RE.exec(id);
  if (!g) continue;
  const poly = flatten(m[2]).map(([x, y]) => [(x + TX) * SCALE, (y + TY) * SCALE]);
  if (poly.length < 3) continue;
  notes.push({ id, name: NOTES_EN[NOTES_LAT.indexOf(g[1])] + g[2], poly, d: m[2] });
}

const circles = [...src.matchAll(/<circle[^>]*cx="([^"]+)"[^>]*cy="([^"]+)"[^>]*r="([^"]+)"/g)]
  .map(m => ({ cx: (+m[1] + TX) * SCALE, cy: (+m[2] + TY) * SCALE, r: +m[3] * SCALE }))
  .sort((a, b) => b.r - a.r);
const C = circles[0];
const RMAX = C.r * 1.02;

// --- Géométrie --------------------------------------------------------------

const dirOf = deg => { const a = deg * Math.PI / 180; return [Math.sin(a), -Math.cos(a)]; };

function inside(poly, [px, py]) {
  let hit = false;
  for (let i = 0, j = poly.length - 1; i < poly.length; j = i++) {
    const [xi, yi] = poly[i], [xj, yj] = poly[j];
    if ((yi > py) !== (yj > py) && px < (xj - xi) * (py - yi) / (yj - yi) + xi) hit = !hit;
  }
  return hit;
}

// Le segment radial [r0, RMAX] à l'angle `deg` traverse-t-il ce polygone ?
function blocks(poly, deg, r0) {
  const [ux, uy] = dirOf(deg);
  if (inside(poly, [C.cx + ux * r0, C.cy + uy * r0])) return true;
  for (let i = 0, j = poly.length - 1; i < poly.length; j = i++) {
    const [ax, ay] = poly[j], [bx, by] = poly[i];
    const ex = bx - ax, ey = by - ay;
    const den = ux * ey - uy * ex;
    if (Math.abs(den) < 1e-12) continue;
    const wx = ax - C.cx, wy = ay - C.cy;
    const t = (wx * ey - wy * ex) / den;          // distance le long du rayon
    const s = (wx * uy - wy * ux) / den;          // position sur l'arête
    if (s >= 0 && s <= 1 && t >= r0 && t <= RMAX) return true;
  }
  return false;
}

// Marge : distance minimale entre la ligne de pli et le contour le plus proche.
// C'est le vrai chiffre utile — la largeur de matière disponible pour une
// charnière, pas la largeur angulaire du couloir.
function clearance(deg, r0) {
  const [ux, uy] = dirOf(deg);
  let best = Infinity;
  for (const note of notes) {
    for (const [x, y] of note.poly) {
      const wx = x - C.cx, wy = y - C.cy;
      const t = Math.max(r0, Math.min(RMAX, wx * ux + wy * uy));
      best = Math.min(best, Math.hypot(wx - ux * t, wy - uy * t));
    }
  }
  return best;
}

// --- Balayage ---------------------------------------------------------------

const centreCovered = notes.some(nt => inside(nt.poly, [C.cx, C.cy]));

function freeAngles(r0) {
  const free = [];
  for (let a = 0; a < 360; a += ANGLE_STEP) {
    if (!notes.some(nt => blocks(nt.poly, a, r0))) free.push(a);
  }
  return free;
}

// Rayon de moyeu minimal permettant SPOKES rayons régulièrement espacés.
function bestOffset(r0) {
  const free = new Set(freeAngles(r0).map(a => a.toFixed(2)));
  const isFree = a => free.has((((a % 360) + 360) % 360).toFixed(2));
  let best = { count: -1, offset: 0 };
  for (let off = 0; off < 360 / SPOKES; off += ANGLE_STEP) {
    let c = 0;
    for (let k = 0; k < SPOKES; k++) if (isFree(off + k * 360 / SPOKES)) c++;
    if (c > best.count) best = { count: c, offset: off };
  }
  return best;
}

// --- Champ de distance aux contours ----------------------------------------
// Une droite radiale ne passe quasiment jamais (voir plus bas). Il faut donc
// des chemins en ZIGZAG qui se faufilent dans les interstices. Pour les
// trouver, on rastérise les notes puis on calcule, pour chaque point libre, sa
// distance au contour le plus proche : c'est la largeur de matière disponible.

const GRID = 0.5;                       // mm par cellule (0.5 : convergé ; 1 est optimiste de ~0,3 mm)
const N = Math.ceil(2 * RMAX / GRID);
const OX = C.cx - RMAX, OY = C.cy - RMAX;
const idx = (i, j) => j * N + i;
const cellR = (i, j) => Math.hypot(OX + (i + .5) * GRID - C.cx, OY + (j + .5) * GRID - C.cy);

const occ = new Uint8Array(N * N);
for (const nt of notes) {                // rastérisation par balayage de lignes
  const ys = nt.poly.map(p => p[1]);
  const j0 = Math.max(0, Math.floor((Math.min(...ys) - OY) / GRID));
  const j1 = Math.min(N - 1, Math.ceil((Math.max(...ys) - OY) / GRID));
  for (let j = j0; j <= j1; j++) {
    const y = OY + (j + .5) * GRID;
    const xs = [];
    for (let a = 0, b = nt.poly.length - 1; a < nt.poly.length; b = a++) {
      const [x1, y1] = nt.poly[b], [x2, y2] = nt.poly[a];
      if ((y1 > y) === (y2 > y)) continue;
      xs.push(x1 + (y - y1) / (y2 - y1) * (x2 - x1));
    }
    xs.sort((p, q) => p - q);
    for (let k = 0; k + 1 < xs.length; k += 2) {
      const i0 = Math.max(0, Math.ceil((xs[k] - OX) / GRID - .5));
      const i1 = Math.min(N - 1, Math.floor((xs[k + 1] - OX) / GRID - .5));
      for (let i = i0; i <= i1; i++) occ[idx(i, j)] = 1;
    }
  }
}

// Transformée de distance par chamfer 3-4 (deux passes), en tiers de mm.
const BIG = 1 << 28;
const dt = new Int32Array(N * N).fill(BIG);
for (let k = 0; k < N * N; k++) if (occ[k]) dt[k] = 0;
const relax = (k, from, w) => { const v = dt[from] + w; if (v < dt[k]) dt[k] = v; };
for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) {
  const k = idx(i, j);
  if (j > 0) relax(k, idx(i, j - 1), 3);
  if (i > 0) relax(k, idx(i - 1, j), 3);
  if (i > 0 && j > 0) relax(k, idx(i - 1, j - 1), 4);
  if (i < N - 1 && j > 0) relax(k, idx(i + 1, j - 1), 4);
}
for (let j = N - 1; j >= 0; j--) for (let i = N - 1; i >= 0; i--) {
  const k = idx(i, j);
  if (j < N - 1) relax(k, idx(i, j + 1), 3);
  if (i < N - 1) relax(k, idx(i + 1, j), 3);
  if (i < N - 1 && j < N - 1) relax(k, idx(i + 1, j + 1), 4);
  if (i > 0 && j < N - 1) relax(k, idx(i - 1, j + 1), 4);
}
// Distance signée : il faut aussi savoir de combien on est À L'INTÉRIEUR d'une
// note, pour pouvoir simuler des tuiles rétrécies. Les contours dessinés par
// panisto sont une convention graphique, pas une cote : rien n'oblige la tuile
// à remplir exactement la forme.
const dtIn = new Int32Array(N * N).fill(BIG);
for (let k = 0; k < N * N; k++) if (!occ[k]) dtIn[k] = 0;
const relaxIn = (k, from, w) => { const v = dtIn[from] + w; if (v < dtIn[k]) dtIn[k] = v; };
for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) {
  const k = idx(i, j);
  if (j > 0) relaxIn(k, idx(i, j - 1), 3);
  if (i > 0) relaxIn(k, idx(i - 1, j), 3);
  if (i > 0 && j > 0) relaxIn(k, idx(i - 1, j - 1), 4);
  if (i < N - 1 && j > 0) relaxIn(k, idx(i + 1, j - 1), 4);
}
for (let j = N - 1; j >= 0; j--) for (let i = N - 1; i >= 0; i--) {
  const k = idx(i, j);
  if (j < N - 1) relaxIn(k, idx(i, j + 1), 3);
  if (i < N - 1) relaxIn(k, idx(i + 1, j), 3);
  if (i < N - 1 && j < N - 1) relaxIn(k, idx(i + 1, j + 1), 4);
  if (i > 0 && j < N - 1) relaxIn(k, idx(i - 1, j + 1), 4);
}

// INSET : de combien chaque tuile est rentrée par rapport au contour dessiné.
// Chaque mm de retrait élargit tous les couloirs de 1 mm de chaque côté.
const INSET = Number(arg('inset', 0));
const clearanceAt = k => (occ[k] ? -(dtIn[k] / 3) * GRID : (dt[k] / 3) * GRID) + INSET;

// Chemin « maximin » : la route du moyeu au bord qui MAXIMISE son passage le
// plus étroit. Dijkstra classique, avec max/min au lieu de somme.
//
// Deux contraintes, apprises à la dure — sans elles le solveur triche :
//
//  - MONOTONE EN RAYON : une charnière va du moyeu vers le bord. Sans ça, la
//    route sort du moyeu par le premier interstice venu puis longe le pourtour
//    extérieur (qui est dégagé) pour rejoindre n'importe quel point du bord.
//    On obtient 12 « routes » qui n'en sont qu'une.
//
//  - DISJOINTES : chaque charnière consomme de la matière. On bloque donc le
//    couloir utilisé avant de chercher la suivante, sinon elles se superposent.
function widestPath(r0, blocked) {
  const val = new Float64Array(N * N);
  const par = new Int32Array(N * N).fill(-1);
  const done = new Uint8Array(N * N);
  const heap = [];
  const push = (k, v) => {
    heap.push([v, k]); let c = heap.length - 1;
    while (c > 0) { const p = (c - 1) >> 1; if (heap[p][0] >= heap[c][0]) break; [heap[p], heap[c]] = [heap[c], heap[p]]; c = p; }
  };
  const pop = () => {
    const top = heap[0], last = heap.pop();
    if (heap.length) { heap[0] = last; let c = 0;
      for (;;) { const l = 2 * c + 1, r = l + 1; let m = c;
        if (l < heap.length && heap[l][0] > heap[m][0]) m = l;
        if (r < heap.length && heap[r][0] > heap[m][0]) m = r;
        if (m === c) break; [heap[m], heap[c]] = [heap[c], heap[m]]; c = m; } }
    return top;
  };

  // Le moyeu est un disque rigide qui ne se plie pas : les notes qui s'y
  // trouvent ne gênent pas, les routes en partent librement.
  for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) {
    if (cellR(i, j) <= r0) { const k = idx(i, j); val[k] = Infinity; push(k, Infinity); }
  }

  let best = { v: 0, k: -1 };
  while (heap.length) {
    const [v, k] = pop();
    if (done[k] || v < val[k]) continue;
    done[k] = 1;
    const i = k % N, j = (k / N) | 0;
    const r = cellR(i, j);
    if (r >= RMAX - GRID * 1.5 && v > best.v) best = { v, k };
    for (let dj = -1; dj <= 1; dj++) for (let di = -1; di <= 1; di++) {
      if (!di && !dj) continue;
      const ni = i + di, nj = j + dj;
      if (ni < 0 || nj < 0 || ni >= N || nj >= N) continue;
      const nr = cellR(ni, nj);
      if (nr > RMAX) continue;
      if (nr < r) continue;                    // monotone : toujours vers le bord
      const nk = idx(ni, nj);
      if (blocked[nk]) continue;
      const c = clearanceAt(nk);
      if (c <= 0) continue;                    // dans une note : infranchissable
      const nv = Math.min(v, c);
      if (nv > val[nk]) { val[nk] = nv; par[nk] = k; push(nk, nv); }
    }
  }

  if (best.k < 0) return null;
  const pts = [];
  for (let k = best.k; k >= 0; k = par[k]) {
    const i = k % N, j = (k / N) | 0;
    pts.push([OX + (i + .5) * GRID, OY + (j + .5) * GRID, i, j]);
    if (cellR(i, j) <= r0) break;
  }
  return { w: best.v, pts: pts.reverse() };
}

console.log(`\n  source     ${SVG_PATH}`);
console.log(`  echelle    ${SCALE}   pan ${(C.r * 2 / 10).toFixed(1)} cm de diametre`);
console.log(`  notes      ${notes.length}`);
console.log(`  centre     ${centreCovered ? 'COUVERT par une note' : 'libre (pas de note au centre exact)'}`);

// --- 1. Plis DROITS ---------------------------------------------------------
// C'est ce qu'exige l'origami rigide : une feuille pliée a besoin de plis
// rectilignes. Verdict ci-dessous.

const straight = [0, 40, 80, 120].map(r0 => ({ r0, ...bestOffset(r0) }));
const totalFree = freeAngles(0).length;

console.log(`\n  --- Plis DROITS (contrainte de l'origami rigide) ---`);
console.log(`  directions radiales ne coupant AUCUNE note : ${totalFree} / ${(360 / ANGLE_STEP)}`);
for (const s of straight) {
  console.log(`  moyeu ${String(s.r0).padStart(3)} mm  ->  ${s.count} / ${SPOKES} rayons reguliers`);
}
if (!straight.some(s => s.count === SPOKES)) {
  console.log(`  => IMPOSSIBLE. Les deux anneaux de notes sont decales angulairement :`);
  console.log(`     une droite qui passe entre deux notes exterieures tape une note du milieu.`);
}

// --- 2. Plis en ZIGZAG ------------------------------------------------------
// Dès que les notes sont des TUILES SÉPARÉES, la charnière n'a plus besoin
// d'être droite : elle peut se faufiler entre elles. C'est tout ce que ça change.

const R0 = Number(arg('hub', 60));
const HINGE = Number(arg('hinge', 3));   // largeur de matière consommée par une charnière, mm

const blocked = new Uint8Array(N * N);
const paths = [];
for (let round = 0; round < SPOKES; round++) {
  const p = widestPath(R0, blocked);
  if (!p || p.w < 0.5) break;
  paths.push(p);
  // La charnière consomme HINGE mm de large : on interdit ce couloir aux suivantes.
  const rad = Math.ceil((HINGE / 2) / GRID);
  for (const [, , i, j] of p.pts) {
    for (let dj = -rad; dj <= rad; dj++) for (let di = -rad; di <= rad; di++) {
      const ni = i + di, nj = j + dj;
      if (ni < 0 || nj < 0 || ni >= N || nj >= N) continue;
      if (Math.hypot(di, dj) * GRID <= HINGE / 2) blocked[idx(ni, nj)] = 1;
    }
  }
}

console.log(`\n  --- Plis en ZIGZAG (tuiles rigides separees) ---`);
console.log(`  moyeu ${R0} mm   charniere ${HINGE} mm   retrait de tuile ${INSET} mm`);
console.log(`  charnieres DISJOINTES moyeu -> bord : ${paths.length} / ${SPOKES}`);
if (paths.length) {
  console.log(`  passage le plus etroit de chacune (mm) :`);
  console.log(`    ${paths.map(p => p.w.toFixed(1).padStart(4)).join(' ')}`);
  console.log(`  goulot global : ${Math.min(...paths.map(p => p.w)).toFixed(1)} mm`);
}

// --- Sortie graphique -------------------------------------------------------

mkdirSync(OUT, { recursive: true });
const W = RMAX * 2;
writeFileSync(join(OUT, 'folding.svg'), `<svg xmlns="http://www.w3.org/2000/svg"
  width="${W.toFixed(1)}mm" height="${W.toFixed(1)}mm" viewBox="${OX.toFixed(1)} ${OY.toFixed(1)} ${W.toFixed(1)} ${W.toFixed(1)}">
<style>
  .note{fill:none;stroke:#000;stroke-width:.4}
  .rim{fill:none;stroke:#000;stroke-width:.6;stroke-dasharray:4 3}
  .hub{fill:none;stroke:#c2410c;stroke-width:1;stroke-dasharray:3 2}
  .fold{fill:none;stroke:#c2410c;stroke-width:${HINGE};stroke-opacity:.35;stroke-linejoin:round;stroke-linecap:round}
  .foldc{fill:none;stroke:#c2410c;stroke-width:.6;stroke-linejoin:round;stroke-linecap:round}
  .lbl{font:6px sans-serif;fill:#c2410c;text-anchor:middle}
</style>
<g transform="scale(${SCALE}) translate(${TX} ${TY})">
  <circle cx="${(C.cx / SCALE - TX).toFixed(2)}" cy="${(C.cy / SCALE - TY).toFixed(2)}" r="${(C.r / SCALE).toFixed(2)}" class="rim"/>
  ${notes.map(nt => `<path d="${nt.d}" class="note"/>`).join('\n  ')}
</g>
<circle cx="${C.cx.toFixed(2)}" cy="${C.cy.toFixed(2)}" r="${R0}" class="hub"/>
${paths.map(p => {
  const pts = p.pts.map(([x, y]) => x.toFixed(1) + ',' + y.toFixed(1)).join(' ');
  return `<polyline class="fold" points="${pts}"/><polyline class="foldc" points="${pts}"/>`;
}).join('\n')}
${paths.map(p => {
  const [x, y] = p.pts[p.pts.length - 1];
  const a = Math.atan2(x - C.cx, -(y - C.cy));
  return `<text x="${(C.cx + Math.sin(a) * (RMAX + 8)).toFixed(1)}" y="${(C.cy - Math.cos(a) * (RMAX + 8)).toFixed(1)}" class="lbl">${p.w.toFixed(1)}</text>`;
}).join('\n')}
</svg>
`);
console.log(`\n  ecrit  ${OUT}/folding.svg   (charnieres en orange, a superposer au gabarit)\n`);
