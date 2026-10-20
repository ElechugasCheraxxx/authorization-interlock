#!/usr/bin/perl
use strict;
use warnings;
use XML::LibXML;
use MIME::Base64;

# artifact_mutator.pl - Motor de Mutación Polimórfica en Perl
# Genera variantes de payloads con firmas dinámicas y envoltorios XML.

sub obfuscate_strings {
    my ($code) = @_;
    # Ofuscación simple: convertir strings a hexadecimal o reversión
    $code =~ s/"([^"]+)"/join('.', map { "chr(".ord($_).")" } split('', $1))/ge;
    return $code;
}

sub add_junk_code {
    my ($code) = @_;
    my @junk = (
        "my \$unused_var = " . int(rand(1000)) . ";",
        "if (0) { print 'debugging'; }",
        "sub _unused_" . int(rand(100)) . " { return 1; }"
    );
    return $code . "\n" . $junk[rand @junk];
}

sub wrap_xml {
    my ($artifact, $type, $os) = @_;
    my $doc = XML::LibXML::Document->new('1.0', 'UTF-8');
    my $root = $doc->createElement('PayloadVariant');
    $doc->setDocumentElement($root);

    $root->setAttribute('type', $type);
    $root->setAttribute('target_os', $os);

    my $content = $doc->createElement('Content');
    $content->appendText(encode_base64($artifact));
    $root->appendChild($content);

    return $doc->toString(1);
}

# Entrada: --mutate --type [b64|xor|junk] --os [win|linux] --code "..."
my %args = @ARGV;

if ($args{'--mutate'}) {
    my $code = $args{'--code'};
    my $type = $args{'--type'} || 'junk';
    my $os   = $args{'--os'}   || 'linux';

    $code = obfuscate_strings($code) if $type eq 'obfuscate';
    $code = add_junk_code($code)    if $type eq 'junk';

    print wrap_xml($code, $type, $os);
}
