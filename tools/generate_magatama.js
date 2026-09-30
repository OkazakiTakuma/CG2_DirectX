const fs = require("fs");
const path = require("path");

const positions = [];
const normals = [];
const texcoords = [];
const indices = [];
const WHITE_UV = [0.25, 0.5];
const DARK_UV = [0.75, 0.5];

function normalize(v) {
  const length = Math.hypot(v[0], v[1], v[2]) || 1;
  return [v[0] / length, v[1] / length, v[2] / length];
}

function addVertex(position, normal, uv) {
  positions.push(...position);
  normals.push(...normalize(normal));
  texcoords.push(...uv);
  return positions.length / 3 - 1;
}

function addEllipsoid(center, radius, segments, rings, uv) {
  const base = positions.length / 3;
  for (let ring = 0; ring <= rings; ++ring) {
    const phi = Math.PI * ring / rings;
    for (let segment = 0; segment <= segments; ++segment) {
      const theta = 2 * Math.PI * segment / segments;
      const unit = [Math.sin(theta) * Math.sin(phi), Math.cos(phi), Math.cos(theta) * Math.sin(phi)];
      addVertex(
        [center[0] + radius[0] * unit[0], center[1] + radius[1] * unit[1], center[2] + radius[2] * unit[2]],
        [unit[0] / radius[0], unit[1] / radius[1], unit[2] / radius[2]],
        uv,
      );
    }
  }
  for (let ring = 0; ring < rings; ++ring) {
    for (let segment = 0; segment < segments; ++segment) {
      const a = base + ring * (segments + 1) + segment;
      const b = a + segments + 1;
      indices.push(a, b, a + 1, a + 1, b, b + 1);
    }
  }
}

function addMagatamaTube() {
  const pathSegments = 16;
  const ringSegments = 10;
  const pathRadius = 0.37;
  const startAngle = -0.35 * Math.PI;
  const sweepAngle = 1.52 * Math.PI;
  const base = positions.length / 3;

  for (let pathIndex = 0; pathIndex <= pathSegments; ++pathIndex) {
    const t = pathIndex / pathSegments;
    const angle = startAngle + sweepAngle * t;
    const tubeRadius = 0.055 + 0.27 * Math.pow(1.0 - t, 0.72);
    const center = [pathRadius * Math.cos(angle), 0, pathRadius * Math.sin(angle)];
    const outward = [Math.cos(angle), 0, Math.sin(angle)];

    for (let ringIndex = 0; ringIndex < ringSegments; ++ringIndex) {
      const ringAngle = 2 * Math.PI * ringIndex / ringSegments;
      const normal = normalize([
        outward[0] * Math.cos(ringAngle),
        Math.sin(ringAngle),
        outward[2] * Math.cos(ringAngle),
      ]);
      addVertex([
        center[0] + tubeRadius * normal[0],
        center[1] + tubeRadius * normal[1],
        center[2] + tubeRadius * normal[2],
      ], normal, WHITE_UV);
    }
  }

  for (let pathIndex = 0; pathIndex < pathSegments; ++pathIndex) {
    for (let ringIndex = 0; ringIndex < ringSegments; ++ringIndex) {
      const nextRing = (ringIndex + 1) % ringSegments;
      const a = base + pathIndex * ringSegments + ringIndex;
      const b = base + (pathIndex + 1) * ringSegments + ringIndex;
      indices.push(a, b, base + pathIndex * ringSegments + nextRing);
      indices.push(base + pathIndex * ringSegments + nextRing, b, base + (pathIndex + 1) * ringSegments + nextRing);
    }
  }

  const headAngle = startAngle;
  const headCenter = [pathRadius * Math.cos(headAngle), 0, pathRadius * Math.sin(headAngle)];
  const tailAngle = startAngle + sweepAngle;
  const tailCenter = [pathRadius * Math.cos(tailAngle), 0, pathRadius * Math.sin(tailAngle)];
  addEllipsoid(headCenter, [0.33, 0.31, 0.33], 12, 6, WHITE_UV);
  addEllipsoid(tailCenter, [0.075, 0.065, 0.075], 8, 4, WHITE_UV);

  // 穴は黒い象嵌として上下両面に置き、上方視点と反射時のどちらでも勾玉と判別できるようにする。
  addEllipsoid([headCenter[0], 0.306, headCenter[2]], [0.105, 0.018, 0.105], 10, 4, DARK_UV);
  addEllipsoid([headCenter[0], -0.306, headCenter[2]], [0.105, 0.018, 0.105], 10, 4, DARK_UV);
}

