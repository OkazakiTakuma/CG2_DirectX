const fs = require("fs");
const path = require("path");

// 2x2 atlas: body / mask / dark details / horn & armor accent.
const BODY_UV = [0.04, 0.04, 0.46, 0.46];
const MASK_UV = [0.54, 0.04, 0.96, 0.46];
const DARK_UV = [0.25, 0.75];
const ACCENT_UV = [0.54, 0.54, 0.96, 0.96];
const resourceDirectory = path.resolve(__dirname, "../project/Resources");

function createMesh() {
  const positions = [];
  const normals = [];
  const texcoords = [];
  const indices = [];

  function normalize(v) {
    const length = Math.hypot(v[0], v[1], v[2]) || 1;
    return [v[0] / length, v[1] / length, v[2] / length];
  }

  function cross(a, b) {
    return [
      a[1] * b[2] - a[2] * b[1],
      a[2] * b[0] - a[0] * b[2],
      a[0] * b[1] - a[1] * b[0],
    ];
  }

  function addVertex(position, normal, uv) {
    positions.push(...position);
    normals.push(...normalize(normal));
    texcoords.push(...uv);
    return positions.length / 3 - 1;
  }

  function mapUv(mapping, u, v) {
    if (mapping.length === 2) return mapping;
    return [
      mapping[0] + (mapping[2] - mapping[0]) * u,
      mapping[1] + (mapping[3] - mapping[1]) * v,
    ];
  }

  function addEllipsoid(center, radius, segments = 10, rings = 5, uv = BODY_UV) {
    const base = positions.length / 3;
    for (let ring = 0; ring <= rings; ++ring) {
      const phi = Math.PI * ring / rings;
      const sinPhi = Math.sin(phi);
      const cosPhi = Math.cos(phi);
      for (let segment = 0; segment <= segments; ++segment) {
        const theta = 2 * Math.PI * segment / segments;
        const x = Math.sin(theta) * sinPhi;
        const y = cosPhi;
        const z = Math.cos(theta) * sinPhi;
        addVertex(
          [center[0] + radius[0] * x, center[1] + radius[1] * y, center[2] + radius[2] * z],
          [x / radius[0], y / radius[1], z / radius[2]],
          mapUv(uv, segment / segments, ring / rings),
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

  function basisFor(start, end) {
    const axis = normalize([end[0] - start[0], end[1] - start[1], end[2] - start[2]]);
    const reference = Math.abs(axis[1]) < 0.9 ? [0, 1, 0] : [1, 0, 0];
    const tangent = normalize(cross(axis, reference));
    return { axis, tangent, bitangent: normalize(cross(axis, tangent)) };
  }

  function addCone(baseCenter, tip, radius, segments = 7, uv = ACCENT_UV) {
    const { axis, tangent, bitangent } = basisFor(baseCenter, tip);
    const sideBase = positions.length / 3;
    for (let i = 0; i < segments; ++i) {
      const angle = 2 * Math.PI * i / segments;
      const radial = [
        Math.cos(angle) * tangent[0] + Math.sin(angle) * bitangent[0],
        Math.cos(angle) * tangent[1] + Math.sin(angle) * bitangent[1],
        Math.cos(angle) * tangent[2] + Math.sin(angle) * bitangent[2],
      ];
      addVertex([
        baseCenter[0] + radius * radial[0],
        baseCenter[1] + radius * radial[1],
        baseCenter[2] + radius * radial[2],
      ], radial, mapUv(uv, i / segments, 0.08));
    }
    const tipIndex = addVertex(tip, axis, mapUv(uv, 0.5, 0.92));
    const capCenter = addVertex(baseCenter, axis.map((value) => -value), mapUv(uv, 0.5, 0.5));
    for (let i = 0; i < segments; ++i) {
      const next = (i + 1) % segments;
      indices.push(sideBase + i, sideBase + next, tipIndex);
      indices.push(capCenter, sideBase + next, sideBase + i);
    }
  }

  function addCylinder(start, end, radius, segments = 8, uv = ACCENT_UV) {
    const { axis, tangent, bitangent } = basisFor(start, end);
    const base = positions.length / 3;
    for (let endIndex = 0; endIndex < 2; ++endIndex) {
      const center = endIndex === 0 ? start : end;
      for (let i = 0; i < segments; ++i) {
        const angle = 2 * Math.PI * i / segments;
        const radial = [
          Math.cos(angle) * tangent[0] + Math.sin(angle) * bitangent[0],
          Math.cos(angle) * tangent[1] + Math.sin(angle) * bitangent[1],
          Math.cos(angle) * tangent[2] + Math.sin(angle) * bitangent[2],
        ];
        addVertex([
          center[0] + radius * radial[0],
          center[1] + radius * radial[1],
          center[2] + radius * radial[2],
        ], radial, mapUv(uv, i / segments, endIndex));
      }
    }
    for (let i = 0; i < segments; ++i) {
      const next = (i + 1) % segments;
      indices.push(base + i, base + segments + i, base + next);
      indices.push(base + next, base + segments + i, base + segments + next);
    }
    const capStart = addVertex(start, axis.map((value) => -value), mapUv(uv, 0.5, 0.5));
    const capEnd = addVertex(end, axis, mapUv(uv, 0.5, 0.5));
    for (let i = 0; i < segments; ++i) {
      const next = (i + 1) % segments;
      indices.push(capStart, base + next, base + i);
      indices.push(capEnd, base + segments + i, base + segments + next);
    }
  }

  return { positions, normals, texcoords, indices, addEllipsoid, addCone, addCylinder };
}

function addMaskFace(mesh, z, eyeStyle = "pair") {
  mesh.addEllipsoid([0, 0.03, z], [0.43, 0.38, 0.10], 10, 5, MASK_UV);
  if (eyeStyle === "single") {
    mesh.addEllipsoid([0, 0.09, z + 0.105], [0.15, 0.14, 0.025], 8, 4, DARK_UV);
  } else {
    mesh.addEllipsoid([-0.16, 0.10, z + 0.105], [0.065, 0.12, 0.025], 6, 3, DARK_UV);
    mesh.addEllipsoid([0.16, 0.10, z + 0.105], [0.065, 0.12, 0.025], 6, 3, DARK_UV);
  }
  mesh.addEllipsoid([0, -0.155, z + 0.108], [0.12, 0.035, 0.02], 6, 3, DARK_UV);
}

function buildChaser(mesh) {
  mesh.addEllipsoid([0, 0.02, 0], [0.60, 0.67, 0.55], 12, 6);
  addMaskFace(mesh, 0.50);
  mesh.addCone([-0.43, 0.42, 0.02], [-0.76, 0.88, -0.12], 0.15);
  mesh.addCone([0.43, 0.42, 0.02], [0.76, 0.88, -0.12], 0.15);
  mesh.addCone([-0.30, -0.42, -0.04], [-0.43, -0.91, -0.18], 0.17);
  mesh.addCone([0.30, -0.42, -0.04], [0.43, -0.91, -0.18], 0.17);
}

function buildShooter(mesh) {
  mesh.addEllipsoid([0, 0.04, -0.05], [0.56, 0.59, 0.52], 12, 6);
  addMaskFace(mesh, 0.46, "single");
  mesh.addCylinder([-0.48, 0.02, 0.18], [-0.48, 0.02, 0.82], 0.13, 8);
  mesh.addCylinder([0.48, 0.02, 0.18], [0.48, 0.02, 0.82], 0.13, 8);
  mesh.addEllipsoid([-0.48, 0.02, 0.16], [0.22, 0.25, 0.22], 8, 4, ACCENT_UV);
  mesh.addEllipsoid([0.48, 0.02, 0.16], [0.22, 0.25, 0.22], 8, 4, ACCENT_UV);
  mesh.addCone([-0.40, 0.30, -0.15], [-0.90, 0.48, -0.30], 0.13);
  mesh.addCone([0.40, 0.30, -0.15], [0.90, 0.48, -0.30], 0.13);
}

function buildCharger(mesh) {
  mesh.addEllipsoid([0, -0.02, -0.07], [0.62, 0.56, 0.72], 12, 6);
  addMaskFace(mesh, 0.60);
  mesh.addCone([-0.34, 0.27, 0.45], [-0.78, 0.42, 1.02], 0.20, 8);
  mesh.addCone([0.34, 0.27, 0.45], [0.78, 0.42, 1.02], 0.20, 8);
  mesh.addCone([-0.45, -0.23, -0.20], [-0.74, -0.42, -0.64], 0.16);
  mesh.addCone([0.45, -0.23, -0.20], [0.74, -0.42, -0.64], 0.16);
  mesh.addEllipsoid([0, 0.38, 0.37], [0.37, 0.13, 0.17], 8, 4, DARK_UV);
}

function buildBomber(mesh) {
  mesh.addEllipsoid([0, 0, 0], [0.67, 0.67, 0.62], 12, 6);
  mesh.addEllipsoid([-0.17, 0.10, 0.595], [0.08, 0.08, 0.025], 6, 3, DARK_UV);
  mesh.addEllipsoid([0.17, 0.10, 0.595], [0.08, 0.08, 0.025], 6, 3, DARK_UV);
  mesh.addEllipsoid([0, -0.16, 0.60], [0.15, 0.05, 0.025], 8, 3, DARK_UV);
  const spikes = [
    [[0, 0.56, 0], [0, 1.05, 0]],
    [[-0.52, 0.20, 0], [-1.00, 0.34, 0]],
    [[0.52, 0.20, 0], [1.00, 0.34, 0]],
    [[-0.46, -0.34, -0.05], [-0.80, -0.78, -0.15]],
    [[0.46, -0.34, -0.05], [0.80, -0.78, -0.15]],
    [[0, 0.08, -0.53], [0, 0.13, -1.02]],
  ];
  for (const [base, tip] of spikes) mesh.addCone(base, tip, 0.16, 7);
  mesh.addCylinder([0, 0.61, 0], [0.19, 0.85, 0.05], 0.055, 7, DARK_UV);
  mesh.addEllipsoid([0.22, 0.89, 0.06], [0.13, 0.13, 0.13], 7, 4);
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

function writeModel(fileName, textureFileName, displayName, build) {
  const mesh = createMesh();
  build(mesh);
  const chunks = [floatBuffer(mesh.positions), floatBuffer(mesh.normals), floatBuffer(mesh.texcoords), uint16Buffer(mesh.indices)];
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
  for (let i = 0; i < mesh.positions.length; i += 3) {
    for (let axis = 0; axis < 3; ++axis) {
      mins[axis] = Math.min(mins[axis], mesh.positions[i + axis]);
      maxs[axis] = Math.max(maxs[axis], mesh.positions[i + axis]);
    }
  }
  const gltf = {
    asset: { version: "2.0", generator: "CG2_DirectX enemy family generator" },
    scene: 0,
    scenes: [{ nodes: [0] }],
    nodes: [{ name: displayName, mesh: 0 }],
    meshes: [{ name: `${displayName}Mesh`, primitives: [{
      attributes: { POSITION: 0, NORMAL: 1, TEXCOORD_0: 2 }, indices: 3, material: 0,
    }] }],
    materials: [{ name: "TintableEnemy", pbrMetallicRoughness: {
      baseColorTexture: { index: 0 }, metallicFactor: 0, roughnessFactor: 0.78,
    } }],
    textures: [{ source: 0 }],
    images: [{ uri: textureFileName }],
    buffers: [{ uri: `data:application/octet-stream;base64,${binary.toString("base64")}`, byteLength: binary.length }],
    bufferViews: [
      { buffer: 0, byteOffset: offsets[0], byteLength: chunks[0].length, target: 34962 },
      { buffer: 0, byteOffset: offsets[1], byteLength: chunks[1].length, target: 34962 },
      { buffer: 0, byteOffset: offsets[2], byteLength: chunks[2].length, target: 34962 },
      { buffer: 0, byteOffset: offsets[3], byteLength: chunks[3].length, target: 34963 },
    ],
    accessors: [
      { bufferView: 0, componentType: 5126, count: mesh.positions.length / 3, type: "VEC3", min: mins, max: maxs },
      { bufferView: 1, componentType: 5126, count: mesh.normals.length / 3, type: "VEC3" },
      { bufferView: 2, componentType: 5126, count: mesh.texcoords.length / 2, type: "VEC2" },
      { bufferView: 3, componentType: 5123, count: mesh.indices.length, type: "SCALAR" },
    ],
  };
  fs.writeFileSync(path.join(resourceDirectory, fileName), `${JSON.stringify(gltf, null, 2)}\n`);
  console.log(`${fileName}: ${mesh.positions.length / 3} vertices, ${mesh.indices.length / 3} triangles`);
}

function makeStudentAtlasBmp(role) {
  const width = 256;
  const height = 256;
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
  function baseValue(x, y) {
    if (y < 128) return x < 128 ? 190 : 235;
    return x < 128 ? 18 : 135;
  }
  function isPattern(x, y) {
    const localX = x % 128;
    const localY = y % 128;
    if (y >= 128 && x < 128) return false;
    if (role === "chaser") {
      if (x < 128 && y < 128) return ((localX - 34) ** 2 + (localY - 38) ** 2 < 90) || ((localX - 88) ** 2 + (localY - 78) ** 2 < 120);
      if (x >= 128 && y < 128) return localX % 32 < 4;
      return localY % 28 < 5;
    }
    if (role === "shooter") {
      if (x < 128 && y < 128) return localX % 42 < 4 || localY % 42 < 4;
      if (x >= 128 && y < 128) {
        const distance = Math.hypot(localX - 64, localY - 64);
        return (distance > 28 && distance < 35) || distance < 9;
      }
      return (localX + localY) % 36 < 8;
    }
    if (role === "charger") {
      if (x < 128 && y < 128) return localX % 58 < 6 || localY % 46 < 6;
      if (x >= 128 && y < 128) return Math.abs(localY - (42 + Math.abs(localX - 64) * 0.42)) < 6;
      return (localX + localY) % 44 < 12;
    }
    // Bomber: deliberately chunky cracks and warning marks.
    if (x < 128 && y < 128) {
      return Math.abs(localX - (22 + localY * 0.42)) < 3 ||
        Math.abs(localX - (106 - localY * 0.50)) < 3 ||
        (localY > 58 && Math.abs(localX - 64) < 4);
    }
    if (x >= 128 && y < 128) {
      return Math.abs(localX - localY) < 7 || Math.abs(localX + localY - 127) < 7;
    }
    return localY % 30 < 7;
  }
  for (let y = 0; y < height; ++y) {
    for (let x = 0; x < width; ++x) {
      let value = baseValue(x, y);
      if (isPattern(x, y)) value = y < 128 && x >= 128 ? 90 : (stdClamp(value - 55, 8, 245));
      const offset = 54 + (height - 1 - y) * rowSize + x * 3;
      file[offset] = value;
      file[offset + 1] = value;
      file[offset + 2] = value;
    }
  }
  return file;
}

function stdClamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

for (const role of ["chaser", "shooter", "charger", "bomber"]) {
  fs.writeFileSync(path.join(resourceDirectory, `enemy_${role}_texture.bmp`), makeStudentAtlasBmp(role));
}
writeModel("enemy_chaser.gltf", "enemy_chaser_texture.bmp", "EnemyChaser", buildChaser);
writeModel("enemy_shooter.gltf", "enemy_shooter_texture.bmp", "EnemyShooter", buildShooter);
writeModel("enemy_charger.gltf", "enemy_charger_texture.bmp", "EnemyCharger", buildCharger);
writeModel("enemy_bomber.gltf", "enemy_bomber_texture.bmp", "EnemyBomber", buildBomber);
