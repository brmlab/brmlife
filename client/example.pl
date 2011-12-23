#!/usr/bin/perl
#
# Example brmlife client (originally based on the btraptor).
#
# This client is meant only as an example of implementation of all
# the main features without sophisticated architecture or decision
# making strategies.
#
# Usage: example.pl [PORT [AGENTID]]
#
# To run e.g. 15 instances of this client, run this command inside screen:
#	for i in `seq 1 15`; do screen ./example.pl; done

use strict;
use warnings;

# Socket communication should use CR-LF line endings, not just LF.
$/ = "\r\n";


# The example agent does most of its decision making in the
# take_action() subroutine. Its policy is to (in this order):
#
# * Attack any other agents encountered (unless low on energy).
# * Eat flowers if in immediate vicinity.
# * Roam around semi-aimlessly, trying to look for food.
#
# This agent can be identified based on pheromone #65536.

# The example agent uses $state to hold its state structure. It is
# a HASHREF with the following fields:
# {
# 	tick => NUMBER (current tick id)
# 	energy => NUMBER (current energy)
# 	visual => [
# 		CHAR[2], ...
# 	] (type-agent character pairs for perceived tiles)
# 	pheromones => [
# 		{ PHID => VALUE, ... }, ...
# 	] (pheromone spectrum for perceived tiles)
# }


# Read server input associated with a single tick and update the state
# structure accordingly.
sub tick($$) {
	my ($socket, $state) = @_;

	my $line = '';
	print "\n";
	while (chomp($line = <$socket>)) {
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

		} elsif ($type eq 'pheromones') {
			$value =~ /^(\((\d+:[0-9.]+,?)*\) ?)+$/ or die "[ee] type pheromones wrong value ($value)\n";
			# Best to read the following from the outside inwards:
			$state->{pheromones} = [
				map { {
					map { split(":", $_) } split(",", $_)
				} } map { y/()//d; $_ } split(" ", $value)
			];
		}
	}
}

# Execute an appropriate action based on the current agent state.
sub take_action($$) {
	my ($socket, $state) = @_;

	# FIXME: We use a common direction choice for both move_dir
	# and attack_dir, but in fact the agent can do both actions
	# in a single tick and they can be in different directions.

	# Relative x,y coordinates for each movement/attack direction.
	my @dirs = ([0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]);

	# Move/attack desires for each direction.
	# We prefer moves in the diagonal direction.
	my @move = (
		(1, 0, 1),
		(0, 0, 0),
		(1, 0, 1));
	my @attack = (
		(0, 0, 0),
		(0, 0, 0),
		(0, 0, 0));

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

	# In the case of shortage of resources, prefer fleeing to fighting.
	my $flee = ($state->{energy} < 10000);

	# Examine our neighborhood and adjust preference for each direction
	# based on what we sense.
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
			if (not $flee) {
				$move[dirindex($dir)] += 7;
				$attack[dirindex($dir)] += 1;
			} else {
				$move[dirindex($dir)] -= 7;
			}
		}

		if ($move[dirindex($dir)] > $move[dirindex($max)] or
		    ($move[dirindex($dir)] == $move[dirindex($max)] and int rand(2))) {
			$max = $dir;
		}
	}

	# Debug print
	print "moves ".join(", ", @move)." => (".dirindex($max).":$max->[0],$max->[1])\n";

	# Execute actions!
	if ($attack[dirindex($max)]) {
		print $socket $state->{tick}." attack_dir $max->[0] $max->[1] 100\r\n";
	} else {
		print $socket $state->{tick}." move_dir $max->[0] $max->[1]\r\n";
	}
	# We unconditionally secrete this pheromone for identification
	# by others of our kin.
	print $socket $state->{tick}." secrete 65536 1\r\n";
	print $socket "\r\n";
}


# Connect

my ($remote_host, $remote_port, $socket);
$remote_host = "localhost";
$remote_port = $ARGV[0];
$remote_port ||= 27753;

use IO::Socket;
$socket = IO::Socket::INET->new(
    PeerAddr => $remote_host,
    PeerPort => $remote_port,
    Proto    => "tcp",
    Type     => SOCK_STREAM
) or die "Couldn't connect to $remote_host:$remote_port : $@\n";
print "[ii] connected\r\n";


# Negotiate attributs

if ($ARGV[1]) {
	print "[ii] recovering agent $ARGV[1]\r\n";
	print $socket "agent_id $ARGV[1]\r\n";
} else {
	# Agent attributes - the default values:
	print $socket "move 1.0\r\n";
	print $socket "attack 0.5\r\n";
	print $socket "defense 0.5\r\n";
}
print $socket "\r\n";
print "[ii] agent created\r\n";


# Start tick loop

my $state = {};
while (1) {
	tick($socket, $state);
	# Debug print
	print $state->{energy} . "\n";
	print "visual [", join('], [', @{$state->{visual}}), "]\n";
	# The following could be written more succintly, but it is here
	# to show possible iteration over pheromones:
	print "pherom ";
	for my $i (0..$#{$state->{pheromones}}) {
		print "[";
		for my $pi (keys %{$state->{pheromones}->[$i]}) {
			print "$pi=>".$state->{pheromones}->[$i]->{$pi}." ";
		}
		print "] ";
	}
	print "\n";

	take_action($socket, $state);
}

shutdown($socket, 2);
