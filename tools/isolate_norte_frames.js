const fs = require('fs');
const path = require('path');
const sharp = require(require.resolve('sharp', {
  paths: ['C:/Users/georg/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules'],
}));

const folder = 'C:/Desenvolvimento/Nintendo 64/Sprites/Norte_Boss';
const selected = process.argv[2];
const names = selected ? [selected] : fs.readdirSync(folder).filter((name) => name.endsWith('.png') && name !== 'k5_jarl_morte.png');

function components(data, width, height, threshold) {
  const labels = new Int32Array(width * height);
  const queue = new Int32Array(width * height);
  const parts = [];
  let label = 0;
  for (let start = 0; start < labels.length; start++) {
    if (labels[start] || data[start * 4 + 3] <= threshold) continue;
    const id = ++label;
    let head = 0;
    let tail = 0;
    queue[tail++] = start;
    labels[start] = id;
    const part = { id, count: 0, minX: width, minY: height, maxX: 0, maxY: 0 };
    while (head < tail) {
      const pos = queue[head++];
      const x = pos % width;
      const y = (pos / width) | 0;
      part.count++;
      if (x < part.minX) part.minX = x;
      if (x > part.maxX) part.maxX = x;
      if (y < part.minY) part.minY = y;
      if (y > part.maxY) part.maxY = y;
      for (let dy = -1; dy <= 1; dy++) {
        const ny = y + dy;
        if (ny < 0 || ny >= height) continue;
        for (let dx = -1; dx <= 1; dx++) {
          const nx = x + dx;
          if (!dx && !dy || nx < 0 || nx >= width) continue;
          const next = ny * width + nx;
          if (labels[next] || data[next * 4 + 3] <= threshold) continue;
          labels[next] = id;
          queue[tail++] = next;
        }
      }
    }
    parts.push(part);
  }
  const large = parts.filter((part) => part.count > 5000);
  return { labels, large };
}

async function isolate(file) {
  const { data, info } = await sharp(file).ensureAlpha().raw().toBuffer({ resolveWithObject: true });
  if (info.width !== 2048 || info.height !== 2048 || info.channels !== 4) throw new Error('unexpected dimensions');
  let result;
  let threshold;
  for (const candidate of [10, 32, 64, 128, 200]) {
    result = components(data, info.width, info.height, candidate);
    if (result.large.length === 16) {
      threshold = candidate;
      break;
    }
  }
  if (result.large.length !== 16) return `skipped (${result.large.length} figures)`;
  const parts = result.large.sort((a, b) => (a.minY + a.maxY) - (b.minY + b.maxY));
  for (let row = 0; row < 4; row++) {
    const section = parts.slice(row * 4, row * 4 + 4).sort((a, b) => (a.minX + a.maxX) - (b.minX + b.maxX));
    parts.splice(row * 4, 4, ...section);
  }
  const maxWidth = Math.max(...parts.map((p) => p.maxX - p.minX + 1));
  const maxHeight = Math.max(...parts.map((p) => p.maxY - p.minY + 1));
  const scale = Math.min(472 / maxWidth, 472 / maxHeight);
  const layers = [];
  for (let i = 0; i < 16; i++) {
    const part = parts[i];
    const width = part.maxX - part.minX + 1;
    const height = part.maxY - part.minY + 1;
    const cut = Buffer.alloc(width * height * 4);
    for (let y = part.minY; y <= part.maxY; y++) {
      for (let x = part.minX; x <= part.maxX; x++) {
        const sourcePixel = y * info.width + x;
        if (result.labels[sourcePixel] !== part.id) continue;
        const targetPixel = (y - part.minY) * width + (x - part.minX);
        data.copy(cut, targetPixel * 4, sourcePixel * 4, sourcePixel * 4 + 4);
      }
    }
    const drawWidth = Math.max(1, Math.round(width * scale));
    const drawHeight = Math.max(1, Math.round(height * scale));
    const image = await sharp(cut, { raw: { width, height, channels: 4 } })
      .resize(drawWidth, drawHeight, { kernel: 'lanczos3' }).png().toBuffer();
    const row = (i / 4) | 0;
    const col = i % 4;
    layers.push({
      input: image,
      left: col * 512 + Math.round((512 - drawWidth) / 2),
      top: (row + 1) * 512 - 20 - drawHeight,
    });
  }
  const output = await sharp({ create: { width: 2048, height: 2048, channels: 4,
    background: { r: 0, g: 0, b: 0, alpha: 0 } } }).composite(layers).png().toBuffer();
  fs.writeFileSync(file, output);
  return `isolated (alpha>${threshold})`;
}

(async () => {
  for (const name of names) {
    const result = await isolate(path.join(folder, name));
    process.stdout.write(`${name}: ${result}\n`);
  }
})().catch((error) => { process.stderr.write(String(error) + '\n'); process.exitCode = 1; });
