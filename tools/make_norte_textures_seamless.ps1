Add-Type -AssemblyName System.Drawing

$root = 'C:\Desenvolvimento\Nintendo 64\Sprites'
$files = @(
    'Norte_Passo/t1_chao_neve.png',
    'Norte_Passo/t2_parede_penhasco.png',
    'Norte_Passo/t3_topo_penhasco.png',
    'Norte_Caverna/t4_chao_gelo.png',
    'Norte_Caverna/t5_parede_gelo.png',
    'Norte_Caverna/t6_topo_gelo.png',
    'Trono_Jarl/t7_chao_salao.png',
    'Trono_Jarl/t8_parede_fortaleza.png',
    'Trono_Jarl/t9_topo_fortaleza.png',
    'Trono_Jarl/t10_agua_gelada.png'
)

function Blend-Color($a, $b, $weight) {
    $r = [int][Math]::Round($a.R * (1 - $weight) + $b.R * $weight)
    $g = [int][Math]::Round($a.G * (1 - $weight) + $b.G * $weight)
    $blue = [int][Math]::Round($a.B * (1 - $weight) + $b.B * $weight)
    return [System.Drawing.Color]::FromArgb(255, $r, $g, $blue)
}

foreach ($file in $files) {
    $path = Join-Path $root $file
    $image = [System.Drawing.Bitmap]::new($path)
    $band = [Math]::Min(64, [Math]::Floor([Math]::Min($image.Width, $image.Height) / 8))

    for ($x = 0; $x -lt $band; $x++) {
        $rightX = $image.Width - 1 - $x
        $weight = [Math]::Pow(($band - $x) / $band, 2)
        for ($y = 0; $y -lt $image.Height; $y++) {
            $leftColor = $image.GetPixel($x, $y)
            $rightColor = $image.GetPixel($rightX, $y)
            $mid = [System.Drawing.Color]::FromArgb(255,
                [int][Math]::Round(($leftColor.R + $rightColor.R) / 2),
                [int][Math]::Round(($leftColor.G + $rightColor.G) / 2),
                [int][Math]::Round(($leftColor.B + $rightColor.B) / 2))
            $image.SetPixel($x, $y, (Blend-Color $leftColor $mid $weight))
            $image.SetPixel($rightX, $y, (Blend-Color $rightColor $mid $weight))
        }
    }
    for ($y = 0; $y -lt $band; $y++) {
        $bottomY = $image.Height - 1 - $y
        $weight = [Math]::Pow(($band - $y) / $band, 2)
        for ($x = 0; $x -lt $image.Width; $x++) {
            $topColor = $image.GetPixel($x, $y)
            $bottomColor = $image.GetPixel($x, $bottomY)
            $mid = [System.Drawing.Color]::FromArgb(255,
                [int][Math]::Round(($topColor.R + $bottomColor.R) / 2),
                [int][Math]::Round(($topColor.G + $bottomColor.G) / 2),
                [int][Math]::Round(($topColor.B + $bottomColor.B) / 2))
            $image.SetPixel($x, $y, (Blend-Color $topColor $mid $weight))
            $image.SetPixel($x, $bottomY, (Blend-Color $bottomColor $mid $weight))
        }
    }
    $tmp = $path + '.seamless.png'
    $image.Save($tmp, [System.Drawing.Imaging.ImageFormat]::Png)
    $image.Dispose()
    Move-Item -LiteralPath $tmp -Destination $path -Force
    Write-Output $file
}
