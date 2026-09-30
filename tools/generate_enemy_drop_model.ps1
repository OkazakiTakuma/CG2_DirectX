param(
    [string]$OutputDirectory = "project/Resources"
)

$culture = [System.Globalization.CultureInfo]::InvariantCulture
$triangles = [System.Collections.Generic.List[object]]::new()

function Add-Triangle {
    param(
        [System.Numerics.Vector3]$A,
        [System.Numerics.Vector3]$B,
        [System.Numerics.Vector3]$C
    )
    # 右手系で外向き法線になるよう、生成時の周回順を反転して保存する。
    $triangles.Add([pscustomobject]@{ A = $A; B = $C; C = $B })
}

# 八角形の上下リングをずらしてつないだ、宝石らしい多面体コア。
$segments = 8
$upper = @()
$lower = @()
for ($index = 0; $index -lt $segments; $index++) {
    $angle = 2.0 * [Math]::PI * $index / $segments
    $lowerAngle = $angle + [Math]::PI / $segments
    $upper += [System.Numerics.Vector3]::new(
        [float]([Math]::Cos($angle) * 0.52), 0.24, [float]([Math]::Sin($angle) * 0.52))
    $lower += [System.Numerics.Vector3]::new(
        [float]([Math]::Cos($lowerAngle) * 0.40), -0.26, [float]([Math]::Sin($lowerAngle) * 0.40))
}
$top = [System.Numerics.Vector3]::new(0.0, 0.92, 0.0)
$bottom = [System.Numerics.Vector3]::new(0.0, -0.82, 0.0)
for ($index = 0; $index -lt $segments; $index++) {
    $next = ($index + 1) % $segments
    Add-Triangle $top $upper[$index] $upper[$next]
    Add-Triangle $upper[$index] $lower[$index] $lower[$next]
    Add-Triangle $upper[$index] $lower[$next] $upper[$next]
    Add-Triangle $bottom $lower[$next] $lower[$index]
}

# 真上からも輪郭を読みやすくする外周リング。少し傾けて立体感を出す。
$majorSegments = 16
$minorSegments = 4
$majorRadius = 0.75
$minorRadius = 0.065
$tilt = 18.0 * [Math]::PI / 180.0
$ring = [System.Collections.Generic.List[System.Numerics.Vector3]]::new()
for ($major = 0; $major -lt $majorSegments; $major++) {
    $majorAngle = 2.0 * [Math]::PI * $major / $majorSegments
    for ($minor = 0; $minor -lt $minorSegments; $minor++) {
        $minorAngle = 2.0 * [Math]::PI * $minor / $minorSegments
        $radius = $majorRadius + $minorRadius * [Math]::Cos($minorAngle)
        $x = $radius * [Math]::Cos($majorAngle)
        $y = $minorRadius * [Math]::Sin($minorAngle)
        $z = $radius * [Math]::Sin($majorAngle)
        $tiltedY = $y * [Math]::Cos($tilt) - $z * [Math]::Sin($tilt)
        $tiltedZ = $y * [Math]::Sin($tilt) + $z * [Math]::Cos($tilt)
        $ring.Add([System.Numerics.Vector3]::new([float]$x, [float]$tiltedY, [float]$tiltedZ))
    }
}
for ($major = 0; $major -lt $majorSegments; $major++) {
    $nextMajor = ($major + 1) % $majorSegments
    for ($minor = 0; $minor -lt $minorSegments; $minor++) {
        $nextMinor = ($minor + 1) % $minorSegments
        $a = $ring[$major * $minorSegments + $minor]
        $b = $ring[$nextMajor * $minorSegments + $minor]
        $c = $ring[$nextMajor * $minorSegments + $nextMinor]
        $d = $ring[$major * $minorSegments + $nextMinor]
        Add-Triangle $a $b $c
        Add-Triangle $a $c $d
    }
}

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("# Procedurally generated enemy drop model")
$lines.Add("mtllib enemy_drop.mtl")
$lines.Add("o SpiritDrop")
$lines.Add("usemtl SpiritDropMaterial")

$vertexIndex = 1
foreach ($triangle in $triangles) {
    $points = @($triangle.A, $triangle.B, $triangle.C)
    $normal = [System.Numerics.Vector3]::Cross($triangle.B - $triangle.A, $triangle.C - $triangle.A)
    if ($normal.LengthSquared() -gt 0.000001) {
        $normal = [System.Numerics.Vector3]::Normalize($normal)
    }

    foreach ($point in $points) {
        $lines.Add([string]::Format($culture, "v {0:F6} {1:F6} {2:F6}", $point.X, $point.Y, $point.Z))
    }
    $lines.Add("vt 0.500000 0.950000")
    $lines.Add("vt 0.050000 0.050000")
    $lines.Add("vt 0.950000 0.050000")
    for ($corner = 0; $corner -lt 3; $corner++) {
        $lines.Add([string]::Format($culture, "vn {0:F6} {1:F6} {2:F6}", $normal.X, $normal.Y, $normal.Z))
    }
    $lines.Add("f $vertexIndex/$vertexIndex/$vertexIndex $($vertexIndex + 1)/$($vertexIndex + 1)/$($vertexIndex + 1) $($vertexIndex + 2)/$($vertexIndex + 2)/$($vertexIndex + 2)")
    $vertexIndex += 3
}

New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
[System.IO.File]::WriteAllLines((Join-Path $OutputDirectory "enemy_drop.obj"), $lines, [System.Text.UTF8Encoding]::new($false))

$material = @(
    "newmtl SpiritDropMaterial",
    "Ka 1.000000 1.000000 1.000000",
    "Kd 1.000000 1.000000 1.000000",
    "Ks 0.700000 0.700000 0.700000",
    "Ns 72.000000",
    "illum 2",
    "map_Kd enemy_drop_texture.png"
)
[System.IO.File]::WriteAllLines((Join-Path $OutputDirectory "enemy_drop.mtl"), $material, [System.Text.UTF8Encoding]::new($false))

Copy-Item -LiteralPath "project/Resources/human/white.png" -Destination (Join-Path $OutputDirectory "enemy_drop_texture.png") -Force

Write-Output "Generated enemy_drop.obj with $($triangles.Count) triangles."
