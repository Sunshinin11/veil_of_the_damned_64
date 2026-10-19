Add-Type -AssemblyName System.Drawing

$root = 'C:\Desenvolvimento\Nintendo 64\Sprites'
$jobs = @(
    @{ File='Barbaro/fx_grito.png'; W=2048; H=512; Cols=4; Rows=1; Mode='fit' },
    @{ File='Barbaro/fx_impacto.png'; W=2048; H=512; Cols=4; Rows=1; Mode='fit' },
    @{ File='Cartas_Barbaro/b1_cartas.png'; W=2048; H=1280 },
    @{ File='Cartas_Barbaro/b2_cartas.png'; W=2048; H=1280 },
    @{ File='Cartas_Barbaro/b3_cartas.png'; W=2048; H=1280 },
    @{ File='Norte_Passo/n1_pinheiros.png'; W=1536; H=512 },
    @{ File='Norte_Passo/n2_rochas.png'; W=1024; H=1024; Cols=2; Rows=2; Mode='objects' },
    @{ File='Norte_Passo/n3_decoracao.png'; W=1024; H=1024; Cols=2; Rows=2; Mode='objects' },
    @{ File='Norte_Passo/n4_fogueira.png'; W=1536; H=512 },
    @{ File='Norte_Passo/n6_portal.png'; W=1536; H=512 },
    @{ File='Norte_Passo/t1_chao_neve.png'; W=1024; H=1024 },
    @{ File='Norte_Passo/t2_parede_penhasco.png'; W=2048; H=1024 },
    @{ File='Norte_Passo/t3_topo_penhasco.png'; W=1024; H=512 },
    @{ File='Norte_Caverna/g1_estalagmites.png'; W=1536; H=512 },
    @{ File='Norte_Caverna/g2_cristais.png'; W=1536; H=512 },
    @{ File='Norte_Caverna/g3_decoracao.png'; W=1024; H=1024; Cols=2; Rows=2; Mode='objects' },
    @{ File='Norte_Caverna/g4_braseiro.png'; W=1536; H=512 },
    @{ File='Norte_Caverna/t4_chao_gelo.png'; W=1024; H=1024 },
    @{ File='Norte_Caverna/t5_parede_gelo.png'; W=2048; H=1024 },
    @{ File='Norte_Caverna/t6_topo_gelo.png'; W=1024; H=512 },
    @{ File='Trono_Jarl/j1_pedras_runicas.png'; W=1024; H=512 },
    @{ File='Trono_Jarl/j3_geiser.png'; W=1536; H=512 },
    @{ File='Trono_Jarl/j4_decoracao.png'; W=1024; H=1024; Cols=2; Rows=2; Mode='objects' },
    @{ File='Trono_Jarl/j5_trono.png'; W=512; H=512 },
    @{ File='Trono_Jarl/j7_braseiro.png'; W=1536; H=512 },
    @{ File='Trono_Jarl/t7_chao_salao.png'; W=1024; H=1024 },
    @{ File='Trono_Jarl/t8_parede_fortaleza.png'; W=2048; H=1024 },
    @{ File='Trono_Jarl/t9_topo_fortaleza.png'; W=1024; H=512 },
    @{ File='Trono_Jarl/t10_agua_gelada.png'; W=1024; H=1024 }
)

Get-ChildItem -LiteralPath (Join-Path $root 'Norte_Boss') -Filter '*.png' -File | ForEach-Object {
    $jobs += @{ File=('Norte_Boss/' + $_.Name); W=2048; H=2048 }
}

function Get-VisibleBounds($bitmap, $cell) {
    $left = $cell.Right
    $top = $cell.Bottom
    $right = $cell.Left
    $bottom = $cell.Top
    for ($y = $cell.Top; $y -lt $cell.Bottom; $y += 2) {
        for ($x = $cell.Left; $x -lt $cell.Right; $x += 2) {
            if ($bitmap.GetPixel($x, $y).A -ge 128) {
                if ($x -lt $left) { $left = $x }
                if ($y -lt $top) { $top = $y }
                if ($x -gt $right) { $right = $x }
                if ($y -gt $bottom) { $bottom = $y }
            }
        }
    }
    if ($right -lt $left) { return $cell }
    $left = [Math]::Max($cell.Left, $left - 4)
    $top = [Math]::Max($cell.Top, $top - 4)
    $right = [Math]::Min($cell.Right, $right + 6)
    $bottom = [Math]::Min($cell.Bottom, $bottom + 6)
    return [System.Drawing.Rectangle]::FromLTRB($left, $top, $right, $bottom)
}

foreach ($job in $jobs) {
    $path = Join-Path $root $job.File
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing asset: $path" }
    $src = [System.Drawing.Bitmap]::new($path)
    if ($src.Width -eq $job.W -and $src.Height -eq $job.H) {
        $src.Dispose()
        continue
    }
    $dest = [System.Drawing.Bitmap]::new($job.W, $job.H, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($dest)
    $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
    $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.Clear([System.Drawing.Color]::Transparent)

    if ($job.Mode -eq 'fit' -or $job.Mode -eq 'objects') {
        $cols = $job.Cols
        $rows = $job.Rows
        $sourceCellWidth = $src.Width / $cols
        $sourceCellHeight = $src.Height / $rows
        $destCellWidth = $job.W / $cols
        $destCellHeight = $job.H / $rows
        for ($row = 0; $row -lt $rows; $row++) {
            for ($col = 0; $col -lt $cols; $col++) {
                $cell = [System.Drawing.Rectangle]::new([int]($col * $sourceCellWidth), [int]($row * $sourceCellHeight), [int]$sourceCellWidth, [int]$sourceCellHeight)
                $crop = if ($job.Mode -eq 'objects') { Get-VisibleBounds $src $cell } else { $cell }
                $margin = if ($job.Mode -eq 'objects') { 20 } else { 0 }
                $scale = [Math]::Min(($destCellWidth - 2 * $margin) / $crop.Width, ($destCellHeight - 2 * $margin) / $crop.Height)
                $drawWidth = $crop.Width * $scale
                $drawHeight = $crop.Height * $scale
                $drawX = $col * $destCellWidth + ($destCellWidth - $drawWidth) / 2
                $drawY = if ($job.Mode -eq 'objects') { ($row + 1) * $destCellHeight - $margin - $drawHeight } else { $row * $destCellHeight + ($destCellHeight - $drawHeight) / 2 }
                $target = [System.Drawing.RectangleF]::new([float]$drawX, [float]$drawY, [float]$drawWidth, [float]$drawHeight)
                $graphics.DrawImage($src, $target, $crop, [System.Drawing.GraphicsUnit]::Pixel)
            }
        }
    } else {
        $target = [System.Drawing.Rectangle]::new(0, 0, $job.W, $job.H)
        $source = [System.Drawing.Rectangle]::new(0, 0, $src.Width, $src.Height)
        $graphics.DrawImage($src, $target, $source, [System.Drawing.GraphicsUnit]::Pixel)
    }

    $tmp = $path + '.normalized.png'
    $dest.Save($tmp, [System.Drawing.Imaging.ImageFormat]::Png)
    $graphics.Dispose()
    $dest.Dispose()
    $src.Dispose()
    Move-Item -LiteralPath $tmp -Destination $path -Force
    Write-Output ("{0} -> {1}x{2}" -f $job.File, $job.W, $job.H)
}
