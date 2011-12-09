#!/usr/bin/perl -w
#
# Example brmlife client.
# Partially based on the btraptor.
#
# To run e.g. 15 instances of this client, run this command inside screen:
#
#	for i in `seq 1 15`; do screen ./example.pl; done

use strict;
use warnings;

use Switch;
use IO::Socket;

$/ = "\r\n";

sub tick($$) {
	my ($socket, $state) = @_;

	# read message from socket and parse it
	my $line = '';
	print "\n";
	while ( chomp($line = <$socket>) ) {
		print "# $line\n";
		last if $line eq '';
		if ($line eq 'DEAD')  {
			print "[ii] im dead\n";
			exit(-2);
			return;
		}

		if ($line eq 'BUMP')  {
			print "[ii] bump\n";
			next;
		}

		$line =~ m/^([^ ]+) (.*)$/;
		my ($type, $value) = ($1, $2);

		if ($type eq 'tick') {
			$value =~ /^\d+$/ or die "[ee] type tick wrong value ($value)\n";
			$state->{tick} =  $value;

		} elsif ($type eq 'energy') {
			$value =~ /^\d+$/ or die "[ee] type energy wrong value ($value)\n";
			$state->{energy} =  $value;

		} elsif ($type eq 'visual') {
			$value =~ /^([^ ][^ ] )+([^ ][^ ])$/ or die "[ee] type visual wrong value ($value)\n";
			$state->{visual} = [ split(" ", $value) ];
		}
	}
}

sub take_action($$) {
	my ($socket, $state) = @_;

	# FIXME: We use a common direction choice for both move_dir
	# and attack_dir, but in fact the agent can do both actions
	# in a single tick and they can be in different directions.

	# Relative x,y coordinates for each movement/attack direction.
	my @dirs = ([0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]);

	# Move/attack desires for each direction.
	# We prefer moves in the diagonal direction.
	my @move = ((1, 0, 1), (0, 0, 0), (1, 0, 1));
	my @attack = ((0, 0, 0), (0, 0, 0), (0, 0, 0));

	# dirindex($x) returns @move, @attack index for given @dirs item.
	sub dirindex { my ($dir) = @_; $dir->[0]+1 + 3*($dir->[1]+1) }

	# Relative x,y coordinates for each visual input, in order.
	my @vdirs = (
		@dirs,
		[0, -2], [1, -2], [2, -2], [2, -1], [2, 0], [2, 1], [2, 2], [1, 2], [0, 2], [-1, 2], [-2, 2], [-2, 1], [-2, 0], [-2, -1], [-2, -2], [-1, -2],
	);

	my $max = $dirs[0];
	# Default direction in case of nothing interesting in the vicinity
	# is [1, -1].

	for my $i (0..$#{$state->{visual}}) {
		my ($type, $agent) = split(//, $state->{visual}->[$i]);
		my $dir = $vdirs[$i];

		if (abs($dir->[0]) > 1 or abs($dir->[1]) > 1) {
			# We do not support processing visual information
			# for inputs two tiles away.
			next;
		}

		if ($agent eq 'x') {
			# Herp
			$move[dirindex($dir)] += 2;
		}

		if ($agent eq 'a') {
			# Corpse
			$move[dirindex($dir)] += 4;
		}

		if ($agent eq 'A') {
			# Agent
			$move[dirindex($dir)] += 7;
			$attack[dirindex($dir)] += 1;
		}

		if ($move[dirindex($dir)] > $move[dirindex($max)] or
		    ($move[dirindex($dir)] == $move[dirindex($max)] and int rand(2))) {
			$max = $dir;
		}
	}

	print "moves ".join(", ", @move)." => (".dirindex($max).":$max->[0],$max->[1])\n";

	if ($attack[dirindex($max)]) {
		print $socket "attack_dir $max->[0] $max->[1]\r\n";
	} else {
		print $socket "move_dir $max->[0] $max->[1]\r\n";
	}
	print $socket "secrete 65536 1\r\n";
	print $socket "\r\n";
}


# connect
my ($remote_host, $remote_port, $socket);
$remote_host = "localhost";
$remote_port = $ARGV[0];
$remote_port ||= 27753;

$socket = IO::Socket::INET->new(
    PeerAddr => $remote_host,
    PeerPort => $remote_port,
    Proto    => "tcp",
    Type     => SOCK_STREAM
) or die "Couldn't connect to $remote_host:$remote_port : $@\n";
print "[ii] connected\r\n";

# negotiate attributs
if ($ARGV[1]) {
	print "[ii] recovering agent $ARGV[1]\r\n";
	print $socket "agent_id $ARGV[1]\r\n";
} else {
	print $socket "move 1.0\r\n";
	print $socket "attack 1.0\r\n";
	print $socket "defense 1.0\r\n";
}
print $socket "\r\n";
print "[ii] agent created\r\n";

my $state = {};
while (1) {
	tick($socket, $state);
	print $state->{energy} . "\n";
	print "[", join('], [', @{$state->{visual}}), "]\n";

	take_action($socket, $state);
}

shutdown($socket, 2);
