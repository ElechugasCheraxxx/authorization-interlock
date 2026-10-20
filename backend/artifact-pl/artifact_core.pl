#!/usr/bin/perl
use strict;
use warnings;
use XML::LibXML;
use Time::HiRes qw(gettimeofday);
use Getopt::Long;
use JSON;

# surgery_core.pl - Motor de Cirugía de Datos de Interlock (M4 Pro Optimized)
# El "Esclavo": Ejecuta procesamiento masivo de logs bajo demanda del Cerebro (Ruby).
# Refactorizado para inyección Mach IPC directa (Bypass Disk I/O).

my $file;
my $mode = 'general'; # identity, network, lateral, general
my $mach_enabled = 1;
my $injector_path = "./backend/core-cpp/build/mach_injector"; # Ajustar según arquitectura de build

GetOptions(
    'file=s' => \$file,
    'mode=s' => \$mode,
    'mach!'  => \$mach_enabled
);

if ($file) {
    surgical_parse($file, $mode);
} else {
    die "Usage: $0 --file <path> [--mode <identity|network|lateral|general>] [--no-mach]\n";
}

sub create_xml_finding {
    my ($data) = @_;

    my $doc = XML::LibXML::Document->new('1.0', 'UTF-8');
    my $root = $doc->createElement('Finding');
    $doc->setDocumentElement($root);

    $root->setAttribute('id', $data->{id} || sprintf("FND-%d-%d-%d", time(), $$, int(rand(1000))));
    $root->setAttribute('timestamp', int(gettimeofday * 1000000));

    foreach my $key (keys %$data) {
        next if $key eq 'id';
        if (ref($data->{$key}) eq 'HASH') {
            my $meta = $doc->createElement('Metadata');
            foreach my $mkey (keys %{$data->{$key}}) {
                my $mitem = $doc->createElement('Entry');
                $mitem->setAttribute('key', $mkey);
                $mitem->appendText($data->{$key}->{$mkey});
                $meta->appendChild($mitem);
            }
            $root->appendChild($meta);
        } else {
            my $node = $doc->createElement($key);
            $node->appendText($data->{$key});
            $root->appendChild($node);
        }
    }

    return $doc;
}

sub ship_to_core {
    my ($doc) = @_;
    my $xml_str = $doc->toString(1);

    if ($mach_enabled && -x $injector_path) {
        # Alta Prioridad: Inyectar vía Mach RPC (Bypass Disco)
        my $sender_id = 9; # PERL_SURGERY
        my $msg_type  = 100; # TACTICAL_XML_FINDING
        my $priority  = 1; # HIGH

        # Escapar XML para paso por argumento (o usar Protobuf en el futuro)
        my $status = system($injector_path, $sender_id, $msg_type, $priority, $xml_str);

        if ($status != 0) {
            warn "[SurgeryCore] Mach injection failed (status $status). Falling back to STDOUT.\n";
            print $xml_str . "\n---\n";
        }
    } else {
        # Fallback: STDOUT para el orquestador Ruby
        print $xml_str . "\n---\n";
    }
}

sub surgical_parse {
    my ($file_path, $op_mode) = @_;
    unless (-e $file_path) {
        warn "File not found: $file_path\n";
        return;
    }

    open(my $fh, '<', $file_path) or die "Cannot open $file_path: $!\n";

    while (<$fh>) {
        chomp;
        my $finding_data = undef;

        # 1. Cirugía de Credenciales
        if (($op_mode eq 'identity' || $op_mode eq 'general') && /\[(\w+)\]\s+Found\s+credential:\s+([^\s:]+):([^\s]+)/i) {
            $finding_data = {
                agent => $1,
                type => "IDENTITY_CRITICAL",
                title => "High-Value Credential Found",
                severity => "CRITICAL",
                artifact => $2 . ":" . $3,
                description => "Extracted plaintext identity from agent stream",
                metadata => { user => $2, secret => $3, source => "MemoryDump" }
            };
        }

        # 2. Cirugía de Red
        elsif (($op_mode eq 'network' || $op_mode eq 'general') && /\[(\w+)\]\s+Service\s+detected:\s+(\d+)\/(\w+)\s+(.+)/i) {
            $finding_data = {
                agent => $1,
                type => "NETWORK_RECON",
                title => "Active Service Discovery",
                severity => "MEDIUM",
                artifact => "$2/$3",
                description => "Port $2 ($3) is open. Banner: $4",
                metadata => { port => $2, proto => $3, banner => $4 }
            };
        }

        # 3. Cirugía de Movimiento Lateral
        elsif (($op_mode eq 'lateral' || $op_mode eq 'general') && /\[(\w+)\]\s+Pivot\s+Successful:\s+([^\s]+)\s+->\s+([^\s]+)/i) {
            $finding_data = {
                agent => $1,
                type => "LATERAL_MOVEMENT",
                title => "New Pivot Point established",
                severity => "HIGH",
                artifact => "$2->$3",
                description => "Successfully pivoted from $2 to $3",
                metadata => { from => $2, to => $3 }
            };
        }

        # 4. Cirugía de Kerberos / Golden Ticket
        elsif (($op_mode eq 'identity' || $op_mode eq 'general') && /krbtgt[:\s]+([a-fA-F0-9]{32})/i) {
            $finding_data = {
                agent => "SYSTEM",
                type => "DOMAIN_CRITICAL",
                title => "KRBTGT Hash Detected",
                severity => "CRITICAL",
                artifact => $1,
                description => "Potential Golden Ticket material found (KRBTGT NTLM Hash)",
                metadata => { account => "krbtgt", hash => $1, type => "NTLM" }
            };
        }

        if ($finding_data) {
            my $doc = create_xml_finding($finding_data);
            ship_to_core($doc);
        }
    }
    close($fh);
}
