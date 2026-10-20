#!/usr/bin/perl
use strict;
use warnings;
use IO::Socket::UNIX;
use JSON::PP;
use Getopt::Long;

# Interlock Data Surgery Engine (Satellite Worker)
# Integrated into the "Hub and Satellites" architecture.
# Uses Binary IPCMessage protocol for communication with C++ Core.

# IPCMessage structure constants (matching IPCBridge.hpp)
# uint32_t message_id;       (L)
# uint32_t sequence_num;     (L)
# uint8_t sender;            (C) -> IPCComponentType
# uint8_t receiver;          (C) -> IPCComponentType
# uint32_t payload_size;     (L)
# uint32_t flags;            (L)
# uint64_t timestamp;        (Q)
# uint8_t artifact[65536];    (a65536)

use constant {
    IPC_HEADER_FORMAT => "LLCCLLQ",
    IPC_HEADER_SIZE   => 4 + 4 + 1 + 1 + 4 + 4 + 8,
    MAX_PAYLOAD       => 65536,
    PERL_SURGERY      => 9,
    CPP_CORE          => 1,
};

my $socket_path = "/tmp/interlock_ipc_9"; # IPCComponentType::PERL_SURGERY = 9
my $standalone = 0;

GetOptions(
    "socket=s" => \$socket_path,
    "standalone" => \$standalone,
);

if ($standalone) {
    print "Running in standalone mode. Connect via --file for manual processing.\n";
    exit(0);
}

# Ensure the socket path exists
unlink($socket_path) if -e $socket_path;

my $server = IO::Socket::UNIX->new(
    Local  => $socket_path,
    Type   => SOCK_STREAM,
    Listen => 5,
) or die "Could not create socket at $socket_path: $!\n";

print "Surgery Satellite listening on $socket_path (Binary Protocol)...\n";

my %patterns = (
    email => qr/[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}/,
    ip    => qr/\b(?:\d{1,3}\.){3}\d{1,3}\b/,
    creds => qr/(?:user|pass|pw|secret|key|token|auth)\s*[:=]\s*["']?([^\s"']+)["']?/i,
);

while (my $client = $server->accept()) {
    while (1) {
        my $header_data;
        my $read = read($client, $header_data, IPC_HEADER_SIZE);
        last unless $read == IPC_HEADER_SIZE;

        my ($msg_id, $seq, $sender, $receiver, $payload_size, $flags, $timestamp) = unpack(IPC_HEADER_FORMAT, $header_data);

        if ($payload_size > MAX_PAYLOAD) {
            warn "Artifact size $payload_size exceeds maximum " . MAX_PAYLOAD . ". Dropping connection.\n";
            last;
        }

        my $artifact = "";
        my $to_read = MAX_PAYLOAD;
        while ($to_read > 0) {
            my $buffer;
            my $bytes = read($client, $buffer, $to_read);
            last unless defined $bytes && $bytes > 0;
            $artifact .= $buffer;
            $to_read -= $bytes;
        }
        last if $to_read > 0; # Partial read of padded artifact

        # Extract actual artifact data
        my $json_payload = substr($artifact, 0, $payload_size);
        my $request = eval { decode_json($json_payload) };

        my $response_json;
        if ($@) {
            $response_json = encode_json({error => "Invalid JSON: $@" });
        } else {
            my $command = $request->{command} || "";
            if ($command eq "SURGERY_EXTRACT") {
                my $data = $request->{data} || "";
                my $type = $request->{type} || "all";
                my @results;

                if ($type eq "all") {
                    foreach my $key (keys %patterns) {
                        push @results, perform_extraction($data, $patterns{$key}, $key);
                    }
                } elsif (exists $patterns{$type}) {
                    push @results, perform_extraction($data, $patterns{$type}, $type);
                }
                $response_json = encode_json({status => "OK", matches => \@results});
            } elsif ($command eq "PING") {
                $response_json = encode_json({status => "PONG"});
            } else {
                $response_json = encode_json({error => "Unknown command: $command"});
            }
        }

        send_ipc_response($client, $msg_id, $seq, $response_json);
    }
}

sub send_ipc_response {
    my ($client, $msg_id, $seq, $json) = @_;
    my $payload_size = length($json);

    if ($payload_size > MAX_PAYLOAD) {
        $json = encode_json({error => "Response too large", original_size => $payload_size});
        $payload_size = length($json);
        # If even the error is too large (unlikely), truncate
        if ($payload_size > MAX_PAYLOAD) {
            $json = substr($json, 0, MAX_PAYLOAD);
            $payload_size = MAX_PAYLOAD;
        }
    }

    my $timestamp = time();

    # Pad artifact to MAX_PAYLOAD
    my $padded_payload = $json . ("\0" x (MAX_PAYLOAD - $payload_size));

    my $header = pack(IPC_HEADER_FORMAT,
        $msg_id,
        $seq,
        PERL_SURGERY,
        CPP_CORE,
        $payload_size,
        0, # flags
        $timestamp
    );

    print $client $header . $padded_payload;
}

sub perform_extraction {
    my ($data, $regex, $label) = @_;
    my @matches;
    while ($data =~ /$regex/g) {
        push @matches, { type => $label, match => $1 || $& };
    }
    return @matches;
}
