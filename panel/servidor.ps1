<#
  Servidor local del panel de aprendizaje.

  Sirve los archivos del repo en http://localhost:8765 y abre el navegador. Usa solo
  PowerShell 5.1, que viene con Windows 10/11: el alumno no instala nada (ADR-0001).
  Escucha SOLO en localhost (Constitution 4): nadie de la red puede entrar.
#>
param([int]$Puerto = 8765, [switch]$SinNavegador)

$ErrorActionPreference = 'Stop'
$Raiz = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))

# --- Tipos de contenido: el navegador exige el correcto para cargar modulos JS -----
$Tipos = @{
    '.html' = 'text/html; charset=utf-8'
    '.js'   = 'text/javascript; charset=utf-8'
    '.css'  = 'text/css; charset=utf-8'
    '.json' = 'application/json; charset=utf-8'
    '.md'   = 'text/markdown; charset=utf-8'
    '.cpp'  = 'text/plain; charset=utf-8'
    '.h'    = 'text/plain; charset=utf-8'
    '.ini'  = 'text/plain; charset=utf-8'
    '.bin'  = 'application/octet-stream'
    '.png'  = 'image/png'
    '.jpg'  = 'image/jpeg'
    '.jpeg' = 'image/jpeg'
    '.svg'  = 'image/svg+xml'
}

function Responder($Ctx, [int]$Codigo, [byte[]]$Cuerpo, [string]$Tipo) {
    $Ctx.Response.StatusCode = $Codigo
    $Ctx.Response.ContentType = $Tipo
    $Ctx.Response.Headers.Add('Cache-Control', 'no-store')
    $Ctx.Response.ContentLength64 = $Cuerpo.Length
    # HEAD (lo usa el latido del panel) pide solo las cabeceras: escribir el cuerpo hace
    # fallar a HttpListener ("sobrepasan el tamano ... Content-Length") y deja la conexion
    # colgada, lo que termina trabando otras descargas del navegador.
    if ($Ctx.Request.HttpMethod -ne 'HEAD') {
        $Ctx.Response.OutputStream.Write($Cuerpo, 0, $Cuerpo.Length)
    }
    $Ctx.Response.Close()
}

function Atender($Ctx) {
    $Ruta = [Uri]::UnescapeDataString($Ctx.Request.Url.AbsolutePath)
    if ($Ruta -eq '/') { $Ctx.Response.Redirect('/panel/'); $Ctx.Response.Close(); return }
    if ($Ruta.EndsWith('/')) { $Ruta += 'index.html' }

    # Anti "path traversal": la ruta final tiene que quedar DENTRO del repo.
    $Archivo = [IO.Path]::GetFullPath((Join-Path $Raiz $Ruta.TrimStart('/')))
    if (-not $Archivo.StartsWith($Raiz, [StringComparison]::OrdinalIgnoreCase) -or
        $Archivo -match '\\(\.git|\.pio)\\' -or $Archivo -match 'secrets\.h$') {
        Responder $Ctx 403 ([Text.Encoding]::UTF8.GetBytes('prohibido')) 'text/plain'
        return
    }
    if (-not (Test-Path -LiteralPath $Archivo -PathType Leaf)) {
        Responder $Ctx 404 ([Text.Encoding]::UTF8.GetBytes("no existe: $Ruta")) 'text/plain'
        return
    }
    $Ext = [IO.Path]::GetExtension($Archivo).ToLower()
    $Tipo = if ($Tipos.ContainsKey($Ext)) { $Tipos[$Ext] } else { 'application/octet-stream' }
    Responder $Ctx 200 ([IO.File]::ReadAllBytes($Archivo)) $Tipo
}

# --- Arranque --------------------------------------------------------------------
$Url = "http://localhost:$Puerto/panel/"

function AbrirNavegador {
    if (-not $SinNavegador) { try { Start-Process 'msedge' $Url } catch { Start-Process $Url } }
}

$Oyente = New-Object Net.HttpListener
$Oyente.Prefixes.Add("http://localhost:$Puerto/")
try { $Oyente.Start() } catch {
    # Caso tipico: doble clic en iniciar.bat con el panel ya abierto. Si el que ocupa el
    # puerto es NUESTRO panel, solo se abre el navegador; no hace falta un segundo servidor.
    try {
        $Resp = Invoke-WebRequest -UseBasicParsing -TimeoutSec 3 $Url
        if ($Resp.Content -match 'Panel ESP32-CAM') {
            Write-Host '[OK] El panel ya estaba abierto: lo abro en el navegador.'
            AbrirNavegador
            Start-Sleep -Seconds 2
            exit 0
        }
    } catch { }
    Write-Host "[ERROR] El puerto $Puerto lo esta usando otro programa."
    Read-Host 'Enter para salir'; exit 1
}
Write-Host ''
Write-Host '  Panel de aprendizaje ESP32-CAM'
Write-Host "  Abierto en: $Url"
Write-Host '  Usa Edge o Chrome (Firefox no permite conectar placas por USB).'
Write-Host '  Para cerrar el panel, cerra esta ventana.'
Write-Host ''
AbrirNavegador

# GetContextAsync + Wait con tiempo limite: deja que Ctrl+C cierre el servidor.
while ($Oyente.IsListening) {
    $Pedido = $Oyente.GetContextAsync()
    while (-not $Pedido.Wait(500)) { }
    try { Atender $Pedido.Result } catch {
        Write-Host "[ERROR] $($_.Exception.Message)"
        # Pase lo que pase, la respuesta se cierra: una conexion abierta traba al navegador.
        try { $Pedido.Result.Response.Abort() } catch { }
    }
}
