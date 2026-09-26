#!/usr/bin/perl
# Servidor local del panel de aprendizaje, para Mac (y Linux).
#
# Equivalente a panel/servidor.ps1 (Windows): sirve los archivos del repo en
# http://localhost:8765 SOLO para esta computadora. Usa Perl y modulos que vienen con macOS,
# asi el alumno no instala nada. Mismos bloqueos que el de Windows: .git, .pio, secrets.h y
# cualquier ruta fuera del repo.
#
# Uso: perl panel/servidor.pl [--puerto 8765] [--sin-navegador]
use strict;
use warnings;
use IO::Socket::INET;
use File::Spec;
use File::Basename qw(dirname);
use Cwd qw(abs_path);

my $puerto = 8765;
my $sin_navegador = 0;
for (my $i = 0; $i < @ARGV; $i++) {
    if ($ARGV[$i] eq '--puerto') { $puerto = $ARGV[++$i]; }
    elsif ($ARGV[$i] eq '--sin-navegador') { $sin_navegador = 1; }
}

my $raiz = abs_path(File::Spec->catdir(dirname(abs_path($0)), '..'));
my $url = "http://localhost:$puerto/panel/";

# El navegador exige el tipo correcto para cargar modulos JavaScript.
my %tipos = (
    html => 'text/html; charset=utf-8', js => 'text/javascript; charset=utf-8',
    css => 'text/css; charset=utf-8', json => 'application/json; charset=utf-8',
    md => 'text/markdown; charset=utf-8', cpp => 'text/plain; charset=utf-8',
    h => 'text/plain; charset=utf-8', ini => 'text/plain; charset=utf-8',
    bin => 'application/octet-stream', png => 'image/png', jpg => 'image/jpeg',
    jpeg => 'image/jpeg', svg => 'image/svg+xml',
);

# Safari no puede conectarse a la placa (no tiene Web Serial): se prefiere Chrome o Edge.
sub abrir_navegador {
    return if $sin_navegador;
    if ($^O eq 'darwin') {
        for my $app ('Google Chrome', 'Microsoft Edge', 'Chromium', 'Brave Browser') {
            return if system('open', '-a', $app, $url) == 0;
        }
        print "\n  [AVISO] No encontre Chrome ni Edge. Safari no puede conectarse a la placa.\n";
        print "  Instala Google Chrome o Microsoft Edge y abri: $url\n\n";
        system('open', $url);
    } else {
        system("xdg-open '$url' >/dev/null 2>&1 &");
    }
}

sub responder {
    my ($cliente, $codigo, $texto, $cuerpo, $tipo, $solo_cabeceras) = @_;
    print $cliente "HTTP/1.1 $codigo $texto\r\n",
        "Content-Type: $tipo\r\n",
        "Content-Length: " . length($cuerpo) . "\r\n",
        "Cache-Control: no-store\r\n",
        "Connection: close\r\n\r\n";
    # HEAD (lo usa el latido del panel) pide solo las cabeceras.
    print $cliente $cuerpo unless $solo_cabeceras;
}

sub leer_archivo {
    my ($ruta) = @_;
    open(my $fh, '<:raw', $ruta) or return undef;
    local $/;
    my $datos = <$fh>;
    close $fh;
    return $datos;
}

sub atender {
    my ($cliente) = @_;
    my $pedido = <$cliente>;
    return unless defined $pedido;
    while (my $linea = <$cliente>) { last if $linea =~ /^\r?\n$/; }   # saltear cabeceras
    my ($metodo, $ruta) = $pedido =~ m{^(GET|HEAD)\s+(\S+)};
    unless ($metodo) { responder($cliente, 405, 'Method Not Allowed', 'solo GET', 'text/plain', 0); return; }
    my $solo_cabeceras = $metodo eq 'HEAD';

    $ruta =~ s/\?.*$//;                                   # sin parametros
    $ruta =~ s/%([0-9A-Fa-f]{2})/chr(hex($1))/eg;         # %2e%2e -> ..
    if ($ruta eq '/') {
        print $cliente "HTTP/1.1 302 Found\r\nLocation: /panel/\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        return;
    }
    $ruta .= 'index.html' if $ruta =~ m{/$};

    # Anti "path traversal": ningun segmento ".." y todo queda DENTRO del repo.
    my @partes = grep { length } split m{/}, $ruta;
    if (grep({ $_ eq '..' } @partes) || grep({ $_ eq '.git' || $_ eq '.pio' } @partes)
        || $ruta =~ /secrets\.h$/) {
        responder($cliente, 403, 'Forbidden', 'prohibido', 'text/plain', $solo_cabeceras);
        return;
    }
    my $archivo = File::Spec->catfile($raiz, @partes);
    my $real = abs_path($archivo);
    unless (defined $real && -f $real && index($real, $raiz) == 0) {
        responder($cliente, 404, 'Not Found', "no existe: $ruta", 'text/plain', $solo_cabeceras);
        return;
    }
    my ($ext) = $real =~ /\.([^.\/]+)$/;
    my $tipo = $tipos{lc($ext // '')} // 'application/octet-stream';
    my $datos = leer_archivo($real);
    responder($cliente, 200, 'OK', $datos // '', $tipo, $solo_cabeceras);
}

# --- Arranque -----------------------------------------------------------------------
my $servidor = IO::Socket::INET->new(
    LocalAddr => '127.0.0.1', LocalPort => $puerto, Proto => 'tcp', Listen => 16, ReuseAddr => 1,
);
unless ($servidor) {
    # Caso tipico: doble clic con el panel ya abierto. Si el puerto es de NUESTRO panel,
    # solo se abre el navegador.
    my $prueba = IO::Socket::INET->new(PeerAddr => '127.0.0.1', PeerPort => $puerto, Proto => 'tcp', Timeout => 3);
    if ($prueba) {
        print $prueba "GET /panel/ HTTP/1.0\r\nHost: localhost\r\n\r\n";
        local $/;
        my $respuesta = <$prueba> // '';
        if ($respuesta =~ /Panel ESP32-CAM/) {
            print "[OK] El panel ya estaba abierto: lo abro en el navegador.\n";
            abrir_navegador();
            exit 0;
        }
    }
    print "[ERROR] El puerto $puerto lo esta usando otro programa.\n";
    exit 1;
}

print "\n  Panel de aprendizaje ESP32-CAM\n";
print "  Abierto en: $url\n";
print "  Usa Chrome o Edge (Safari no permite conectar placas por USB).\n";
print "  Para cerrar el panel, cerra esta ventana (o Ctrl+C).\n\n";
abrir_navegador();

while (my $cliente = $servidor->accept()) {
    binmode($cliente);
    eval { atender($cliente); 1 } or print "[ERROR] $@";
    close $cliente;   # siempre se cierra: una conexion colgada traba al navegador
}