function align4(buffer) {
  const padding = (4 - buffer.length % 4) % 4;
  return padding ? Buffer.concat([buffer, Buffer.alloc(padding)]) : buffer;
}

function floatBuffer(values) {
  const buffer = Buffer.alloc(values.length * 4);
  values.forEach((value, index) => buffer.writeFloatLE(value, index * 4));
  return buffer;
}

function uint16Buffer(values) {
  const buffer = Buffer.alloc(values.length * 2);
  values.forEach((value, index) => buffer.writeUInt16LE(value, index * 2));
  return buffer;
}

function makePaletteBmp() {
  const width = 4;
  const height = 4;
  const rowSize = width * 3;
  const file = Buffer.alloc(54 + rowSize * height);
  file.write("BM", 0, "ascii");
  file.writeUInt32LE(file.length, 2);
  file.writeUInt32LE(54, 10);
  file.writeUInt32LE(40, 14);
  file.writeInt32LE(width, 18);
  file.writeInt32LE(height, 22);
  file.writeUInt16LE(1, 26);
  file.writeUInt16LE(24, 28);
  file.writeUInt32LE(rowSize * height, 34);
  for (let y = 0; y < height; ++y) {
    for (let x = 0; x < width; ++x) {
      const offset = 54 + y * rowSize + x * 3;
      const value = x < 2 ? 245 : 12;
      file[offset] = value;
      file[offset + 1] = value;
      file[offset + 2] = value;
    }
  }
  return file;
}

addMagatamaTube();
const chunks = [floatBuffer(positions), floatBuffer(normals), floatBuffer(texcoords), uint16Buffer(indices)];
const offsets = [];
const packed = [];
let byteOffset = 0;
for (const chunk of chunks) {
  offsets.push(byteOffset);
  const aligned = align4(chunk);
  packed.push(aligned);
  byteOffset += aligned.length;
}
const binary = Buffer.concat(packed);
const mins = [Infinity, Infinity, Infinity];
const maxs = [-Infinity, -Infinity, -Infinity];
for (let i = 0; i < positions.length; i += 3) {
  for (let axis = 0; axis < 3; ++axis) {
    mins[axis] = Math.min(mins[axis], positions[i + axis]);
    maxs[axis] = Math.max(maxs[axis], positions[i + axis]);
  }
}

const gltf = {
  asset: { version: "2.0", generator: "CG2_DirectX magatama generator" },
  scene: 0,
  scenes: [{ nodes: [0] }],
  nodes: [{ name: "Magatama", mesh: 0 }],
  meshes: [{ name: "MagatamaMesh", primitives: [{
    attributes: { POSITION: 0, NORMAL: 1, TEXCOORD_0: 2 }, indices: 3, material: 0,
  }] }],
  materials: [{ name: "TintableMagatama", pbrMetallicRoughness: {
    baseColorTexture: { index: 0 }, metallicFactor: 0.08, roughnessFactor: 0.42,
  } }],
  textures: [{ source: 0 }],
  images: [{ uri: "magatama_palette.bmp" }],
  buffers: [{ uri: `data:application/octet-stream;base64,${binary.toString("base64")}`, byteLength: binary.length }],
  bufferViews: [
    { buffer: 0, byteOffset: offsets[0], byteLength: chunks[0].length, target: 34962 },
    { buffer: 0, byteOffset: offsets[1], byteLength: chunks[1].length, target: 34962 },
    { buffer: 0, byteOffset: offsets[2], byteLength: chunks[2].length, target: 34962 },
    { buffer: 0, byteOffset: offsets[3], byteLength: chunks[3].length, target: 34963 },
  ],
  accessors: [
    { bufferView: 0, componentType: 5126, count: positions.length / 3, type: "VEC3", min: mins, max: maxs },
    { bufferView: 1, componentType: 5126, count: normals.length / 3, type: "VEC3" },
    { bufferView: 2, componentType: 5126, count: texcoords.length / 2, type: "VEC2" },
    { bufferView: 3, componentType: 5123, count: indices.length, type: "SCALAR" },
  ],
};

const resourceDirectory = path.resolve(__dirname, "../project/Resources");
fs.writeFileSync(path.join(resourceDirectory, "magatama.gltf"), `${JSON.stringify(gltf, null, 2)}\n`);
fs.writeFileSync(path.join(resourceDirectory, "magatama_palette.bmp"), makePaletteBmp());
console.log(`magatama.gltf: ${positions.length / 3} vertices, ${indices.length / 3} triangles`);
